#include <WiFi.h>
#include <WebServer.h>

#define LED_PIN 45
const char* ssid     = "CMCC-aquk";
const char* password = "wfv5kabh";

WebServer server(80);
bool ledState = false;

String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 LED Control</title>
</head>
<body>
  <h1>ESP32 LED Control</h1>
  <p>State: <b>)rawliteral";
  html += (ledState ? "ON" : "OFF");
  html += R"rawliteral(</b></p>
  <form action="/led/on"  method="get"><button>ON</button></form><br>
  <form action="/led/off" method="get"><button>OFF</button></form>
</body>
</html>)rawliteral";
  return html;
}

void handleLedOn()  { ledState = true;  digitalWrite(LED_PIN, HIGH); server.send(200, "text/html; charset=UTF-8", getHTML()); }
void handleLedOff() { ledState = false; digitalWrite(LED_PIN, LOW);  server.send(200, "text/html; charset=UTF-8", getHTML()); }
void handleRoot()   { server.send(200, "text/html; charset=UTF-8", getHTML()); }

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWi-Fi connected\nIP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/led/on", handleLedOn);
  server.on("/led/off", handleLedOff);
  server.begin();
}

void loop() {
  server.handleClient();
}