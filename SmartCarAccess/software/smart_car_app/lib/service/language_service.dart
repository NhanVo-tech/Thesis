import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

class LanguageService extends ChangeNotifier {
  static const String _languageKey = 'app_language';
  static LanguageService? _instance;
  Locale _currentLocale = const Locale('en');

  // Singleton pattern
  static LanguageService get instance {
    _instance ??= LanguageService._internal();
    return _instance!;
  }

  LanguageService._internal() {
    _loadLanguage();
  }

  factory LanguageService() => instance;

  Locale get currentLocale => _currentLocale;
  String get currentLanguage => _currentLocale.languageCode;

  // Translations
  final Map<String, Map<String, String>> _translations = {
    'en': {
      // Profile Menu
      'edit_profile': 'Edit Profile',
      'edit_profile_subtitle': 'Update your information',
      'security': 'Security',
      'security_subtitle': 'Password and authentication',
      'notifications': 'Notifications',
      'notifications_subtitle': 'Push notifications and alerts',
      'help_support': 'Help & Support',
      'help_support_subtitle': 'Get help and contact us',
      'test_nfc': 'Test NFC HCE',
      'test_nfc_subtitle': 'Debug NFC Host Card Emulation service',
      'about': 'About',
      'about_subtitle': 'App version and information',
      'sign_out': 'Sign Out',
      'sign_out_subtitle': 'Sign out of your account',
      'confirm_sign_out': 'Are you sure you want to sign out?',
      'my_cars': 'My Cars',
      'digital_keys': 'Digital Keys',
      'language': 'Language',
      'language_subtitle': 'English / Tiếng Việt',
      'confirm_language_change': 'Change Language',
      'language_changed': 'Language changed to English',
      
      // Navigation
      'home': 'Home',
      'location': 'Location',
      'settings': 'Settings',
      'profile': 'Profile',
      
      // Dashboard
      'smart_car_access': 'Smart Car Access',
      'total_cars': 'Total Cars',
      'active_keys': 'Active Keys',
      'available': 'Available',
      'quick_actions': 'Quick Actions',
      'control_vehicles': 'Control your vehicles',
      'my_vehicles': 'My Vehicles',
      'lock': 'Lock',
      'unlock': 'Unlock',
      'engine_start': 'Engine Start',
      'add_sample_data': 'Add Sample Data',
      'updated': 'Updated',
      'just_now': 'just now',
      'ago': 'ago',
      'loading': 'Loading...',
      'failed_to_load_vehicles': 'Failed to load vehicles',
      'failed_to_load_keys': 'Failed to load digital keys',
      'add_car': 'Add Car',
      'add_key': 'Add Digital Key',
      'no_digital_keys': 'No digital keys',
      'create_digital_key': 'Create digital key',
      'no_keys_created': 'No digital keys created yet',
      'create_keys_description': 'Create digital keys to share vehicle access',
      'failed_create_key': 'Failed to create digital key',
      'coming_soon': 'Coming Soon',
      'permissions': 'Permissions',
      'cancel': 'Cancel',
      'apply': 'Apply',
      
      // Settings
      'app_settings': 'Settings',
      'general': 'General',
      'account': 'Account',
      'theme': 'Theme',
      'dark_mode': 'Dark Mode',
      'light_mode': 'Light Mode',
      
      // Location
      'my_location': 'My Location',
      'current_location': 'Current Location',
      'search_location': 'Search Location',
      
      // Common
      'cancel': 'Cancel',
      'apply': 'Apply',
      'save': 'Save',
      'delete': 'Delete',
      'edit': 'Edit',
      'close': 'Close',
      'ok': 'OK',
      'error': 'Error',
      'success': 'Success',
      'loading': 'Loading...',
      'details': 'Details',
      'copy_qr': 'Copy QR',
      'share': 'Share',
      'add_sample_data': 'Add Sample Data',
      
      // Anomaly Detection
      'anomaly_detected': 'Anomaly Detected',
      'anomaly_detected_desc': 'Suspicious access attempt detected',
      'anomaly_time': 'Time Anomaly',
      'anomaly_location': 'Location Anomaly',
      'anomaly_frequency': 'Frequency Anomaly',
      'anomaly_high_risk': 'High Risk',
      'anomaly_medium_risk': 'Medium Risk',
      'anomaly_low_risk': 'Low Risk',
      'anomaly_blocked': 'Access Blocked',
      'anomaly_allowed': 'Access Allowed',
      'anomaly_review': 'Review Required',
      'anomaly_details': 'Anomaly Details',
      'anomaly_reason': 'Reason',
      'anomaly_confidence': 'Confidence',
      'anomaly_severity': 'Severity',
      'anomaly_timestamp': 'Timestamp',
      'anomaly_location_info': 'Location Info',
      'anomaly_device_info': 'Device Info',
      'anomaly_user_history': 'User History',
      'anomaly_patterns': 'Access Patterns',
      'anomaly_training': 'Training Model',
      'anomaly_learning': 'Machine Learning',
      'anomaly_claude_ai': 'Claude AI Analysis',
    },
    'vi': {
      // Profile Menu
      'edit_profile': 'Chỉnh Sửa Hồ Sơ',
      'edit_profile_subtitle': 'Cập nhật thông tin của bạn',
      'security': 'Bảo Mật',
      'security_subtitle': 'Mật khẩu và xác thực',
      'notifications': 'Thông Báo',
      'notifications_subtitle': 'Thông báo đẩy và cảnh báo',
      'help_support': 'Trợ Giúp & Hỗ Trợ',
      'help_support_subtitle': 'Nhận trợ giúp và liên hệ với chúng tôi',
      'test_nfc': 'Kiểm Tra NFC HCE',
      'test_nfc_subtitle': 'Debug dịch vụ NFC Host Card Emulation',
      'about': 'Về Ứng Dụng',
      'about_subtitle': 'Phiên bản ứng dụng và thông tin',
      'sign_out': 'Đăng Xuất',
      'sign_out_subtitle': 'Đăng xuất khỏi tài khoản của bạn',
      'confirm_sign_out': 'Bạn có chắc chắn muốn đăng xuất?',
      'my_cars': 'Xe Của Tôi',
      'digital_keys': 'Khoá Kỹ Thuật Số',
      'language': 'Ngôn Ngữ',
      'language_subtitle': 'English / Tiếng Việt',
      'confirm_language_change': 'Thay Đổi Ngôn Ngữ',
      'language_changed': 'Đã thay đổi ngôn ngữ sang Tiếng Việt',
      
      // Navigation
      'home': 'Trang Chủ',
      'location': 'Vị Trí',
      'settings': 'Cài Đặt',
      'profile': 'Hồ Sơ',
      
      // Dashboard
      'smart_car_access': 'Truy Cập Xe Thông Minh',
      'total_cars': 'Tổng Xe',
      'active_keys': 'Khoá Hoạt Động',
      'available': 'Có Sẵn',
      'quick_actions': 'Thao Tác',
      'control_vehicles': 'Điều Khiển Xe',
      'my_vehicles': 'Xe Của Tôi',
      'lock': 'Khóa',
      'unlock': 'Mở Khóa',
      'engine_start': 'Khởi Động',
      'add_sample_data': 'Thêm Dữ Liệu Mẫu',
      'updated': 'Cập Nhật',
      'just_now': 'vừa xong',
      'ago': 'trước đây',      'loading': 'Đang Tải...',
      'failed_to_load_vehicles': 'Không thể tải xe',
      'failed_to_load_keys': 'Không thể tải khoá điện tử',
      'add_car': 'Thêm Xe',
      'add_key': 'Thêm Khoá Xe',
      'no_digital_keys': 'Không có khoá điện tử',
      'create_digital_key': 'Tạo khoá điện tử',
      'no_keys_created': 'Chưa tạo khoá điện tử nào',
      'create_keys_description': 'Tạo khoá điện tử để chia sẻ quyền truy cập xe',
      'failed_create_key': 'Không thể tạo khoá điện tử',
      'coming_soon': 'Sắp ra mắt',
      'permissions': 'Quyền hạn',
      'permissions': 'Quyền hạn',
      'no_digital_keys': 'Không có khoá điện tử',
      'create_digital_key': 'Tạo khoá điện tử',
      'no_digital_keys': 'Không có khoá điện tử',
      'create_digital_key': 'Tạo khoá điện tử',
      'edit_profile': 'Chỉnh Sửa Hồ Sơ',
      'security': 'Bảo Mật',
      'notifications': 'Thông Báo',
      'help_support': 'Trợ Giúp & Hỗ Trợ',
      'test_nfc': 'Kiểm Tra NFC HCE',
      'about': 'Về Ứng Dụng',
      'sign_out': 'Đăng Xuất',      
      // Settings
      'app_settings': 'Cài Đặt',
      'general': 'Chung',
      'account': 'Tài Khoản',
      'theme': 'Chủ Đề',
      'dark_mode': 'Chế Độ Tối',
      'light_mode': 'Chế Độ Sáng',
      
      // Location
      'my_location': 'Vị Trí Của Tôi',
      'current_location': 'Vị Trí Hiện Tại',
      'search_location': 'Tìm Kiếm Vị Trí',
      
      // Common
      'cancel': 'Hủy',
      'apply': 'Áp Dụng',
      'save': 'Lưu',
      'delete': 'Xóa',
      'edit': 'Chỉnh Sửa',
      'close': 'Đóng',
      'ok': 'Đồng Ý',
      'error': 'Lỗi',
      'success': 'Thành Công',
      'loading': 'Đang Tải...',
      'details': 'Chi Tiết',
      'copy_qr': 'Sao Chép QR',
      'share': 'Chia Sẻ',
      'add_sample_data': 'Thêm Dữ Liệu Mẫu',
      
      // Anomaly Detection
      'anomaly_detected': 'Phát Hiện Bất Thường',
      'anomaly_detected_desc': 'Phát hiện nỗ lực truy cập đáng ngờ',
      'anomaly_time': 'Bất Thường Thời Gian',
      'anomaly_location': 'Bất Thường Vị Trí',
      'anomaly_frequency': 'Bất Thường Tần Suất',
      'anomaly_high_risk': 'Rủi Ro Cao',
      'anomaly_medium_risk': 'Rủi Ro Trung Bình',
      'anomaly_low_risk': 'Rủi Ro Thấp',
      'anomaly_blocked': 'Truy Cập Bị Chặn',
      'anomaly_allowed': 'Truy Cập Được Cho Phép',
      'anomaly_review': 'Cần Xem Xét',
      'anomaly_details': 'Chi Tiết Bất Thường',
      'anomaly_reason': 'Lý Do',
      'anomaly_confidence': 'Độ Tin Cậy',
      'anomaly_severity': 'Mức Độ Nghiêm Trọng',
      'anomaly_timestamp': 'Thời Gian',
      'anomaly_location_info': 'Thông Tin Vị Trí',
      'anomaly_device_info': 'Thông Tin Thiết Bị',
      'anomaly_user_history': 'Lịch Sử Người Dùng',
      'anomaly_patterns': 'Mẫu Truy Cập',
      'anomaly_training': 'Đào Tạo Mô Hình',
      'anomaly_learning': 'Học Máy',
      'anomaly_claude_ai': 'Phân Tích Claude AI',
    },
  };

  Future<void> _loadLanguage() async {
    final prefs = await SharedPreferences.getInstance();
    final languageCode = prefs.getString(_languageKey) ?? 'en';
    _currentLocale = Locale(languageCode);
    notifyListeners();
  }

  Future<void> setLanguage(String languageCode) async {
    if (languageCode == _currentLocale.languageCode) return;

    _currentLocale = Locale(languageCode);
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_languageKey, languageCode);
    print('Language changed to: $languageCode'); // Debug log
    notifyListeners();
  }

  String translate(String key) {
    return _translations[_currentLocale.languageCode]?[key] ?? key;
  }

  List<Locale> get supportedLocales => [
    const Locale('en'),
    const Locale('vi'),
  ];
}
