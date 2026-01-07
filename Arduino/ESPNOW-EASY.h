#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

/******************************************************************************************************/
/********************************************VARIABLES*************************************************/
/******************************************************************************************************/

#define MAX_SLAVES 10

// Global variable to store the slave's and masters MAC address
extern uint8_t MasterMacAddress[6];
extern uint8_t SlavesMacAddresses[MAX_SLAVES][6]; // Array of slave MAC addresses
extern uint8_t slavesCount;                       // Number of paired slaves
extern uint8_t BroadcastMacAddress[6];

extern bool receivedMessageOnMonitor;

// Global variable to store the pairing status
extern bool pairingMode;

extern uint8_t localPairingCycle; // This is a variable that will store the local pairing cycle.

// Global variable to store the peer information
extern esp_now_peer_info_t peerInfo;

// Enum for debug setting
enum DebugSetting
{
  DEBUG_ON,
  DEBUG_OFF,
};
extern DebugSetting debugSetting;

// Enum for device type
enum DeviceType
{
  MASTER,
  SLAVE,
};
extern DeviceType deviceType;

// Enum for message type
enum MessageType
{
  PAIRING,
  DATA,
};
extern MessageType messageType;

// Struct for message
typedef struct struct_message
{
  uint8_t msgType;
  char dataText[32];
  uint8_t dataValue;
} struct_message;
extern struct_message sendingData;   // data to send
extern struct_message recievingData; // data received

// Struct for pairing
typedef struct struct_pairing
{
  uint8_t msgType;
  uint8_t macAddr[6];
  uint8_t pairingCycle;
  char pairingText[32];
} struct_pairing;
extern struct_pairing pairingData;

// Struct for slave information
typedef struct struct_slave_info
{
  uint8_t macAddr[6];
  bool isPaired;
  bool isActive;
  uint8_t slaveId;        // 0-9 identifier
  char slaveName[16];     // Optional friendly name
  unsigned long lastSeen; // Last message timestamp
} struct_slave_info;
extern struct_slave_info slaves[MAX_SLAVES];

// Enum for pairing mode
enum PairingMode
{
  PAIRING_CLOSED, // Not accepting new slaves
  PAIRING_OPEN,   // Accepting one new slave
  PAIRING_FULL    // All 10 slots filled
};
extern PairingMode pairingModeState;

// Struct for ESP-NOW status
typedef struct struct_status
{
  bool isPairingActive;
  bool isPaired;
  uint8_t deviceType; // MASTER or SLAVE
  uint8_t pairedMacAddr[6];
  uint8_t pairingCycle;
  // Multi-slave fields
  uint8_t slavesCount;  // Number of paired slaves
  uint8_t activeSlaves; // Number of active slaves
  uint8_t maxSlaves;    // MAX_SLAVES constant
} struct_status;

/******************************************************************************************************/
/********************************************FUNCTIONS**/ ***********************************************/
    /******************************************************************************************************/

    /// @brief This function will initialize ESP-NOW depending on the deviceType.
    /// @param DEVICE_TYPE
    /// @param DEBUG_SETTING
    bool initESPNOW(uint8_t DEVICE_TYPE, uint8_t DEBUG_SETTING);

/// @brief This function write out all the data in the struct_message.
void printDebugData(uint8_t messageType);

/// @brief This function will set the device type.
/// @param deviceType MASTER or SLAVE
bool setDeviceType(uint8_t type);

/// @brief This function will set the debug setting.
/// @param debugSetting DEBUG_ON or DEBUG_OFF
bool setDebugSetting(uint8_t setting);

/// @param WAIT_TIME_MS Must be greater than 1000 milliseconds.
void checkPairingModeStatus(unsigned long WAIT_TIME_MS);

/// @brief This function will tell the program to write the incoming data to the Serial.Monitor.
void setReceivedMessageOnMonitor(bool state);

/// @brief Send data to a specific slave by ID (MASTER only).
/// @param slaveId Slave ID (0-9)
/// @param messageType
/// @param dataText
/// @param dataValue
/// @return true if sent successfully, false otherwise
bool sendDataToSlave(uint8_t slaveId, uint8_t messageType, char *dataText, uint8_t dataValue);

/// @brief Broadcast data to all paired slaves (MASTER only).
/// @param messageType
/// @param dataText
/// @param dataValue
void broadcastData(uint8_t messageType, char *dataText, uint8_t dataValue);

/// @brief Start pairing process for one new slave (MASTER only).
/// @return true if pairing started, false if already at max slaves
bool startPairingForNewSlave();

/// @brief Stop accepting new slaves.
void stopPairing();

/// @brief Remove a specific slave by ID.
/// @param slaveId Slave ID (0-9)
/// @return true if removed, false if slave not found
bool removeSlave(uint8_t slaveId);

/// @brief Remove all paired slaves.
void removeAllSlaves();

/// @brief Get information about a specific slave.
/// @param slaveId Slave ID (0-9)
/// @return Pointer to slave info, or NULL if not found
struct_slave_info *getSlaveInfo(uint8_t slaveId);

/// @brief Get list of all paired slave IDs.
/// @param slaveIds Array to store slave IDs
/// @param maxCount Maximum number of IDs to return
/// @return Number of paired slaves
uint8_t getPairedSlaves(uint8_t *slaveIds, uint8_t maxCount);

/// @brief Check if a slave is active (recently communicated).
/// @param slaveId Slave ID (0-9)
/// @param timeoutMs Timeout in milliseconds (default 30000)
/// @return true if active, false otherwise
bool isSlaveActive(uint8_t slaveId, unsigned long timeoutMs = 30000);

/// @brief Print status of all paired slaves to Serial.
void printSlavesStatus();

/// @brief Check for inactive slaves and remove them automatically.
/// @param timeoutMs Timeout in milliseconds (default 30000)
/// @return Number of slaves removed
uint8_t removeInactiveSlaves(unsigned long timeoutMs = 30000);

/// @brief Get list of currently inactive slaves.
/// @param slaveIds Array to store inactive slave IDs
/// @param maxCount Maximum number of IDs to return
/// @param timeoutMs Timeout in milliseconds (default 30000)
/// @return Number of inactive slaves found
uint8_t getInactiveSlaves(uint8_t *slaveIds, uint8_t maxCount, unsigned long timeoutMs = 30000);

/// @brief This function will return the current status of the ESP-NOW connection.
/// @return struct_status containing pairing and connection information
struct_status getESPNOWStatus();
