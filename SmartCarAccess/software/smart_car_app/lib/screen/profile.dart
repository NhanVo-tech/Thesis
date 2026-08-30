import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:smart_car_app/screen/login.dart';
import 'package:smart_car_app/service/car_service.dart';
import 'package:smart_car_app/service/nfc_provisioning_service.dart';
import 'package:smart_car_app/service/language_service.dart';
import 'package:smart_car_app/theme/app_colors.dart';

class ProfileScreen extends StatefulWidget {
  const ProfileScreen({super.key});

  @override
  State<ProfileScreen> createState() => _ProfileScreenState();
}

class _ProfileScreenState extends State<ProfileScreen> {
  User? currentUser;
  final CarService _carService = CarService();
  int _carCount = 0;
  int _keyCount = 0;
  bool _isLoading = true;
  late LanguageService _languageService;

  @override
  void initState() {
    super.initState();
    currentUser = FirebaseAuth.instance.currentUser;
    _languageService = LanguageService.instance;
    // Listen to language changes and rebuild UI
    _languageService.addListener(_onLanguageChanged);
    _loadUserData();
  }

  void _onLanguageChanged() {
    print('Profile: Language changed, rebuilding UI'); // Debug log
    if (mounted) {
      setState(() {});
    }
  }

  @override
  void dispose() {
    _languageService.removeListener(_onLanguageChanged);
    super.dispose();
  }

