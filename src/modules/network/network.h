#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../config/config.h"

// Function declarations
void networkInit();
void networkUpdate();
boolean mqttReconnect();
bool sendImageMQTT(uint8_t *imageData, size_t imageSize);

// External MQTT client
extern WiFiClientSecure espClient;
extern PubSubClient mqttClient;

#endif // NETWORK_H 