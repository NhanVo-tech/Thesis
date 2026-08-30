# 4. Hiện thực hệ thống phần ứng dụng

## 4.1. Vai trò của ứng dụng Flutter trong toàn hệ thống

Phần ứng dụng Flutter là lớp giao diện và điều phối nghiệp vụ ở phía người dùng trong hệ thống khóa xe thông minh. Nếu phần firmware trên vi điều khiển và dịch vụ native Android đảm nhiệm các thao tác cận phần cứng như NFC, BLE hoặc HCE, thì ứng dụng Flutter đóng vai trò trung tâm ở mức trải nghiệm: quản lý tài khoản, đồng bộ dữ liệu xe và khóa số với Firestore, thực hiện các thao tác provisioning, hỗ trợ kết nối BLE, lấy vị trí GPS, gửi dữ liệu vị trí đã đóng gói và hiển thị cảnh báo bất thường.

Trong phạm vi đồ án, mã nguồn Flutter không chỉ là một giao diện minh họa đơn thuần mà đã được tổ chức thành nhiều lớp dịch vụ và màn hình chuyên biệt. Các lớp này phối hợp với nhau để bảo đảm ứng dụng có thể hoạt động như một “trạm điều khiển” cho toàn bộ hệ thống, đồng thời vẫn đủ linh hoạt để kiểm thử từng thành phần độc lập.

## 4.2. Khởi tạo ứng dụng và điều phối vòng đời

Điểm vào chính của ứng dụng nằm trong `main.dart`. Tại đây, ứng dụng thực hiện chuỗi khởi tạo theo đúng thứ tự phụ thuộc của hệ thống:

- Đảm bảo Flutter binding đã sẵn sàng trước khi thao tác với các plugin.
- Khởi tạo lớp provisioning NFC để chuẩn bị cho luồng HCE và session provisioning.
- Nạp các cờ triển khai của PKE như `backgroundMode`, `fastTransaction` và `bondingEnforce`.
- Đăng ký listener handoff cho luồng chuyển giao giữa nền và giao diện chính.
- Khởi tạo dịch vụ nền theo cấu hình rollout.
- Khởi tạo Firebase trước khi xây dựng giao diện.
- Khởi tạo dịch vụ thông báo đẩy để ứng dụng sẵn sàng nhận thông tin từ hệ thống.

Từ cấu trúc này có thể thấy ứng dụng được thiết kế theo hướng “khởi tạo sớm các hạ tầng dùng chung”, thay vì đợi đến khi người dùng mở một màn hình cụ thể mới tạo các service. Cách làm này đặc biệt quan trọng với các module như NFC, BLE và background service, vì chúng cần trạng thái ổn định từ đầu phiên làm việc.

Trong `main.dart` còn có một entrypoint riêng `hceMain()`. Hàm này không sinh giao diện mà chỉ dùng để đăng ký channel phục vụ native HCE trong một isolate khác. Điều này phản ánh đúng đặc thù của phần HCE: xử lý ở nền và tách khỏi luồng UI chính để tránh xung đột vòng đời.

## 4.3. Kiến trúc giao diện và điều hướng chính

Sau khi khởi tạo, ứng dụng đi thẳng vào `Dashboard`. Trong mã nguồn hiện tại, đây là màn hình trung tâm để người dùng làm việc với xe, khóa số, vị trí và các chức năng thử nghiệm. Cách tổ chức này phù hợp với giai đoạn phát triển đồ án, khi mục tiêu là tạo một không gian thao tác tập trung cho toàn bộ nghiệp vụ thay vì phân tán qua nhiều màn hình phức tạp.

`Dashboard` sử dụng cấu trúc tab nội bộ với nhiều ngăn chức năng:

- Tab tổng quan xe và khóa số.
- Tab vị trí, kết nối BLE và đồng bộ GPS.
- Tab hồ sơ người dùng.
- Tab kiểm thử AI.

Ngoài ra, phần app bar còn mở rộng sang các màn hình nghiệp vụ và tiện ích khác như cài đặt, thông báo, kiểm thử Phase A/B và kiểm thử UWB/OOB. Cách bố trí này cho thấy ứng dụng vừa phục vụ người dùng cuối, vừa phục vụ mục đích kỹ thuật và kiểm thử trong quá trình hoàn thiện đồ án.

