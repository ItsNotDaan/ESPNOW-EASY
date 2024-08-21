#include <Arduino.h>
#include <FastLED.h>
#include <ESPNOW-EASY.h>

// INIT the Button
#define BUTTON_PIN 6
bool buttonState = LOW;

// INIT the LED
#define DATA_PIN 5
#define NUM_LEDS 1
#define LED_TYPE WS2812B
CRGB leds[NUM_LEDS];

// Device type (MASTER or SLAVE)
#define DEVICE_TYPE MASTER

// Debug setting (DEBUG_ON or DEBUG_OFF)
#define DEBUG_SETTING DEBUG_ON

void setup()
{
  Serial.begin(115200);

  // Initialize the button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize the LED(s).
  FastLED.addLeds<LED_TYPE, DATA_PIN>(leds, NUM_LEDS);

  // Initialize ESP-NOW
  if (initESPNOW(DEVICE_TYPE, DEBUG_SETTING) == false)
  {
    Serial.println("ESP-NOW initialization failed");
    ESP.restart();
  }

  // Start the pairing process
  startPairingProcess();

  // Set setReceivedMessageOnMonitor to true to print the received message on the monitor
  setReceivedMessageOnMonitor(true);
}

void loop()
{
  // Main loop can be empty or include other tasks

  // Check the pairing mode status every 5 seconds if pairing mode is active.
  checkPairingModeStatus(5000);

  // Check if the button is pressed
  if (digitalRead(BUTTON_PIN) == LOW && buttonState == LOW)
  {
    // Send a message to the other device
    sendData(DATA, "Hello, Slave!", 42);

    // Set the button state to true to prevent multiple messages from being sent
    buttonState = HIGH;
  }
  buttonState = digitalRead(BUTTON_PIN); // Update the button state

}