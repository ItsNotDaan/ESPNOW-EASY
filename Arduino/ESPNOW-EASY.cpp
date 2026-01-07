#include <ESPNOW-EASY.h>

// Global variable to store the slave's and masters MAC address
uint8_t MasterMacAddress[6] = {0};
uint8_t SlavesMacAddresses[MAX_SLAVES][6] = {0}; // Array of slave MAC addresses
uint8_t slavesCount = 0;                         // Number of paired slaves
uint8_t BroadcastMacAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool receivedMessageOnMonitor = false;

// Global variable to store the pairing status
bool pairingMode = false;

uint8_t localPairingCycle = 0; // This is a variable that will store the local pairing cycle.

// Global variable to store the peer information
esp_now_peer_info_t peerInfo;

DeviceType deviceType;
DebugSetting debugSetting;
MessageType messageType;
PairingMode pairingModeState = PAIRING_CLOSED;

// Create 2 struct_message and 1 struct_pairing
struct_message sendingData;   // data to send
struct_message receivingData; // data received
struct_pairing pairingData;   // pairing data

// Array to track slaves
struct_slave_info slaves[MAX_SLAVES] = {0};

// Variable to track which slave is currently being paired
int8_t currentPairingSlaveId = -1;

/***************************************checkPairingModeStatus********************************************/
/// @brief This function will check if the pairing mode is active and reset the pairing process if the WAIT_TIME_MS has passed.
/// @param WAIT_TIME_MS Must be greater than 1000 milliseconds.
void checkPairingModeStatus(unsigned long WAIT_TIME_MS)
{
  static unsigned long lastEventTime = millis();
  static unsigned long EVENT_INTERVAL_MS;

  // Set the EVENT_INTERVAL_MS to 5 seconds if the WAIT_TIME_MS is less than 1 seconds.
  if (WAIT_TIME_MS < 1000)
  {
    EVENT_INTERVAL_MS = 5000;
  }
  else
  {
    EVENT_INTERVAL_MS = WAIT_TIME_MS;
  }

  // Check if the pairing mode is active
  while (pairingMode == true)
  {
    // Reset pairing if timeout has passed.
    if ((millis() - lastEventTime) > EVENT_INTERVAL_MS)
    {
      Serial.println("Pairing cycle timeout, restarting pairing process");

      lastEventTime = millis();

      // Reset the pairing process and start back at the beginning.
      if (deviceType == MASTER)
      {
        startPairingForNewSlave();
      }
      else
      {
        // For slave, just reset pairing mode to wait for master
        pairingMode = true;
        localPairingCycle = 1;
      }
    }
  }
}

