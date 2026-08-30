#pragma once
#include <stdint.h>

namespace FSM {

// Tất cả các trạng thái của hệ thống
enum State {
  INIT,                         // Khởi tạo hệ thống
  IDLE,                         // Chờ đợi sự kiện (rảnh rỗi)
  PROVISIONING_WAIT_TAP,        // Phase A: Chờ quét NFC để provision
  PROVISIONING_SELECT_AID,      // Phase A: Đang SELECT AID (kết nối HCE app)
  PROVISIONING_EXCHANGE_KEYS,   // Phase A: Đang trao đổi public key
  PROVISIONING_STORE_CREDS,     // Phase A: Đang lưu credentials vào NVS
  AUTH_WAIT_AUTH0,              // Phase B (CCC tunnel): Chờ AUTH0 từ điện thoại
  AUTH_PROCESSING_AUTH0_STD,    // Phase B (CCC tunnel): Đang xử lý AUTH0 chuẩn (P1=0x11)
  AUTH_WAIT_AUTH1_RESP,         // Phase B (CCC tunnel): Đã gửi AUTH1, chờ AUTH1 response
  AUTH_SECURE_CHANNEL_READY,    // Phase B (CCC tunnel): Kênh bảo mật đã sẵn sàng

  // Legacy auth states (kept for backward compatibility during migration)
  AUTH_WAIT_CONNECT,            // Legacy: Chờ điện thoại kết nối BLE
  AUTH_HANDSHAKE,               // Phase B: Đang bắt tay (trao đổi ephemeral keys)
  AUTH_VERIFY_KEYS,             // Phase B: Đang xác minh chữ ký và tạo session keys
  AUTH_SESSION_READY,           // Phase B: Xác thực xong, session sẵn sàng
  UNLOCKING_CHECK_PROXIMITY,    // Unlock: Kiểm tra khoảng cách (gần xe)
  UNLOCKING_VERIFY_AUTH,        // Unlock: Kiểm tra session còn hợp lệ
  UNLOCKING_EXECUTE,            // Unlock: Thực hiện mở khóa (bật relay)
  UNLOCKING_COMPLETE,           // Unlock: Hoàn tất mở khóa
  ADMIN_MODE,                   // Chế độ admin (cấu hình/debug)
  ERROR_HANDLER                 // Xử lý lỗi
};

// Các sự kiện kích hoạt chuyển trạng thái
enum Event {
  SYSTEM_READY,                 // Hệ thống đã sẵn sàng
  TIMEOUT,                      // Hết thời gian chờ
  ERROR_OCCURRED,               // Có lỗi xảy ra
  RESET_REQUESTED,              // Yêu cầu reset hệ thống
  PROVISION_START,              // Bắt đầu provision (Phase A)
  NFC_CARD_DETECTED,            // Phát hiện thẻ/điện thoại NFC
  NFC_CARD_REMOVED,             // Thẻ/điện thoại NFC đã rời xa
  SELECT_AID_SUCCESS,           // SELECT AID thành công (kết nối HCE app)
  SELECT_AID_FAILED,            // SELECT AID thất bại
  KEYS_EXCHANGED,               // Đã trao đổi public key thành công
  KEYS_INVALID,                 // Public key không hợp lệ
  CREDENTIALS_STORED,           // Đã lưu credentials vào NVS
  BLE_CLIENT_CONNECTED,         // Điện thoại kết nối BLE
  BLE_CLIENT_DISCONNECTED,      // Điện thoại ngắt kết nối BLE

  // CCC tunnel step events
  BLE_AUTH0_RECEIVED,           // Đã nhận AUTH0
  BLE_AUTH0_RESP_SENT,          // Đã gửi AUTH0 response
  BLE_AUTH1_SENT,               // Đã gửi AUTH1 challenge/command
  BLE_AUTH1_RESP_RECEIVED,      // Đã nhận AUTH1 response
  BLE_EXCHANGE_RECEIVED,        // Đã nhận EXCHANGE
  BLE_EXCHANGE_RESP_SENT,       // Đã gửi EXCHANGE response
  BLE_CONTROL_FLOW_RECEIVED,    // Đã nhận CONTROL FLOW
  BLE_CONTROL_FLOW_RESP_SENT,   // Đã gửi CONTROL FLOW response

