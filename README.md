# PRJCV – Xe điều khiển Bluetooth với RFID và cảm biến vật cản

Firmware C chạy trên ESP32, sử dụng ESP-IDF và FreeRTOS. Xe được mở/khóa bằng thẻ RFID RC522, nhận lệnh qua Bluetooth Classic SPP và dừng khi phát hiện vật cản hoặc lỗi đo khoảng cách.

## Chức năng

- Khi khởi động, xe ở trạng thái khóa và động cơ dừng.
- Quét UID hợp lệ để mở khóa và bật Bluetooth; quét lại để khóa và tắt Bluetooth.
- Điều khiển tiến, lùi, trái, phải, dừng và chọn 9 mức tốc độ.
- Khi mở khóa, cảm biến siêu âm đo khoảng cách. Khoảng cách ≤ 20 cm hoặc lỗi đo sẽ chặn toàn bộ lệnh di chuyển.
- Sau khi hết vật cản, cần gửi lệnh di chuyển mới. Xe cũng dừng khi client Bluetooth ngắt kết nối.
- Nếu tắt Bluetooth thất bại, xe vẫn khóa và cần reset để phục hồi.

## Phần cứng và môi trường

- ESP32 hỗ trợ Bluetooth Classic SPP. Chọn đúng module để các GPIO bên dưới không trùng chân dành cho phần cứng tích hợp; đặc biệt GPIO16 có thể dùng cho PSRAM trên một số module.
- Hai động cơ DC và mạch cầu H có ngõ EN/PWM, IN1, IN2 cho mỗi động cơ.
- Đầu đọc RFID RC522 và thẻ tương thích.
- Cảm biến siêu âm HC-SR05, nguồn cấp phù hợp và dây nối.
- ESP-IDF **5.5.5** (theo `sdkconfig` và `dependencies.lock`).
- Thư viện `abobija/rc522` **4.0.1** (theo lockfile).
- Thiết bị/app hỗ trợ Bluetooth Classic SPP để gửi ký tự, ví dụ ứng dụng terminal SPP trên Android.

ESP32-S2/S3/C3 không hỗ trợ Bluetooth Classic SPP như ESP32 dùng trong project này. Manifest hiện ghi IDF `>=4.1.0`, nhưng đó không phải cam kết tương thích: mã nguồn dùng driver và API mới, nên dùng phiên bản 5.5.5 đã lưu.

## Kết nối GPIO

Các chân dưới đây lấy từ `main/PRJCV.c` và `components/car/car.c`.

| Bộ phận | Tín hiệu | GPIO ESP32 |
| --- | --- | --- |
| Động cơ trái | EN/PWM | 5 |
| Động cơ trái | IN1 | 22 |
| Động cơ trái | IN2 | 21 |
| Động cơ phải | EN/PWM | 23 |
| Động cơ phải | IN1 | 19 |
| Động cơ phải | IN2 | 18 |
| RC522 | MISO | 16 |
| RC522 | MOSI | 13 |
| RC522 | SCLK | 4 |
| RC522 | SDA/SS (SPI chip select) | 15 |
| RC522 | RST | 2 |
| HC-SR05 | TRIG | 25 |
| HC-SR05 | ECHO | 26 |

Nối chung GND. Không cấp động cơ trực tiếp từ chân ESP32. Cấp RC522 theo thông số module (thường 3,3 V). Nếu ECHO của cảm biến ra 5 V, dùng chia áp hoặc chuyển mức trước GPIO26 vì ESP32 không chịu được tín hiệu 5 V. GPIO2/5/15 là chân strapping trên ESP32; kiểm tra trạng thái ngoại vi khi boot nếu board không khởi động/nạp được.

## Cấu trúc

```text
.
├── CMakeLists.txt          # Khai báo project PRJCV
├── sdkconfig              # Cấu hình ESP-IDF hiện tại
├── dependencies.lock      # Khóa phiên bản dependency
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── PRJCV.c            # Khởi tạo và xử lý khóa/mở xe
└── components/
    ├── bluetooth/         # Bluetooth Classic SPP
    ├── car/               # Lệnh, tốc độ, trạng thái và task điều khiển xe
    ├── hcsr05/            # Đo khoảng cách qua ngắt GPIO và queue
    ├── motor/             # GPIO chiều quay và LEDC PWM
    ├── rc/                # RC522, kiểm tra UID, gửi sự kiện RFID
    └── safety/            # Giám sát vật cản
```

