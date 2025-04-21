#ifndef CONFIG_H
#define CONFIG_H

// WiFi credentials
extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;

// MQTT Configuration
extern const char* MQTT_SERVER;
extern const int MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASSWORD;
extern const char* MQTT_TOPIC_IMAGE;

// MQTT packet size and chunk size
#define MQTT_CHUNK_SIZE 4096
#define MQTT_PUBLISH_DELAY 100

// Flash LED pin and configuration
#define FLASH_LED_PIN 4
#define FLASH_BRIGHTNESS 200

// LEDC configuration for PWM flash
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          FLASH_LED_PIN
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT 
#define LEDC_FREQUENCY          5000

#endif // CONFIG_H 