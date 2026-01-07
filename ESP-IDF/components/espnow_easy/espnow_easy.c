#include "espnow_easy.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ESPNOW_EASY";

/******************************************************************************************************/
/********************************************GLOBALS***************************************************/
/******************************************************************************************************/

// Global variables for MAC addresses
static uint8_t g_master_mac[6] = {0};
static uint8_t g_slaves_mac[MAX_SLAVES][6] = {0};
static uint8_t g_slaves_count = 0;
static uint8_t g_broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Global settings
static bool g_print_received = false;
static bool g_pairing_mode = false;
static uint8_t g_local_pairing_cycle = 0;

// ESP-NOW peer info
static esp_now_peer_info_t g_peer_info;

// Device settings
static device_type_t g_device_type;
static debug_setting_t g_debug_setting;
static pairing_mode_t g_pairing_mode_state = PAIRING_CLOSED;

// Message structures
static espnow_message_t g_sending_data;
static espnow_message_t g_receiving_data;
static espnow_pairing_t g_pairing_data;

// Slave tracking
static slave_info_t g_slaves[MAX_SLAVES] = {0};
static int8_t g_current_pairing_slave_id = -1;

/******************************************************************************************************/
/********************************************FORWARD DECLARATIONS**************************************/
/******************************************************************************************************/

static void pairing_process_master(void);
static void pairing_process_slave(void);
static void on_data_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);
static bool start_pairing_broadcast(void);

/******************************************************************************************************/
/********************************************HELPER FUNCTIONS******************************************/
/******************************************************************************************************/

static void print_mac(const uint8_t *mac) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static uint64_t get_time_us(void) {
    return esp_timer_get_time();
}

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/******************************************************************************************************/
/********************************************PAIRING FUNCTIONS*****************************************/
/******************************************************************************************************/

static void pairing_process_master(void) {
    /* CYCLE 2 OF 3 - RECEIVE THE SLAVE'S MAC ADDRESS AND SEND BACK "M-CYCLE-2/3" */
    if (g_pairing_data.pairingCycle == 1 && g_local_pairing_cycle == 2) {
        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle %d: Slave MAC Address received", g_local_pairing_cycle);
        }

        // Find next available slave slot
        if (g_current_pairing_slave_id == -1) {
            for (uint8_t i = 0; i < MAX_SLAVES; i++) {
                if (!g_slaves[i].isPaired) {
                    g_current_pairing_slave_id = i;
                    break;
                }
            }
        }

        if (g_current_pairing_slave_id == -1) {
            ESP_LOGE(TAG, "No available slave slots!");
            g_pairing_mode = false;
            g_pairing_mode_state = PAIRING_FULL;
            return;
        }

        // Save the MAC address of the slave
        memcpy(g_slaves[g_current_pairing_slave_id].macAddr, g_pairing_data.macAddr, 6);
        memcpy(g_slaves_mac[g_current_pairing_slave_id], g_pairing_data.macAddr, 6);

        // Add the new peer with the slave's MAC address
        memcpy(g_peer_info.peer_addr, g_slaves[g_current_pairing_slave_id].macAddr, 6);
        g_peer_info.channel = ESPNOW_CHANNEL;
        g_peer_info.encrypt = false;

        // Add the peer
        esp_err_t err = esp_now_add_peer(&g_peer_info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(err));
        }

        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Added slave #%d as new peer (broadcast peer kept active)", 
                     g_current_pairing_slave_id);
        }

        // Send Local Pairing Cycle 2 to the slave
        g_pairing_data.msgType = PAIRING;
        g_pairing_data.pairingCycle = g_local_pairing_cycle;
        strcpy(g_pairing_data.pairingText, "M-CYCLE-2/3");
        memcpy(g_pairing_data.macAddr, g_master_mac, 6);

        // Send the OK response to the slave
        esp_now_send(g_slaves[g_current_pairing_slave_id].macAddr, 
                    (const uint8_t *)&g_pairing_data, sizeof(g_pairing_data));

        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle 2: Slave MAC saved and M-CYCLE-2/3 sent");
        }

        g_local_pairing_cycle++;
    }
    /* CYCLE 3 OF 3 - RECEIVE FINAL RESPONSE AND COMPLETE PAIRING */
    else if (g_pairing_data.pairingCycle == 2 && g_local_pairing_cycle == 3) {
        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle %d: Final response received from slave", 
                     g_local_pairing_cycle);
        }

        // Send last pairing message to the slave
        g_pairing_data.msgType = PAIRING;
        g_pairing_data.pairingCycle = g_local_pairing_cycle;
        strcpy(g_pairing_data.pairingText, "M-CYCLE-3/3");
        memcpy(g_pairing_data.macAddr, g_master_mac, 6);
        esp_now_send(g_slaves[g_current_pairing_slave_id].macAddr, 
                    (const uint8_t *)&g_pairing_data, sizeof(g_pairing_data));

        // Mark slave as paired and active
        g_slaves[g_current_pairing_slave_id].isPaired = true;
        g_slaves[g_current_pairing_slave_id].isActive = true;
        g_slaves[g_current_pairing_slave_id].slaveId = g_current_pairing_slave_id;
        g_slaves[g_current_pairing_slave_id].lastSeen = get_time_us();
        snprintf(g_slaves[g_current_pairing_slave_id].slaveName, 16, 
                "Slave_%d", g_current_pairing_slave_id);
        g_slaves_count++;

        // Set pairing mode to false
        g_pairing_mode = false;
        g_pairing_mode_state = (g_slaves_count >= MAX_SLAVES) ? PAIRING_FULL : PAIRING_CLOSED;
        g_current_pairing_slave_id = -1;

        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle 3: Final response M-CYCLE-3/3 sent");
            ESP_LOGI(TAG, "Pairing complete! Total slaves paired: %d", g_slaves_count);
        }
    }
}

