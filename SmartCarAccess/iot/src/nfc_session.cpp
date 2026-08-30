// NFC session implementation: encapsulates PN532 setup, polling, provisioning APDUs, and admin commands.
// Refactored for readability: clearer function names, smaller helpers, consistent naming and comments.

#include "nfc_session.h"
#include "provisioning_phase.h"
#include "fsm/fsm.h"
#include "fsm/fsm_integration.h"
#include "ccc_mailbox.h"
#include <esp_system.h>
#include <esp_log.h>
#include <cstring>
#include <mbedtls/md.h>

namespace {
    const bool kDebugApdu = true;
    // PN532 objects and session state
    PN532_HSU* hsu = nullptr;
    PN532* nfc = nullptr;
    bool samConfigured = false;
    bool waitingForRemoval = false;
    bool targetActive = false;
    uint32_t removalWaitStartedMs = 0;
    uint32_t lastRemovalWaitLogMs = 0;
    uint32_t nextCardPollAllowedMs = 0;

    // Startup guard to ignore early serial noise
    uint32_t bootMillis = 0;

    // HCE AID used when SELECTing the phone applet (CCC Release 3)
    uint8_t aid[] = {0xA0,0x00,0x00,0x08,0x09,0x43,0x43,0x43,0x44,0x4B,0x46,0x76,0x31};

    // Master card binding fallback (used only if CCC mailbox vehicleId is missing)
    const uint8_t EXPECTED_VEHICLE_ID[8] = {0x56, 0x4E, 0x38, 0x38, 0x38, 0x38, 0x42, 0x4B};
    const uint8_t MASTER_SECRET[32] = {
        0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18,
        0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18,
        0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18,
        0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18
    };

    const uint8_t kInsSpake2Request = 0x30;
    const uint8_t kInsSpake2Verify = 0x32;
    const uint8_t kInsWriteData = 0xD4;
    const uint8_t kInsOpControl = 0x3C;
    const uint8_t kTlvTagChallenge = 0x50;
    const uint8_t kTlvTagMac = 0x58;

