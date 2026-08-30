import asyncio
import argparse
import os
import sys
from dataclasses import dataclass

from bleak import BleakScanner, BleakClient
from cryptography.hazmat.primitives import serialization, hashes, hmac
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.ciphers.aead import AESGCM


DEVICE_NAME = "ESP-Smart-Car-ECU"

# UUIDs (must match firmware)
ADMIN_SVC = "9a9b9c9d-0000-4000-8000-9a9b9c9d0000"
ADMIN_INFO = "9a9b9c9d-0003-4000-8000-9a9b9c9d0003"      # notify
ADMIN_MODE = "9a9b9c9d-0001-4000-8000-9a9b9c9d0001"      # read/write (0=normal,1=enroll,2=remove)
ADMIN_CMD = "9a9b9c9d-0002-4000-8000-9a9b9c9d0002"        # write-only small commands
ADMIN_PHONEKEY = "9a9b9c9d-0004-4000-8000-9a9b9c9d0004"  # write

AUTH_SVC = "d0d0d0d0-0000-4000-8000-d0d0d0d00000"
AUTH_CLIENT_HELLO = "d0d0d0d0-0001-4000-8000-d0d0d0d00001"  # write
AUTH_SERVER_HELLO = "d0d0d0d0-0002-4000-8000-d0d0d0d00002"  # read/notify
AUTH_CLIENT_CFM = "d0d0d0d0-0003-4000-8000-d0d0d0d00003"    # write
AUTH_SERVER_CFM = "d0d0d0d0-0004-4000-8000-d0d0d0d00004"    # read/notify
AUTH_SEC_ECHO_IN = "d0d0d0d0-0005-4000-8000-d0d0d0d00005"    # write
AUTH_SEC_ECHO_OUT = "d0d0d0d0-0006-4000-8000-d0d0d0d00006"   # read/notify


@dataclass
class ServerHello:
    version: int
    pub: bytes
    sig: bytes


def load_or_create_phone_key(path: str):
    if os.path.exists(path):
        with open(path, "rb") as f:
            return serialization.load_pem_private_key(f.read(), password=None)
    priv = ec.generate_private_key(ec.SECP256R1())
    pem = priv.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.TraditionalOpenSSL,
        encryption_algorithm=serialization.NoEncryption(),
    )
    with open(path, "wb") as f:
        f.write(pem)
    return priv


def phone_pub_pem(priv) -> str:
    pub = priv.public_key()
    pem = pub.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    return pem.decode("utf-8")


def uncompressed_pub_bytes(pubkey) -> bytes:
    n = pubkey.public_numbers()
    x = n.x.to_bytes(32, "big")
    y = n.y.to_bytes(32, "big")
    return b"\x04" + x + y  # uncompressed point 65 bytes


def ecdsa_sign_der(priv, data: bytes) -> bytes:
    return priv.sign(data, ec.ECDSA(hashes.SHA256()))  # DER-encoded ECDSA


def hkdf_sha256(ikm: bytes, info: bytes, length=32) -> bytes:
    hk = HKDF(
        algorithm=hashes.SHA256(),
        length=length,
        salt=None,  # equivalent to zero salt per HKDF spec when None
        info=info,
    )
    return hk.derive(ikm)


def hmac_sha256(key: bytes, data: bytes) -> bytes:
    h = hmac.HMAC(key, hashes.SHA256())
    h.update(data)
    return h.finalize()


    


async def find_device_by_name(name: str):
    devs = await BleakScanner.discover()
    for d in devs:
        if d.name == name:
            return d
    return None


async def ble_upload_phone_pubkey(client: BleakClient, pem: str):
    # Enable AdminInfo notifications to see status
    async def _on_info(_, data: bytearray):
        try:
            print(f"[AdminInfo] {data.decode('utf-8', 'ignore')}")
        except Exception:
            print(f"[AdminInfo] {data!r}")

    try:
        await client.start_notify(ADMIN_INFO, _on_info)
    except Exception:
        pass

    data = pem.encode("utf-8")
    # Try single write first (response=True helps long writes)
    try:
        await client.write_gatt_char(ADMIN_PHONEKEY, data, response=True)
        await asyncio.sleep(0.3)
        return
    except Exception as e:
        print(f"Single write failed, trying chunked: {e}")

    # Chunked: server buffers until END footer
    chunksz = 160
    begin = pem.find("-----BEGIN PUBLIC KEY-----")
    if begin == -1:
        raise RuntimeError("PEM missing BEGIN header")
    for i in range(0, len(data), chunksz):
        await client.write_gatt_char(ADMIN_PHONEKEY, data[i:i+chunksz], response=True)
        await asyncio.sleep(0.05)
    await asyncio.sleep(0.3)


