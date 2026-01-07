#include <Arduino.h>
#include <ESPNOW-EASY.h>

// Device type
#define DEVICE_TYPE SLAVE
#define DEBUG_SETTING DEBUG_ON

// RGB LED pins (adjust for your board)
#define LED_R_PIN 25
#define LED_G_PIN 26
#define LED_B_PIN 27

// LED states
enum LEDState
{
    LED_UNPAIRED, // Red - Not paired
    LED_PAIRING,  // Blinking Yellow - Pairing in progress
    LED_PAIRED    // Green - Successfully paired
};

LEDState currentLEDState = LED_UNPAIRED;

void setLED(LEDState state)
{
    currentLEDState = state;

    switch (state)
    {
    case LED_UNPAIRED:
        // Red - Not paired
        digitalWrite(LED_R_PIN, HIGH);
        digitalWrite(LED_G_PIN, LOW);
        digitalWrite(LED_B_PIN, LOW);
        break;

    case LED_PAIRING:
        // Yellow - Pairing (will blink in loop)
        digitalWrite(LED_R_PIN, HIGH);
        digitalWrite(LED_G_PIN, HIGH);
        digitalWrite(LED_B_PIN, LOW);
        break;

    case LED_PAIRED:
        // Green - Paired
        digitalWrite(LED_R_PIN, LOW);
        digitalWrite(LED_G_PIN, HIGH);
        digitalWrite(LED_B_PIN, LOW);
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Initialize FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(50); // Set brightness (0-255)
    Serial.println("ESP-NOW Slave with LED Status");
    Serial.println("========================================");
    Serial.println("LED States:");
    Serial.println("  Red           = Not paired");
    Serial.println("  Yellow (blink)= Pairing...");
    Serial.println("  Green         = Paired");
    Serial.println("========================================\n");

    // Initialize ESP-NOW
    if (!initESPNOW(DEVICE_TYPE, DEBUG_SETTING))
    {
        Serial.println("ESP-NOW initialization failed!");
        ESP.restart();
    }

    Serial.println("Slave initialized - Waiting for master\n");

    setReceivedMessageOnMonitor(true);
}

void loop()
{
    checkPairingModeStatus(5000);

    struct_status status = getESPNOWStatus();

    // Update LED based on pairing status
    static bool wasPaired = false;
    static bool wasPairing = false;

    if (status.isPairingActive && !wasPairing)
    {
        setLED(LED_PAIRING);
        Serial.println("\n>>> LED: Pairing in progress (Yellow)\n");
        wasPairing = true;
    }
    else if (status.isPaired && !wasPaired)
    {
        setLED(LED_PAIRED);
        Serial.println("\n>>> LED: Successfully paired! (Green)\n");
        wasPaired = true;
        wasPairing = false;
    }
    else if (!status.isPaired && !status.isPairingActive && wasPaired)
    {
        setLED(LED_UNPAIRED);
        Serial.println("\n>>> LED: Connection lost (Red)\n");
        wasPaired = false;
        wasPairing = false;
    }

    // Blink LED during pairing
    static unsigned long lastBlink = 0;
    static bool blinkState = false;
    if (currentLEDState == LED_PAIRING && millis() - lastBlink > 500)
    {
        blinkState = !blinkState;
        leds[0] = blinkState ? CRGB::Yellow : CRGB::Black;
        FastLED.show();
        {
            Serial.println("\n--- Slave Status ---");
            Serial.print("Paired: ");
            Serial.println(status.isPaired ? "Yes" : "No");
            Serial.print("Pairing Active: ");
            Serial.println(status.isPairingActive ? "Yes" : "No");
            Serial.println("-------------------\n");

            lastStatusPrint = millis();
        }

        // Send heartbeat to master every 8 seconds if paired
        static unsigned long lastHeartbeat = 0;
        if (millis() - lastHeartbeat > 8000 && status.isPaired)
        {
            static uint8_t heartbeatCount = 0;
            broadcastData(DATA, "Heartbeat", heartbeatCount++);
            lastHeartbeat = millis();
        }
    }
