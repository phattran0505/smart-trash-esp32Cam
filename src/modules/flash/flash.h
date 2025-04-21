#ifndef FLASH_H
#define FLASH_H

#include "driver/ledc.h"
#include <Arduino.h>
#include "../config/config.h"

// Function declarations
void flashInit();
void setFlashBrightness(int brightness);

#endif // FLASH_H 