    // Small utility: print hex bytes to Serial
    void printHex(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            Serial.printf("%02X", data[i]);
            if (i + 1 < len) Serial.print(' ');
        }
        Serial.println();
    }

    bool hmacSha256(const uint8_t* key, size_t keyLen,
                    const uint8_t* data, size_t dataLen,
                    uint8_t* out32) {
        if (!key || !data || !out32) return false;
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (!info) { mbedtls_md_free(&ctx); return false; }
        if (mbedtls_md_setup(&ctx, info, 1) != 0) { mbedtls_md_free(&ctx); return false; }
        int rc = mbedtls_md_hmac_starts(&ctx, key, keyLen);
        if (rc == 0) rc = mbedtls_md_hmac_update(&ctx, data, dataLen);
        if (rc == 0) rc = mbedtls_md_hmac_finish(&ctx, out32);
        mbedtls_md_free(&ctx);
        return rc == 0;
    }

    bool apduRetry(const uint8_t* apdu, uint8_t apduLen, uint8_t* resp, uint8_t* respLen,
                                 uint8_t attempts, uint16_t backoffMs);
    bool apduRetryWithReselect(const uint8_t* apdu, uint8_t apduLen, uint8_t* resp, uint8_t* respLen,
                               uint8_t attempts, uint16_t backoffMs, uint32_t selectTimeoutMs,
                               uint8_t inPlaceTries);
    bool releaseTarget();
    bool ensureSelected(uint32_t timeoutMs);

    bool unwrap7F24(const uint8_t* payload, int payloadLen, const uint8_t** out, int* outLen) {
        if (!payload || payloadLen < 3 || !out || !outLen) return false;
        if (payload[0] != 0x7F || payload[1] != 0x24) return false;
        int idx = 2;
        if (idx >= payloadLen) return false;
        uint8_t lenByte = payload[idx++];
        int len = 0;
        if (lenByte == 0x81) {
            if (idx >= payloadLen) return false;
            len = payload[idx++];
        } else if (lenByte <= 0x7F) {
            len = lenByte;
        } else {
            return false;
        }
        if (idx + len > payloadLen) return false;
        *out = payload + idx;
        *outLen = len;
        return true;
    }

    bool extractTlv(uint8_t tag, const uint8_t* buf, int len, const uint8_t** value, int* valueLen) {
        if (!buf || len < 2 || !value || !valueLen) return false;
        int idx = 0;
        while (idx + 2 <= len) {
            uint8_t t = buf[idx++];
            uint8_t l = buf[idx++];
            if (idx + l > len) return false;
            if (t == tag) {
                *value = buf + idx;
                *valueLen = l;
                return true;
            }
            idx += l;
        }
        return false;
    }

    bool isExpectedSelectPayload(const uint8_t* payload, int payloadLen) {
        static const uint8_t kSelectPayload[] = {
            0x5A, 0x03, 0x02, 0x00, 0x00,
            0x5C, 0x04, 0x01, 0x00, 0x01, 0x00
        };
        return payload && payloadLen == (int)sizeof(kSelectPayload)
            && memcmp(payload, kSelectPayload, sizeof(kSelectPayload)) == 0;
    }

    bool sendSpake2Mastercard(const uint8_t* challenge, size_t challengeLen) {
        if (!challenge || challengeLen == 0 || challengeLen > 32) return false;

        delay(90);

        // SPAKE2+ REQUEST (INS 0x30) with TLV tag 0x50 carrying the challenge
        uint8_t reqApdu[5 + 2 + 32 + 1];
        uint8_t idx = 0;
        uint8_t tlvLen = (uint8_t)(2 + challengeLen);
        reqApdu[idx++] = 0x00; reqApdu[idx++] = kInsSpake2Request; reqApdu[idx++] = 0x00; reqApdu[idx++] = 0x00;
        reqApdu[idx++] = tlvLen;
        reqApdu[idx++] = kTlvTagChallenge;
        reqApdu[idx++] = (uint8_t)challengeLen;
        memcpy(reqApdu + idx, challenge, challengeLen); idx += (uint8_t)challengeLen;
        reqApdu[idx++] = 0x00; // Le

        // SPAKE2+ challenge state may be session-scoped on some devices.
        // Retry as full REQUEST->VERIFY rounds after any reselection.
        uint8_t verApdu[] = {0x00, kInsSpake2Verify, 0x00, 0x00, 0x00};

        for (uint8_t round = 1; round <= 4; ++round) {
            if (round > 1) {
                Serial.printf("[PhaseA] SPAKE2+ round %u: reselection\n", round);
                releaseTarget();
                delay(140);
                if (!ensureSelected(4000)) {
                    Serial.println("[PhaseA] SPAKE2+ reselect failed");
                    continue;
                }
                delay(130);
            }

            uint8_t reqResp[64];
            uint8_t reqLen = sizeof(reqResp);
            bool reqOk = apduRetry(reqApdu, idx, reqResp, &reqLen, 3, 120)
                && reqLen >= 2 && reqResp[reqLen - 2] == 0x90 && reqResp[reqLen - 1] == 0x00;

            if (!reqOk) {
                // Recover REQUEST first; do not send VERIFY without a confirmed challenge write.
                reqLen = sizeof(reqResp);
                reqOk = apduRetryWithReselect(reqApdu, idx, reqResp, &reqLen, 2, 140, 3500, 1)
                    && reqLen >= 2 && reqResp[reqLen - 2] == 0x90 && reqResp[reqLen - 1] == 0x00;
            }

            if (!reqOk) {
                // Some phone/HCE combinations log the request as stored but still fail the PN532 ack path.
                // Continue to VERIFY after a reselect so we can use the cached challenge instead of discarding the round.
                Serial.printf("[PhaseA] SPAKE2+ round %u: REQUEST ack flaky; trying VERIFY anyway after reselect\n", round);
                releaseTarget();
                delay(80);
                if (!ensureSelected(4000)) {
                    Serial.printf("[PhaseA] SPAKE2+ round %u: reselect before VERIFY failed\n", round);
                    continue;
                }
                delay(120);
            }

            uint8_t verResp[96];
            uint8_t verLen = sizeof(verResp);
            // Even with REQUEST=9000, many phones momentarily drop RF before VERIFY.
            // Use reselect-capable retries in the same round to avoid dead-ending.
            delay(140);
            bool verOk = apduRetryWithReselect(verApdu, sizeof(verApdu), verResp, &verLen, 3, 180, 3500, 1)
                && verLen >= 2 && verResp[verLen - 2] == 0x90 && verResp[verLen - 1] == 0x00;

            if (!verOk) {
                Serial.printf("[PhaseA] SPAKE2+ round %u: VERIFY failed\n", round);
                continue;
            }

            int payloadLen = verLen - 2;
            const uint8_t* payload = verResp;
            const uint8_t* mac = nullptr;
            int macLen = 0;
            if (!extractTlv(kTlvTagMac, payload, payloadLen, &mac, &macLen) || macLen != 32) {
                Serial.printf("[PhaseA] SPAKE2+ round %u: VERIFY missing MAC tag 0x58\n", round);
                continue;
            }

            uint8_t macExpect[32];
            if (!hmacSha256(MASTER_SECRET, sizeof(MASTER_SECRET), challenge, challengeLen, macExpect)) {
                Serial.println("[PhaseA] SPAKE2+ HMAC compute failed");
                return false;
            }

            bool macOk = (memcmp(mac, macExpect, 32) == 0);
            Serial.printf("[PhaseA] SPAKE2+ MAC verify=%s (round=%u)\n", macOk ? "OK" : "FAIL", round);
            if (macOk) {
                return true;
            }
        }

        Serial.println("[PhaseA] SPAKE2+ REQUEST/VERIFY rounds exhausted");
        return false;
    }

    bool sendOpControlFlowSuccess() {
        // Commit transaction state on the applet side before persisting owner data.
        if (!ensureSelected(1200)) {
            Serial.println("[PhaseA] SELECT before OP CONTROL FLOW failed");
            return false;
        }
        uint8_t apdu[] = {0x00, kInsOpControl, 0x11, 0x00, 0x00};
        uint8_t resp[16]; uint8_t rlen = sizeof(resp);
        bool ok = apduRetryWithReselect(apdu, sizeof(apdu), resp, &rlen, 3, 120, 2500, 1)
            && rlen >= 2 && resp[rlen - 2] == 0x90 && resp[rlen - 1] == 0x00;
        if (!ok) {
            Serial.println("[PhaseA] OP CONTROL FLOW failed");
        }
        return ok;
    }

    bool getVehicleIdBytes(uint8_t out[8]) {
        const char* vid = CCCMailbox::vehicleId();
        if (vid && strlen(vid) == 8) {
            memcpy(out, vid, 8);
            return true;
        }
        memcpy(out, EXPECTED_VEHICLE_ID, 8);
        return false;
    }

    bool sendVehicleWriteData() {
        // CCC WRITE DATA (INS 0xD4): push vehicle_id + vehicle_pub to phone side.
        uint8_t vehicleId[8];
        uint8_t vehiclePub[65];
        bool vidOk = getVehicleIdBytes(vehicleId);
        bool vpubOk = CCCMailbox::getVehiclePub(vehiclePub, sizeof(vehiclePub));
        if (!vpubOk) {
            Serial.println("[PhaseA] WRITE DATA skipped: vehicle public key unavailable");
            return false;
        }

        if (!vidOk) {
            Serial.println("[PhaseA] WARNING: CCC vehicleId missing; using fallback for WRITE DATA");
        }

        // TLV payload layout:
        // 0x80 0x08 <vehicle_id(8)> 0x81 0x41 <vehicle_pub(65)>
        uint8_t payload[2 + 8 + 2 + 65];
        uint8_t p = 0;
        payload[p++] = 0x80;
        payload[p++] = 0x08;
        memcpy(payload + p, vehicleId, sizeof(vehicleId));
        p += sizeof(vehicleId);
        payload[p++] = 0x81;
        payload[p++] = 0x41;
        memcpy(payload + p, vehiclePub, sizeof(vehiclePub));
        p += sizeof(vehiclePub);

        uint8_t apdu[5 + sizeof(payload) + 1];
        uint8_t idx = 0;
        apdu[idx++] = 0x00;
        apdu[idx++] = kInsWriteData;
        apdu[idx++] = 0x00;
        apdu[idx++] = 0x00;
        apdu[idx++] = (uint8_t)sizeof(payload);
        memcpy(apdu + idx, payload, sizeof(payload));
        idx += (uint8_t)sizeof(payload);
        apdu[idx++] = 0x00; // Le

        uint8_t resp[16];
        uint8_t rlen = sizeof(resp);
        bool ok = apduRetryWithReselect(apdu, idx, resp, &rlen, 6, 140, 5000, 2)
            && rlen >= 2 && resp[rlen - 2] == 0x90 && resp[rlen - 1] == 0x00;
        if (!ok) {
            Serial.println("[PhaseA] WRITE DATA (vehicle key push) failed");
            return false;
        }

        Serial.println("[PhaseA] WRITE DATA accepted by phone");
        return true;
    }

    // Repeatedly try an INDATAEXCHANGE (APDU) with retry/backoff
    bool apduRetry(const uint8_t* apdu, uint8_t apduLen, uint8_t* resp, uint8_t* respLen,
                                 uint8_t attempts = 3, uint16_t backoffMs = 60) {
        if (!respLen) return false;
        uint8_t maxLen = *respLen;
        for (uint8_t attempt = 1; attempt <= attempts; ++attempt) {
            uint8_t len = maxLen;
            bool ok = nfc->inDataExchange((uint8_t*)apdu, apduLen, resp, &len);
            if (kDebugApdu) {
                Serial.printf("[APDU] try=%u ok=%s len=%u\n", attempt, ok ? "YES" : "NO", len);
                if (ok && len >= 2) {
                    Serial.printf("[APDU] SW=%02X%02X\n", resp[len - 2], resp[len - 1]);
                }
                if (ok && len > 0 && len <= 40) {
                    Serial.print("[APDU] resp: "); printHex(resp, len);
                }
            }
            if (ok && len >= 2) {
                *respLen = len;
                return true;
            }
            if (attempt < attempts) delay(backoffMs);
        }
        *respLen = 0;
        return false;
    }

    bool apduRetryWithReselect(const uint8_t* apdu, uint8_t apduLen, uint8_t* resp, uint8_t* respLen,
                               uint8_t attempts, uint16_t backoffMs, uint32_t selectTimeoutMs,
                               uint8_t inPlaceTries = 3) {
        if (!respLen) return false;
        uint8_t maxLen = *respLen;
        // First try in-place with a few retries; avoid hammering the HCE with immediate reselects.
        *respLen = maxLen;
        if (apduRetry(apdu, apduLen, resp, respLen, inPlaceTries, backoffMs)) {
            return true;
        }
        for (uint8_t attempt = 1; attempt <= attempts; ++attempt) {
            // Hard-reset the target state before trying to reselect.
            releaseTarget();
            delay(80);
            if (!ensureSelected(selectTimeoutMs)) {
                if (attempt < attempts) delay(backoffMs);
                continue;
            }
            delay(80);
            *respLen = maxLen;
            bool ok = apduRetry(apdu, apduLen, resp, respLen, 1, 0);
            if (ok) return true;
            if (attempt < attempts) delay(backoffMs);
        }
        *respLen = 0;
        return false;
    }

    // Try to configure the SAM (Secure Access Module) with retries
    bool ensureSAMConfig(uint8_t attempts = 5, uint16_t backoffMs = 150) {
        for (uint8_t i = 1; i <= attempts; ++i) {
            if (nfc->SAMConfig()) return true;
            delay(backoffMs);
        }
        return false;
    }

    // Explicitly release any active target/session on the PN532 to return to idle.
    // This prevents the next poll from requiring a "priming" tap.
    bool releaseTarget() {
        if (!targetActive) return true;
        bool ok = nfc->inRelease();
        if (kDebugApdu) {
            Serial.printf("[PN532] inRelease=%s\n", ok ? "OK" : "FAIL");
        }
        targetActive = false;
        return ok;
    }

    // Non-blocking card poll. Call once per tick; returns true only when stable card presence is reached.
    bool pollCardPresenceNonBlocking(uint32_t timeoutMs, uint8_t stableReads, uint16_t pollMs, bool& timedOut) {
        static bool waiting = false;
        static uint32_t waitStartMs = 0;
        static uint32_t lastPollMs = 0;
        static uint32_t lastDotMs = 0;
        static uint8_t stableCount = 0;

        timedOut = false;
        const uint32_t now = millis();

        if (!waiting) {
            releaseTarget();
            waiting = true;
            waitStartMs = now;
            lastPollMs = 0;
            lastDotMs = 0;
            stableCount = 0;
            Serial.println("Waiting for card (adaptive poll)...");
        }

        if (pollMs > 0 && (now - lastPollMs) < pollMs) {
            return false;
        }
        lastPollMs = now;

        bool present = nfc->inListPassiveTarget();
        if (present) {
            if (++stableCount >= stableReads) {
                Serial.println("Card detected (stable)");
                waiting = false;
                stableCount = 0;
                targetActive = true;
                return true;
            }
        } else {
            stableCount = 0;
        }

        if (lastDotMs == 0 || (now - lastDotMs) >= 2000) {
            Serial.print('.');
            lastDotMs = now;
        }

        if (timeoutMs && (now - waitStartMs) >= timeoutMs) {
            Serial.println("\nTimeout waiting for card");
            waiting = false;
            stableCount = 0;
            timedOut = true;
            return false;
        }

        return false;
    }

    // Non-blocking card removal poll. Call once per tick while waitingForRemoval is true.
    bool pollCardRemovalNonBlocking(uint8_t stableAbsentReads, uint16_t pollMs) {
        static uint32_t lastPollMs = 0;
        static uint8_t absentCount = 0;

        const uint32_t now = millis();
        if (pollMs > 0 && (now - lastPollMs) < pollMs) {
            return false;
        }
        lastPollMs = now;

        bool present = nfc->inListPassiveTarget();
        if (!present) {
            if (++absentCount >= stableAbsentReads) {
                absentCount = 0;
                Serial.println("Card removed (stable)\n");
                return true;
            }
        } else {
            absentCount = 0;
        }
        return false;
    }

    bool selectAid(uint8_t& sw1, uint8_t& sw2, int& payloadLen);

    bool ensureSelected(uint32_t timeoutMs = 2500) {
        // Fast path only when we already have an active target.
        uint8_t sw1 = 0, sw2 = 0; int payloadLen = 0;
        if (targetActive) {
            if (selectAid(sw1, sw2, payloadLen) && sw1 == 0x90 && sw2 == 0x00) {
                return true;
            }
        }

        const uint32_t start = millis();
        uint8_t stablePresentReads = 0;
        while (millis() - start < timeoutMs) {
            if (!nfc->inListPassiveTarget()) {
                stablePresentReads = 0;
                delay(50);
                continue;
            }

            // Require a short stable presence window after reselection to reduce 0x27/invalid-frame bursts.
            if (++stablePresentReads < 2) {
                delay(35);
                continue;
            }

            targetActive = true;
            sw1 = 0; sw2 = 0; payloadLen = 0;
            if (selectAid(sw1, sw2, payloadLen) && sw1 == 0x90 && sw2 == 0x00) {
                return true;
            }
            releaseTarget();
            stablePresentReads = 0;
            delay(120);
        }
        return false;
    }

    // After a session (successful SELECT and optional provisioning), either wait briefly for removal
    // or continue to the next cycle to keep UX snappy. For failed SELECT, just short backoff.
    void postSessionCooldown(bool hadSelectSuccess) {
        if (hadSelectSuccess) {
            // Defer removal handling so FSM can process the queued event immediately.
            waitingForRemoval = true;
            removalWaitStartedMs = millis();
            lastRemovalWaitLogMs = 0;
            return;
        }
        // Wrong tag / SELECT failed: small backoff to avoid tight loop (non-blocking)
        nextCardPollAllowedMs = millis() + 200;
        // Release just in case there is a half-open session
        removalWaitStartedMs = 0;
        lastRemovalWaitLogMs = 0;
        releaseTarget();
    }

    // Build and send SELECT AID APDU; returns SW1, SW2 and payload length
    bool selectAid(uint8_t& sw1, uint8_t& sw2, int& payloadLen) {
        uint8_t sel[32];
        uint8_t idx = 0;
        sel[idx++] = 0x00; sel[idx++] = 0xA4; sel[idx++] = 0x04; sel[idx++] = 0x00;
        sel[idx++] = sizeof(aid);
        for (size_t i = 0; i < sizeof(aid); ++i) sel[idx++] = aid[i];
        sel[idx++] = 0x00; // Le

        uint8_t resp[255];
        uint8_t rlen = sizeof(resp);
        if (kDebugApdu) {
            Serial.print("[APDU] SELECT cmd: "); printHex(sel, idx);
        }
        if (!apduRetry(sel, idx, resp, &rlen, 2, 40)) {
            if (kDebugApdu) Serial.println("[APDU] SELECT failed (no response)");
            return false;
        }

        sw1 = (rlen >= 2) ? resp[rlen - 2] : 0x00;
        sw2 = (rlen >= 2) ? resp[rlen - 1] : 0x00;
        payloadLen = (rlen >= 2) ? (rlen - 2) : 0;

        if (sw1 == 0x90 && sw2 == 0x00 && !isExpectedSelectPayload(resp, payloadLen)) {
            if (kDebugApdu) {
                Serial.printf("[APDU] SELECT payload mismatch len=%d (likely stale response)\n", payloadLen);
                if (payloadLen > 0) {
                    Serial.print("[APDU] SELECT unexpected payload: ");
                    printHex(resp, payloadLen);
                }
            }
            return false;
        }

        if (kDebugApdu) {
            Serial.printf("[APDU] SELECT SW=%02X%02X payloadLen=%d\n", sw1, sw2, payloadLen);
        }
        return true;
    }

    bool sendProvisionResult() {
        // Notify phone HCE that provisioning succeeded: INS=0xDA, data=0x01
        uint8_t sw1a = 0, sw2a = 0; int pla = 0;
        if (!selectAid(sw1a, sw2a, pla) || sw1a != 0x90 || sw2a != 0x00) {
            Serial.println("[PhaseA] SELECT before provision result failed");
            return false;
        }
        delay(80);
        uint8_t apdu[] = {0x00, 0xDA, 0x00, 0x00, 0x01, 0x01};
        uint8_t resp[16];
        uint8_t rlen = sizeof(resp);
        bool ok = apduRetry(apdu, sizeof(apdu), resp, &rlen, 2, 80);
        if (ok && rlen >= 2) {
            Serial.printf("[PhaseA] Provision result ack SW=%02X%02X len=%u\n", resp[rlen - 2], resp[rlen - 1], rlen);
        } else {
            Serial.println("[PhaseA] Provision result ack failed");
        }
        return ok && rlen >= 2 && resp[rlen - 2] == 0x90 && resp[rlen - 1] == 0x00;
    }

    // Parse the "Base GET_CHALLENGE" response layout (keyId is ignored in CCC flow):
    // [keyIdLen][keyId..][pubKey65..][certLen(2)][cert..]
    struct BaseInfo {
        uint8_t keyIdLen = 0;
        const uint8_t* pubKey65 = nullptr;
        uint16_t certLen = 0;
        const uint8_t* cert = nullptr;
        bool hasPub = false;
        bool hasFastArtifact = false;
        uint8_t fastArtifactVersion = 0;
        uint8_t fastArtifactKey[32] = {0};
    };

    bool parseBaseResponse(const uint8_t* data, int dataLen, BaseInfo& out) {
        if (dataLen < 1 + 65 + 2) return false; // minimal sizes
        int idx = 0;
        uint8_t keyIdLen = data[idx++];
        if (keyIdLen > dataLen - idx) return false;
        idx += keyIdLen; // keyId is ignored in CCC flow

        if (idx + 65 + 2 > dataLen) return false;
        const uint8_t* pubKey65 = data + idx; idx += 65;
        uint16_t certLen = (uint16_t)(data[idx] << 8 | data[idx + 1]); idx += 2;
        if (idx + certLen > dataLen) return false;
        const uint8_t* cert = data + idx;

        // Optional extension TLVs after cert payload:
        // - 0x90 len=1  => fast artifact version
        // - 0x91 len=32 => fast artifact key
        const int extOffset = idx + certLen;
        const int extLen = dataLen - extOffset;
        bool hasFastArtifact = false;
        uint8_t fastArtifactVersion = 0;
        uint8_t fastArtifactKey[32] = {0};
        if (extLen > 0) {
            const uint8_t* ext = data + extOffset;
            const uint8_t* versionTlv = nullptr;
            int versionLen = 0;
            const uint8_t* keyTlv = nullptr;
            int keyLen = 0;
            bool hasVersion = extractTlv(0x90, ext, extLen, &versionTlv, &versionLen) && versionLen == 1;
            bool hasKey = extractTlv(0x91, ext, extLen, &keyTlv, &keyLen) && keyLen == 32;
            if (hasVersion && hasKey) {
                fastArtifactVersion = versionTlv[0];
                memcpy(fastArtifactKey, keyTlv, 32);
                hasFastArtifact = fastArtifactVersion != 0;
            }
        }

        out.keyIdLen = keyIdLen;
        out.pubKey65 = pubKey65;
        out.certLen = certLen;
        out.cert = cert;
        out.hasPub = (pubKey65[0] == 0x04);
        out.hasFastArtifact = hasFastArtifact;
        out.fastArtifactVersion = fastArtifactVersion;
        if (hasFastArtifact) {
            memcpy(out.fastArtifactKey, fastArtifactKey, 32);
        }
        return true;
    }

    // Execute the two-step provisioning exchange when appropriate.
    bool performProvisioningIfNeeded(uint8_t sw1, uint8_t sw2) {
        bool alreadyProvisioned = ProvisioningPhase::isProvisioned();
        bool shouldForce = ProvisioningPhase::isForceProvisioning();
        
        // If already provisioned and force mode is OFF, reject provisioning attempt
        if (alreadyProvisioned && !shouldForce) {
            Serial.println("[PhaseA] ⚠️  Device already provisioned. Use 'f' or 'F' command to force re-provisioning.");
            return true;
        }
        
        bool shouldProvision = (shouldForce || !alreadyProvisioned) && (sw1 == 0x90 && sw2 == 0x00);
        if (!shouldProvision) return true;

        // ------------------------------
        // Phase 1: SPAKE2+ auth challenge (before key exchange)
        // ------------------------------
        uint8_t nonce[16];
        for (int i = 0; i < 16; ++i) nonce[i] = (uint8_t)esp_random();

        uint8_t challenge[24];
        uint8_t vehicleId[8];
        bool vidOk = getVehicleIdBytes(vehicleId);
        memcpy(challenge, vehicleId, 8);
        memcpy(challenge + 8, nonce, 16);

        if (!vidOk) {
            Serial.println("[PhaseA] WARNING: CCC vehicleId missing; using fallback");
        }
        Serial.print("[PhaseA] vehicleId: "); printHex(vehicleId, 8);
        Serial.print("[PhaseA] nonce: "); printHex(nonce, 16);
        Serial.print("[PhaseA] challenge(24): "); printHex(challenge, 24);

        if (!sendSpake2Mastercard(challenge, sizeof(challenge))) {
            Serial.println("[PhaseA] MasterCard SPAKE2+ verify failed");
            return false;
        }

        // ------------------------------
        // Step 1: Base GET DATA (Lc=0)
        // ------------------------------
        Serial.println("[PhaseA] Step1: Base GET DATA (Lc=0)");
        uint8_t baseApduLe0[] = {0x00, 0xCA, 0x00, 0x00, 0x00};
        uint8_t baseResp[255]; uint8_t baseLen = sizeof(baseResp);

        Serial.print("[PhaseA] OUT Base APDU (Le=0): "); printHex(baseApduLe0, sizeof(baseApduLe0));
        delay(120);
        BaseInfo info;
        const uint8_t* basePayload = nullptr;
        int basePayloadLen = 0;
        bool baseOk = false;
        for (uint8_t pass = 1; pass <= 3; ++pass) {
            baseLen = sizeof(baseResp);
            bool exchangeOk = apduRetryWithReselect(baseApduLe0, sizeof(baseApduLe0), baseResp, &baseLen, 4, 150, 4000, 1)
                && baseLen >= 2 && baseResp[baseLen - 2] == 0x90 && baseResp[baseLen - 1] == 0x00;
            if (!exchangeOk) {
                continue;
            }

            Serial.printf("[PhaseA] IN Base Resp len=%u SW=%02X%02X\n", baseLen, baseResp[baseLen - 2], baseResp[baseLen - 1]);
            int payloadLen = baseLen - 2;
            const uint8_t* payload = baseResp;
            const uint8_t* candidatePayload = payload;
            int candidateLen = payloadLen;
            if (payloadLen >= 2 && payload[0] == 0x7F && payload[1] == 0x24) {
                const uint8_t* inner = nullptr;
                int innerLen = 0;
                if (!unwrap7F24(payload, payloadLen, &inner, &innerLen)) {
                    Serial.println("[PhaseA] GET DATA 7F24 parse failed");
                    releaseTarget();
                    delay(120);
                    continue;
                }
                candidatePayload = inner;
                candidateLen = innerLen;
                Serial.printf("[PhaseA] GET DATA 7F24 unwrapped len=%d\n", candidateLen);
            }

            BaseInfo candidateInfo;
            if (!parseBaseResponse(candidatePayload, candidateLen, candidateInfo)) {
                Serial.printf("[PhaseA] Base payload parse error (len=%d), retrying...\n", candidateLen);
                Serial.print("[PhaseA] Base payload: "); printHex(candidatePayload, candidateLen);
                releaseTarget();
                delay(120);
                continue;
            }

            info = candidateInfo;
            basePayload = candidatePayload;
            basePayloadLen = candidateLen;
            baseOk = true;
            break;
        }

        if (!baseOk) {
            Serial.println("[PhaseA] Base GET DATA failed");
            if (baseLen > 0) {
                Serial.print("[PhaseA] Last base resp: "); printHex(baseResp, baseLen);
            } else {
                Serial.println("[PhaseA] Last base resp: <none>");
            }
            return false;
        }

        Serial.printf("[PhaseA] Parsed Base: keyIdLen=%u pubKeyFirst=%02X certLen=%u\n",
                        info.keyIdLen, info.pubKey65 ? info.pubKey65[0] : 0, info.certLen);
        if (info.hasPub) {
            Serial.print("[PhaseA] pubKey65: "); printHex(info.pubKey65, 65);
        }
        if (info.certLen > 0) {
            Serial.print("[PhaseA] cert (first up to 32 bytes): ");
            size_t show = info.certLen < 32 ? info.certLen : 32;
            printHex(info.cert, show);
        } else {
            Serial.println("[PhaseA] cert: <none>");
        }
        if (info.hasFastArtifact) {
            Serial.printf("[PhaseA] fast artifact extension detected ver=%u\n", (unsigned)info.fastArtifactVersion);
        } else {
            Serial.println("[PhaseA] fast artifact extension: <none>");
        }

        // Some phones drop RF after base response; let WRITE DATA handle reselection internally
        // instead of forcing a release here, which can increase stalls.
        Serial.println("[PhaseA] Proceeding to WRITE DATA with adaptive reselect");
        delay(80);

        // ------------------------------
        // Step 1.5: WRITE DATA (Lc=77) vehicle_id + vehicle_pub
        // ------------------------------
        if (!sendVehicleWriteData()) {
            return false;
        }
        delay(100);

        // ------------------------------
        // Step 2: Signature GET DATA (Lc=24)
        // ------------------------------
        // Some phones deselect the AID after the base response; retries handle reselect if needed.
        Serial.println("[PhaseA] Step2: Signature GET DATA (Lc=24 challenge)");

        // Build signature APDU: CLA INS P1 P2 Lc [challenge]
        uint8_t sigApdu[5 + sizeof(challenge)];
        uint8_t sIdx = 0;
        sigApdu[sIdx++] = 0x00; sigApdu[sIdx++] = 0xCA; sigApdu[sIdx++] = 0x00; sigApdu[sIdx++] = 0x00;
        sigApdu[sIdx++] = sizeof(challenge);
        memcpy(sigApdu + sIdx, challenge, sizeof(challenge)); sIdx += sizeof(challenge);

        Serial.print("[PhaseA] OUT Sig APDU: "); printHex(sigApdu, sIdx);
        uint8_t sigResp[255]; uint8_t sigTotalLen = sizeof(sigResp);
        bool sigOk = apduRetryWithReselect(sigApdu, sIdx, sigResp, &sigTotalLen, 5, 150, 2500, 2)
            && sigTotalLen >= 2 && sigResp[sigTotalLen - 2] == 0x90 && sigResp[sigTotalLen - 1] == 0x00;
        if (!sigOk) {
            Serial.println("[PhaseA] Signature GET DATA failed (reselect + retry)");
            uint8_t sw1c = 0, sw2c = 0; int plc = 0;
            releaseTarget();
            delay(80);
            if (selectAid(sw1c, sw2c, plc) && sw1c == 0x90 && sw2c == 0x00) {
                delay(160);
                sigTotalLen = sizeof(sigResp);
                sigOk = apduRetryWithReselect(sigApdu, sIdx, sigResp, &sigTotalLen, 5, 150, 2500, 2)
                    && sigTotalLen >= 2 && sigResp[sigTotalLen - 2] == 0x90 && sigResp[sigTotalLen - 1] == 0x00;
            }
        }
        if (!sigOk) {
            Serial.println("[PhaseA] Signature GET DATA failed");
            Serial.print("[PhaseA] Last sig resp: "); printHex(sigResp, sigTotalLen);
            return false;
        }

        Serial.printf("[PhaseA] IN Sig Resp len=%u SW=%02X%02X\n", sigTotalLen, sigResp[sigTotalLen - 2], sigResp[sigTotalLen - 1]);
        int sigPayloadLen = sigTotalLen - 2;
        if (sigPayloadLen < 2) { Serial.println("[PhaseA] Signature payload too short"); return false; }

        const uint8_t* sigDer = nullptr;
        uint16_t sigLen = 0;
        sigLen = (uint16_t)(sigResp[0] << 8 | sigResp[1]);
        if (2 + sigLen > sigPayloadLen) { Serial.println("[PhaseA] sigLen OOB"); return false; }
        sigDer = sigResp + 2;

        Serial.print("[PhaseA] sigDer (first up to 32 bytes): ");
        size_t showSig = sigLen < 32 ? sigLen : 32;
        printHex(sigDer, showSig);

        bool sigOK = false;
        if (sigLen > 0 && info.hasPub) {
            sigOK = ProvisioningPhase::verifySignatureP256(info.pubKey65, challenge, sizeof(challenge), sigDer, sigLen);
        }

        Serial.printf("[PhaseA] sigLen=%u verify=%s\n", sigLen, sigOK ? "OK" : "FAIL");
        if (!sigOK) { Serial.println("[PhaseA] Credentials NOT stored (signature failure)"); return false; }

        // Decide whether to store provisioning data
        if (!info.hasPub) {
            Serial.println("[PhaseA] Missing endpoint public key; provisioning rejected");
            return false;
        }

        if (!sendOpControlFlowSuccess()) {
            Serial.println("[PhaseA] Provisioning aborted: OP CONTROL FLOW commit failed");
            return false;
        }

        if (!ProvisioningPhase::setOwnerProvisioned(info.pubKey65, shouldForce)) {
            Serial.println("[PhaseA] Owner provisioning rejected (already provisioned?)");
            return false;
        }
        if (info.certLen > 0) ProvisioningPhase::storeCertChain(info.cert, info.certLen);
        if (info.hasFastArtifact) {
            if (!ProvisioningPhase::storeFastArtifact(info.fastArtifactVersion, info.fastArtifactKey)) {
                Serial.println("[PhaseA] Warning: failed to persist fast artifact extension");
            }
        }
        Serial.println("[PhaseA] Provisioning data persisted (CCC mailbox owner slot)");

        // Notify app after local persist succeeds.
        if (!sendProvisionResult()) {
            Serial.println("[PhaseA] Warning: failed to notify phone of provisioning result");
        }

        // Trigger FSM provision success event
        FSMIntegration::NFC::onCredentialsStored();

        // Auto-disable force mode after successful provisioning to prevent automatic re-provisioning
        if (shouldForce) {
            ProvisioningPhase::setForceProvisioningFlag(false);
            ProvisioningPhase::setOneShotForce(false);
            Serial.println("[PhaseA] ✓ Force provisioning auto-disabled after successful provisioning");
        }

        return true;
    }

    // Process incoming serial admin commands (debounced and guarded by startup time)
    void handleSerialCommands() {
        static uint32_t lastCmdMs = 0;
        static char lastCmd = 0;

        // Ignore serial input for the first 1500ms after boot to avoid garbage triggering destructive commands
        if (millis() - bootMillis < 1500) {
            for (size_t drained = 0; Serial.available() > 0 && drained < 32; ++drained) {
                Serial.read();
            }
            return;
        }

        if (!Serial.available()) {
            return;
        }

        const char c = static_cast<char>(Serial.read());
        if (c < 32 || c > 126) {
            return;
        }

        uint32_t now = millis();
        if (c == lastCmd && now - lastCmdMs < 250) {
            return;
        }
        lastCmd = c; lastCmdMs = now;

        if (c == 'p') {
            // Show FSM diagnostics
            FSMIntegration::SerialCmd::printDiagnostics();
        } else if (c == 'f') {
            FSMIntegration::SerialCmd::toggleForceProvision();
        } else if (c == 'F') {
            FSMIntegration::SerialCmd::armOneShotForce();
        } else if (c == 'r') {
            FSMIntegration::SerialCmd::clearKeys();
        } else if (c == 'C') {
            FSMIntegration::SerialCmd::clearAll();
        } else if (c == 'v') {
            bool ok = ProvisioningPhase::validateStoredCertMatchesStoredPub();
            Serial.printf("[Admin] Cert vs pub match: %s\n", ok ? "YES" : "NO/UNAVAILABLE");
        } else if (c == 'x') {
            uint8_t pub[65]; size_t pubLen = ProvisioningPhase::getPhonePubRaw(pub, sizeof(pub));
            if (pubLen == 65) { Serial.print("[Admin] pubKey65: "); printHex(pub, 65); }
            else Serial.println("[Admin] pubKey not stored");
        } else if (c == 'h') {
            Serial.println("Commands: p f F r C v x h");
        }
    }
} // anonymous namespace