Về mặt giao diện, ứng dụng dùng một bộ widget tái sử dụng cho thẻ thống kê, thẻ xe, hộp thoại thao tác, thành phần vị trí và các thành phần khóa số. Việc tách riêng các widget giúp UI nhất quán, dễ mở rộng và giảm lặp mã ở những màn hình có cấu trúc giống nhau.

## 4.4. Hiện thực chức năng tài khoản và hồ sơ người dùng

Phần xác thực tài khoản được triển khai trong `service/auth.dart`, dựa trên Firebase Authentication và Google Sign-In. Mã nguồn hiện hỗ trợ hai phương thức đăng nhập chính:

- Đăng nhập bằng tài khoản Google.
- Đăng nhập bằng email và mật khẩu, chủ yếu phục vụ kiểm thử.

Sau khi đăng nhập thành công, ứng dụng tạo một bản ghi người dùng trong Firestore thông qua lớp `DatabaseMethods`. Thông tin được lưu bao gồm UID, email, tên hiển thị và ảnh đại diện. Cách làm này giúp ứng dụng có thể đồng bộ hồ sơ người dùng với dữ liệu xe và khóa số ở các collection khác.

Màn hình hồ sơ `ProfileScreen` lấy thông tin người dùng hiện tại từ Firebase Auth và hiển thị các số liệu tổng hợp như số xe và số khóa số hiện có. Ngoài phần thông tin cá nhân, màn hình này còn đóng vai trò là nơi người dùng thực hiện đăng xuất. Khi đăng xuất, ứng dụng chuyển về màn hình đăng nhập `LogIn`, qua đó hoàn tất vòng đời phiên làm việc.

Một thành phần hỗ trợ quan trọng trong nhóm màn hình này là `LanguageService`. Dịch vụ này quản lý ngôn ngữ giao diện theo kiểu singleton và phát tín hiệu thay đổi để các màn hình tự xây dựng lại khi người dùng đổi ngôn ngữ. Trong mã hiện tại, ứng dụng hỗ trợ tiếng Anh và tiếng Việt, đồng thời lưu lựa chọn vào SharedPreferences để giữ trạng thái giữa các lần mở app.

## 4.5. Đồng bộ dữ liệu xe và khóa số với Firestore

Lớp trung tâm của phần dữ liệu là `CarService`. Đây là cầu nối giữa giao diện Flutter và Firestore, chịu trách nhiệm quản lý hai nhóm dữ liệu chính:

- Collection `cars` để lưu thông tin xe.
- Collection `digital_keys` để lưu khóa số gắn với từng xe.

Tất cả thao tác ghi dữ liệu đều kiểm tra trạng thái người dùng hiện tại trước khi thực hiện. Điều này đảm bảo dữ liệu chỉ được tạo hoặc chỉnh sửa khi tài khoản đã xác thực hợp lệ.

Các hàm chính trong `CarService` được tổ chức theo đúng nghiệp vụ:

- `addCar()` tạo mới hồ sơ xe và gắn `ownerId` của người dùng hiện tại.
- `updateCar()` cập nhật các thuộc tính của xe và tự động thêm `updatedAt`.
- `deleteCar()` xóa xe và dọn sạch toàn bộ khóa số liên quan trước khi xóa bản ghi xe.
- `addDigitalKey()` và `updateDigitalKey()` quản lý vòng đời khóa số.
- `getUserCars()` và `getUserDigitalKeys()` trả về stream dữ liệu theo thời gian thực để giao diện tự cập nhật.

Điểm đáng chú ý trong cách hiện thực là ứng dụng không chỉ lấy dữ liệu một lần mà dùng listener thời gian thực từ Firestore. Nhờ đó, danh sách xe và khóa số trên Dashboard luôn phản ánh trạng thái mới nhất mà không cần người dùng làm mới thủ công.

Trong các stream này, dữ liệu được sắp xếp lại trên phía client theo thời điểm tạo, nhằm tránh phụ thuộc vào composite index phức tạp ở Firestore. Đây là một chi tiết nhỏ nhưng có ý nghĩa thực tế, vì nó giúp cấu hình dữ liệu đơn giản hơn trong giai đoạn phát triển đồ án.

