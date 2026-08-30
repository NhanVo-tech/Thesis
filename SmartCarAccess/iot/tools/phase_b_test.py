#!/usr/bin/env python3
"""
Phase B Authentication Test Script
Simulates the phone side of the BLE authentication handshake for testing the ESP32 implementation.
"""

import asyncio
import struct
import hashlib
from typing import Optional
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
from bleak import BleakClient, BleakScanner
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.ec import EllipticCurvePrivateKey
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.backends import default_backend

# BLE Service and Characteristic UUIDs (must match ESP32)
AUTH_SERVICE_UUID = "0000aaaa-1234-5678-9abc-def012345678"
HANDSHAKE_WRITE_UUID = "0000aaab-1234-5678-9abc-def012345678"  # Write: phone -> ECU
HANDSHAKE_READ_UUID = "0000aaac-1234-5678-9abc-def012345678"   # Read/Notify: ECU -> phone
STATUS_UUID = "0000aaad-1234-5678-9abc-def012345678"          # Notify: status

class PhoneBAuthSimulator:
    def __init__(self):
        self.phone_ephemeral_key = None
        self.phone_identity_key = None  # This should be the same key used in Phase A
        self.ecu_ephemeral_pub = None
        self.shared_secret = None
        self.session_enc_key = None
        self.session_mac_key = None
        
    def generate_ephemeral_key(self):
        """Generate a fresh ephemeral keypair for this session"""
        self.phone_ephemeral_key = ec.generate_private_key(ec.SECP256R1(), default_backend())
        print(f"[PHONE] Generated ephemeral keypair")
        
        # Print public key in uncompressed format for debugging
        ephemeral_pub = self.phone_ephemeral_key.public_key()
        # Use standard serialization to get uncompressed point (0x04 || X || Y)
        pub_bytes = ephemeral_pub.public_bytes(Encoding.X962, PublicFormat.UncompressedPoint)
        # pub_bytes is 65 bytes (0x04 + X + Y)
        print(f"[PHONE] Ephemeral public key (uncompressed 65 bytes): {pub_bytes.hex().upper()}")
        
    def load_identity_key(self):
        """Load or generate the phone's identity key (same as used in Phase A)"""
        try:
            # Try to load existing key (you should save this from Phase A)
            with open("phone_identity_key.pem", "rb") as f:
                self.phone_identity_key = serialization.load_pem_private_key(
                    f.read(), password=None, backend=default_backend()
                )
            print("[PHONE] Loaded existing identity key")
        except FileNotFoundError:
            # Generate new key for testing
            self.phone_identity_key = ec.generate_private_key(ec.SECP256R1(), default_backend())
            # Save for future use
            with open("phone_identity_key.pem", "wb") as f:
                f.write(self.phone_identity_key.private_bytes(
                    encoding=serialization.Encoding.PEM,
                    format=serialization.PrivateFormat.PKCS8,
                    encryption_algorithm=serialization.NoEncryption()
                ))
            print("[PHONE] Generated and saved new identity key")
            
        # Print public key for provisioning verification
        identity_pub = self.phone_identity_key.public_key()
        # Serialize to uncompressed point format
        uncompressed = identity_pub.public_bytes(Encoding.X962, PublicFormat.UncompressedPoint)
        print(f"[PHONE] Identity public key (65 bytes): {uncompressed.hex().upper()}")
    def sign_ephemeral_key(self):
        """Sign the ephemeral public key with the identity key"""
        # Get ephemeral public key in uncompressed format
        if self.phone_ephemeral_key is None:
            raise ValueError("Ephemeral key not generated")
        if self.phone_identity_key is None:
            raise ValueError("Identity key not loaded")
        # Ensure identity key is an EC private key
        if not isinstance(self.phone_identity_key, EllipticCurvePrivateKey):
            raise TypeError("Identity key is not an EC private key")
        ephemeral_pub = self.phone_ephemeral_key.public_key()
        ephemeral_pub_bytes = ephemeral_pub.public_bytes(Encoding.X962, PublicFormat.UncompressedPoint)
        
        # Hash the ephemeral public key for debug only
        digest = hashlib.sha256(ephemeral_pub_bytes).digest()
        print(f"[PHONE] Hash of ephemeral key: {digest.hex().upper()}")
        
        # Sign the raw ephemeral public key with identity key using ECDSA(SHA256)
        signature = self.phone_identity_key.sign(ephemeral_pub_bytes, ec.ECDSA(hashes.SHA256()))
        print(f"[PHONE] Signature ({len(signature)} bytes): {signature.hex().upper()}")
        
        return ephemeral_pub_bytes, signature
        return ephemeral_pub_bytes, signature
        
    def build_handshake_packet(self):
        """Build the phone handshake packet: [ephemeral_pub(65)] + [sig_len(2)] + [signature]"""
        ephemeral_pub_bytes, signature = self.sign_ephemeral_key()
        
        packet = bytearray()
        packet.extend(ephemeral_pub_bytes)  # 65 bytes
        packet.extend(struct.pack('<H', len(signature)))  # 2 bytes, little endian
        packet.extend(signature)  # variable length
        
        print(f"[PHONE] Built handshake packet ({len(packet)} bytes): {packet.hex().upper()}")
        return bytes(packet)
        
    def parse_ecu_handshake(self, data):
        """Parse ECU handshake: [ephemeral_pub(65)] + [sig_len(2)] + [signature]"""
        if len(data) < 67:  # Minimum: 65 + 2
            raise ValueError(f"ECU handshake too short: {len(data)} bytes")
            
        ecu_ephemeral_pub = data[:65]
        sig_len = struct.unpack('<H', data[65:67])[0]
        
        if len(data) != 67 + sig_len:
            raise ValueError(f"Length mismatch: expected {67 + sig_len}, got {len(data)}")
            
        ecu_signature = data[67:67 + sig_len] if sig_len > 0 else b''
        
        print(f"[PHONE] ECU ephemeral pub (65 bytes): {ecu_ephemeral_pub.hex().upper()}")
        print(f"[PHONE] ECU signature ({sig_len} bytes): {ecu_signature.hex().upper()}")
        
        self.ecu_ephemeral_pub = ecu_ephemeral_pub
        return ecu_ephemeral_pub, ecu_signature
        
    def compute_shared_secret(self):
        """Compute ECDH shared secret"""
        if not self.ecu_ephemeral_pub:
            raise ValueError("ECU ephemeral public key not available")
            
        # Parse ECU public key (skip 0x04 prefix, extract X and Y coordinates)
        if self.ecu_ephemeral_pub[0] != 0x04:
            raise ValueError("Invalid ECU public key format")
            
        x_bytes = self.ecu_ephemeral_pub[1:33]
        y_bytes = self.ecu_ephemeral_pub[33:65]
        x = int.from_bytes(x_bytes, 'big')
        y = int.from_bytes(y_bytes, 'big')
        
        # Create ECU public key object
        ecu_pub_numbers = ec.EllipticCurvePublicNumbers(x, y, ec.SECP256R1())
        ecu_pub_key = ecu_pub_numbers.public_key(default_backend())
        
        # Ensure phone ephemeral private key exists and is the correct type
        if self.phone_ephemeral_key is None:
            raise ValueError("Phone ephemeral private key not generated")
        if not isinstance(self.phone_ephemeral_key, EllipticCurvePrivateKey):
            raise TypeError("Phone ephemeral key is not an EC private key")
        
        # Compute shared secret
        shared_key = self.phone_ephemeral_key.exchange(ec.ECDH(), ecu_pub_key)
        
        # Pad to 32 bytes (ECDH result might be shorter)
        self.shared_secret = shared_key.rjust(32, b'\x00')
        print(f"[PHONE] Shared secret (32 bytes): {self.shared_secret.hex().upper()}")
        
    def derive_session_keys(self):
        """Derive session keys using HKDF"""
        if not self.shared_secret:
            raise ValueError("Shared secret not computed")
        if self.phone_ephemeral_key is None:
            raise ValueError("Phone ephemeral private key not generated")
        if not isinstance(self.phone_ephemeral_key, EllipticCurvePrivateKey):
            raise TypeError("Phone ephemeral key is not an EC private key")
        if not self.ecu_ephemeral_pub:
            raise ValueError("ECU ephemeral public key not available")
        if len(self.ecu_ephemeral_pub) != 65 or self.ecu_ephemeral_pub[0] != 0x04:
            raise ValueError("Invalid ECU ephemeral public key format")
            
        # Get phone ephemeral public key bytes
        phone_ephemeral_pub = self.phone_ephemeral_key.public_key()
        pub_numbers = phone_ephemeral_pub.public_numbers()
        x_bytes = pub_numbers.x.to_bytes(32, 'big')
        y_bytes = pub_numbers.y.to_bytes(32, 'big')
        phone_ephemeral_bytes = b'\x04' + x_bytes + y_bytes
        
        # Derive encryption key: HKDF(shared_secret, "SmartCarAuth-ENC" || ecu_pub || phone_pub)
        info_enc = b"SmartCarAuth-ENC" + self.ecu_ephemeral_pub + phone_ephemeral_bytes
        hkdf_enc = HKDF(
            algorithm=hashes.SHA256(),
            length=32,
            salt=None,
            info=info_enc,
            backend=default_backend()
        )
        self.session_enc_key = hkdf_enc.derive(self.shared_secret)
        
        # Derive MAC key: HKDF(shared_secret, "SmartCarAuth-MAC" || ecu_pub || phone_pub)
        info_mac = b"SmartCarAuth-MAC" + self.ecu_ephemeral_pub + phone_ephemeral_bytes
        hkdf_mac = HKDF(
            algorithm=hashes.SHA256(),
            length=32,
            salt=None,
            info=info_mac,
            backend=default_backend()
        )
        self.session_mac_key = hkdf_mac.derive(self.shared_secret)
        
        print(f"[PHONE] Session encryption key: {self.session_enc_key.hex().upper()}")
        print(f"[PHONE] Session MAC key: {self.session_mac_key.hex().upper()}")