namespace NfcSession {
    void begin(HardwareSerial& uart, int rxPin, int txPin, uint32_t baud) {
        uart.begin(baud, SERIAL_8N1, rxPin, txPin);
        delay(100);

        hsu = new PN532_HSU(uart);
        nfc = new PN532(*hsu);

        Serial.println("\n=== PN532 HCE READER (refactored) ===");
        nfc->begin();

        ProvisioningPhase::begin();
        esp_log_level_set("Preferences", ESP_LOG_NONE); // suppress noisy NVS NOT_FOUND logs

        bootMillis = millis(); // mark startup time to guard serial noise

        uint32_t ver = nfc->getFirmwareVersion();
        if (!ver) {
            Serial.println("PN532 NOT FOUND!");
            return;
        }
        Serial.printf("PN532 Firmware %d.%d\n", (int)((ver >> 16) & 0xFF), (int)((ver >> 8) & 0xFF));

        Serial.print("Configuring SAM...");
        samConfigured = ensureSAMConfig(5, 200);
        Serial.println(samConfigured ? "OK" : "SAM CONFIG FAILED (will still attempt to poll)");

        Serial.printf("[PhaseA] Provisioned=%s  Force=%s\n",
                                    ProvisioningPhase::isProvisioned() ? "YES" : "NO",
                                    ProvisioningPhase::isForceProvisioning() ? "ON" : "OFF");

        Serial.println("[PhaseA] Serial: p=print, f=toggle persistent force, F=one-shot force, r=reset phone, C=clear ALL, v=validate cert, x=dump pub, h=help");
    }