Ngoài các thao tác CRUD cơ bản, `CarService` còn hiện thực một luồng provisioning riêng là `registerOwnerProvisioningRecord()`. Hàm này lưu đồng thời các thông tin quan trọng như `vehicle_id`, `owner_uid`, `device_pub_key`, `vehicle_pub_key`, danh sách slot sở hữu và dữ liệu provisioning đã viết. Ở nhánh ghi chính, dữ liệu được cập nhật vào document xe hiện có. Ở nhánh phụ, hệ thống cố gắng ghi thêm vào collection `Vehicles` như một registry chuẩn hóa, nhưng vẫn xử lý mềm trường hợp không đủ quyền ghi. Cách thiết kế này thể hiện rõ tư duy “best effort” để giảm lỗi hệ thống trong các môi trường triển khai khác nhau.

## 4.6. Hiện thực luồng NFC và provisioning bằng Master Card

Phần NFC được tách thành lớp dịch vụ `MasterCardProvisioningService` và màn hình luồng `MasterCardFlow`. Trong kiến trúc hiện tại, Flutter đảm nhiệm phần đọc thẻ, điều phối session và quản lý trạng thái, còn phần HCE thực thi thực tế trên native Android.

Luồng hiện thực của NFC gồm các bước chính:

1. Người dùng đưa master card vào vùng đọc.
2. Ứng dụng bật reader mode để giảm nhiễu giao diện hệ thống khi quét NFC.
3. `readMasterCard()` đọc nội dung NDEF từ thẻ và cố gắng trích xuất payload dạng văn bản.
4. Payload được phân tích theo định dạng JSON hoặc chuỗi CSV rút gọn, trong đó có `vid` và `msk`.
5. Dữ liệu hợp lệ sẽ được chuyển thành `MasterCardPayload` gồm `vehicleId` 8 byte và `masterSecret` 32 byte.

Sau khi đọc thành công, `activateHceSession()` truyền dữ liệu vào native service thông qua method channel `smartcar/mastercard`. Hàm này chỉ làm nhiệm vụ kích hoạt session trong một khoảng thời gian sống có hạn, mặc định là 60 giây, nhằm bảo đảm dữ liệu provisioning không tồn tại quá lâu trong bộ nhớ.

Ứng dụng cũng có cơ chế lưu payload chờ xử lý vào secure storage, nhờ đó nếu quá trình provisioning bị gián đoạn, người dùng vẫn có thể tiếp tục theo luồng đã khởi tạo trước đó. Điều này đặc biệt hữu ích khi người dùng thao tác NFC không thành công ngay từ lần đầu hoặc cần quay lại sau một bước trung gian.

Về mặt kỹ thuật, phần Dart trong dự án hiện chỉ giữ vai trò điều phối. Nhận xét này rất quan trọng khi viết báo cáo, vì nó phản ánh đúng phạm vi đã hiện thực: giao thức trao đổi APDU và HCE cốt lõi nằm ở native Android, còn Flutter là lớp điều khiển và giao diện người dùng.

## 4.7. Hiện thực BLE, PKE và dịch vụ nền

Luồng BLE trong ứng dụng được triển khai theo mô hình PKE, tức là xác thực xe bằng kết hợp giữa BLE, thông tin định danh thiết bị và cơ chế bắt tay bảo mật. Thành phần trung tâm là `PkeAuthOrchestrator`, nơi thực hiện phần lớn logic kết nối và xác thực.

Trong orchestrator này, ứng dụng thực hiện các công việc sau:

- Tìm thiết bị BLE theo địa chỉ đã lưu hoặc theo thiết bị được phát hiện trong lúc quét.
- Kết nối GATT với xe.
- Tìm service và hai characteristic `CCC_RX` và `CCC_TX`.
- Bật notify để nhận dữ liệu phản hồi từ thiết bị.
- Thực hiện luồng xác thực nhiều bước với cơ chế retry khi gặp lỗi tạm thời.

