# Anomaly Detection Test Guide

## Overview
This guide shows how to test the anomaly detection and notification system with dummy test cases.

## What's Included

### 1. Unit Tests (`test/anomaly_detection_test.dart`)
Run unit tests from command line:

```bash
cd software/smart_car_app
flutter test test/anomaly_detection_test.dart
```

**Test Cases:**
- **Test 1:** Normal access (Low Risk) → ALLOW
- **Test 2:** Unusual time + moderate distance (Medium Risk) → CONFIRM
- **Test 3:** Multiple risk factors (High Risk) → BLOCK
- **Test 4:** High frequency access (Medium Risk)
- **Test 5:** Very far location 500km (High Risk)

**Expected Output:**
```
✅ Test 1 PASSED - Normal access detected
✅ Test 2 PASSED - Unusual access detected
✅ Test 3 PASSED - Suspicious access detected
✅ Test 4 PASSED - High frequency access detected
✅ Test 5 PASSED - Far location detected
```

### 2. UI Test Screen (`lib/screen/test_anomaly_notifications.dart`)
Interactive test screen to see notifications in the app.

**How to add to your app:**
1. Update `lib/screen/profile.dart` or any navigation to include the test screen:

```dart
import 'package:smart_car_app/screen/test_anomaly_notifications.dart';

// In your navigation or menu:
ListTile(
  title: const Text('Test Anomaly Notifications'),
  onTap: () {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => const TestAnomalyNotificationsScreen(),
      ),
    );
  },
)
```

Or add directly in `lib/main.dart` as a route:

```dart
'/test-anomaly': (context) => const TestAnomalyNotificationsScreen(),
```

## Running Tests

### Option 1: Command Line (Unit Tests)
```bash
cd software/smart_car_app
flutter test test/anomaly_detection_test.dart -v
```

### Option 2: Run App with UI Tests
```bash
cd software/smart_car_app
flutter run
# Navigate to Test Anomaly Notifications screen
```

## Test Scenarios

| Test | Risk Level | Scenario | Expected Action |
|------|-----------|----------|-----------------|
| Test 1 | 🟢 Low | Normal morning access, 2.5km away, 1 access/hour | ✅ ALLOW |
| Test 2 | 🟡 Medium | 2 AM access, 45km away, 2 accesses/hour | ⚠️ CONFIRM |
| Test 3 | 🔴 High | 3 AM, Sunday, 250km away, 9 accesses/hour | 🚨 BLOCK |
| Test 4 | 🟡 Medium | Normal time, 1km away, 6 accesses/hour | ⚠️ CONFIRM |
| Test 5 | 🔴 High | 10 AM, 500km away (impossible distance) | 🚨 BLOCK |

## Notification Messages

**English (en):**
- Low: No notification
- Medium: "Confirm Access" / "Unusual access detected, please confirm"
- High: "Access Blocked" / "Suspicious activity detected, verification required"

**Vietnamese (vi):**
- Low: Không có thông báo
- Medium: "Xác nhận truy cập" / "Có dấu hiệu truy cập bất thường, vui lòng xác nhận"
- High: "Truy cập bị chặn" / "Phát hiện hoạt động đáng ngờ, yêu cầu xác minh"

## Expected Behavior

1. **When running unit tests:**
   - Each test creates a dummy AnomalyInput
   - Calls AnomalyScorer.evaluateAnomaly()
   - Verifies severity, action, and notification flags
   - Prints results to console

2. **When using UI test screen:**
   - Click a test button
   - See detailed results in card above
   - Notification appears as SnackBar
   - Console logs notification details
   - Messages respect current language setting

## Risk Score Calculation

The system calculates three independent risk factors (0.0-1.0):

```
Time Risk:
  - 0.0 (6-9am, 5-10pm)
  - 1.0 (other hours)

Location Risk:
  - 0.0 (< 10km from usual)
  - 0.5 (10-100km)
  - 1.0 (> 100km)

Frequency Risk:
  - 0.0 (≤ 3 accesses/hour)
  - 0.5 (4-8 accesses/hour)
  - 1.0 (> 8 accesses/hour)

Average Risk = (Time + Location + Frequency) / 3

Decision:
  - > 0.7 (67%) → HIGH RISK (BLOCK)
  - 0.5-0.7 → MEDIUM RISK (CONFIRM)
  - < 0.5 → LOW RISK (ALLOW)
```

## Troubleshooting

**Tests fail to compile:**
- Make sure all imports are correct
- Check that AnomalyScorer, Location, and other services exist

**Notifications not showing:**
- Check LanguageService.instance is initialized
- Verify NotificationService is properly imported
- Check that context is valid (use `if (mounted)`)

**Wrong language:**
- Notifications auto-detect language from LanguageService
- Change language in app settings and test again

## Next Steps

1. ✅ Run unit tests to verify logic
2. ✅ Add UI test screen to app navigation
3. ✅ Test in Flutter app for visual feedback
4. ✅ Verify notifications in both languages
5. ✅ Test with real access events

---

**Created:** April 2026
**Status:** Ready to test