static void pairing_process_slave(void) {
    /* CYCLE 1 OF 3 - RECEIVE MASTER'S MAC AND SEND BACK "S-CYCLE-1/3" */
    if (g_pairing_data.pairingCycle == 1 && g_local_pairing_cycle == 1) {
        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle %d: Master broadcast received", g_local_pairing_cycle);
        }

        // Save master's MAC address
        memcpy(g_master_mac, g_pairing_data.macAddr, 6);

        // Remove broadcast peer
        esp_now_del_peer(g_broadcast_mac);

        // Add master as peer
        memcpy(g_peer_info.peer_addr, g_master_mac, 6);
        g_peer_info.channel = ESPNOW_CHANNEL;
        g_peer_info.encrypt = false;
        esp_now_add_peer(&g_peer_info);

        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Added master as peer, sending response");
        }

        // Send response to master
        g_pairing_data.msgType = PAIRING;
        g_pairing_data.pairingCycle = g_local_pairing_cycle;
        strcpy(g_pairing_data.pairingText, "S-CYCLE-1/3");
        
        // Get slave's MAC address
        uint8_t slave_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, slave_mac);
        memcpy(g_pairing_data.macAddr, slave_mac, 6);

        esp_now_send(g_master_mac, (const uint8_t *)&g_pairing_data, sizeof(g_pairing_data));

        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle 1: Master MAC saved and S-CYCLE-1/3 sent");
        }

        g_local_pairing_cycle++;
    }
    /* CYCLE 2 OF 3 - RECEIVE MASTER'S CONFIRMATION */
    else if (g_pairing_data.pairingCycle == 2 && g_local_pairing_cycle == 2) {
        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle %d: Master confirmation received", g_local_pairing_cycle);
        }

        // Send final response
        g_pairing_data.msgType = PAIRING;
        g_pairing_data.pairingCycle = g_local_pairing_cycle;
        strcpy(g_pairing_data.pairingText, "S-CYCLE-2/3");
        
        uint8_t slave_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, slave_mac);
        memcpy(g_pairing_data.macAddr, slave_mac, 6);

        esp_now_send(g_master_mac, (const uint8_t *)&g_pairing_data, sizeof(g_pairing_data));

        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle 2: S-CYCLE-2/3 sent to master");
        }

        g_local_pairing_cycle++;
    }
    /* CYCLE 3 OF 3 - RECEIVE FINAL CONFIRMATION AND COMPLETE */
    else if (g_pairing_data.pairingCycle == 3 && g_local_pairing_cycle == 3) {
        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing cycle %d: Final confirmation received", g_local_pairing_cycle);
        }

        g_pairing_mode = false;

        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Pairing complete! Paired with master");
        }
    }
}

