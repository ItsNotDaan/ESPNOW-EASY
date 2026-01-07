#include <Arduino.h>
#include <ESPNOW-EASY.h>

// Device type
#define DEVICE_TYPE MASTER
#define DEBUG_SETTING DEBUG_ON

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("ESP-NOW Master - Multi-Slave Example");
    Serial.println("========================================\n");

    // Initialize ESP-NOW
    if (!initESPNOW(DEVICE_TYPE, DEBUG_SETTING))
    {
        Serial.println("ESP-NOW initialization failed!");
        ESP.restart();
    }

    Serial.println("Master initialized successfully");
    Serial.println("Ready to pair slaves...\n");

    // Start pairing first slave
    startPairingForNewSlave();

    setReceivedMessageOnMonitor(true);
}

void loop()
{
    // Check pairing status
    checkPairingModeStatus(5000);

    struct_status status = getESPNOWStatus();

    // Auto-pair up to 3 slaves for demo
    static uint8_t targetSlaves = 3;
    if (status.slavesCount < targetSlaves && !status.isPairingActive)
    {
        delay(2000); // Wait between pairings
        Serial.print("\nPairing slave #");
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
        }
        lastTimeoutCheck = millis();
    }

    // Print status every 10 seconds
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 10000)
    {
        printSlavesStatus();
        lastStatusPrint = millis();
    }

    // Broadcast message to all slaves every 5 seconds
    static unsigned long lastBroadcast = 0;
    if (millis() - lastBroadcast > 5000 && status.slavesCount > 0)
    {
        static uint8_t counter = 0;
        char message[32];
        snprintf(message, sizeof(message), "Broadcast #%d", counter++);

        Serial.print("\n>>> Broadcasting: ");
        Serial.println(message);
        broadcastData(DATA, message, counter);

        lastBroadcast = millis();
    }

    // Send targeted message to slave 0 every 7 seconds
    static unsigned long lastTargeted = 0;
    if (millis() - lastTargeted > 7000 && status.slavesCount > 0)
    {
        Serial.println("\n>>> Sending targeted message to Slave #0");
        sendDataToSlave(0, DATA, "Private msg", 42);

        lastTargeted = millis();
    }
}
