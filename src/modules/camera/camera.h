#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Arduino.h>
#include "../config/config.h"

// Camera model
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

// Function declarations
void cameraInit();
bool restartCamera();
camera_fb_t* capturePhoto();

// ESP32 PSRAM detection
extern bool psramFound();

#endif // CAMERA_H 