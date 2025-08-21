#include <WiFi.h>

#define LED_PIN 45
#define INTERVAL 1000

const char* ssid     = "CMCC-aquk";
const char* password = "wfv5kabh";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    Serial.print("Local IP address: ");
    Serial.println(WiFi.localIP());
    pinMode(LED_PIN, OUTPUT);
  } else {
    Serial.println("\nFailed to connect Wi-Fi!");
  }
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(INTERVAL);
  digitalWrite(LED_PIN, LOW);
  delay(INTERVAL);
}