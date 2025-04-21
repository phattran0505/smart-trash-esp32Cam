#include "flash.h"

void flashInit() {
  // Cài đặt flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW); // Tắt LED ban đầu
}

// Bật đèn flash với độ sáng điều chỉnh được
void setFlashBrightness(int brightness) {
  if (brightness > 0) {
    digitalWrite(FLASH_LED_PIN, HIGH);
  } else {
    digitalWrite(FLASH_LED_PIN, LOW);
  }
} 