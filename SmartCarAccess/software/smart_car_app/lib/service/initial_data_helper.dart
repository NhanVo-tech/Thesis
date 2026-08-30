import 'package:flutter/material.dart';
import 'package:smart_car_app/service/car_service.dart';

class InitialDataHelper {
  static final CarService _carService = CarService();

  static Future<void> addSampleDataIfNeeded(BuildContext context) async {
    try {
      // Check if user already has cars
      final cars = await _carService.getUserCars().first;
      
      if (cars.isEmpty) {
        await _showAddSampleDataDialog(context);
      }
    } catch (e) {
      print('Error checking for existing data: $e');
    }
  }

  static Future<void> _showAddSampleDataDialog(BuildContext context) async {
    final result = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text(
          'Welcome to Smart Car App!',
          style: TextStyle(
            fontWeight: FontWeight.bold,
            color: Color(0xFF273671),
          ),
        ),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              Icons.directions_car,
              size: 64,
              color: Color(0xFF41a5de),
            ),
            SizedBox(height: 16),
            Text(
              'Would you like to add some sample vehicles to get started?',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 16),
            ),
            SizedBox(height: 8),
            Text(
              'You can always add your own vehicles later.',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 14, color: Colors.grey),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Skip'),
          ),
          ElevatedButton(
            onPressed: () => Navigator.pop(context, true),
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFF273671),
              foregroundColor: Colors.white,
            ),
            child: const Text('Add Sample Data'),
          ),
        ],
      ),
    );

    if (result == true) {
      await _addSampleData(context);
    }
  }

  static Future<void> _addSampleData(BuildContext context) async {
    try {
      // Show loading dialog
      showDialog(
        context: context,
        barrierDismissible: false,
        builder: (context) => const AlertDialog(
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              CircularProgressIndicator(color: Color(0xFF273671)),
              SizedBox(height: 16),
              Text('Adding sample vehicles...'),
            ],
          ),
        ),
      );

      // Add sample cars
      final sampleCars = [
        _carService.createDefaultCarData(
          name: 'BMW X5',
          model: '2023',
          location: 'Home Garage',
          batteryLevel: 85,
          color: 'blue',
          isLocked: false,
        ),
        _carService.createDefaultCarData(
          name: 'Mercedes-Benz C-Class',
          model: '2023',
          location: 'Office Parking',
          batteryLevel: 92,
          color: 'purple',
          isLocked: true,
        ),
        _carService.createDefaultCarData(
          name: 'Toyota Camry',
          model: '2022',
          location: 'Mall Parking',
          batteryLevel: 67,
          color: 'orange',
          isLocked: true,
          keyStatus: 'Active',
        ),
      ];

      List<String> carIds = [];
      for (var carData in sampleCars) {
        await _carService.addCar(carData);
      }

      // Get the car IDs by querying the added cars
      final carsStream = _carService.getUserCars();
      final cars = await carsStream.first;
      carIds = cars.map((car) => car['id'] as String).toList();

      // Add sample digital keys
      final sampleKeys = [
        _carService.createDefaultDigitalKeyData(
          carId: carIds[0],
          name: 'Main Key',
          type: 'Owner',
          permissions: ['unlock', 'lock', 'start_engine', 'trunk', 'lights'],
          validUntil: DateTime.now().add(const Duration(days: 365)),
        ),
        _carService.createDefaultDigitalKeyData(
          carId: carIds[1],
          name: 'Family Key',
          type: 'Shared',
          permissions: ['unlock', 'lock', 'start_engine'],
          validUntil: DateTime.now().add(const Duration(days: 180)),
        ),
        _carService.createDefaultDigitalKeyData(
          carId: carIds[2],
          name: 'Guest Key',
          type: 'Temporary',
          permissions: ['unlock', 'lock'],
          validUntil: DateTime.now().add(const Duration(days: 30)),
        ),
      ];

      for (var keyData in sampleKeys) {
        await _carService.addDigitalKey(keyData);
      }

      // Close loading dialog
      Navigator.pop(context);

      // Show success message
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Sample data added successfully!'),
          backgroundColor: Color(0xFF273671),
        ),
      );
    } catch (e) {
      // Close loading dialog if still open
      Navigator.pop(context);
      
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to add sample data: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }
}