  Future<void> _loadUserData() async {
    try {
      // Load cars count
      _carService.getUserCars().listen((cars) {
        if (mounted) {
          setState(() {
            _carCount = cars.length;
          });
        }
      });

      // Load digital keys count
      _carService.getUserDigitalKeys().listen((keys) {
        if (mounted) {
          setState(() {
            _keyCount = keys.length;
            _isLoading = false;
          });
        }
      });
    } catch (e) {
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }

  Future<void> _signOut(BuildContext context) async {
    try {
      await FirebaseAuth.instance.signOut();
      if (mounted) {
        Navigator.pushAndRemoveUntil(
          context,
          MaterialPageRoute(builder: (_) => const LogIn()),
          (route) => false,
        );
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error signing out: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    print('Profile: Building UI, current language: ${_languageService.currentLanguage}');
    return Scaffold(
      backgroundColor: AppColors.background,
      body: SingleChildScrollView(
        child: Column(
          children: [
            _buildProfileHeader(),
            const SizedBox(height: 20),
            _buildProfileContent(),
          ],
        ),
      ),
    );
  }

  Widget _buildProfileHeader() {
    return Container(
      width: double.infinity,
      decoration: const BoxDecoration(
        gradient: AppColors.primaryGradient,
        borderRadius: BorderRadius.only(
          bottomLeft: Radius.circular(30),
          bottomRight: Radius.circular(30),
        ),
      ),
      child: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            children: [
              const SizedBox(height: 20),
              // Profile Avatar
              Container(
                width: 120,
                height: 120,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  border: Border.all(color: Colors.white, width: 4),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withOpacity(0.1),
                      blurRadius: 10,
                      offset: const Offset(0, 5),
                    ),
                  ],
                ),
                child: CircleAvatar(
                  radius: 56,
                  backgroundColor: Colors.white,
                  backgroundImage: currentUser?.photoURL != null 
                    ? NetworkImage(currentUser!.photoURL!)
                    : null,
                  child: currentUser?.photoURL == null 
                    ? Icon(
                        Icons.person,
                        size: 60,
                        color: Colors.grey[400],
                      )
                    : null,
                ),
              ),
              const SizedBox(height: 16),
              // User Name
              Text(
                currentUser?.displayName ?? 'User Name',
                style: const TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              const SizedBox(height: 8),
              // User Email
              Text(
                currentUser?.email ?? 'user@example.com',
                style: TextStyle(
                  fontSize: 16,
                  color: Colors.white.withOpacity(0.9),
                ),
              ),
              const SizedBox(height: 20),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildProfileContent() {
    return Padding(
      padding: const EdgeInsets.all(20),
      child: Column(
        children: [
          _buildStatsCards(),
          const SizedBox(height: 20),
          _buildMenuItems(),
        ],
      ),
    );
  }

  Widget _buildStatsCards() {
    return Row(
      children: [
        Expanded(
          child: _buildStatCard(
            icon: Icons.directions_car,
            title: _languageService.translate('my_cars'),
            value: _isLoading ? '...' : '$_carCount',
            color: AppColors.primary,
          ),
        ),
        const SizedBox(width: 16),
        Expanded(
          child: _buildStatCard(
            icon: Icons.vpn_key,
            title: _languageService.translate('digital_keys'),
            value: _isLoading ? '...' : '$_keyCount',
            color: AppColors.secondary,
          ),
        ),
      ],
    );
  }

  Widget _buildStatCard({
    required IconData icon,
    required String title,
    required String value,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 10,
            offset: const Offset(0, 2),
          ),
        ],
      ),
      child: Column(
        children: [
          Container(
            width: 50,
            height: 50,
            decoration: BoxDecoration(
              color: color.withOpacity(0.1),
              borderRadius: BorderRadius.circular(12),
            ),
            child: Icon(
              icon,
              color: color,
              size: 24,
            ),
          ),
          const SizedBox(height: 12),
          Text(
            value,
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
              color: color,
            ),
          ),
          const SizedBox(height: 4),
          Text(
            title,
            style: TextStyle(
              fontSize: 12,
              color: Colors.grey[600],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildMenuItems() {
    return Column(
      children: [
        _buildMenuItem(
          icon: Icons.person_outline,
          title: _languageService.translate('edit_profile'),
          subtitle: _languageService.translate('edit_profile_subtitle'),
          onTap: () {
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(content: Text(_languageService.translate('coming_soon'))),
            );
          },
        ),
        _buildMenuItem(
          icon: Icons.security,
          title: _languageService.translate('security'),
          subtitle: _languageService.translate('security_subtitle'),
          onTap: () {
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(content: Text('Security Settings - Coming Soon')),
            );
          },
        ),
        _buildMenuItem(
          icon: Icons.notifications,
          title: _languageService.translate('notifications'),
          subtitle: _languageService.translate('notifications_subtitle'),
          onTap: () {
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(content: Text(_languageService.translate('coming_soon'))),
            );
          },
        ),
        _buildMenuItem(
          icon: Icons.help_outline,
          title: _languageService.translate('help_support'),
          subtitle: _languageService.translate('help_support_subtitle'),
          onTap: () {
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(content: Text(_languageService.translate('coming_soon'))),
            );
          },
        ),
        _buildMenuItem(
          icon: Icons.nfc,
          title: _languageService.translate('test_nfc'),
          subtitle: _languageService.translate('test_nfc_subtitle'),
          onTap: () {
            _testHceService();
          },
        ),
        _buildMenuItem(
          icon: Icons.language,
          title: _languageService.translate('language'),
          subtitle: _languageService.translate('language_subtitle'),
          onTap: () {
            _showLanguageDialog();
          },
        ),
        const SizedBox(height: 10),
        _buildMenuItem(
          icon: Icons.logout,
          title: _languageService.translate('sign_out'),
          subtitle: _languageService.translate('sign_out_subtitle'),
          onTap: () => _showLogoutDialog(),
          isDestructive: true,
        ),
      ],
    );
  }

  Widget _buildMenuItem({
    required IconData icon,
    required String title,
    required String subtitle,
    required VoidCallback onTap,
    bool isDestructive = false,
  }) {
    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 10,
            offset: const Offset(0, 2),
          ),
        ],
      ),
      child: ListTile(
        onTap: onTap,
        leading: Container(
          width: 40,
          height: 40,
          decoration: BoxDecoration(
            color: isDestructive 
              ? Colors.red.withOpacity(0.1)
              : const Color(0xFF273671).withOpacity(0.1),
            borderRadius: BorderRadius.circular(10),
          ),
          child: Icon(
            icon,
            color: isDestructive ? Colors.red : const Color(0xFF273671),
            size: 20,
          ),
        ),
        title: Text(
          title,
          style: TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w600,
            color: isDestructive ? Colors.red : const Color(0xFF273671),
          ),
        ),
        subtitle: Text(
          subtitle,
          style: TextStyle(
            fontSize: 14,
            color: Colors.grey[600],
          ),
        ),
        trailing: Icon(
          Icons.arrow_forward_ios,
          size: 16,
          color: Colors.grey[400],
        ),
      ),
    );
  }

  void _showLogoutDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(16),
          ),
          title: Text(
            _languageService.translate('sign_out'),
            style: const TextStyle(
              fontWeight: FontWeight.bold,
              color: Color(0xFF273671),
            ),
          ),
          content: Text(_languageService.translate('confirm_sign_out')),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: Text(
                _languageService.translate('cancel'),
                style: TextStyle(color: Colors.grey[600]),
              ),
            ),
            ElevatedButton(
              onPressed: () {
                Navigator.pop(context);
                _signOut(context);
              },
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(8),
                ),
              ),
              child: Text(
                _languageService.translate('sign_out'),
                style: TextStyle(color: Colors.white),
              ),
            ),
          ],
        );
      },
    );
  }

  void _showAboutDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(16),
          ),
          title: const Text(
            'About Smart Car Access',
            style: TextStyle(
              fontWeight: FontWeight.bold,
              color: Color(0xFF273671),
            ),
          ),
          content: const Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('Version: 1.0.0'),
              SizedBox(height: 8),
              Text('Smart Car Access App allows you to manage your vehicles and digital keys remotely.'),
              SizedBox(height: 16),
              Text(
                'Features:',
                style: TextStyle(fontWeight: FontWeight.bold),
              ),
              Text('• Remote vehicle control'),
              Text('• Digital key management'),
              Text('• Real-time monitoring'),
              Text('• Secure authentication'),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text(
                'Close',
                style: TextStyle(color: Color(0xFF273671)),
              ),
            ),
          ],
        );
      },
    );
  }

  void _testHceService() async {
    try {
      // Check if NFC provisioning service is available
      final result = await NfcProvisioningService.testHceService();
      
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('HCE Test Result: $result'),
          backgroundColor: result.contains('success') ? Colors.green : Colors.orange,
          duration: const Duration(seconds: 5),
        ),
      );
      
      // Show detailed dialog
      showDialog(
        context: context,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('HCE Service Test Results'),
            content: Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text('Test Results:', 
                  style: TextStyle(fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                Text(result),
                const SizedBox(height: 16),
                const Text('Next Steps:', 
                  style: TextStyle(fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                const Text('1. Check Android logs: adb logcat -s ProvisioningHostApduService'),
                const Text('2. Tap phone to ESP32 NFC reader'),
                const Text('3. Monitor for APDU exchanges'),
              ],
            ),
            actions: [
              TextButton(
                onPressed: () => Navigator.pop(context),
                child: const Text('OK'),
              ),
            ],
          );
        },
      );
      
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('HCE Test Error: $e'),
          backgroundColor: Colors.red,
          duration: const Duration(seconds: 5),
        ),
      );
    }
  }

  void _showLanguageDialog() {
    String selectedLanguage = _languageService.currentLanguage;
    
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return StatefulBuilder(
          builder: (BuildContext context, StateSetter setState) {
            return AlertDialog(
              backgroundColor: const Color(0xFFE8E8E8), // Darker version of app background
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(16),
              ),
              title: Text(
                _languageService.translate('language'),
                style: const TextStyle(
                  fontWeight: FontWeight.bold,
                  color: Color(0xFF273671),
                  fontSize: 18,
                ),
              ),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  RadioListTile<String>(
                    title: const Text(
                      'English',
                      style: TextStyle(fontSize: 16),
                    ),
                    value: 'en',
                    groupValue: selectedLanguage,
                    onChanged: (String? value) {
                      if (value != null) {
                        setState(() {
                          selectedLanguage = value;
                        });
                      }
                    },
                    activeColor: const Color(0xFF273671),
                  ),
                  RadioListTile<String>(
                    title: const Text(
                      'Tiếng Việt',
                      style: TextStyle(fontSize: 16),
                    ),
                    value: 'vi',
                    groupValue: selectedLanguage,
                    onChanged: (String? value) {
                      if (value != null) {
                        setState(() {
                          selectedLanguage = value;
                        });
                      }
                    },
                    activeColor: const Color(0xFF273671),
                  ),
                ],
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(context),
                  child: Text(
                    _languageService.translate('cancel'),
                    style: TextStyle(color: Colors.grey[600]),
                  ),
                ),
                ElevatedButton(
                  onPressed: () async {
                    print('Profile: Apply button pressed, selectedLanguage: $selectedLanguage, currentLanguage: ${_languageService.currentLanguage}');
                    if (selectedLanguage != _languageService.currentLanguage) {
                      print('Profile: Calling setLanguage with: $selectedLanguage');
                      await _languageService.setLanguage(selectedLanguage);
                      print('Profile: setLanguage completed');
                      // Force rebuild entire app
                      WidgetsBinding.instance.reassembleApplication();
                      Navigator.pop(context);
                    } else {
                      print('Profile: Language not changed, same as current');
                      Navigator.pop(context);
                    }
                  },
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF273671),
                  ),
                  child: Text(
                    _languageService.translate('apply'),
                    style: TextStyle(color: Colors.white),
                  ),
                ),
              ],
            );
          },
        );
      },
    );
  }
}
