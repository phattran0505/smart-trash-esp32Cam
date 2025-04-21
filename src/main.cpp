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

#include <Arduino.h>
#include "modules/camera/camera.h"
#include "modules/network/network.h"
#include "modules/flash/flash.h"
#include "modules/storage/storage.h"
#include "modules/image_processing/image_processing.h"

void mainInit() {
  Serial.begin(115200);
  Serial.println("\nKhởi động ESP32-CAM...");

  // Khởi tạo các module
  flashInit();
  networkInit();
  storageInit();
  cameraInit();
}

void cameraUpdate() {
  // Kiểm tra và cập nhật trạng thái network
  networkUpdate();
  
  // Kiểm tra trạng thái camera
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
  
  // Chụp và xử lý ảnh
  captureAndProcessPhoto();
  
  // Đảm bảo tất cả dữ liệu được ghi xuống thẻ SD
  storageUpdate();
  
  delay(10000); // Chờ 10 giây trước khi chụp ảnh tiếp theo
}

void setup() {
  mainInit();
}

void loop() {
  cameraUpdate();
}