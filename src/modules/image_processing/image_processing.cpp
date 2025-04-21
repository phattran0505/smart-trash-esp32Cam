#include "image_processing.h"
#include "../camera/camera.h"
#include "../flash/flash.h"
#include "../storage/storage.h"
#include "../network/network.h"

void captureAndProcessPhoto() {
  // Bật đèn flash trước khi chụp với độ sáng cao
  setFlashBrightness(FLASH_BRIGHTNESS);
  
  // Thời gian đợi cho đèn flash để phòng tối tự điều chỉnh
  delay(800);
  
  // Lấy ảnh
  camera_fb_t *fb = capturePhoto();
  
  // Tắt đèn flash sau khi chụp
  setFlashBrightness(0);
  
  if (!fb) {
    return;
  }
  
  // Lưu ảnh vào SD card
  savePhotoToSD(fb->buf, fb->len);
  sendImageMQTT(fb->buf, fb->len);
  
  // Giải phóng bộ nhớ
  esp_camera_fb_return(fb);
} 