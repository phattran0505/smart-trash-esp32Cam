#ifndef STORAGE_H
#define STORAGE_H

#include "FS.h"
#include "SD_MMC.h"
#include <Arduino.h>

// Function declarations
void storageInit();
bool savePhotoToSD(uint8_t *imageData, size_t imageSize);
void storageUpdate();

#endif // STORAGE_H 