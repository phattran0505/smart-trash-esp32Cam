//
//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//
//
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SD_MMC.h"
#include "driver/ledc.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

// Flash LED pin (4 cho ESP32-CAM AI-Thinker)
#define FLASH_LED_PIN 4

// WiFi credentials
const char *ssid = "AEPTIT";
const char *password = "20242024";

// MQTT Configuration
const char* mqtt_server = "192.168.1.13";  // MQTT broker IP
const int mqtt_port = 1883;                // MQTT port
const char* mqtt_user = "";                // MQTT username (if required)
const char* mqtt_password = "";            // MQTT password (if required)
const char* mqtt_topic = "esp32/camera";   // MQTT topic for sending images

WiFiClient espClient;
PubSubClient client(espClient);

// Cài đặt độ sáng LED (0-255) - tăng từ 50 lên 200
#define FLASH_BRIGHTNESS 200

// Cấu hình LEDC cho PWM đèn flash
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          FLASH_LED_PIN
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // 8-bit độ phân giải cho duty cycle (0-255)
#define LEDC_FREQUENCY          5000 // Tần số 5KHz

void setup() {
  Serial.begin(9600);
  Serial.println("\nKhởi động ESP32-CAM...");

  // Cài đặt flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW); // Tắt LED ban đầu
  
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  if (!SD_MMC.begin()) {
    Serial.println("Lỗi khi khởi động thẻ SD!");
    return;
  }
  Serial.println("Thẻ SD đã sẵn sàng!");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_1; // Dùng channel 1 để tránh xung đột
  config.ledc_timer = LEDC_TIMER_1;     // Dùng timer 1 để tránh xung đột
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SXGA;    // Use SXGA (1280x1024) for higher resolution
    config.jpeg_quality = 5;               // Better JPEG quality (lower value = higher quality)
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera thất bại! Lỗi 0x%x\n", err);
    return;
  }
  
  // Điều chỉnh các thông số camera để cải thiện chất lượng ảnh
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    // Điều chỉnh độ sáng (giá trị từ -2 đến 2)
    s->set_brightness(s, 1);
    
    // Tăng độ tương phản (giá trị từ -2 đến 2)
    s->set_contrast(s, 2);
    
    // Tăng độ bão hòa màu (giá trị từ -2 đến 2)
    s->set_saturation(s, 2);
    
    // Special FX (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_special_effect(s, 0);
    
    // Cài đặt AWB (Auto White Balance) - rất quan trọng cho màu sắc chính xác
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    
    // Tăng gain để cải thiện trong điều kiện ánh sáng yếu (0-511)
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 30);
    
    // Điều chỉnh tự động mức sáng (0-1023)
    s->set_aec_value(s, 500);
    
    // Bật tự động phơi sáng (từ -2 đến 2)
    s->set_ae_level(s, 0);
    
    // Cài đặt thời gian phơi sáng
    s->set_exposure_ctrl(s, 1);
    
    // Giảm nhiễu và tăng độ sắc nét
    s->set_denoise(s, 1);
    s->set_sharpness(s, 1);
    
    // Cài đặt BLC (Black Level Compensation)
    s->set_dcw(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    
    // Cài đặt WPC (White Pixel Correction)
    s->set_wpc(s, 1);
    
    // Cài đặt BPC (Black Pixel Correction)
    s->set_bpc(s, 1);
  }
  
  Serial.println("Camera đã sẵn sàng với các cài đặt tối ưu!");

  // Cấu hình MQTT
  client.setServer(mqtt_server, mqtt_port);
}

// Bật đèn flash với độ sáng điều chỉnh được sử dụng ESP-IDF LEDC
void setFlashBrightness(int brightness) {
  // Đơn giản hơn, chỉ dùng digitalWrite nếu gặp vấn đề với LEDC
  if (brightness > 0) {
    digitalWrite(FLASH_LED_PIN, HIGH);
  } else {
    digitalWrite(FLASH_LED_PIN, LOW);
  }
}

bool savePhotoToSD(uint8_t *imageData, size_t imageSize) {
  String path = "/photo_" + String(millis()) + ".jpg";
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Lưu ảnh thất bại!");
    return false;
  }
  
  size_t written = file.write(imageData, imageSize);
  if (written != imageSize) {
    Serial.printf("Lỗi khi ghi ảnh: chỉ ghi được %d/%d bytes\n", written, imageSize);
    file.close();
    return false;
  }
  
  file.flush(); // Đảm bảo dữ liệu được ghi xuống thẻ SD
  file.close();
  Serial.println("Ảnh đã được lưu vào SD: " + path);
  return true;
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32CAM-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

