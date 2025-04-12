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

#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

// Flash LED pin (4 cho ESP32-CAM AI-Thinker)
#define FLASH_LED_PIN 4

const char *ssid = "H 08";
const char *password = "000000000";
const char *serverURL = "http://192.168.1.12:5000/predict";

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
    config.frame_size = FRAMESIZE_VGA;    // Giảm xuống VGA (640x480) thay vì SXGA
    config.jpeg_quality = 12;             // Tăng mức nén JPEG (số cao hơn = chất lượng thấp hơn)
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;    // Dùng VGA cho cả 2 trường hợp
    config.jpeg_quality = 15;             // Tăng mức nén JPEG
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
    s->set_contrast(s, 1);
    
    // Tăng độ bão hòa màu (giá trị từ -2 đến 2)
    s->set_saturation(s, 1);
    
    // Special FX (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_special_effect(s, 0);
    
    // Cài đặt AWB (Auto White Balance)
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

bool sendPhoto(uint8_t *imageData, size_t imageSize) {
  HTTPClient http;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to reconnect to WiFi. Cannot send image.");
    return false;
  }
  
  WiFiClient client;
  http.begin(client, serverURL);
  http.addHeader("Content-Type", "image/jpeg");
  
  // Tăng timeout và thêm retry logic
  http.setConnectTimeout(15000);  // 15 giây
  http.setTimeout(30000);        // 30 giây
  
  int httpResponseCode = http.POST(imageData, imageSize);
  
  if (httpResponseCode > 0) {
    Serial.printf("Server response code: %d\n", httpResponseCode);
    String response = http.getString();
    
    // Parse JSON response
    JsonDocument doc; // Adjust size as needed
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      Serial.println("JSON response received:");
      serializeJsonPretty(doc, Serial); // In ra đẹp, có xuống dòng
    } else {
      Serial.printf("Failed to parse JSON: %s\n", error.c_str());
    }
    
    http.end();
    return httpResponseCode == 200;
  } else {
    Serial.printf("Error sending image! Error code: %d\n", httpResponseCode);
    if (httpResponseCode == -1) {
      Serial.println("Could not connect to server. Check server URL and network connection.");
    }
    http.end();
    return false;
  }
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
    config.frame_size = FRAMESIZE_VGA;    // Giảm xuống VGA (640x480)
    config.jpeg_quality = 12;             // Tăng mức nén JPEG
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;    // Dùng VGA cho cả 2 trường hợp
    config.jpeg_quality = 15;             // Tăng mức nén JPEG
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
    s->set_contrast(s, 1);
    s->set_saturation(s, 1);
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
