#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>

// 引脚定义
#define LDR_PIN 4
constexpr uint8_t BUZZER_PIN = 14;   // 蜂鸣器 I/O（低电平触发）
constexpr uint8_t LED_PIN    = 45;   // LED引脚
constexpr uint8_t SDA_PIN    = 18;   // I2C数据线
constexpr uint8_t SCL_PIN    = 17;   // I2C时钟线
constexpr uint8_t OLED_RST   = -1;   // OLED复位引脚

// 显示参数
constexpr uint8_t SCREEN_W = 128;
constexpr uint8_t SCREEN_H = 64;
constexpr uint8_t OLED_MIN = 1;
constexpr uint8_t OLED_MAX = 255;

// 传感器读取周期（毫秒）
constexpr uint32_t SENSOR_PERIOD = 1000;
constexpr uint32_t OLED_PERIOD   = 100;
constexpr uint32_t MQTT_PERIOD   = 5000;

// 环境光传感器平滑参数
float ldrSmooth = 0;
const float alpha = 0.05;

// 温度报警阈值
constexpr float TEMP_ALARM = 31.0;

// WiFi和MQTT配置
const char* ssid        = "CMCC-aquk";
const char* password    = "wfv5kabh";
const char* mqtt_server = "n7e64e62.ala.cn-hangzhou.emqxsl.cn";
constexpr uint16_t mqtt_port = 8883;
const char* mqtt_user   = "admin";
const char* mqtt_pass   = "admin";
const char* mqtt_client = "ESP32-S3-Client-001";
const char* topic_pub   = "esp32s3/data";
const char* topic_sub   = "esp32s3/command";

// 证书
static const char caCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

// 传感器数据结构
typedef struct {
    float t;  // 温度
    float h;  // 湿度
    float p;  // 气压
} EnvData_t;

// 全局对象
Adafruit_BME280  bme;
Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, OLED_RST);
WiFiClientSecure net;
PubSubClient     mqtt(net);
WebServer        web(80);

// 全局变量
QueueHandle_t xQueueEnv = NULL;
SemaphoreHandle_t xLedMutex = NULL;
bool ledState = false;

// 任务函数声明
void tSensor(void*);
void tOled(void*);
void tMqtt(void*);
void tWeb(void*);
void tAutoBrightness(void*);
void tBuzzer(void*);
void mqttCallback(char*, byte*, unsigned int);
String makeWebPage();
void webHandleRoot();
void webHandleLedOn();
void webHandleLedOff();
void webHandleAjax();