/******************************************************************************************************/
/********************************************CALLBACKS*************************************************/
/******************************************************************************************************/

static void on_data_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    const uint8_t *mac_addr = recv_info->src_addr;
    
    // Check if this is a pairing message
    if (len == sizeof(espnow_pairing_t)) {
        memcpy(&g_pairing_data, data, sizeof(espnow_pairing_t));
        
        if (g_pairing_data.msgType == PAIRING && g_pairing_mode) {
            if (g_device_type == MASTER) {
                pairing_process_master();
            } else {
                pairing_process_slave();
            }
            return;
        }
    }

    // Check if this is a data message
    if (len == sizeof(espnow_message_t)) {
        memcpy(&g_receiving_data, data, sizeof(espnow_message_t));
        
        if (g_receiving_data.msgType == DATA) {
            // Update lastSeen for the slave that sent this message
            if (g_device_type == MASTER) {
                for (uint8_t i = 0; i < MAX_SLAVES; i++) {
                    if (g_slaves[i].isPaired && 
                        memcmp(g_slaves[i].macAddr, mac_addr, 6) == 0) {
                        g_slaves[i].lastSeen = get_time_us();
                        g_slaves[i].isActive = true;
                        break;
                    }
                }
            } else {
                // Slave received data from master - update last seen
                // (For slaves, we could track when master was last heard from)
            }

            if (g_print_received) {
                printf("\n[DATA RECEIVED]\n");
                printf("From MAC: ");
                print_mac(mac_addr);
                printf("\n");
                printf("Text: %s\n", g_receiving_data.dataText);
                printf("Value: %d\n\n", g_receiving_data.dataValue);
            }
        }
    }
}

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (g_debug_setting == DEBUG_ON) {
        if (status == ESP_NOW_SEND_SUCCESS) {
            ESP_LOGD(TAG, "Send success to ");
            print_mac(mac_addr);
            printf("\n");
        } else {
            ESP_LOGW(TAG, "Send failed to ");
            print_mac(mac_addr);
            printf("\n");
        }
    }
}

/******************************************************************************************************/
/********************************************INITIALIZATION*******************************************/
/******************************************************************************************************/

bool espnow_easy_init(device_type_t device_type, debug_setting_t debug_setting) {
    g_device_type = device_type;
    g_debug_setting = debug_setting;

    // Initialize WiFi
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCP/IP stack: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi channel: %s", esp_err_to_name(ret));
        return false;
    }

    // Get device MAC address
    if (device_type == MASTER) {
        esp_wifi_get_mac(WIFI_IF_STA, g_master_mac);
        if (debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Master MAC: ");
            print_mac(g_master_mac);
            printf("\n");
        }
    } else {
        uint8_t slave_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, slave_mac);
        if (debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Slave MAC: ");
            print_mac(slave_mac);
            printf("\n");
        }
    }

    // Initialize ESP-NOW
    ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ESP-NOW: %s", esp_err_to_name(ret));
        return false;
    }

    // Register callbacks
    ret = esp_now_register_recv_cb(on_data_recv);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register recv callback: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_now_register_send_cb(on_data_sent);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register send callback: %s", esp_err_to_name(ret));
        return false;
    }

    // Add broadcast peer for pairing
    memset(&g_peer_info, 0, sizeof(esp_now_peer_info_t));
    memcpy(g_peer_info.peer_addr, g_broadcast_mac, 6);
    g_peer_info.channel = ESPNOW_CHANNEL;
    g_peer_info.encrypt = false;

    ret = esp_now_add_peer(&g_peer_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add broadcast peer: %s", esp_err_to_name(ret));
        return false;
    }

    // Set slave to pairing mode automatically
    if (device_type == SLAVE) {
        g_pairing_mode = true;
        g_local_pairing_cycle = 1;
        if (debug_setting == DEBUG_ON) {
            ESP_LOGI(TAG, "Slave ready for pairing");
        }
    }

    ESP_LOGI(TAG, "ESP-NOW Easy initialized successfully");
    return true;
}