bool sendPhoto(uint8_t *imageData, size_t imageSize) {
  if (!client.connected()) {
    reconnect();
  }
  
  // Chia ảnh thành các phần nhỏ để gửi qua MQTT
  const size_t chunkSize = 1024;  // Kích thước mỗi phần
  size_t totalChunks = (imageSize + chunkSize - 1) / chunkSize;
  
  // Tạo JSON header
  StaticJsonDocument<200> header;
  header["total_chunks"] = totalChunks;
  header["image_size"] = imageSize;
  header["timestamp"] = millis();
  
  String headerStr;
  serializeJson(header, headerStr);
  
  // Gửi header
  if (!client.publish(mqtt_topic, headerStr.c_str())) {
    Serial.println("Failed to send header");
    return false;
  }
  
  // Gửi từng phần của ảnh
  for (size_t i = 0; i < totalChunks; i++) {
    size_t start = i * chunkSize;
    size_t end = min(start + chunkSize, imageSize);
    size_t currentChunkSize = end - start;
    
    // Tạo JSON cho mỗi phần
    StaticJsonDocument<200> chunk;
    chunk["chunk_index"] = i;
    chunk["total_chunks"] = totalChunks;
    chunk["data"] = base64::encode(&imageData[start], currentChunkSize);
    
    String chunkStr;
    serializeJson(chunk, chunkStr);
    
    if (!client.publish(mqtt_topic, chunkStr.c_str())) {
      Serial.println("Failed to send chunk");
      return false;
    }
    
    delay(10);  // Đợi một chút giữa các lần gửi
  }
  
  return true;
}

void captureAndProcessPhoto() {
  // Bật đèn flash trước khi chụp với độ sáng cao
  setFlashBrightness(FLASH_BRIGHTNESS);
  
  // Thời gian đợi cho đèn flash để phòng tối tự điều chỉnh
  delay(1000);
  
  // Xóa bộ đệm camera bằng cách lấy và trả về vài frame
  for (int i = 0; i < 5; i++) {
    camera_fb_t *fb_flush = esp_camera_fb_get();
    if (fb_flush) {
      esp_camera_fb_return(fb_flush);
      delay(100);
    }
  }
  
  // Đợi thêm một chút để cảm biến ổn định
  delay(300);
  
  // Lấy ảnh
  camera_fb_t *fb = esp_camera_fb_get();
  
  // Tắt đèn flash sau khi chụp
  digitalWrite(FLASH_LED_PIN, LOW);
  
  if (!fb) {
    Serial.println("Chụp ảnh thất bại!");
    return;
  }
  
  // Kiểm tra kích thước ảnh
  if (fb->len < 100) {
    Serial.println("Ảnh lỗi hoặc quá nhỏ!");
    esp_camera_fb_return(fb);
    return;
  }
  
  // Lưu và gửi ảnh
  savePhotoToSD(fb->buf, fb->len);
  sendPhoto(fb->buf, fb->len);
  
  // Giải phóng bộ nhớ
  esp_camera_fb_return(fb);
}

bool restartCamera() {
  // Giải phóng tài nguyên camera
  esp_camera_deinit();
  delay(100);
  
  // Khởi tạo lại camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_1;
  config.ledc_timer = LEDC_TIMER_1;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SXGA;    // Use SXGA (1280x1024) for higher resolution
    config.jpeg_quality = 5;               // Better JPEG quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo lại camera thất bại! Lỗi 0x%x\n", err);
    return false;
  }
  
  // Điều chỉnh các thông số camera để cải thiện chất lượng ảnh (giống như ở setup)
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 1);
    s->set_contrast(s, 2);
    s->set_saturation(s, 2);
    s->set_special_effect(s, 0);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 30);
    s->set_aec_value(s, 500);
    s->set_ae_level(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_denoise(s, 1);
    s->set_sharpness(s, 1);
    s->set_dcw(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    s->set_wpc(s, 1);
    s->set_bpc(s, 1);
  }
  
  Serial.println("Camera đã khởi động lại!");
  return true;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi mất kết nối! Đang kết nối lại...");
    WiFi.reconnect();
    delay(5000);
    return;
  }
  
  static int errorCount = 0;
  camera_fb_t *test_fb = esp_camera_fb_get();
  if (!test_fb) {
    errorCount++;
    Serial.println("Lỗi camera, thử khởi động lại...");
    if (errorCount > 3) {
      restartCamera();
      errorCount = 0;
    }
    delay(1000);
    return;
  }
  esp_camera_fb_return(test_fb);
  errorCount = 0;
  
  captureAndProcessPhoto();
  
  // Đảm bảo tất cả dữ liệu được ghi xuống thẻ SD
  SD_MMC.end();
  delay(500);
  SD_MMC.begin();
  
  delay(10000); // Chờ 10 giây trước khi chụp ảnh tiếp theo
}