/***************************************pairingProcessMaster********************************************/
/// @brief This function will process the pairing steps if the deviceType is MASTER and return the pairing status.
void pairingProcessMaster()
{
  /*CYCLE 2 OF 3 - RECEIVE THE SLAVE'S MAC ADDRESS AND SEND BACK "M-CYCLE-2/3" + LOCAL CYCLE TO THE SLAVE*/
  if (pairingData.pairingCycle == 1 && localPairingCycle == 2)
  {
    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Pairing cycle ");
      Serial.print(localPairingCycle);
      Serial.println(": Slave MAC Address received");
    }

    // Find next available slave slot
    if (currentPairingSlaveId == -1)
    {
      for (uint8_t i = 0; i < MAX_SLAVES; i++)
      {
        if (!slaves[i].isPaired)
        {
          currentPairingSlaveId = i;
          break;
        }
      }
    }

    if (currentPairingSlaveId == -1)
    {
      Serial.println("ERROR: No available slave slots!");
      pairingMode = false;
      pairingModeState = PAIRING_FULL;
      return;
    }

    // Save the MAC address of the slave
    memcpy(slaves[currentPairingSlaveId].macAddr, pairingData.macAddr, 6);
    memcpy(SlavesMacAddresses[currentPairingSlaveId], pairingData.macAddr, 6);

    // DON'T remove broadcast peer - keep it for discovering new slaves
    // esp_now_del_peer(BroadcastMacAddress);  // REMOVED FOR MULTI-SLAVE SUPPORT

    // Add the new peer with the slave's MAC address
    memcpy(peerInfo.peer_addr, slaves[currentPairingSlaveId].macAddr, 6);
    peerInfo.channel = 0; // Use the current Wi-Fi channel
    peerInfo.encrypt = false;

    // Add the peer
    esp_now_add_peer(&peerInfo);

    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Added slave #");
      Serial.print(currentPairingSlaveId);
      Serial.println(" as new peer (broadcast peer kept active)");
    }

    // Send Local Pairing Cycle 2 to the slave together with the pairing text.
    pairingData.msgType = PAIRING;
    pairingData.pairingCycle = localPairingCycle;
    strcpy(pairingData.pairingText, "M-CYCLE-2/3"); // Master in cycle 2 of 3.
    memcpy(pairingData.macAddr, MasterMacAddress, 6);

    // Send the OK response to the slave
    esp_now_send(slaves[currentPairingSlaveId].macAddr, (const uint8_t *)&pairingData, sizeof(pairingData));

    if (debugSetting == DEBUG_ON)
    {
      Serial.println("Pairing cycle 2: Slave MAC Address saved and M-CYCLE-2/3 sent to slave");
    }

    // Add one to the localPairingCycle to 3.
    localPairingCycle++;
  }

  /*CYCLE 3 OF 3 - RECEIVE THE FINAL RESPONSE FROM THE SLAVE AND SEND BACK "M-CYCLE-3/3" + LOCAL CYCLE TO THE SLAVE AND SET PAIRING MODE TO "PAIRED"*/
  else if (pairingData.pairingCycle == 2 && localPairingCycle == 3)
  {
    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Pairing cycle ");
      Serial.print(localPairingCycle);
      Serial.println(": Final response received from slave");
    }

    // Send last pairing message to the slave
    pairingData.msgType = PAIRING;
    pairingData.pairingCycle = localPairingCycle;
    strcpy(pairingData.pairingText, "M-CYCLE-3/3"); // Master in cycle 3 of 3.
    memcpy(pairingData.macAddr, MasterMacAddress, 6);
    esp_now_send(slaves[currentPairingSlaveId].macAddr, (const uint8_t *)&pairingData, sizeof(pairingData));

    // Mark slave as paired and active
    slaves[currentPairingSlaveId].isPaired = true;
    slaves[currentPairingSlaveId].isActive = true;
    slaves[currentPairingSlaveId].slaveId = currentPairingSlaveId;
    slaves[currentPairingSlaveId].lastSeen = millis();
    snprintf(slaves[currentPairingSlaveId].slaveName, 16, "Slave_%d", currentPairingSlaveId);
    slavesCount++;

    // Set pairing mode to false.
    pairingMode = false;
    pairingModeState = (slavesCount >= MAX_SLAVES) ? PAIRING_FULL : PAIRING_CLOSED;
    currentPairingSlaveId = -1; // Reset for next pairing

    if (debugSetting == DEBUG_ON)
    {
      Serial.println("Pairing cycle 3: Final response M-CYCLE-3/3 sent to slave");
      Serial.print("Pairing complete! Total slaves paired: ");
      Serial.println(slavesCount);
    }
  }
  return;
}