Quá trình xác thực có thiết kế rõ ràng về kiểm soát lỗi. Ứng dụng giới hạn số lần thử, sử dụng backoff tăng dần và xử lý riêng các trường hợp lỗi tạm thời như ngắt kết nối hoặc lỗi giao tiếp GATT. Cách này giúp hệ thống tránh lặp kết nối liên tục trong điều kiện môi trường không ổn định.

Về phía nền, `PkeBackgroundService` đảm nhiệm việc duy trì dịch vụ foreground trên Android. Dịch vụ này được cấu hình với notification channel riêng và chỉ kích hoạt trên nền tảng Android. Trong quá trình chạy, service định kỳ quét BLE, theo dõi kết quả scan, kích hoạt xác thực khi phát hiện thiết bị phù hợp và quản lý luồng handoff giữa nền và giao diện chính.

Một điểm đặc biệt trong thiết kế là sau khi xác thực BLE thành công, background service chưa dừng ngay mà còn giữ kết nối đủ lâu để chuyển tiếp sang bước UWB handoff. Điều này thể hiện tư duy phối hợp đa giao thức: BLE được dùng cho xác thực ban đầu, còn UWB được chuẩn bị như một lớp bổ sung để tăng độ tin cậy trong bước xác minh khoảng cách.

Để đảm bảo ứng dụng không bị hệ điều hành giới hạn quá mức, phần Dashboard còn có hai lớp kiểm tra khi khởi động:

- Kiểm tra quyền runtime cho BLE.
- Kiểm tra trạng thái exemption khỏi tối ưu pin Doze.

Nếu một trong hai điều kiện chưa đạt, ứng dụng hiển thị cảnh báo và cho phép người dùng mở màn hình cấp quyền hoặc trang cài đặt hệ thống. Đây là chi tiết rất thực tế, vì các luồng BLE nền trên Android thường bị ảnh hưởng trực tiếp bởi chính sách tiết kiệm pin và quyền truy cập thiết bị lân cận.

## 4.8. Hiện thực lấy vị trí GPS và đóng gói dữ liệu vị trí

Chức năng vị trí được xây dựng trong `GpsService` và được sử dụng mạnh nhất ở màn hình `LocationContent`. Đây là module phục vụ hai mục tiêu: hiển thị vị trí hiện tại cho người dùng và chuẩn bị dữ liệu để gửi đến thiết bị qua BLE.

`GpsService` thực hiện ba nhiệm vụ chính:

- Kiểm tra và xin quyền vị trí trên thiết bị.
- Lấy vị trí GPS hiện tại với độ chính xác cao.
- Chuyển tọa độ thành địa chỉ dễ đọc bằng reverse geocoding.

Sau khi lấy được vị trí, dịch vụ đóng gói dữ liệu thành một cấu trúc nhị phân cố định 32 byte gồm kinh độ, vĩ độ, độ cao, độ chính xác và timestamp. Sau đó dữ liệu này được mã hóa theo lớp service và gắn HMAC-SHA256 để bảo đảm tính toàn vẹn khi truyền qua BLE. Trong bản hiện tại của mã, hàm mã hóa đã được tách riêng theo kiến trúc an toàn, thuận lợi cho việc thay thế bằng thuật toán mạnh hơn khi cần hoàn thiện sâu hơn.

Trên giao diện, màn hình vị trí hỗ trợ một luồng rất rõ ràng:

- Người dùng nhập hoặc nạp sẵn địa chỉ MAC của thiết bị ESP32.
- Ứng dụng kiểm tra quyền BLE và thực hiện xác thực Phase B.
- Nếu xác thực thành công, GPS được lấy định kỳ và đóng gói để gửi sang thiết bị.
- Dữ liệu vị trí được tự động đồng bộ mỗi 30 giây.

Màn hình này cho thấy rõ triết lý hiện thực của đồ án: mỗi module không chỉ tồn tại độc lập mà còn tạo thành chuỗi từ xác thực đến trao đổi dữ liệu thực tế.

## 4.9. Phát hiện bất thường và thông báo cảnh báo

Để tăng tính an toàn của hệ thống, ứng dụng có một cụm dịch vụ phân tích bất thường xoay quanh `AnomalyDetectionService`. Đây là nơi tổng hợp dữ liệu từ nhiều detector khác nhau để đưa ra đánh giá cuối cùng cho một lần truy cập.

