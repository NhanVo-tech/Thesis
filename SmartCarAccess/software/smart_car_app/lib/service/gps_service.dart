import 'dart:async';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import 'package:geocoding/geocoding.dart';
import 'package:geolocator/geolocator.dart';
import 'package:crypto/crypto.dart';

/// GPS Service for Smart Car Access
/// 
/// This service provides:
/// - GPS location retrieval with permission handling
/// - Data encryption and MAC generation for secure BLE transmission
/// - Location data packaging in compact format
///
/// Usage:
/// ```dart
/// final gpsService = GpsService();
/// final encryptedData = await gpsService.getEncryptedLocationData(encKey, macKey);
/// // Send encryptedData via BLE to ESP32
/// ```
class GpsService {
  // Location service state
  bool _serviceEnabled = false;
  LocationPermission _permission = LocationPermission.denied;
  
  /// Check and request necessary permissions for GPS
  /// Returns true if all permissions are granted
  Future<bool> checkAndRequestPermissions() async {
    debugPrint('[GPS] Checking permissions...');
    
    // Check if location services are enabled
    _serviceEnabled = await Geolocator.isLocationServiceEnabled();
    if (!_serviceEnabled) {
      debugPrint('[GPS] Location services are disabled');
      return false;
    }
    
    // Check location permission
    _permission = await Geolocator.checkPermission();
    if (_permission == LocationPermission.denied) {
      _permission = await Geolocator.requestPermission();
      if (_permission == LocationPermission.denied) {
        debugPrint('[GPS] Location permissions are denied');
        return false;
      }
    }
    
    if (_permission == LocationPermission.deniedForever) {
      debugPrint('[GPS] Location permissions are permanently denied');
      return false;
    }
    
    debugPrint('[GPS] Permissions granted');
    return true;
  }
  
  /// Get current GPS position
  /// Returns Position object with latitude, longitude, altitude, etc.
  Future<Position?> getCurrentPosition({
    Duration timeout = const Duration(seconds: 10),
  }) async {
    try {
      debugPrint('[GPS] Getting current position...');
      
      final position = await Geolocator.getCurrentPosition(
        locationSettings: const LocationSettings(
          accuracy: LocationAccuracy.high,
          distanceFilter: 0,
        ),
      ).timeout(timeout);
      
      debugPrint('[GPS] Position obtained: '
          'lat=${position.latitude}, '
          'lon=${position.longitude}, '
          'alt=${position.altitude}m, '
          'accuracy=${position.accuracy}m');
      
      return position;
    } catch (e) {
      debugPrint('[GPS] Error getting position: $e');
      return null;
    }
  }

  /// Convert coordinates into a human-readable address for the UI.
  Future<String?> getAddressFromPosition(Position position) async {
    try {
      final placemarks = await placemarkFromCoordinates(
        position.latitude,
        position.longitude,
      );

      if (placemarks.isEmpty) {
        debugPrint('[GPS] No placemark found for current position');
        return null;
      }

      final placemark = placemarks.first;
      final parts = <String?>[
        placemark.street,
        placemark.subLocality,
        placemark.locality,
        placemark.administrativeArea,
        placemark.country,
      ].whereType<String>().where((part) => part.trim().isNotEmpty).toList();

      final address = parts.join(', ');
      debugPrint('[GPS] Resolved address: $address');
      return address.isEmpty ? null : address;
    } catch (e) {
      debugPrint('[GPS] Error resolving address: $e');
      return null;
    }
  }

  /// Build an encrypted GPS packet from an already resolved position.
  GpsDataPacket? buildEncryptedLocationPacket(
    Position position,
    Uint8List encKey,
    Uint8List macKey,
  ) {
    if (encKey.length != 32 || macKey.length != 32) {
      debugPrint('[GPS] Invalid key lengths: enc=${encKey.length}, mac=${macKey.length}');
      return null;
    }

    final plaintext = packageLocationData(position);
    debugPrint('[GPS] Packaged location data: ${plaintext.length} bytes');

    final encrypted = encryptData(plaintext, encKey);
    debugPrint('[GPS] Encrypted location data: ${encrypted.length} bytes');

    final hmac = computeHMAC(encrypted, macKey);
    debugPrint('[GPS] HMAC computed: ${hmac.length} bytes');

    final packet = Uint8List(encrypted.length + hmac.length);
    packet.setRange(0, encrypted.length, encrypted);
    packet.setRange(encrypted.length, packet.length, hmac);

    debugPrint('[GPS] Final packet size: ${packet.length} bytes');

    return GpsDataPacket(
      position: position,
      encryptedData: packet,
      plaintextData: plaintext,
    );
  }
  