/***************************************pairingProcessSlave********************************************/
/// @brief This function will process the pairing steps if the deviceType is SLAVE and return the pairing status.
void pairingProcessSlave()
{
  /*CYCLE 1 OF 3 - RECEIVE THE MASTER'S MAC ADDRESS AND SEND BACK "S-CYCLE-1/3" + LOCAL CYCLE TO THE MASTER*/
  if (pairingData.pairingCycle == 1 && localPairingCycle == 1)
  {
    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Pairing cycle ");
      Serial.print(localPairingCycle);
      Serial.println(": Master MAC Address received");
    }

    // Save the MAC address of the master
    memcpy(MasterMacAddress, pairingData.macAddr, 6);

    // Add the master to the peer list
    memcpy(peerInfo.peer_addr, MasterMacAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    if (debugSetting == DEBUG_ON)
    {
      Serial.println("Master MAC Address saved");
      Serial.println("Master added to peer list");
    }

    // Send Local Pairing Cycle 1 to the master together with the pairing text.
    pairingData.msgType = PAIRING;
    pairingData.pairingCycle = localPairingCycle;
    strcpy(pairingData.pairingText, "S-CYCLE-1/3");        // Slave in cycle 1 of 3.
    memcpy(pairingData.macAddr, SlavesMacAddresses[0], 6); // Use first slot for slave's own MAC

    // Send the slave's MAC address to the master
    esp_now_send(MasterMacAddress, (const uint8_t *)&pairingData, sizeof(pairingData));

    if (debugSetting == DEBUG_ON)
    {
      Serial.println("Pairing cycle 1: Master MAC Address saved and S-CYCLE-1/3 sent to master");
    }

    // Add one to the localPairingCycle to 2.
    localPairingCycle++;
  }

  /*CYCLE 2 OF 3 - RECEIVE THE RESPONSE FROM THE MASTER AND SEND BACK "S-CYCLE-2/3" + LOCAL CYCLE TO THE MASTER*/
  else if (pairingData.pairingCycle == 2 && localPairingCycle == 2)
  {
    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Pairing cycle ");
      Serial.print(localPairingCycle);
      Serial.println(": Final response received from master");
    }

    // Send last pairing message to the master
    pairingData.msgType = PAIRING;
    pairingData.pairingCycle = localPairingCycle;
    strcpy(pairingData.pairingText, "S-CYCLE-2/3");        // Slave in cycle 2 of 3.
    memcpy(pairingData.macAddr, SlavesMacAddresses[0], 6); // Use first slot for slave's own MAC
    esp_now_send(MasterMacAddress, (const uint8_t *)&pairingData, sizeof(pairingData));

    if (debugSetting == DEBUG_ON)
    {
      Serial.println("Pairing cycle 2: Received responce and send S-CYCLE-2/3 sent to master");
    }

    // Add one to the localPairingCycle to 3.
    localPairingCycle++;
  }

  /*CYCLE 3 OF 3 - RECEIVE THE FINAL RESPONSE FROM THE MASTER AND SET PAIRING MODE TO "PAIRED"*/
  else if (pairingData.pairingCycle == 3 && localPairingCycle == 3)
  {
    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Pairing cycle ");
      Serial.print(localPairingCycle);
      Serial.println(": Final response received from master");
    }

    // Set pairing mode to false.
    pairingMode = false;

    if (debugSetting == DEBUG_ON)
    {
      Serial.println("Pairing complete");
    }
  }

  // If the master is trying to pair again, restart the pairing process.
  else if (pairingData.pairingCycle == 1 && pairingMode == false)
  {
    Serial.println("Master seems to be trying to pair again, restarting pairing process");
    // For slave, just reset to waiting mode
    pairingMode = true;
    localPairingCycle = 1;
  }
}

/***************************************Internal Helper: startPairingBroadcast********************************************/
/// @brief Internal helper function to start the pairing broadcast
static void startPairingBroadcast()
{
  // Set pairing mode to true.
  pairingMode = true;

  // Set the pairing cycle to 1.
  localPairingCycle = 1;

  // Check which device type is selected and start the pairing process accordingly.
  switch (deviceType)
  {
  case MASTER:
    /*CYCLE 1 OF 3 - BROADCAST THE MASTER'S MAC ADDRESS TO THE SLAVE TOGETHER WITH "M-CYCLE-1/3" + LOCAL CYCLE TO THE SLAVE AND SET PAIRING MODE TO "PAIRED"*/

    // Create the first pairing message.
    pairingData.msgType = PAIRING;
    pairingData.pairingCycle = localPairingCycle;
    strcpy(pairingData.pairingText, "M-CYCLE-1/3"); // Master in cycle 1 of 3.
    memcpy(pairingData.macAddr, MasterMacAddress, sizeof(pairingData.macAddr));

    // Add broadcast peer if not already present
    if (!esp_now_is_peer_exist(BroadcastMacAddress))
    {
      memcpy(peerInfo.peer_addr, BroadcastMacAddress, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);

      if (debugSetting == DEBUG_ON)
      {
        Serial.println("Broadcast peer added");
      }
    }

    // Send the pairing message to the slave/global address.
    esp_now_send(BroadcastMacAddress, (const uint8_t *)&pairingData, sizeof(pairingData));

    // FOR DEBUGGING
    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Pairing cycle ");
      Serial.print(localPairingCycle);
      Serial.println(": Master MAC Address broadcasted together with M-CYCLE-1/3");
    }

    // Increase the local pairing cycle. Master has done its first pairing cycle.
    localPairingCycle++;
    pairingModeState = PAIRING_OPEN;
    break;

  case SLAVE:
    // FOR DEBUGGING
    if (debugSetting == DEBUG_ON)
    {
      Serial.print("Pairing cycle ");
      Serial.print(localPairingCycle);
      Serial.println(": Waiting for Master MAC Address broadcast");
    }
    break;

  default:
    Serial.println("No device type selected \n Please select a device type to start the pairing process.");
    break;
  }

  // // FOR DEBUGGING
  // if (debugSetting == DEBUG_ON)
  // {
  //   Serial.print("Pairing cycle: ");
  //   Serial.println(localPairingCycle);
  // }
}

