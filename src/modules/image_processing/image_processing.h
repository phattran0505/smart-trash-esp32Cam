#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include "mbedtls/base64.h"
#include <Arduino.h>

// Function declarations
String encodeImageToBase64(uint8_t *imageData, size_t imageSize);
void captureAndProcessPhoto();

#endif // IMAGE_PROCESSING_H 