async def find_esp32():
    """Scan for ESP32 device"""
    print("[SCAN] Scanning for ESP-Smart-Car-ECU...")
    devices = await BleakScanner.discover()
    
    for device in devices:
        if device.name and "ESP-Smart-Car-ECU" in device.name:
            print(f"[SCAN] Found device: {device.name} ({device.address})")
            return device.address
    
    print("[SCAN] ESP32 device not found")
    return None

async def test_phase_b_authentication():
    """Main test function"""
    print("=== Phase B Authentication Test ===")
    
    # Find ESP32
    device_address = await find_esp32()
    if not device_address:
        print("[ERROR] Cannot find ESP32 device")
        return False
    
    simulator = PhoneBAuthSimulator()
    
    # Prepare phone side
    simulator.load_identity_key()
    simulator.generate_ephemeral_key()
    
    try:
        async with BleakClient(device_address) as client:
            print(f"[BLE] Connected to {device_address}")
            
            # Check if auth service is available
            # `get_services` is a coroutine on BleakClient but some static checkers
            # (Pylance) may not expose it on the stub; silence that with a narrow ignore.
            services = await client.get_services()  # type: ignore[attr-defined]
            auth_service = None
            for service in services:
                if service.uuid.lower() == AUTH_SERVICE_UUID.lower():
                    auth_service = service
                    break
                    
            if not auth_service:
                print(f"[ERROR] Auth service {AUTH_SERVICE_UUID} not found")
                return False
                
            print("[BLE] Auth service found")
            
            # Enable status notifications
            def status_callback(sender, data):
                status = data.decode('utf-8', errors='ignore')
                print(f"[STATUS] {status}")
            
            await client.start_notify(STATUS_UUID, status_callback)
            print("[BLE] Status notifications enabled")
            
            # Read ECU handshake (this should trigger ECU to send its ephemeral key + signature)
            print("[BLE] Reading ECU handshake...")
            ecu_handshake = await client.read_gatt_char(HANDSHAKE_READ_UUID)
            
            if len(ecu_handshake) == 0:
                print("[ERROR] No ECU handshake received - ECU may not be ready")
                return False
                
            print(f"[BLE] Received ECU handshake ({len(ecu_handshake)} bytes)")
            
            # Parse ECU handshake
            try:
                simulator.parse_ecu_handshake(ecu_handshake)
            except ValueError as e:
                print(f"[ERROR] Failed to parse ECU handshake: {e}")
                return False
                
            # Compute shared secret and derive keys
            simulator.compute_shared_secret()
            simulator.derive_session_keys()
            
            # Send phone handshake
            phone_handshake = simulator.build_handshake_packet()
            await client.write_gatt_char(HANDSHAKE_WRITE_UUID, phone_handshake)
            print("[BLE] Sent phone handshake")
            
            # Wait a bit for processing
            await asyncio.sleep(2)
            
            print("[SUCCESS] Phase B authentication test completed")
            return True
            
    except Exception as e:
        print(f"[ERROR] BLE communication failed: {e}")
        return False