async def admin_set_mode(client: BleakClient, mode: int, enable_info=True):
    # mode: 0 normal, 1 enroll, 2 remove
    if enable_info:
        async def _on_info(_, data: bytearray):
            try:
                print(f"[AdminInfo] {data.decode('utf-8', 'ignore')}")
            except Exception:
                print(f"[AdminInfo] {data!r}")
        try:
            await client.start_notify(ADMIN_INFO, _on_info)
        except Exception:
            pass
    await client.write_gatt_char(ADMIN_MODE, bytes([mode & 0xFF]), response=True)
    await asyncio.sleep(0.1)


async def admin_list_tags(client: BleakClient):
    # Triggers device to print authorized tags on serial/log; also sets AdminInfo
    await client.write_gatt_char(ADMIN_CMD, bytes([0x10]), response=True)
    await asyncio.sleep(0.2)


async def admin_start_provisioning(client: BleakClient):
    # Triggers NFC provisioning via BLE Admin command 0x20
    print("[Admin] Triggering NFC provisioning...")
    await client.write_gatt_char(ADMIN_CMD, bytes([0x20]), response=True)
    await asyncio.sleep(0.5)  # Give time for command to process


def parse_server_hello(payload: bytes) -> ServerHello:
    if len(payload) < 1 + 2:
        raise ValueError("ServerHello too short")
    ver = payload[0]
    idx = 1
    pub_len = payload[idx] | (payload[idx+1] << 8)
    idx += 2
    if idx + pub_len + 2 > len(payload):
        raise ValueError("ServerHello bad pub_len")
    pub = payload[idx:idx+pub_len]
    idx += pub_len
    sig_len = payload[idx] | (payload[idx+1] << 8)
    idx += 2
    if idx + sig_len > len(payload):
        raise ValueError("ServerHello bad sig_len")
    sig = payload[idx:idx+sig_len]
    return ServerHello(ver, pub, sig)


async def handshake(client: BleakClient, phone_priv):
    # 1) Generate phone ephemeral
    eph_priv = ec.generate_private_key(ec.SECP256R1())
    eph_pub_bytes = uncompressed_pub_bytes(eph_priv.public_key())  # 65 bytes

    # 2) Sign phone ephemeral public key with phone long-term key
    sig_der = ecdsa_sign_der(phone_priv, eph_pub_bytes)

    # 3) Build and send ClientHello: [ver=1][pubLenLE][pub][sigLenLE][sig]
    ver = b"\x01"
    pkt = bytearray()
    pkt += ver
    pkt += len(eph_pub_bytes).to_bytes(2, "little")
    pkt += eph_pub_bytes
    pkt += len(sig_der).to_bytes(2, "little")
    pkt += sig_der

    # Prepare to receive ServerHello and ServerConfirm
    server_hello_ev = asyncio.Event()
    server_cfm_ev = asyncio.Event()
    recv_srv_hello = {}
    recv_srv_cfm = {}

    def _on_srv_hello(_, data: bytearray):
        recv_srv_hello["data"] = bytes(data)
        server_hello_ev.set()

    def _on_srv_cfm(_, data: bytearray):
        recv_srv_cfm["data"] = bytes(data)
        server_cfm_ev.set()

    await client.start_notify(AUTH_SERVER_HELLO, _on_srv_hello)
    await client.start_notify(AUTH_SERVER_CFM, _on_srv_cfm)

    # Send ClientHello in small chunks to avoid Windows long-write issues
    chunksz = 20  # fits default ATT MTU (23) payload (ATT_MTU-3)
    for i in range(0, len(pkt), chunksz):
        await client.write_gatt_char(AUTH_CLIENT_HELLO, bytes(pkt[i:i+chunksz]), response=True)
        await asyncio.sleep(0.01)

    # Wait for ServerHello
    if not await asyncio.wait_for(server_hello_ev.wait(), timeout=5.0):
        raise TimeoutError("Timed out waiting for ServerHello")
    sh = parse_server_hello(recv_srv_hello["data"])
    if sh.version != 1:
        raise RuntimeError(f"Bad ServerHello version: {sh.version}")

    # 4) Compute shared secret S via ECDH
    if len(sh.pub) != 65 or sh.pub[0] != 0x04:
        raise RuntimeError("Unexpected server pubkey format")
    x = int.from_bytes(sh.pub[1:33], "big")
    y = int.from_bytes(sh.pub[33:65], "big")
    peer_pub = ec.EllipticCurvePublicNumbers(x, y, ec.SECP256R1()).public_key()
    shared = eph_priv.exchange(ec.ECDH(), peer_pub)  # 32 bytes

    # 5) Derive session keys with HKDF
    info_enc = b"BLE-AUTH" + eph_pub_bytes + sh.pub
    info_mac = b"BLE-AUTH-MAC" + eph_pub_bytes + sh.pub
    k_enc = hkdf_sha256(shared, info_enc, 32)
    k_mac = hkdf_sha256(shared, info_mac, 32)

    # 6) ClientConfirm = HMAC(K_mac, "AUTH-PROOF" || cliPub || srvPub)
    msg = b"AUTH-PROOF" + eph_pub_bytes + sh.pub
    tag = hmac_sha256(k_mac, msg)
    await client.write_gatt_char(AUTH_CLIENT_CFM, tag, response=True)

    # 7) Read ServerConfirm and verify
    if not await asyncio.wait_for(server_cfm_ev.wait(), timeout=5.0):
        raise TimeoutError("Timed out waiting for ServerConfirm")
    srv_tag = recv_srv_cfm["data"]
    msg2 = b"AUTH-OK" + sh.pub + eph_pub_bytes
    ref = hmac_sha256(k_mac, msg2)
    if srv_tag != ref:
        raise RuntimeError("Server HMAC mismatch")

    print("Session established. Keys ready.")
    print("K_enc:", k_enc.hex())
    print("K_mac:", k_mac.hex())

    # After handshake, run a secure echo test
    await secure_echo_test(client, k_enc)