## Build và nạp firmware

1. Cài ESP-IDF **5.5.5** theo [hướng dẫn Espressif](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32/get-started/index.html).
2. Mở terminal đã kích hoạt môi trường ESP-IDF, chuyển đến thư mục project chứa `CMakeLists.txt` gốc.
3. Build:

```sh
idf.py build
```

Target `esp32`, Bluetooth Classic, Bluedroid và SPP đã bật trong `sdkconfig`. ESP-IDF Component Manager sẽ tải thư viện RC522 vào `managed_components/`; không cần đưa thư mục này lên GitHub.

4. Nạp và xem log (thay `PORT` bằng cổng thật, ví dụ `COM5` trên Windows hoặc `/dev/ttyUSB0` trên Linux):

```sh
idf.py -p PORT flash monitor
```

Thoát monitor bằng `Ctrl+]`. Để xem/chỉnh cấu hình:

```sh
idf.py menuconfig
```

Cấu hình đang lưu: flash **2 MB**, bảng phân vùng **Single factory app** mặc định của ESP-IDF. Không cần `partitions.csv` riêng. Nếu build báo ứng dụng vượt kích thước phân vùng, chọn layout phù hợp với dung lượng flash thật của board trong `menuconfig`, rồi build lại.

## Sử dụng

1. Bật nguồn; xe mặc định khóa.
2. Quét thẻ có UID được chấp nhận. UID hiện được khai báo trong mảng `valid_uid` tại `components/rc/rc.c`; đổi mảng này theo thẻ của bạn rồi build/nạp lại.
3. Kết nối Bluetooth đến thiết bị **ESP32_BT**, mở kết nối SPP.
4. Gửi từng ký tự theo bảng dưới, không cần ký tự xuống dòng. Sau khi mở khóa, xe chỉ nhận lệnh di chuyển khi cảm biến đã có kết quả hợp lệ và đường phía trước thông thoáng.
5. Quét lại thẻ hợp lệ để khóa xe.

| Ký tự | Tác dụng |
| --- | --- |
| `w` | Tiến |
| `s` | Lùi |
| `a` | Rẽ trái |
| `d` | Rẽ phải |
| `x` | Dừng |
| `1` … `9` | Chọn mức tốc độ, mặc định 5 |

Tốc độ được đổi thành PWM theo `600 + level * 44`, dùng LEDC 10 bit, 5 kHz. Mức tốc độ chỉ thay đổi PWM; muốn dừng cần gửi `x`. Các ký tự khác bị bỏ qua. Xe giữ lệnh di chuyển cho đến khi có lệnh mới, bị khóa, bị cảm biến chặn hoặc mất kết nối; hiện chưa có thời gian tự dừng khi không nhận lệnh.

Ngưỡng dừng và chu kỳ task có thể chỉnh tại `components/safety/safety.h`: `SAFETY_STOP_CM = 20.0f`, `SAFETY_PERIOD_MS = 70`. Khoảng thời gian thực giữa các lần đo còn bao gồm thời gian đo/chờ cảm biến.

## Kiểm tra trên phần cứng

Bản đóng gói này chưa được build hoặc chạy trên ESP32 trong môi trường chuẩn bị repository. Nên nâng bánh xe khỏi mặt đất khi thử lần đầu và kiểm tra:

- Boot: động cơ dừng, Bluetooth chưa bật.
- Thẻ sai: không mở khóa; thẻ đúng: bật Bluetooth.
- Gửi lệnh hướng/tốc độ: kiểm tra chiều quay theo cách đấu motor thực tế.
- Vật cản trong 20 cm hoặc lỗi đo: xe dừng; hết chặn không tự chạy lại lệnh cũ.
- Ngắt kết nối Bluetooth hoặc quét thẻ để khóa: xe dừng.
- Quét khóa/mở nhiều lần: kiểm tra khởi tạo và tắt Bluetooth qua log.

## File đưa lên GitHub

Giữ `main/`, `components/`, `CMakeLists.txt`, `sdkconfig`, `dependencies.lock`, `README.md` và `.gitignore`. `sdkconfig.old` là cấu hình dự phòng, được bỏ qua cùng `build/` và `managed_components/`. Mã nguồn và cấu hình gốc được giữ nguyên khi chuẩn bị bản này.
