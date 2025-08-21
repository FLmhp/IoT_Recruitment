#include <Adafruit_NeoPixel.h>
// Define the pins for the LEDs
#define LED_TX_PIN 43
#define LED_RX_PIN 44
#define RGB_LED_PIN 48
// Define the number of RGB LEDs (assuming 1 WS2812 LED)
#define NUM_RGB_LEDS 1
// Create an instance of the Adafruit_NeoPixel class
Adafruit_NeoPixel rgb_led = Adafruit_NeoPixel(NUM_RGB_LEDS, RGB_LED_PIN, NEO_GRB +
NEO_KHZ800);
void setup() {
  // Initialize the serial communication
  Serial.begin(115200);
  // Initialize the LED pins
  pinMode(LED_TX_PIN, OUTPUT);
  pinMode(LED_RX_PIN, OUTPUT);
  // Initialize the RGB LED
  rgb_led.begin();
  rgb_led.show(); // Initialize all pixels to 'off'
  // Start a test sequence
  Serial.println("Starting LED test sequence...");
}
void loop() {
  // Blink the TX LED
  digitalWrite(LED_TX_PIN, HIGH);
  delay(500);
  digitalWrite(LED_TX_PIN, LOW);
  delay(500);
  // Blink the RX LED
  digitalWrite(LED_RX_PIN, HIGH);
  delay(500);
  digitalWrite(LED_RX_PIN, LOW);
  delay(500);
  // Test the RGB LED
  testRGBLED();
}
void testRGBLED() {
  // Set RGB LED to red
  rgb_led.setPixelColor(0, rgb_led.Color(255, 0, 0)); // Red
  rgb_led.show();
  delay(500);
  // Set RGB LED to green
  rgb_led.setPixelColor(0, rgb_led.Color(0, 255, 0)); // Green
  rgb_led.show();
  delay(500);
  // Set RGB LED to blue
  rgb_led.setPixelColor(0, rgb_led.Color(0, 0, 255)); // Blue
  rgb_led.show();
  delay(500);
  // Turn off the RGB LED
  rgb_led.setPixelColor(0, rgb_led.Color(0, 0, 0)); // Off
  rgb_led.show();
  delay(500);
}