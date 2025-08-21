#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>

#define LED_PIN 45
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BME280  bme;
const char* ssid     = "CMCC-aquk";
const char* password = "wfv5kabh";
WebServer server(80);

float t, h, p;
bool  ledState = false;
unsigned long lastSensor = 0;
const unsigned long SENSOR_PERIOD = 1000;

void readSensor();
void oledShow();
void handleRoot();
void handleLedOn();
void handleLedOff();
void handleAjax();

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(18, 17);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println("OLED FAIL"); while (1); }
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 20);
  oled.println("Starting...");
  oled.display();

  if (!bme.begin(0x76)) { Serial.println("BME280 FAIL"); while (1); }

  WiFi.begin(ssid, password);
  Serial.print("Connecting Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print("\nIP: "); Serial.println(WiFi.localIP());

  server.on("/",        handleRoot);
  server.on("/led/on",  handleLedOn);
  server.on("/led/off", handleLedOff);
  server.on("/data",    handleAjax);
  server.begin();
}

void loop() {
  server.handleClient();
  if (millis() - lastSensor >= SENSOR_PERIOD) {
    readSensor();
    oledShow();
    lastSensor = millis();
  }
}

void readSensor() {
  t = bme.readTemperature();
  h = bme.readHumidity();
  p = bme.readPressure() / 100.0F;
}

void oledShow() {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.printf("T:%.1f C\n", t);
  oled.printf("H:%.1f %%\n", h);
  oled.printf("P:%.0f hPa", p);
  oled.display();
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width,initial-scale=1"/>
  <title>ESP32 Env Monitor</title>
  <style>
    body{font-family:Arial,Helvetica,sans-serif;text-align:center;background:#f2f2f2;margin:0;padding:40px 10px}
    .card{background:#fff;border-radius:12px;max-width:400px;margin:auto;padding:25px;box-shadow:0 4px 10px rgba(0,0,0,.15)}
    h1{font-size:28px;margin-bottom:20px;color:#333}
    .data{font-size:26px;margin:8px 0;color:#007acc}
    .btn-row{margin-top:25px}
    button{font-size:20px;padding:12px 28px;margin:6px;border:none;border-radius:8px;cursor:pointer;transition:.3s}
    .on{background:#4caf50;color:#fff}
    .off{background:#f44336;color:#fff}
    button:hover{opacity:.85}
  </style>
  <script>
    function fetchData(){
      fetch('/data').then(r=>r.json()).then(d=>{
        document.getElementById('t').innerText   = d.t.toFixed(1);
        document.getElementById('h').innerText   = d.h.toFixed(1);
        document.getElementById('p').innerText   = d.p.toFixed(0);
        document.getElementById('led').innerText = d.led ? "ON" : "OFF";
      });
    }
    setInterval(fetchData,1000);
    window.onload = fetchData;
  </script>
</head>
<body>
  <div class="card">
    <h1>Env Monitor & LED</h1>
    <div class="data">Temp: <span id="t">--</span> °C</div>
    <div class="data">Hum:  <span id="h">--</span> %</div>
    <div class="data">Pres: <span id="p">--</span> hPa</div>
    <div class="data">LED:  <span id="led">--</span></div>
    <div class="btn-row">
      <form action="/led/on"  method="get" style="display:inline"><button class="on">ON</button></form>
      <form action="/led/off" method="get" style="display:inline"><button class="off">OFF</button></form>
    </div>
  </div>
</body>
</html>)rawliteral";
  server.send(200, "text/html; charset=UTF-8", html);
}

void handleLedOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  server.sendHeader("Location", "/");
  server.send(302);
}

void handleLedOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  server.sendHeader("Location", "/");
  server.send(302);
}

void handleAjax() {
  String json = "{\"t\":" + String(t, 1) + ",\"h\":" + String(h, 1) + ",\"p\":" + String(p, 0) + ",\"led\":" + String(ledState ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}