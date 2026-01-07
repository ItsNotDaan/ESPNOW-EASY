#include <Arduino.h>
#include <ESPNOW-EASY.h>

// Device type
#define DEVICE_TYPE SLAVE
#define DEBUG_SETTING DEBUG_ON

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("ESP-NOW Slave Example");
    Serial.println("========================================\n");

    // Initialize ESP-NOW
    if (!initESPNOW(DEVICE_TYPE, DEBUG_SETTING))
    {
        Serial.println("ESP-NOW initialization failed!");
        ESP.restart();
    }

    Serial.println("Slave initialized successfully");
    Serial.println("Waiting for master to pair...\n");

    setReceivedMessageOnMonitor(true);
}

void loop()
{
    // Check pairing status
    checkPairingModeStatus(5000);

    struct_status status = getESPNOWStatus();

    // Print status every 10 seconds
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 10000)
    {
        Serial.println("\n--- Slave Status ---");
        Serial.print("Paired to Master: ");
        Serial.println(status.isPaired ? "Yes" : "No");
        Serial.print("Pairing Active: ");
        Serial.println(status.isPairingActive ? "Yes" : "No");
        Serial.println("-------------------\n");

        lastStatusPrint = millis();
    }

    // Send response to master every 8 seconds if paired
    static unsigned long lastResponse = 0;
    if (millis() - lastResponse > 8000 && status.isPaired)
    {
        static uint8_t responseCount = 0;
        char message[32];
        snprintf(message, sizeof(message), "Response #%d", responseCount++);

        Serial.print(">>> Sending to Master: ");
        Serial.println(message);
        broadcastData(DATA, message, responseCount);

        lastResponse = millis();
    }
}