/******************************************************************************************************/
/********************************************PUBLIC FUNCTIONS******************************************/
/******************************************************************************************************/

void espnow_easy_check_pairing_status(uint32_t wait_time_ms) {
    static uint32_t last_event_time = 0;
    static uint32_t event_interval_ms = 0;

    if (last_event_time == 0) {
        last_event_time = get_time_ms();
    }

    // Set minimum timeout
    if (wait_time_ms < 1000) {
        event_interval_ms = 5000;
    } else {
        event_interval_ms = wait_time_ms;
    }

    // Check if pairing mode is active
    if (g_pairing_mode) {
        uint32_t current_time = get_time_ms();
        
        // Reset pairing if timeout has passed
        if ((current_time - last_event_time) > event_interval_ms) {
            ESP_LOGW(TAG, "Pairing cycle timeout, restarting pairing process");
            last_event_time = current_time;

            // Reset the pairing process
            if (g_device_type == MASTER) {
                espnow_easy_start_pairing();
            } else {
                // For slave, just reset pairing mode to wait for master
                g_pairing_mode = true;
                g_local_pairing_cycle = 1;
            }
        }
    }
}

bool espnow_easy_set_device_type(device_type_t type) {
    g_device_type = type;
    return true;
}

bool espnow_easy_set_debug(debug_setting_t setting) {
    g_debug_setting = setting;
    return true;
}

void espnow_easy_set_print_received(bool state) {
    g_print_received = state;
}

bool espnow_easy_send_to_slave(uint8_t slave_id, message_type_t msg_type, 
                               const char *data_text, uint8_t data_value) {
    if (g_device_type != MASTER) {
        ESP_LOGE(TAG, "Only master can send to specific slaves");
        return false;
    }

    if (slave_id >= MAX_SLAVES || !g_slaves[slave_id].isPaired) {
        ESP_LOGE(TAG, "Slave %d not paired", slave_id);
        return false;
    }

    g_sending_data.msgType = msg_type;
    strncpy(g_sending_data.dataText, data_text, sizeof(g_sending_data.dataText) - 1);
    g_sending_data.dataText[sizeof(g_sending_data.dataText) - 1] = '\0';
    g_sending_data.dataValue = data_value;

    esp_err_t result = esp_now_send(g_slaves[slave_id].macAddr, 
                                   (const uint8_t *)&g_sending_data, 
                                   sizeof(g_sending_data));

    if (result == ESP_OK) {
        if (g_debug_setting == DEBUG_ON) {
            ESP_LOGD(TAG, "Sent to slave %d", slave_id);
        }
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to send to slave %d: %s", slave_id, esp_err_to_name(result));
        return false;
    }
}

void espnow_easy_broadcast(message_type_t msg_type, const char *data_text, uint8_t data_value) {
    g_sending_data.msgType = msg_type;
    strncpy(g_sending_data.dataText, data_text, sizeof(g_sending_data.dataText) - 1);
    g_sending_data.dataText[sizeof(g_sending_data.dataText) - 1] = '\0';
    g_sending_data.dataValue = data_value;

    if (g_device_type == MASTER) {
        // Send to all paired slaves
        for (uint8_t i = 0; i < MAX_SLAVES; i++) {
            if (g_slaves[i].isPaired) {
                esp_now_send(g_slaves[i].macAddr, (const uint8_t *)&g_sending_data, 
                           sizeof(g_sending_data));
            }
        }
    } else {
        // Slave sends to master
        esp_now_send(g_master_mac, (const uint8_t *)&g_sending_data, sizeof(g_sending_data));
    }
}