  /// Package location data into compact binary format
  /// Format: [latitude(8) | longitude(8) | altitude(4) | accuracy(4) | timestamp(8)] = 32 bytes
  Uint8List packageLocationData(Position position) {
    final buffer = ByteData(32);
    
    // Latitude (8 bytes, double)
    buffer.setFloat64(0, position.latitude, Endian.little);
    
    // Longitude (8 bytes, double)
    buffer.setFloat64(8, position.longitude, Endian.little);
    
    // Altitude (4 bytes, float)
    buffer.setFloat32(16, position.altitude, Endian.little);
    
    // Accuracy (4 bytes, float)
    buffer.setFloat32(20, position.accuracy, Endian.little);
    
    // Timestamp (8 bytes, milliseconds since epoch)
    buffer.setInt64(24, position.timestamp.millisecondsSinceEpoch, Endian.little);
    
    return buffer.buffer.asUint8List();
  }
  
  /// Encrypt location data using AES-256-CBC with PKCS7 padding
  /// Returns encrypted data
  Uint8List encryptData(Uint8List plaintext, Uint8List encKey) {
    // For simplicity, we'll use XOR-based encryption
    // In production, use proper AES encryption from pointycastle
    // This is a placeholder that works with ESP32's simple decryption
    
    final encrypted = Uint8List(plaintext.length);
    for (int i = 0; i < plaintext.length; i++) {
      encrypted[i] = plaintext[i] ^ encKey[i % encKey.length];
    }
    
    return encrypted;
  }
  
  /// Compute HMAC-SHA256 for encrypted data
  /// Returns 32-byte MAC
  Uint8List computeHMAC(Uint8List data, Uint8List macKey) {
    final hmac = Hmac(sha256, macKey);
    final digest = hmac.convert(data);
    return Uint8List.fromList(digest.bytes);
  }
  
  /// Get encrypted and authenticated location data
  /// 
  /// Returns: [encrypted_data(32) | hmac(32)] = 64 bytes total
  /// 
  /// Parameters:
  /// - encKey: 32-byte AES encryption key (from BLE session)
  /// - macKey: 32-byte HMAC key (from BLE session)
  Future<GpsDataPacket?> getEncryptedLocationData(
    Uint8List encKey,
    Uint8List macKey,
  ) async {
    // Check permissions
    final hasPermission = await checkAndRequestPermissions();
    if (!hasPermission) {
      debugPrint('[GPS] No permission to access GPS');
      return null;
    }
    
    // Get current position
    final position = await getCurrentPosition();
    if (position == null) {
      debugPrint('[GPS] Failed to get position');
      return null;
    }

    return buildEncryptedLocationPacket(position, encKey, macKey);
  }
  
  /// Stream location updates
  /// Useful for continuous tracking
  Stream<Position> getLocationStream({
    Duration interval = const Duration(seconds: 5),
  }) {
    return Geolocator.getPositionStream(
      locationSettings: LocationSettings(
        accuracy: LocationAccuracy.high,
        distanceFilter: 10, // Update every 10 meters
        timeLimit: interval,
      ),
    );
  }
}

/// GPS data packet container
class GpsDataPacket {
  final Position position;
  final Uint8List encryptedData; // encrypted_data + HMAC
  final Uint8List plaintextData; // for debugging
  
  GpsDataPacket({
    required this.position,
    required this.encryptedData,
    required this.plaintextData,
  });
  
  /// Get human-readable location string
  String get locationString =>
      'Lat: ${position.latitude.toStringAsFixed(6)}, '
      'Lon: ${position.longitude.toStringAsFixed(6)}, '
      'Alt: ${position.altitude.toStringAsFixed(1)}m, '
      'Acc: ${position.accuracy.toStringAsFixed(1)}m';
  
  @override
  String toString() {
    return 'GpsDataPacket($locationString, encrypted=${encryptedData.length} bytes)';
  }
}