/***************************************initESPNOW********************************************/
/// @brief This function will initialize ESP-NOW depending on the deviceType.
/// @param DEVICE_TYPE MASTER or SLAVE
/// @param DEBUG_SETTING DEBUG_ON or DEBUG_OFF
bool initESPNOW(uint8_t DEVICE_TYPE, uint8_t DEBUG_SETTING)
{
  bool initSuccess = true;

  // Set the device type and debug setting
  if (!setDeviceType(DEVICE_TYPE))
  {
    initSuccess = false;
  }
  if (!setDebugSetting(DEBUG_SETTING))
  {
    initSuccess = false;
  }

  // Initialize WiFi and register the callback function of ESP-NOW.
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    initSuccess = false;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Save the mac address of the selected device type to the global variable.
  switch (deviceType)
  {
  case MASTER:
    esp_read_mac(MasterMacAddress, ESP_MAC_WIFI_STA);
    break;
  case SLAVE:
    esp_read_mac(SlaveMacAddress, ESP_MAC_WIFI_STA);
    break;
  default:
    Serial.println("No device type selected \n Please select a device type to save the MAC address.");
    initSuccess = false;
    break;
  }

  // FOR DEBUGGING
  if (debugSetting == DEBUG_ON)
  {
    Serial.println("ESP-NOW initialized");
    Serial.println("The device type is: ");
    Serial.println(deviceType);

    Serial.print("MAC Address: ");
    uint8_t tempMacAddress[6] = {0}; // Temporary MAC address storage
    esp_read_mac(tempMacAddress, ESP_MAC_WIFI_STA);
    for (int i = 0; i < 6; i++)
    {
      Serial.printf("%02X", tempMacAddress[i]);
      if (i < 5)
        Serial.print(":");
    }
    Serial.println();
  }
  return initSuccess;
}

/***************************************OnDataRecv********************************************/
/// @brief  Pairing: This will pair the master and slave depending on the selected device type. Data: This will receive data and save it to the receivingData struct. Outside of the library the user can access the data by using receivingData struct.
/// @param mac_addr
/// @param incomingData
/// @param len
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  uint8_t type = incomingData[0]; // first message byte is the type of message

  switch (type)
  {
  case PAIRING: // the message is pairing type
    memcpy(&pairingData, incomingData, sizeof(pairingData));

    switch (deviceType)
    {
    case MASTER:
      pairingProcessMaster();
      break;
    case SLAVE:
      pairingProcessSlave();
      break;
    default:
      Serial.println("No device type selected \n Please select a device type to start the pairing process.");
      break;
    }
    break;

  case DATA: // the message is data type
    memcpy(&receivingData, incomingData, sizeof(receivingData));

    // Update last seen timestamp for the slave that sent this data
    if (deviceType == MASTER)
    {
      for (uint8_t i = 0; i < MAX_SLAVES; i++)
      {
        if (slaves[i].isPaired && memcmp(slaves[i].macAddr, mac_addr, 6) == 0)
        {
          slaves[i].lastSeen = millis();
          slaves[i].isActive = true;
          break;
        }
      }
    }

    // FOR DEBUGGING
    if (debugSetting == DEBUG_ON)
    {
      Serial.print(len);
      Serial.println(" bytes of new data received.");
    }
    if (receivedMessageOnMonitor)
    {
      printDebugData(type);
    }

    break;

  default:
    Serial.println("Unknown message type");
    break;
  }
}

