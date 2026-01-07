#include <Arduino.h>
#include <ESPNOW-EASY.h>

// Device type (MASTER or SLAVE)
#define DEVICE_TYPE MASTER

// Debug setting (DEBUG_ON or DEBUG_OFF)
#define DEBUG_SETTING DEBUG_ON

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("ESP-NOW Multi-Slave Example");
  Serial.println("========================================\n");

  // Initialize ESP-NOW
  if (!initESPNOW(DEVICE_TYPE, DEBUG_SETTING))
  {
    Serial.println("ESP-NOW initialization failed!");
    ESP.restart();
  }

  Serial.println("ESP-NOW initialized successfully");
  Serial.println("Ready to pair slaves...\n");

  // Start pairing first slave
  startPairingForNewSlave();

  setReceivedMessageOnMonitor(true);
}

void loop()
{
  // Check pairing mode status
  checkPairingModeStatus(5000);

  struct_status status = getESPNOWStatus();

  // Auto-pair up to 3 slaves for demonstration
  static uint8_t targetSlaves = 3;
  if (status.slavesCount < targetSlaves && !status.isPairingActive)
  {
    delay(2000); // Short delay between pairings
    Serial.print("\nPairing next slave (#");
    Serial.print(status.slavesCount);
    Serial.println(")...\n");
    startPairingForNewSlave();
  }

  // Print detailed slave status every 15 seconds
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 15000)
  {
    printSlavesStatus();
    lastStatusPrint = millis();
  }

  // Broadcast message to all slaves every 5 seconds
  static unsigned long lastBroadcast = 0;
  if (millis() - lastBroadcast > 5000 && status.slavesCount > 0)
  {
    static uint8_t counter = 0;
    broadcastData(DATA, "Broadcast", counter++);

    Serial.print(">>> Broadcasted message #");
    Serial.println(counter);

    lastBroadcast = millis();
  }

  // Send targeted message to specific slave every 7 seconds
  static unsigned long lastTargeted = 0;
  if (millis() - lastTargeted > 7000 && status.slavesCount > 0)
  {
    // Send to slave 0
    sendDataToSlave(0, DATA, "Private msg", 42);
    Serial.println(">>> Sent private message to Slave #0");

    lastTargeted = millis();
  }
}