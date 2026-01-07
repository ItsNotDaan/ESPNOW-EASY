#ifndef ESPNOW_EASY_H
#define ESPNOW_EASY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_now.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************************************/
/********************************************CONSTANTS*************************************************/
/******************************************************************************************************/

#define MAX_SLAVES 10
#define ESPNOW_CHANNEL 1
#define ESPNOW_PMK "pmk1234567890123"
#define ESPNOW_LMK "lmk1234567890123"

/******************************************************************************************************/
/********************************************TYPES*****************************************************/
/******************************************************************************************************/

// Enum for debug setting
typedef enum {
    DEBUG_ON,
    DEBUG_OFF,
} debug_setting_t;

// Enum for device type
typedef enum {
    MASTER,
    SLAVE,
} device_type_t;

// Enum for message type
typedef enum {
    PAIRING,
    DATA,
} message_type_t;

// Enum for pairing mode
typedef enum {
    PAIRING_CLOSED, // Not accepting new slaves
    PAIRING_OPEN,   // Accepting one new slave
    PAIRING_FULL    // All 10 slots filled
} pairing_mode_t;

// Struct for message
typedef struct {
    uint8_t msgType;
    char dataText[32];
    uint8_t dataValue;
} espnow_message_t;

// Struct for pairing
typedef struct {
    uint8_t msgType;
    uint8_t macAddr[6];
    uint8_t pairingCycle;
    char pairingText[32];
} espnow_pairing_t;

// Struct for slave information
typedef struct {
    uint8_t macAddr[6];
    bool isPaired;
    bool isActive;
    uint8_t slaveId;        // 0-9 identifier
    char slaveName[16];     // Optional friendly name
    uint64_t lastSeen;      // Last message timestamp (esp_timer_get_time())
} slave_info_t;

// Struct for ESP-NOW status
typedef struct {
    bool isPairingActive;
    bool isPaired;
    uint8_t deviceType;         // MASTER or SLAVE
    uint8_t pairedMacAddr[6];
    uint8_t pairingCycle;
    // Multi-slave fields
    uint8_t slavesCount;        // Number of paired slaves
    uint8_t activeSlaves;       // Number of active slaves
    uint8_t maxSlaves;          // MAX_SLAVES constant
} espnow_status_t;

/******************************************************************************************************/
/********************************************FUNCTIONS*************************************************/
/******************************************************************************************************/

/**
 * @brief Initialize ESP-NOW
 * 
 * @param device_type MASTER or SLAVE
 * @param debug_setting DEBUG_ON or DEBUG_OFF
 * @return true if successful, false otherwise
 */
bool espnow_easy_init(device_type_t device_type, debug_setting_t debug_setting);

/**
 * @brief Set the device type
 * 
 * @param type MASTER or SLAVE
 * @return true if successful, false otherwise
 */
bool espnow_easy_set_device_type(device_type_t type);

/**
 * @brief Set the debug setting
 * 
 * @param setting DEBUG_ON or DEBUG_OFF
 * @return true if successful, false otherwise
 */
bool espnow_easy_set_debug(debug_setting_t setting);

/**
 * @brief Check pairing mode status and handle timeouts
 * 
 * @param wait_time_ms Timeout in milliseconds (minimum 1000)
 */
void espnow_easy_check_pairing_status(uint32_t wait_time_ms);

/**
 * @brief Enable/disable printing received messages to console
 * 
 * @param state true to enable, false to disable
 */
void espnow_easy_set_print_received(bool state);

/**
 * @brief Send data to a specific slave by ID (MASTER only)
 * 
 * @param slave_id Slave ID (0-9)
 * @param msg_type Message type
 * @param data_text Text data
 * @param data_value Numeric value
 * @return true if sent successfully, false otherwise
 */
bool espnow_easy_send_to_slave(uint8_t slave_id, message_type_t msg_type, const char *data_text, uint8_t data_value);

/**
 * @brief Broadcast data to all paired slaves (MASTER only)
 * 
 * @param msg_type Message type
 * @param data_text Text data
 * @param data_value Numeric value
 */
void espnow_easy_broadcast(message_type_t msg_type, const char *data_text, uint8_t data_value);

/**
 * @brief Start pairing process for one new slave (MASTER only)
 * 
 * @return true if pairing started, false if already at max slaves
 */
bool espnow_easy_start_pairing(void);

/**
 * @brief Stop accepting new slaves
 */
void espnow_easy_stop_pairing(void);

/**
 * @brief Remove a specific slave by ID
 * 
 * @param slave_id Slave ID (0-9)
 * @return true if removed, false if slave not found
 */
bool espnow_easy_remove_slave(uint8_t slave_id);

/**
 * @brief Remove all paired slaves
 */
void espnow_easy_remove_all_slaves(void);

/**
 * @brief Get information about a specific slave
 * 
 * @param slave_id Slave ID (0-9)
 * @return Pointer to slave info, or NULL if not found
 */
slave_info_t* espnow_easy_get_slave_info(uint8_t slave_id);

/**
 * @brief Get list of all paired slave IDs
 * 
 * @param slave_ids Array to store slave IDs
 * @param max_count Maximum number of IDs to return
 * @return Number of paired slaves
 */
uint8_t espnow_easy_get_paired_slaves(uint8_t *slave_ids, uint8_t max_count);

/**
 * @brief Check if a slave is active (recently communicated)
 * 
 * @param slave_id Slave ID (0-9)
 * @param timeout_us Timeout in microseconds (default 30000000 = 30s)
 * @return true if active, false otherwise
 */
bool espnow_easy_is_slave_active(uint8_t slave_id, uint64_t timeout_us);

/**
 * @brief Print status of all paired slaves
 */
void espnow_easy_print_slaves_status(void);

/**
 * @brief Check for inactive slaves and remove them automatically
 * 
 * @param timeout_us Timeout in microseconds (default 30000000 = 30s)
 * @return Number of slaves removed
 */
uint8_t espnow_easy_remove_inactive_slaves(uint64_t timeout_us);

/**
 * @brief Get list of currently inactive slaves
 * 
 * @param slave_ids Array to store inactive slave IDs
 * @param max_count Maximum number of IDs to return
 * @param timeout_us Timeout in microseconds (default 30000000 = 30s)
 * @return Number of inactive slaves found
 */
uint8_t espnow_easy_get_inactive_slaves(uint8_t *slave_ids, uint8_t max_count, uint64_t timeout_us);

/**
 * @brief Get the current status of the ESP-NOW connection
 * 
 * @return espnow_status_t containing pairing and connection information
 */
espnow_status_t espnow_easy_get_status(void);

#ifdef __cplusplus
}
#endif

#endif // ESPNOW_EASY_H