/***************************************printDebugData********************************************/
/// @brief This function write out all the data in the struct_message.
void printDebugData(uint8_t messageType)
{
  switch (messageType)
  {
  case DATA:
    Serial.println("Data recieved:");
    Serial.print("Data Text: ");
    Serial.println(receivingData.dataText);
    Serial.print("Data Value: ");
    Serial.println(receivingData.dataValue);
    Serial.println();
    break;

  case PAIRING:
    Serial.println("Pairing data recieved:");
    Serial.print("Pairing Cycle: ");
    Serial.println(pairingData.pairingCycle);
    Serial.print("MAC Address: ");
    for (int i = 0; i < 6; i++)
    {
      Serial.printf("%02X", pairingData.macAddr[i]);
      if (i < 5)
        Serial.print(":");
    }
    Serial.println();
    break;

  default:
    Serial.println("Unknown message type");
    break;
  }
}

/***********************************Set Device Type******************************************/
/// @brief This function will set the device type. Returns false if no device type is selected.
/// @param deviceType MASTER or SLAVE
bool setDeviceType(uint8_t type)
{
  bool success = true;
  switch (type)
  {
  case MASTER:
    deviceType = MASTER;
    break;
  case SLAVE:
    deviceType = SLAVE;
    break;
  default:
    Serial.println("No device type selected");
    success = false;
    break;
  }
  return success;
}

/***********************************Set Debug Setting******************************************/
/// @brief This function will set the debug setting. Returns false if no debug setting is selected.
/// @param debugSetting DEBUG_ON or DEBUG_OFF
bool setDebugSetting(uint8_t setting)
{
  bool success = true;
  switch (setting)
  {
  case DEBUG_ON:
    debugSetting = DEBUG_ON;
    break;
  case DEBUG_OFF:
    debugSetting = DEBUG_OFF;
    break;
  default:
    Serial.println("No debug setting selected");
    success = false;
    break;
  }
  return success;
}

/*******************************setReceivedMessageOnMonitor*************************************/
/// @brief This function will tell the program to write the incoming data to the Serial.Monitor.
/// @param state true or false (default is false)
void setReceivedMessageOnMonitor(bool state)
{
  receivedMessageOnMonitor = state;
}

/***************************************sendDataToSlave********************************************/
/// @brief Send data to a specific slave by ID (MASTER only).
/// @param slaveId Slave ID (0-9)
/// @param messageType
/// @param dataText
/// @param dataValue
/// @return true if sent successfully, false otherwise
bool sendDataToSlave(uint8_t slaveId, uint8_t messageType, char *dataText, uint8_t dataValue)
{
  if (deviceType != MASTER)
  {
    Serial.println("ERROR: sendDataToSlave() only works for MASTER device");
    return false;
  }

  if (slaveId >= MAX_SLAVES)
  {
    Serial.println("ERROR: Invalid slave ID");
    return false;
  }

  if (!slaves[slaveId].isPaired)
  {
    Serial.print("ERROR: Slave #");
    Serial.print(slaveId);
    Serial.println(" is not paired");
    return false;
  }

  sendingData.msgType = messageType;
  strcpy(sendingData.dataText, dataText);
  sendingData.dataValue = dataValue;

  esp_err_t result = esp_now_send(slaves[slaveId].macAddr, (const uint8_t *)&sendingData, sizeof(sendingData));
  return (result == ESP_OK);
}

/***************************************broadcastData********************************************/
/// @brief Broadcast data to all paired slaves (MASTER only).
/// @param messageType
/// @param dataText
/// @param dataValue
void broadcastData(uint8_t messageType, char *dataText, uint8_t dataValue)
{
  if (deviceType != MASTER)
  {
    Serial.println("ERROR: broadcastData() only works for MASTER device");
    return;
  }

  sendingData.msgType = messageType;
  strcpy(sendingData.dataText, dataText);
  sendingData.dataValue = dataValue;

  for (uint8_t i = 0; i < MAX_SLAVES; i++)
  {
    if (slaves[i].isPaired)
    {
      esp_now_send(slaves[i].macAddr, (const uint8_t *)&sendingData, sizeof(sendingData));
    }
  }
}

