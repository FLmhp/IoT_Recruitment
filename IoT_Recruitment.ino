#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>

static const char caCert[] PROGMEM =
"-----BEGIN CERTIFICATE-----\n"
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
"-----END CERTIFICATE-----\n";

constexpr uint8_t  LED_PIN = 45;
constexpr uint16_t SCREEN_W = 128;
constexpr uint16_t SCREEN_H = 64;
constexpr int8_t   OLED_RESET = -1;
constexpr uint32_t SENSOR_PERIOD = 1000;
constexpr uint32_t OLED_PERIOD   = 1000;
constexpr uint32_t MQTT_PERIOD   = 5000;

const char* ssid     = "CMCC-aquk";
const char* password = "wfv5kabh";
const char* mqtt_server = "n7e64e62.ala.cn-hangzhou.emqxsl.cn";
constexpr uint16_t mqtt_port = 8883;
const char* mqtt_user = "admin";
const char* mqtt_pass = "admin";
const char* mqtt_client = "ESP32-S3-Client-001";
const char* topic_pub = "esp32/s3/data";
const char* topic_sub = "esp32/s3/command";

Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
Adafruit_BME280  bme;
WiFiClientSecure net;
PubSubClient     mqtt(net);
WebServer        web(80);

struct EnvData {
  float t = 0, h = 0, p = 0;
  void read() {
    t = bme.readTemperature();
    h = bme.readHumidity();
    p = bme.readPressure() / 100.0F;
  }
  StaticJsonDocument<256> toJsonDoc(bool led) const {
    StaticJsonDocument<256> doc;
    doc["t"] = t; doc["h"] = h; doc["p"] = p; doc["led"] = led;
    return doc;
  }
} env;

bool ledState = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(18, 17);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println("OLED FAIL"); while (1); }
  if (!bme.begin(0x76)) { Serial.println("BME FAIL");  while (1); }

  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 20);
  oled.println("Starting...");
  oled.display();

  WiFi.begin(ssid, password);
  Serial.print("Wi-Fi: ");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(WiFi.localIP());

  net.setCACert(caCert);
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback([](char* t, byte* p, unsigned int l){
    p[l] = 0;
    StaticJsonDocument<128> doc;
    deserializeJson(doc, (char*)p);
    const char* cmd = doc["led"];
    if (cmd) {
      ledState = !strcmp(cmd, "on");
      digitalWrite(LED_PIN, ledState);
    }
  });

  web.on("/", []{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width,initial-scale=1"/>
  <title>ESP32 Monitor</title>
  <style>
    body{font-family:Arial,Helvetica,sans-serif;text-align:center;background:#f2f2f2;margin:0;padding:40px 10px}
    .card{background:#fff;border-radius:12px;max-width:400px;margin:auto;padding:25px;box-shadow:0 4px 10px rgba(0,0,0,.15)}
    h1{font-size:28px;margin-bottom:20px;color:#333}
    .data{font-size:26px;margin:8px 0;color:#007acc}
    .btn-row{margin-top:25px}
    button{font-size:20px;padding:12px 28px;margin:6px;border:none;border-radius:8px;cursor:pointer}
    .on{background:#4caf50;color:#fff} .off{background:#f44336;color:#fff}
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
    <h1>Env & LED Control</h1>
    <div class="data">Temp: <span id="t">--</span> °C</div>
    <div class="data">Hum:  <span id="h">--</span> %</div>
    <div class="data">Pres: <span id="p">--</span> hPa</div>
    <div class="data">LED:  <span id="led">--</span></div>
    <div class="btn-row">
      <form action="/led/on"  method="get"><button class="on">ON</button></form>
      <form action="/led/off" method="get"><button class="off">OFF</button></form>
    </div>
  </div>
</body>
</html>)rawliteral";
    web.send(200, "text/html; charset=UTF-8", html);
  });
  web.on("/led/on",  []{ ledState = true;  digitalWrite(LED_PIN, HIGH); web.sendHeader("Location","/"); web.send(302); });
  web.on("/led/off", []{ ledState = false; digitalWrite(LED_PIN, LOW);  web.sendHeader("Location","/"); web.send(302); });
  web.on("/data", []{
    auto doc = env.toJsonDoc(ledState);
    String json;
    serializeJson(doc, json);
    web.send(200, "application/json", json);
  });
  web.begin();
}

void loop() {
  static uint32_t lastSensor = 0, lastOled = 0, lastMqtt = 0;
  uint32_t now = millis();

  if (!mqtt.connected()) {
    Serial.print("MQTT...");
    if (mqtt.connect(mqtt_client, mqtt_user, mqtt_pass)) {
      Serial.println("OK");
      mqtt.subscribe(topic_sub);
    } else {
      Serial.println(mqtt.state());
      delay(5000);
    }
  }
  mqtt.loop();
  web.handleClient();

  if (now - lastSensor >= SENSOR_PERIOD) {
    env.read();
    lastSensor = now;
  }
  if (now - lastOled >= OLED_PERIOD) {
    oled.clearDisplay();
    oled.setTextSize(2);
    oled.setCursor(0, 0);
    oled.printf("T:%.1f C\n", env.t);
    oled.printf("H:%.1f %%\n", env.h);
    oled.printf("P:%.0f hPa", env.p);
    oled.display();
    lastOled = now;
  }
  if (now - lastMqtt >= MQTT_PERIOD) {
    auto doc = env.toJsonDoc(ledState);
    char buf[128];
    serializeJson(doc, buf);
    mqtt.publish(topic_pub, buf);
    Serial.println("Pub: " + String(buf));
    lastMqtt = now;
  }
}