void setup() {
    Serial.begin(115200);
    
    // 初始化引脚
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // 蜂鸣器初始化（低电平触发，先拉高关闭）
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
    
    // 环境光传感器初始化
    analogReadResolution(12);
    analogSetPinAttenuation(LDR_PIN, ADC_11db);
    
    // 创建队列和互斥量
    xQueueEnv = xQueueCreate(1, sizeof(EnvData_t));
    xLedMutex = xSemaphoreCreateMutex();
    
    // 初始化I2C总线
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // 初始化BME280传感器
    Serial.println("Initializing BME280 sensor...");
    if (!bme.begin(0x76)) { 
        Serial.println("ERROR: BME280 initialization failed!");
        while (1); 
    }
    Serial.println("BME280 sensor initialized successfully.");
    
    // 初始化OLED显示屏
    Serial.println("Initializing OLED display...");
    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println("ERROR: OLED initialization failed!");
        while (1); 
    }
    Serial.println("OLED display initialized successfully.");
    
    // 显示启动信息
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setTextSize(2);
    oled.setCursor(0, 20);
    oled.println("Booting...");
    oled.display();
    
    // 连接WiFi
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500); 
        Serial.print("."); 
    }
    Serial.println(" WiFi connected successfully.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP().toString());
    
    // 配置MQTT
    net.setCACert(caCert);
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(mqttCallback);
    
    // 配置Web服务器路由
    web.on("/", webHandleRoot);
    web.on("/led/on", webHandleLedOn);
    web.on("/led/off", webHandleLedOff);
    web.on("/data", webHandleAjax);
    web.begin();
    
    // 配置OTA
    ArduinoOTA.setHostname("esp32s3");
    ArduinoOTA.setPassword("admin");
    ArduinoOTA.begin();
    Serial.println("OTA service started.");
    
    // 创建任务
    xTaskCreatePinnedToCore(tSensor,         "Sensor",   4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(tOled,           "Oled",     4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(tMqtt,           "Mqtt",     6144, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(tWeb,            "Web",      6144, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(tAutoBrightness, "AutoBrt",  2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(tBuzzer,         "Buzzer",   2048, NULL, 2, NULL, 1);
    
    // 删除setup和loop任务
    vTaskDelete(NULL);
}

void loop() {}

// 蜂鸣器任务
void tBuzzer(void*) {
    EnvData_t env;
    
    for (;;) {
        if (xQueuePeek(xQueueEnv, &env, 0) == pdTRUE && env.t > TEMP_ALARM) {
            tone(BUZZER_PIN, 2000, 200);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            noTone(BUZZER_PIN);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// 自动亮度调节任务
void tAutoBrightness(void*) {
    for (;;) {
        int raw = analogRead(LDR_PIN);
        ldrSmooth = alpha * raw + (1 - alpha) * ldrSmooth;
        uint8_t contrast = map(
            constrain((int)ldrSmooth, 0, 4095),
            0, 4095,
            OLED_MAX, OLED_MIN);
        oled.ssd1306_command(SSD1306_SETCONTRAST);
        oled.ssd1306_command(contrast);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 传感器读取任务
void tSensor(void*) {
    EnvData_t env;
    TickType_t xLast = xTaskGetTickCount();
    
    for (;;) {
        env.t = bme.readTemperature();
        env.h = bme.readHumidity();
        env.p = bme.readPressure() / 100.0F;
        xQueueOverwrite(xQueueEnv, &env);
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(SENSOR_PERIOD));
    }
}

// OLED显示任务
void tOled(void*) {
    EnvData_t env;
    TickType_t xLast = xTaskGetTickCount();
    
    for (;;) {
        if (xQueuePeek(xQueueEnv, &env, 0) == pdTRUE) {
            oled.clearDisplay();
            oled.setTextSize(2);
            oled.setCursor(0, 0);
            oled.printf("T:%.1f C\nH:%.1f %%\nP:%.0f hPa", env.t, env.h, env.p);
            oled.display();
        }
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(OLED_PERIOD));
    }
}

// MQTT通信任务
void tMqtt(void*) {
    EnvData_t env;
    TickType_t xLast = xTaskGetTickCount();
    
    for (;;) {
        if (!mqtt.connected()) {
            Serial.println("Connecting to MQTT broker...");
            if (mqtt.connect(mqtt_client, mqtt_user, mqtt_pass)) {
                Serial.println("MQTT connected successfully.");
                mqtt.subscribe(topic_sub);
                Serial.print("Subscribed to topic: ");
                Serial.println(topic_sub);
            } else {
                Serial.print("MQTT connection failed. Error code: ");
                Serial.println(mqtt.state());
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }
        
        mqtt.loop();
        
        if (xQueuePeek(xQueueEnv, &env, 0) == pdTRUE) {
            JsonDocument doc;
            doc["t"] = env.t;
            doc["h"] = env.h;
            doc["p"] = env.p;
            
            xSemaphoreTake(xLedMutex, portMAX_DELAY);
            doc["led"] = ledState;
            xSemaphoreGive(xLedMutex);
            
            char buf[128];
            serializeJson(doc, buf);
            bool published = mqtt.publish(topic_pub, buf);
            if (!published) {
                Serial.println("ERROR: Failed to publish MQTT message.");
            }
        }
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(MQTT_PERIOD));
    }
}

// Web服务器任务
void tWeb(void*) {
    for (;;) {
        web.handleClient();
        ArduinoOTA.handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// MQTT消息回调函数
void mqttCallback(char* t, byte* p, unsigned int l) {
    Serial.print("MQTT message received on topic: ");
    Serial.println(t);
    
    JsonDocument doc;
    p[l] = 0;
    DeserializationError error = deserializeJson(doc, (char*)p);
    
    if (error) {
        Serial.print("ERROR: Failed to parse JSON. Error: ");
        Serial.println(error.c_str());
        return;
    }
    
    const char* cmd = doc["led"];
    if (cmd) {
        xSemaphoreTake(xLedMutex, portMAX_DELAY);
        ledState = !strcmp(cmd, "on");
        digitalWrite(LED_PIN, ledState);
        xSemaphoreGive(xLedMutex);
        Serial.print("LED state changed to: ");
        Serial.println(ledState ? "ON" : "OFF");
    } else {
        Serial.println("WARNING: Unknown command received via MQTT.");
    }
}

// 生成Web页面
String makeWebPage() {
    return R"rawliteral(
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
    .btn-container{display:flex;justify-content:center;gap:10px}
    button{font-size:20px;padding:12px 28px;border:none;border-radius:8px;cursor:pointer}
    .on{background:#4caf50;color:#fff}.off{background:#f44336;color:#fff}button:hover{opacity:.85}
  </style>
  <script>
    function fetchData(){
      fetch('/data').then(r=>r.json()).then(d=>{
        document.getElementById('t').innerText=d.t.toFixed(1);
        document.getElementById('h').innerText=d.h.toFixed(1);
        document.getElementById('p').innerText=d.p.toFixed(0);
        document.getElementById('led').innerText=d.led?"ON":"OFF";
      });
    }
    setInterval(fetchData,1000);
    window.onload=fetchData;
  </script>
</head>
<body>
  <div class="card">
    <h1>Env & LED Control</h1>
    <div class="data">Temp:<span id="t">--</span>°C</div>
    <div class="data">Hum:<span id="h">--</span>%</div>
    <div class="data">Pres:<span id="p">--</span>hPa</div>
    <div class="data">LED:<span id="led">--</span></div>
    <div class="btn-container">
      <form action="/led/on" method="get"><button class="on">ON</button></form>
      <form action="/led/off" method="get"><button class="off">OFF</button></form>
    </div>
  </div>
</body>
</html>)rawliteral";
}

// Web页面处理函数
void webHandleRoot() {
    web.send(200, "text/html; charset=UTF-8", makeWebPage());
}

void webHandleLedOn() {
    xSemaphoreTake(xLedMutex, portMAX_DELAY);
    ledState = true;
    digitalWrite(LED_PIN, HIGH);
    xSemaphoreGive(xLedMutex);
    web.sendHeader("Location", "/");
    web.send(302);
}

void webHandleLedOff() {
    xSemaphoreTake(xLedMutex, portMAX_DELAY);
    ledState = false;
    digitalWrite(LED_PIN, LOW);
    xSemaphoreGive(xLedMutex);
    web.sendHeader("Location", "/");
    web.send(302);
}

void webHandleAjax() {
    EnvData_t env;
    JsonDocument doc;
    
    if (xQueuePeek(xQueueEnv, &env, 0) == pdTRUE) {
        doc["t"] = env.t;
        doc["h"] = env.h;
        doc["p"] = env.p;
    } else {
        doc["t"] = 0;
        doc["h"] = 0;
        doc["p"] = 0;
    }
    
    xSemaphoreTake(xLedMutex, portMAX_DELAY);
    doc["led"] = ledState;
    xSemaphoreGive(xLedMutex);
    
    String json;
    serializeJson(doc, json);
    web.send(200, "application/json", json);
}