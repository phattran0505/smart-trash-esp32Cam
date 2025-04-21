#include "network.h"
#include "mbedtls/base64.h"

// MQTT clients
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Xử lý tự động kết nối lại MQTT khi mất kết nối
unsigned long lastReconnectAttempt = 0;

void networkInit() {
  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());

  espClient.setInsecure();
  
  // Thiết lập MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  // Kết nối MQTT
  Serial.println("Đang kết nối MQTT...");
  if (mqttClient.connect("ESP32CAM", MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("MQTT đã kết nối!");
  } else {
    Serial.print("MQTT kết nối thất bại, lỗi = ");
    Serial.println(mqttClient.state());
  }
}

void networkUpdate() {
  // Kiểm tra kết nối WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi mất kết nối! Đang kết nối lại...");
    WiFi.reconnect();
    delay(5000);
    return;
  }
  
  // Kiểm tra và duy trì kết nối MQTT
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Thử kết nối lại
      if (mqttReconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Xử lý gói tin MQTT nếu có
    mqttClient.loop();
  }
}

boolean mqttReconnect() {
  if (mqttClient.connect("ESP32CAM", MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("Đã kết nối lại MQTT");
    return mqttClient.connected();
  }
  return false;
}

// Hàm chuyển đổi ảnh sang định dạng base64
String encodeImageToBase64(uint8_t *imageData, size_t imageSize) {
  size_t outputLength = 0;
  mbedtls_base64_encode(NULL, 0, &outputLength, imageData, imageSize);
  
  char *output = (char *)malloc(outputLength);
  if (output == NULL) {
    return "";
  }
  
  mbedtls_base64_encode((unsigned char *)output, outputLength, &outputLength, imageData, imageSize);
  String base64Image = String(output);
  free(output);
  
  return base64Image;
}

// Gửi ảnh qua MQTT (sử dụng base64)
bool sendImageMQTT(uint8_t *imageData, size_t imageSize) {
  // Kiểm tra kết nối MQTT
  if (!mqttClient.connected()) {
    Serial.println("MQTT chưa kết nối! Đang thử kết nối lại...");
    if (!mqttReconnect()) {
      Serial.println("Kết nối MQTT thất bại, không thể gửi ảnh");
      return false;
    }
  }
  
  // Chuyển đổi ảnh sang base64
  Serial.println("Chuyển đổi ảnh sang base64...");
  String base64Image = encodeImageToBase64(imageData, imageSize);
  
  if (base64Image.length() == 0) {
    Serial.println("Mã hóa base64 thất bại!");
    return false;
  }
  
  // Gửi ảnh qua MQTT
  Serial.print("Gửi ảnh qua MQTT (kích thước base64: ");
  Serial.print(base64Image.length());
  Serial.println(" bytes)");
  
  // Chia thành nhiều phần nếu kích thước quá lớn
  const int chunk_size = MQTT_CHUNK_SIZE;
  int num_chunks = (base64Image.length() + chunk_size - 1) / chunk_size;
  
  Serial.printf("Chia ảnh thành %d phần để gửi\n", num_chunks);
  
  // Gửi thông tin về tổng số phần trước
  DynamicJsonDocument infoDoc(256);
  infoDoc["total_chunks"] = num_chunks;
  infoDoc["total_size"] = base64Image.length();
  infoDoc["timestamp"] = millis();
  
  String infoStr;
  serializeJson(infoDoc, infoStr);
  
  if (!mqttClient.publish("trash_classification/info", infoStr.c_str())) {
    Serial.println("Không thể gửi thông tin ảnh!");
    return false;
  }
  
  // Đợi 100ms để broker có thời gian xử lý
  delay(100);
  
  // Gửi từng phần của ảnh
  bool success = true;
  for (int i = 0; i < num_chunks; i++) {
    int start = i * chunk_size;
    int length = min(chunk_size, (int)base64Image.length() - start);
    
    String chunk = base64Image.substring(start, start + length);
    String chunk_topic = String(MQTT_TOPIC_IMAGE) + "/chunk/" + String(i);
    
    Serial.printf("Đang gửi phần %d/%d (kích thước: %d bytes)...\n", i+1, num_chunks, length);
    
    if (!mqttClient.publish(chunk_topic.c_str(), chunk.c_str())) {
      Serial.printf("Không thể gửi phần %d!\n", i+1);
      success = false;
      break;
    }
    
    // Đợi một lúc giữa các lần gửi để tránh quá tải
    yield();
    delay(MQTT_PUBLISH_DELAY);
  }
  
  // Gửi thông báo hoàn thành
  if (success) {
    mqttClient.publish("trash_classification/status", "complete");
    Serial.println("Ảnh đã được gửi thành công qua MQTT!");
  } else {
    Serial.println("Gửi ảnh qua MQTT thất bại!");
  }
  
  return success;
} 