bool espnow_easy_start_pairing(void) {
    if (g_device_type != MASTER) {
        ESP_LOGE(TAG, "Only master can initiate pairing");
        return false;
    }

    if (g_slaves_count >= MAX_SLAVES) {
        ESP_LOGW(TAG, "Maximum number of slaves reached");
        g_pairing_mode_state = PAIRING_FULL;
        return false;
    }

    if (g_pairing_mode) {
        ESP_LOGW(TAG, "Pairing already in progress");
        return false;
    }

    // Start pairing process
    g_pairing_mode = true;
    g_pairing_mode_state = PAIRING_OPEN;
    g_local_pairing_cycle = 1;
    g_current_pairing_slave_id = -1;

    if (g_debug_setting == DEBUG_ON) {
        ESP_LOGI(TAG, "Starting pairing for new slave...");
    }

    // Send pairing broadcast
    g_pairing_data.msgType = PAIRING;
    g_pairing_data.pairingCycle = 1;
    strcpy(g_pairing_data.pairingText, "M-CYCLE-1/3");
    memcpy(g_pairing_data.macAddr, g_master_mac, 6);

    esp_now_send(g_broadcast_mac, (const uint8_t *)&g_pairing_data, sizeof(g_pairing_data));

    if (g_debug_setting == DEBUG_ON) {
        ESP_LOGI(TAG, "Pairing broadcast sent");
    }

    g_local_pairing_cycle = 2;
    return true;
}

void espnow_easy_stop_pairing(void) {
    g_pairing_mode = false;
    g_pairing_mode_state = PAIRING_CLOSED;
    g_current_pairing_slave_id = -1;
    
    if (g_debug_setting == DEBUG_ON) {
        ESP_LOGI(TAG, "Pairing stopped");
    }
}

bool espnow_easy_remove_slave(uint8_t slave_id) {
    if (g_device_type != MASTER) {
        ESP_LOGE(TAG, "Only master can remove slaves");
        return false;
    }

    if (slave_id >= MAX_SLAVES || !g_slaves[slave_id].isPaired) {
        ESP_LOGW(TAG, "Slave %d not found", slave_id);
        return false;
    }

    // Remove peer
    esp_now_del_peer(g_slaves[slave_id].macAddr);

    // Clear slave info
    memset(&g_slaves[slave_id], 0, sizeof(slave_info_t));
    memset(g_slaves_mac[slave_id], 0, 6);
    g_slaves_count--;

    // Update pairing mode state
    if (g_slaves_count < MAX_SLAVES) {
        g_pairing_mode_state = PAIRING_CLOSED;
    }

    if (g_debug_setting == DEBUG_ON) {
        ESP_LOGI(TAG, "Removed slave %d. Remaining: %d", slave_id, g_slaves_count);
    }

    return true;
}

void espnow_easy_remove_all_slaves(void) {
    if (g_device_type != MASTER) {
        ESP_LOGE(TAG, "Only master can remove slaves");
        return;
    }

    for (uint8_t i = 0; i < MAX_SLAVES; i++) {
        if (g_slaves[i].isPaired) {
            esp_now_del_peer(g_slaves[i].macAddr);
            memset(&g_slaves[i], 0, sizeof(slave_info_t));
            memset(g_slaves_mac[i], 0, 6);
        }
    }

    g_slaves_count = 0;
    g_pairing_mode_state = PAIRING_CLOSED;

    if (g_debug_setting == DEBUG_ON) {
        ESP_LOGI(TAG, "All slaves removed");
    }
}