/***************************************startPairingForNewSlave********************************************/
/// @brief Start pairing process for one new slave (MASTER only).
/// @return true if pairing started, false if already at max slaves
bool startPairingForNewSlave()
{
  if (deviceType != MASTER)
  {
    Serial.println("ERROR: startPairingForNewSlave() only works for MASTER device");
    return false;
  }

  if (slavesCount >= MAX_SLAVES)
  {
    Serial.println("ERROR: Maximum number of slaves already paired");
    pairingModeState = PAIRING_FULL;
    return false;
  }

  if (pairingMode)
  {
    Serial.println("ERROR: Already in pairing mode");
    return false;
  }

  // Find next available slot
  for (uint8_t i = 0; i < MAX_SLAVES; i++)
  {
    if (!slaves[i].isPaired)
    {
      currentPairingSlaveId = i;
      break;
    }
  }

  startPairingBroadcast();
  return true;
}

/***************************************stopPairing********************************************/
/// @brief Stop accepting new slaves.
void stopPairing()
{
  pairingMode = false;
  pairingModeState = (slavesCount >= MAX_SLAVES) ? PAIRING_FULL : PAIRING_CLOSED;
  currentPairingSlaveId = -1;

  if (debugSetting == DEBUG_ON)
  {
    Serial.println("Pairing stopped");
  }
}

/***************************************removeSlave********************************************/
/// @brief Remove a specific slave by ID.
/// @param slaveId Slave ID (0-9)
/// @return true if removed, false if slave not found
bool removeSlave(uint8_t slaveId)
{
  if (deviceType != MASTER)
  {
    Serial.println("ERROR: removeSlave() only works for MASTER device");
    return false;
  }

  if (slaveId >= MAX_SLAVES)
  {
    Serial.println("ERROR: Invalid slave ID");
    return false;
  }

  if (!slaves[slaveId].isPaired)
  {
    Serial.print("Slave #");
    Serial.print(slaveId);
    Serial.println(" is not paired");
    return false;
  }

  // Remove ESP-NOW peer
  esp_now_del_peer(slaves[slaveId].macAddr);

  // Clear slave info
  memset(&slaves[slaveId], 0, sizeof(struct_slave_info));
  slaves[slaveId].slaveId = slaveId;
  slavesCount--;

  // Update pairing mode state
  pairingModeState = (slavesCount >= MAX_SLAVES) ? PAIRING_FULL : PAIRING_CLOSED;

  if (debugSetting == DEBUG_ON)
  {
    Serial.print("Slave #");
    Serial.print(slaveId);
    Serial.print(" removed. Total slaves: ");
    Serial.println(slavesCount);
  }

  return true;
}

/***************************************removeAllSlaves********************************************/
/// @brief Remove all paired slaves.
void removeAllSlaves()
{
  if (deviceType != MASTER)
  {
    Serial.println("ERROR: removeAllSlaves() only works for MASTER device");
    return;
  }

  for (uint8_t i = 0; i < MAX_SLAVES; i++)
  {
    if (slaves[i].isPaired)
    {
      esp_now_del_peer(slaves[i].macAddr);
      memset(&slaves[i], 0, sizeof(struct_slave_info));
      slaves[i].slaveId = i;
    }
  }

  slavesCount = 0;
  pairingModeState = PAIRING_CLOSED;

  if (debugSetting == DEBUG_ON)
  {
    Serial.println("All slaves removed");
  }
}

/***************************************getSlaveInfo********************************************/
/// @brief Get information about a specific slave.
/// @param slaveId Slave ID (0-9)
/// @return Pointer to slave info, or NULL if not found
struct_slave_info *getSlaveInfo(uint8_t slaveId)
{
  if (slaveId >= MAX_SLAVES)
  {
    return NULL;
  }

  if (!slaves[slaveId].isPaired)
  {
    return NULL;
  }

  return &slaves[slaveId];
}

/***************************************getPairedSlaves********************************************/
/// @brief Get list of all paired slave IDs.
/// @param slaveIds Array to store slave IDs
/// @param maxCount Maximum number of IDs to return
/// @return Number of paired slaves
uint8_t getPairedSlaves(uint8_t *slaveIds, uint8_t maxCount)
{
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_SLAVES && count < maxCount; i++)
  {
    if (slaves[i].isPaired)
    {
      slaveIds[count] = i;
      count++;
    }
  }
  return count;
}