async def secure_echo_test(client: BleakClient, k_enc: bytes):
    # Prepare listener for echo response
    echo_ev = asyncio.Event()
    recv = {}
    def _on_echo(_, data: bytearray):
        recv["data"] = bytes(data)
        echo_ev.set()
    await client.start_notify(AUTH_SEC_ECHO_OUT, _on_echo)

    # Build an encrypted echo frame using AES-GCM with AAD=b"ECHO1"
    aesgcm = AESGCM(k_enc)
    nonce = os.urandom(12)
    aad = b"ECHO1"
    plaintext = b"hello-secure-echo"
    ct = aesgcm.encrypt(nonce, plaintext, aad)  # returns ciphertext||tag
    tag = ct[-16:]
    body = ct[:-16]
    frame = nonce + body + tag

    await client.write_gatt_char(AUTH_SEC_ECHO_IN, frame, response=True)
    await asyncio.wait_for(echo_ev.wait(), timeout=5.0)
    frame_out = recv["data"]
    if len(frame_out) < 12 + 16:
        raise RuntimeError("Echo frame too short")
    n2 = frame_out[:12]
    ct2 = frame_out[12:-16]
    tag2 = frame_out[-16:]
    pt2 = AESGCM(k_enc).decrypt(n2, ct2 + tag2, aad)
    print("Secure echo reply:", pt2.decode("utf-8", "ignore"))


async def main():
    ap = argparse.ArgumentParser(description="ESP Smart Car BLE Mutual Auth Demo")
    ap.add_argument("--device-name", default=DEVICE_NAME, help="BLE device name")
    ap.add_argument("--keyfile", default="phone_key.pem", help="path to phone long-term key (PEM)")
    ap.add_argument("--upload-key", action="store_true", help="upload phone public key PEM to device via Admin")
    ap.add_argument("--provision", action="store_true", help="trigger NFC provisioning via BLE Admin command")
    ap.add_argument("--enroll", action="store_true", help="set Admin mode to ENROLL and wait for tag present")
    ap.add_argument("--remove", action="store_true", help="set Admin mode to REMOVE and wait for tag present")
    ap.add_argument("--list-tags", action="store_true", help="request device to list authorized tags")
    args = ap.parse_args()

    phone_priv = load_or_create_phone_key(args.keyfile)
    pem = phone_pub_pem(phone_priv)
    print("Phone public key PEM (store on device once if needed):\n")
    print(pem)

    dev = await find_device_by_name(args.device_name)
    if not dev:
        print(f"Device '{args.device_name}' not found. Make sure it's advertising.")
        return 2

    # Connect once; route to the selected action and exit thereafter.
    async with BleakClient(dev) as client:
        print("Connected:", dev)
        if args.upload_key:
            print("Uploading phone public key to AdminPhoneKey...")
            await ble_upload_phone_pubkey(client, pem)
            print("Upload complete. Re-run this script without --upload-key to start the handshake.")
            return 0
        if args.provision:
            print("Triggering NFC provisioning via BLE Admin command...")
            await admin_start_provisioning(client)
            print("Provisioning command sent. Check device serial output.")
            print("Bring your Android HCE phone close to the ECU now.")
            return 0
        if args.enroll:
            print("Entering ENROLL mode. Present an NFC tag to the ECU...")
            await admin_set_mode(client, 1)
            await asyncio.sleep(10.0)
            print("Returning to NORMAL mode.")
            await admin_set_mode(client, 0, enable_info=False)
            if args.list_tags:
                print("Requesting tag list...")
                await admin_list_tags(client)
            return 0
        if args.remove:
            print("Entering REMOVE mode. Present an authorized NFC tag to remove...")
            await admin_set_mode(client, 2)
            await asyncio.sleep(10.0)
            print("Returning to NORMAL mode.")
            await admin_set_mode(client, 0, enable_info=False)
            if args.list_tags:
                print("Requesting tag list...")
                await admin_list_tags(client)
            return 0
        if args.list_tags:
            print("Requesting tag list...")
            await admin_list_tags(client)
        print("Starting mutual-auth handshake...")
        await handshake(client, phone_priv)

    return 0


if __name__ == "__main__":
    try:
        rc = asyncio.run(main())
        sys.exit(rc)
    except KeyboardInterrupt:
        sys.exit(1)