slave_info_t* espnow_easy_get_slave_info(uint8_t slave_id) {
    if (slave_id >= MAX_SLAVES || !g_slaves[slave_id].isPaired) {
        return NULL;
    }
    return &g_slaves[slave_id];
}

uint8_t espnow_easy_get_paired_slaves(uint8_t *slave_ids, uint8_t max_count) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_SLAVES && count < max_count; i++) {
        if (g_slaves[i].isPaired) {
            slave_ids[count++] = i;
        }
    }
    return count;
}

bool espnow_easy_is_slave_active(uint8_t slave_id, uint64_t timeout_us) {
    if (slave_id >= MAX_SLAVES || !g_slaves[slave_id].isPaired) {
        return false;
    }

    uint64_t current_time = get_time_us();
    uint64_t elapsed = current_time - g_slaves[slave_id].lastSeen;

    return (elapsed < timeout_us);
}

void espnow_easy_print_slaves_status(void) {
    printf("\n========== SLAVES STATUS ==========\n");
    printf("Total paired: %d / %d\n", g_slaves_count, MAX_SLAVES);
    printf("Pairing mode: ");
    
    switch (g_pairing_mode_state) {
        case PAIRING_CLOSED:
            printf("CLOSED\n");
            break;
        case PAIRING_OPEN:
            printf("OPEN\n");
            break;
        case PAIRING_FULL:
            printf("FULL\n");
            break;
    }
    
    printf("\nPaired Slaves:\n");
    for (uint8_t i = 0; i < MAX_SLAVES; i++) {
        if (g_slaves[i].isPaired) {
            printf("  [%d] %s - ", i, g_slaves[i].slaveName);
            print_mac(g_slaves[i].macAddr);
            
            uint64_t elapsed = get_time_us() - g_slaves[i].lastSeen;
            printf(" - Last seen: %llu ms ago", elapsed / 1000);
            
            if (espnow_easy_is_slave_active(i, 30000000)) {
                printf(" [ACTIVE]\n");
            } else {
                printf(" [INACTIVE]\n");
            }
        }
    }
    printf("===================================\n\n");
}

uint8_t espnow_easy_remove_inactive_slaves(uint64_t timeout_us) {
    uint8_t removed = 0;
    
    for (uint8_t i = 0; i < MAX_SLAVES; i++) {
        if (g_slaves[i].isPaired && !espnow_easy_is_slave_active(i, timeout_us)) {
            if (g_debug_setting == DEBUG_ON) {
                ESP_LOGI(TAG, "Removing inactive slave %d", i);
            }
            espnow_easy_remove_slave(i);
            removed++;
        }
    }
    
    return removed;
}

uint8_t espnow_easy_get_inactive_slaves(uint8_t *slave_ids, uint8_t max_count, uint64_t timeout_us) {
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < MAX_SLAVES && count < max_count; i++) {
        if (g_slaves[i].isPaired && !espnow_easy_is_slave_active(i, timeout_us)) {
            slave_ids[count++] = i;
        }
    }
    
    return count;
}

espnow_status_t espnow_easy_get_status(void) {
    espnow_status_t status = {0};
    
    status.isPairingActive = g_pairing_mode;
    status.deviceType = g_device_type;
    status.pairingCycle = g_local_pairing_cycle;
    status.slavesCount = g_slaves_count;
    status.maxSlaves = MAX_SLAVES;
    
    if (g_device_type == MASTER) {
        status.isPaired = (g_slaves_count > 0);
        // Count active slaves
        status.activeSlaves = 0;
        for (uint8_t i = 0; i < MAX_SLAVES; i++) {
            if (g_slaves[i].isPaired && espnow_easy_is_slave_active(i, 30000000)) {
                status.activeSlaves++;
            }
        }
    } else {
        // For slave, isPaired means paired to master
        status.isPaired = (g_local_pairing_cycle > 1 && !g_pairing_mode);
        memcpy(status.pairedMacAddr, g_master_mac, 6);
        status.activeSlaves = 0;
    }
    
    return status;
}