    void tick() {
        // Process admin serial commands
        handleSerialCommands();

        if (waitingForRemoval) {
            bool removed = pollCardRemovalNonBlocking(2, 80);
            if (removed) {
                waitingForRemoval = false;
                removalWaitStartedMs = 0;
                lastRemovalWaitLogMs = 0;
                releaseTarget();
                return;
            }

            uint32_t now = millis();
            if (removalWaitStartedMs == 0) removalWaitStartedMs = now;

            // Prevent indefinite wait loops when some phones keep the RF field "present".
            // Use a shorter cap to keep post-success UX snappy.
            if (now - removalWaitStartedMs >= 4500) {
                Serial.println("[Session] Removal wait max reached; continuing");
                waitingForRemoval = false;
                removalWaitStartedMs = 0;
                lastRemovalWaitLogMs = 0;
                releaseTarget();
                return;
            }

            if (lastRemovalWaitLogMs == 0 || (now - lastRemovalWaitLogMs >= 3000)) {
                Serial.println("[Session] Waiting for removal...");
                lastRemovalWaitLogMs = now;
            }
            return;
        }

        if (nextCardPollAllowedMs != 0 && millis() < nextCardPollAllowedMs) {
            return;
        }
        nextCardPollAllowedMs = 0;

        // Non-blocking wait for card; SELECT validation still filters non-matching/noisy targets.
        bool cardPollTimedOut = false;
        if (!pollCardPresenceNonBlocking(15000, 1, 60, cardPollTimedOut)) {
            if (cardPollTimedOut && !samConfigured) {
                Serial.println("Re-attempting SAMConfig...");
                samConfigured = ensureSAMConfig(3, 200);
                if (samConfigured) Serial.println("SAM configured successfully");
            }
            return;
        }

        // SELECT the HCE AID on the phone
        uint8_t sw1 = 0, sw2 = 0;
        int payloadLen = 0;
        bool selectOk = selectAid(sw1, sw2, payloadLen);
        if (!selectOk) {
            Serial.println("SELECT AID failed");
            postSessionCooldown(false);
            return;
        }

        Serial.printf("SELECT SW1SW2=%02X%02X payloadLen=%d  Provisioned=%s  Force=%s\n",
                                    sw1, sw2, payloadLen,
                                    ProvisioningPhase::isProvisioned() ? "YES" : "NO",
                                    ProvisioningPhase::isForceProvisioning() ? "ON" : "OFF");

        // Possibly perform provisioning sequence after a successful SELECT
        bool provisioningOk = performProvisioningIfNeeded(sw1, sw2);

        if (!provisioningOk) {
            Serial.println("[PhaseA] Transient provisioning failure; retrying without waiting for card removal");
        }

        // Keep flow smooth: wait briefly for removal only when SELECT succeeded
        postSessionCooldown((sw1 == 0x90 && sw2 == 0x00) && provisioningOk);
    }

    // ---- Admin control API (delegates to ProvisioningPhase) ----
    void setPersistentForce(bool on) {
        ProvisioningPhase::setForceProvisioningFlag(on);
    }
    void armOneShotForce() {
        ProvisioningPhase::setOneShotForce(true);
    }
    bool getPersistentForce() { 
        return ProvisioningPhase::isForceProvisioning(); 
    }
    bool isOneShotArmed() { 
        return ProvisioningPhase::isForceProvisioning(); 
    }
}