  CLIENT_HELLO_RECEIVED,        // Nhận CLIENT_HELLO từ điện thoại
  SERVER_HELLO_SENT,            // Đã gửi SERVER_HELLO cho điện thoại
  CLIENT_CONFIRM_RECEIVED,      // Nhận xác nhận từ điện thoại
  AUTH_VERIFIED,                // Xác thực thành công (chữ ký hợp lệ)
  AUTH_FAILED,                  // Xác thực thất bại
  UNLOCK_REQUESTED,             // Yêu cầu mở khóa từ app
  PROXIMITY_OK,                 // Điện thoại đủ gần (trong phạm vi)
  PROXIMITY_TOO_FAR,            // Điện thoại quá xa
  AUTH_SESSION_VALID,           // Session xác thực còn hợp lệ
  AUTH_SESSION_EXPIRED,         // Session xác thực đã hết hạn
  UNLOCK_EXECUTED,              // Đã thực hiện mở khóa (relay đã bật)
  ADMIN_COMMAND,                // Lệnh admin từ Serial/BLE
  FORCE_PROVISION_ON,           // Bật chế độ force provision
  FORCE_PROVISION_OFF,          // Tắt chế độ force provision
  CLEAR_KEYS,                   // Xóa keys đã lưu
  DIAGNOSTICS_REQUEST           // Yêu cầu hiển thị diagnostics
};

// Mã lỗi hệ thống
enum ErrorCode {
  NONE = 0,                     // Không có lỗi
  NFC_INIT_FAILED,              // Khởi tạo PN532 thất bại
  NFC_SAM_CONFIG_FAILED,        // Cấu hình SAM thất bại
  NFC_TIMEOUT,                  // Timeout chờ NFC card
  NFC_APDU_FAILED,              // Gửi APDU thất bại
  NFC_SELECT_AID_FAILED,        // SELECT AID thất bại
  BLE_INIT_FAILED,              // Khởi tạo BLE thất bại
  BLE_DISCONNECT_UNEXPECTED,    // BLE ngắt kết nối bất ngờ
  BLE_INVALID_DATA,             // Dữ liệu BLE không hợp lệ
  BLE_AUTH_FAILED,              // Xác thực BLE thất bại
  CRYPTO_SIGNATURE_INVALID,     // Chữ ký không hợp lệ
  CRYPTO_ECDH_FAILED,           // ECDH shared secret thất bại
  CRYPTO_HMAC_MISMATCH,         // HMAC không khớp
  INVALID_KEY_FORMAT,           // Format public key sai
  STORAGE_WRITE_FAILED,         // Ghi NVS thất bại
  STORAGE_READ_FAILED,          // Đọc NVS thất bại
  INVALID_STATE_TRANSITION,     // Chuyển trạng thái không hợp lệ
  NOT_PROVISIONED,              // Chưa provision (chưa có keyId)
  SESSION_NOT_READY,            // Session chưa sẵn sàng
  SESSION_EXPIRED,              // Session đã hết hạn
  AUTH_TIMEOUT                  // Timeout xác thực
};

// Context trạng thái - dữ liệu được mang theo qua các chuyển trạng thái
struct StateContext {
  // Dữ liệu phiên NFC (Phase A)
  uint8_t nfc_uid[4];                   // UID của thẻ NFC (4 bytes)
  uint8_t phone_pub_key[65];            // Public key điện thoại (65 bytes uncompressed)
  bool phone_key_valid;                 // Phone key đã được lưu và hợp lệ
  
  // Dữ liệu phiên BLE (Phase B)
  uint8_t ecu_ephemeral_pub[65];        // ECU ephemeral public key (bắt tay)
  uint8_t phone_ephemeral_pub[65];      // Phone ephemeral public key (bắt tay)
  uint8_t shared_secret[32];            // ECDH shared secret
  uint8_t session_enc_key[32];          // AES-256 encryption key
  uint8_t session_mac_key[32];          // HMAC key
  bool session_keys_ready;              // Session keys đã sẵn sàng
  
  // Thời gian và bộ đếm
  uint32_t retry_count;                 // Số lần thử lại
  uint32_t last_activity_ms;            // Thời điểm hoạt động cuối (ms)
  
  // Theo dõi lỗi
  ErrorCode last_error;                 // Lỗi gần nhất
  uint32_t error_count;                 // Số lỗi đã xảy ra
  
  void reset() {
    phone_key_valid = false;
    session_keys_ready = false;
    retry_count = 0;
    error_count = 0;
    last_error = ErrorCode::NONE;
  }
};

// Thông tin trạng thái cho debug
struct StateInfo {
  State current;                        // Trạng thái hiện tại
  State previous;                       // Trạng thái trước đó
  Event lastEvent;                      // Sự kiện cuối cùng đã xử lý
  ErrorCode lastError;                  // Lỗi cuối cùng
  uint32_t enter_time;                  // Thời điểm vào trạng thái hiện tại
  uint32_t transition_count;            // Số lần chuyển trạng thái
};

// Định nghĩa chuyển trạng thái
struct StateTransition {
  State from_state;                     // Từ trạng thái
  Event on_event;                       // Khi có sự kiện
  State to_state;                       // Chuyển sang trạng thái
  bool (*guard)(const StateContext& ctx);  // Điều kiện kiểm tra (optional)
};

// Callback hành động khi vào/ra trạng thái
typedef void (*StateAction)(StateContext& ctx);

// Hàm chuyển đổi sang chuỗi để hiển thị
const char* stateToString(State state);
const char* eventToString(Event event);
const char* errorToString(ErrorCode error);

} // namespace FSM