/***************************************isSlaveActive********************************************/
/// @brief Check if a slave is active (recently communicated).
/// @param slaveId Slave ID (0-9)
/// @param timeoutMs Timeout in milliseconds (default 30000)
/// @return true if active, false otherwise
bool isSlaveActive(uint8_t slaveId, unsigned long timeoutMs)
{
  if (slaveId >= MAX_SLAVES || !slaves[slaveId].isPaired)
  {
    return false;
  }

  unsigned long timeSinceLastSeen = millis() - slaves[slaveId].lastSeen;
  return (timeSinceLastSeen < timeoutMs);
}

/***************************************printSlavesStatus********************************************/
/// @brief Print status of all paired slaves to Serial.
void printSlavesStatus()
{
  Serial.println("\n========== SLAVES STATUS ==========");
  Serial.print("Total Paired Slaves: ");
  Serial.print(slavesCount);
  Serial.print("/");
  Serial.println(MAX_SLAVES);
  Serial.println("-----------------------------------");

  for (uint8_t i = 0; i < MAX_SLAVES; i++)
  {
    if (slaves[i].isPaired)
    {
      Serial.print("Slave #");
      Serial.print(i);
      Serial.print(" [");
      Serial.print(slaves[i].slaveName);
      Serial.print("] ");

      // Print MAC address
      for (int j = 0; j < 6; j++)
      {
        if (slaves[i].macAddr[j] < 0x10)
          Serial.print("0");
        Serial.print(slaves[i].macAddr[j], HEX);
        if (j < 5)
          Serial.print(":");
      }

      Serial.print(" - ");
      Serial.print(slaves[i].isActive ? "ACTIVE" : "INACTIVE");
      Serial.print(" (Last seen: ");
      Serial.print((millis() - slaves[i].lastSeen) / 1000);
      Serial.println("s ago)");
    }
  }
  Serial.println("===================================\n");
}

/***************************************getESPNOWStatus********************************************/
/// @brief This function will return the current status of the ESP-NOW connection.
/// @return struct_status containing pairing and connection information
struct_status getESPNOWStatus()
{
  struct_status status;
  status.isPairingActive = pairingMode;
  status.isPaired = (!pairingMode && (localPairingCycle > 1));
  status.deviceType = deviceType;
  status.pairingCycle = localPairingCycle;

  // Multi-slave fields
  status.slavesCount = slavesCount;
  status.maxSlaves = MAX_SLAVES;

  // Count active slaves
  status.activeSlaves = 0;
  for (uint8_t i = 0; i < MAX_SLAVES; i++)
  {
    if (slaves[i].isPaired && isSlaveActive(i))
    {
      status.activeSlaves++;
    }
  }

  // Copy the paired device MAC address
  if (deviceType == MASTER)
  {
    // For master, copy first paired slave MAC if available
    if (slavesCount > 0 && slaves[0].isPaired)
    {
      memcpy(status.pairedMacAddr, slaves[0].macAddr, 6);
    }
  }
  else
  {
    memcpy(status.pairedMacAddr, MasterMacAddress, 6);
  }

  return status;
}
/***************************************removeInactiveSlaves********************************************/
/// @brief Check for inactive slaves and remove them automatically.
/// @param timeoutMs Timeout in milliseconds (default 30000)
/// @return Number of slaves removed
uint8_t removeInactiveSlaves(unsigned long timeoutMs)
{
  uint8_t removedCount = 0;

  for (uint8_t i = 0; i < MAX_SLAVES; i++)
  {
    if (slaves[i].isPaired && !isSlaveActive(i, timeoutMs))
    {
      if (debugSetting == DEBUG_ON)
      {
        Serial.print("[TIMEOUT] Removing inactive slave #");
        Serial.println(i);
      }
      removeSlave(i);
      removedCount++;
    }
  }

  return removedCount;
}

/***************************************getInactiveSlaves********************************************/
/// @brief Get list of currently inactive slaves.
/// @param slaveIds Array to store inactive slave IDs
/// @param maxCount Maximum number of IDs to return
/// @param timeoutMs Timeout in milliseconds (default 30000)
/// @return Number of inactive slaves found
uint8_t getInactiveSlaves(uint8_t *slaveIds, uint8_t maxCount, unsigned long timeoutMs)
{
  uint8_t count = 0;

  for (uint8_t i = 0; i < MAX_SLAVES && count < maxCount; i++)
  {
    if (slaves[i].isPaired && !isSlaveActive(i, timeoutMs))
    {
      slaveIds[count] = i;
      count++;
    }
  }

  return count;
}