Cụm hiện thực này gồm ba tầng chính:

- `TimeAnomalyDetector` đánh giá tính bất thường theo thời điểm truy cập.
- `LocationAnomalyDetector` đánh giá bất thường theo vị trí và lịch sử truy cập.
- `AnomalyScorer` và `AIService` xử lý phần chấm điểm nâng cao hoặc hỗ trợ mô hình AI khi cần.

Khi nhận một `AccessEvent`, hệ thống phân tích đồng thời yếu tố thời gian và vị trí, sau đó tổng hợp thành mức độ rủi ro chung. Kết quả được ghi vào Firestore để phục vụ truy vết và thống kê sau này. Bên cạnh đó, ứng dụng còn có nhánh phân tích với AI để tạo ra quyết định giàu ngữ nghĩa hơn, bao gồm trạng thái cho phép, yêu cầu xác nhận hoặc chặn truy cập.

Phần thông báo cũng được tách riêng thành hai lớp:

- `NotificationService` dùng để hiển thị cảnh báo ngay trong ứng dụng bằng SnackBar hoặc hộp thoại.
- `PushNotificationService` chuẩn bị thông báo hệ thống để người dùng vẫn nhận được cảnh báo ngay cả khi ứng dụng không ở tiền cảnh.

Việc tách in-app notification và system notification cho thấy ứng dụng không chỉ dừng ở mức hiển thị thông tin mà đã quan tâm đến tính liên tục của cảnh báo trong môi trường di động thực tế.

## 4.10. Các màn hình kiểm thử và hỗ trợ phát triển

Ngoài các màn hình chính phục vụ người dùng, mã nguồn còn có nhiều màn hình kiểm thử chuyên dụng như `TestPhaseABScreen`, `AITestHarnessV2`, `TestUwbScreen`, `GpsTestScreen` và các màn hình test cảnh báo. Đây là phần rất hữu ích trong giai đoạn đồ án vì cho phép kiểm tra từng cấu phần một cách độc lập mà không cần đi qua toàn bộ quy trình vận hành.

Nhóm màn hình kiểm thử này có ý nghĩa thực tiễn ở ba mức:

- Giúp kiểm tra kết nối và logic xác thực BLE/NFC theo từng phase.
- Giúp đánh giá chất lượng thuật toán bất thường với các bộ dữ liệu mẫu.
- Giúp quan sát trạng thái GPS, UWB và cảnh báo theo thời gian thực.

Ở tầng giao diện, các màn hình này được truy cập trực tiếp từ Dashboard thông qua các nút hành động ở thanh công cụ. Vì vậy, ứng dụng vừa là sản phẩm dùng thử, vừa là bộ công cụ kiểm thử tích hợp cho toàn hệ thống.

## 4.11. Nhận xét tổng quát về mức độ hiện thực

Từ cấu trúc mã nguồn có thể khẳng định phần ứng dụng Flutter đã được hiện thực theo đúng hướng kiến trúc của hệ thống khóa xe thông minh, không phải là một giao diện mô phỏng đơn lẻ. Ứng dụng đã bao phủ đầy đủ các lớp chức năng quan trọng gồm:

- Khởi tạo và điều phối dịch vụ nền.
- Đăng nhập, hồ sơ người dùng và đa ngôn ngữ.
- Quản lý xe và khóa số trên Firestore theo thời gian thực.
- Provisioning NFC bằng master card và HCE session.
- Xác thực BLE/PKE và handoff sang UWB.
- Lấy vị trí GPS, đóng gói dữ liệu và đồng bộ sang thiết bị.
- Phân tích bất thường và phát cảnh báo.
- Các màn hình kiểm thử kỹ thuật phục vụ đánh giá hệ thống.

Nếu nhìn dưới góc độ đồ án, đây là một hiện thực có tính mô-đun cao, bám sát đặc tả kỹ thuật và đủ rõ ràng để tiếp tục mở rộng sang các giai đoạn hoàn thiện tiếp theo như tối ưu bảo mật, chuẩn hóa thuật toán mã hóa, hoặc tích hợp sâu hơn với native Android và phần cứng ngoài thực địa.

