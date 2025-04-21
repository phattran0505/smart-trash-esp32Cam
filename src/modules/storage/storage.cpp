#include "storage.h"

void storageInit() {
  if (!SD_MMC.begin()) {
    Serial.println("Lỗi khi khởi động thẻ SD!");
    return;
  }
  Serial.println("Thẻ SD đã sẵn sàng!");
}

bool savePhotoToSD(uint8_t *imageData, size_t imageSize) {  
  String path = "/common_" + String(millis()) + ".jpg";
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

void storageUpdate() {
  // Đảm bảo tất cả dữ liệu được ghi xuống thẻ SD
  SD_MMC.end();
  delay(500);
  SD_MMC.begin();
} 