async def test_admin_commands():
    """Test BLE admin commands"""
    print("\n=== Testing BLE Admin Commands ===")
    
    device_address = await find_esp32()
    if not device_address:
        return False
    
    try:
        async with BleakClient(device_address) as client:
            print(f"[BLE] Connected to {device_address}")
            
            # Admin service UUIDs
            ADMIN_SERVICE_UUID = "9a9b9c9d-0000-4000-8000-9a9b9c9d0000"
            ADMIN_CMD_UUID = "9a9b9c9d-0002-4000-8000-9a9b9c9d0002"
            ADMIN_INFO_UUID = "9a9b9c9d-0003-4000-8000-9a9b9c9d0003"
            
            # Enable info notifications
            def admin_info_callback(sender, data):
                info = data.decode('utf-8', errors='ignore')
                print(f"[ADMIN-INFO] {info}")
            
            await client.start_notify(ADMIN_INFO_UUID, admin_info_callback)
            
            # Test commands
            commands = [
                (0x33, "Status query"),
                (0x40, "Auth test ready"),
                (0x41, "Auth session status"),
                (0x43, "Auth statistics"),
            ]
            
            for cmd, desc in commands:
                print(f"[ADMIN] Sending command 0x{cmd:02X}: {desc}")
                await client.write_gatt_char(ADMIN_CMD_UUID, bytes([cmd]))
                await asyncio.sleep(1)
                
            return True
            
    except Exception as e:
        print(f"[ERROR] Admin command test failed: {e}")
        return False

async def main():
    """Main function"""
    print("ESP32 Phase B Authentication Test Tool")
    print("Make sure your ESP32 is running and BLE is enabled")
    
    # Test admin commands first
    await test_admin_commands()
    
    # Test Phase B authentication
    await test_phase_b_authentication()

if __name__ == "__main__":
    asyncio.run(main())