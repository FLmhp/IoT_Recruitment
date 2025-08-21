#define LED_PIN 45
#define INTERVAL 1000

void setup() {
  pinMode(LED_PIN, OUTPUT);   // set LED pin as output
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(INTERVAL);
  digitalWrite(LED_PIN, LOW);
  delay(INTERVAL);
}