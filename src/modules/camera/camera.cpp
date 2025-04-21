#include "camera.h"
#include <Arduino.h>

void cameraInit() {
  // Tắt phát hiện brownout để tránh reset
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Khởi tạo camera
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
  
  // Điều chỉnh các thông số camera để cải thiện chất lượng ảnh (giống như ở cameraInit)
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

camera_fb_t* capturePhoto() {
  // Xóa bộ đệm camera bằng cách lấy và trả về vài frame
  for (int i = 0; i < 3; i++) {
    camera_fb_t *fb_flush = esp_camera_fb_get();
    if (fb_flush) {
      esp_camera_fb_return(fb_flush);
      delay(100);
    }
  }
  
  // Lấy ảnh
  camera_fb_t *fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("Chụp ảnh thất bại!");
    return NULL;
  }
  
  // Kiểm tra kích thước ảnh
  if (fb->len < 100) {
    Serial.println("Ảnh lỗi hoặc quá nhỏ!");
    esp_camera_fb_return(fb);
    return NULL;
  }
  
  Serial.printf("Ảnh đã chụp: %d bytes\n", fb->len);
  return fb;
} 