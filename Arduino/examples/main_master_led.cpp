#include <Arduino.h>
#include <ESPNOW-EASY.h>
#include <FastLED.h>

// Device type
#define DEVICE_TYPE MASTER
#define DEBUG_SETTING DEBUG_ON

// FastLED configuration
#define LED_PIN 5        // Data pin for LED
#define NUM_LEDS 1       // Number of LEDs
#define LED_TYPE WS2812B // LED type (WS2812B, NEOPIXEL, etc.)
#define COLOR_ORDER GRB  // Color order

CRGB leds[NUM_LEDS];

// LED color based on slave count using gradient
void setLEDColor(uint8_t slaveCount)
{
    // Create a gradient effect based on slave count
    // Cycles through different colors as more slaves are added
    uint8_t hue = slaveCount * 25; // 0-255 hue range (0=red, ~85=green, ~170=blue)

    // CHSV means Hue, Saturation, Value.
    leds[0] = CHSV(hue, 255, 255); // Full saturation and brightness
    FastLED.show();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Initialize FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(50); // Set brightness (0-255)

    // Start with red (no slaves)
    setLEDColor(0);

    Serial.println("\n========================================");
    Serial.println("ESP-NOW Master with LED Status");
    Serial.println("========================================");
    Serial.println("LED Colors:");
    Serial.println("  Color gradient based on slave count");
    Serial.println("  0 slaves  = Red");
    Serial.println("  3 slaves  = Yellow/Green");
    Serial.println("  6 slaves  = Cyan");
    Serial.println("  10 slaves = Purple/Pink");
    if (!initESPNOW(DEVICE_TYPE, DEBUG_SETTING))
    {
        Serial.println("ESP-NOW initialization failed!");
        ESP.restart();
    }

    Serial.println("Master initialized - Ready to pair\n");

    // Start pairing first slave
    startPairingForNewSlave();

    setReceivedMessageOnMonitor(true);
}

void loop()
{
    checkPairingModeStatus(5000);

    struct_status status = getESPNOWStatus();

    // Update LED based on slave count
    static uint8_t lastSlaveCount = 0;
    if (status.slavesCount != lastSlaveCount)
    {
        setLEDColor(status.slavesCount);
        Serial.print("\n>>> LED updated: ");
        Serial.print(status.slavesCount);
        Serial.println(" slaves paired\n");
        lastSlaveCount = status.slavesCount;
    }

    // Auto-pair up to 4 slaves
    static uint8_t targetSlaves = 4;
    if (status.slavesCount < targetSlaves && !status.isPairingActive)
    {
        delay(2000);
        Serial.print("Pairing slave #");
        Serial.println(status.slavesCount);
        startPairingForNewSlave();
    }

    // Check for inactive slaves and remove them (30 second timeout)
    static unsigned long lastTimeoutCheck = 0;
    if (millis() - lastTimeoutCheck > 15000) // Check every 15 seconds
    {
        uint8_t removed = removeInactiveSlaves(30000); // 30 second timeout
        if (removed > 0)
        {
            Serial.print("\n[TIMEOUT] Removed ");
            Serial.print(removed);
            Serial.println(" inactive slave(s)");
            // LED will update automatically in next loop via lastSlaveCount check
        }
        lastTimeoutCheck = millis();
    }

    // Print status every 15 seconds
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 15000)
    {
        printSlavesStatus();
        lastStatusPrint = millis();
    }

    // Broadcast to all slaves every 5 seconds
    static unsigned long lastBroadcast = 0;
    if (millis() - lastBroadcast > 5000 && status.slavesCount > 0)
    {
        static uint8_t counter = 0;
        broadcastData(DATA, "Heartbeat", counter++);
        lastBroadcast = millis();
    }
}
