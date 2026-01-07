# ESPNOW-EASY - ESP-IDF Implementation

A comprehensive ESP-NOW library for ESP32 built with native ESP-IDF (no Arduino compatibility layer).

## Features

- **Multi-Slave Support**: Connect up to 10 encrypted slave devices to one master
- **Automatic Pairing**: Sequential pairing with 3-way handshake protocol
- **Heartbeat & Timeout**: Automatic detection and removal of inactive slaves
- **LED Status Indicators**: Visual feedback using WS2812B LEDs via RMT driver
- **Native ESP-IDF**: Built with pure ESP-IDF APIs for optimal performance

## Project Structure

```
ESPNOW-EASY/
├── components/
│   └── espnow_easy/          # Main ESP-NOW library component
│       ├── espnow_easy.c
│       ├── espnow_easy.h
│       └── CMakeLists.txt
├── examples/
│   ├── master/               # Basic master example
│   ├── slave/                # Basic slave example
│   ├── multi_slave/          # Multi-slave demonstration
│   ├── master_led/           # Master with LED status
│   └── slave_led/            # Slave with LED status
├── main/                     # Default main application
├── CMakeLists.txt            # Root CMakeLists
└── sdkconfig.defaults        # Default configuration
```

## Requirements

- **ESP-IDF**: v5.0 or later
- **Hardware**: ESP32, ESP32-S2, ESP32-S3, or ESP32-C3
- **Optional**: WS2812B LED strip for LED examples

## Installation & Build

### 1. Install ESP-IDF

Follow the [official ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).

### 2. Clone this repository

```bash
git clone https://github.com/ItsNotDaan/ESPNOW-EASY.git
cd ESPNOW-EASY
```

### 3. Select an example to build

Edit `CMakeLists.txt` and modify the `EXTRA_COMPONENT_DIRS` to select which example to use:

```cmake
# For master example:
set(EXTRA_COMPONENT_DIRS 
    components
    examples/master
)

# For multi-slave example:
set(EXTRA_COMPONENT_DIRS 
    components
    examples/multi_slave
)

# For master with LED:
set(EXTRA_COMPONENT_DIRS 
    components
    examples/master_led
)
```

### 4. Configure and Build

```bash
# Set ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Configure (optional - uses sdkconfig.defaults)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## API Reference

### Initialization

```c
#include "espnow_easy.h"

// Initialize as master
bool espnow_easy_init(MASTER, DEBUG_ON);

// Initialize as slave
bool espnow_easy_init(SLAVE, DEBUG_ON);
```

### Master Functions

#### Start Pairing

```c
// Start pairing for one new slave
bool espnow_easy_start_pairing(void);

// Stop accepting new slaves
void espnow_easy_stop_pairing(void);
```

#### Sending Data

```c
// Send to specific slave by ID (0-9)
bool espnow_easy_send_to_slave(uint8_t slave_id, message_type_t msg_type, 
                               const char *data_text, uint8_t data_value);

// Broadcast to all paired slaves
void espnow_easy_broadcast(message_type_t msg_type, const char *data_text, 
                           uint8_t data_value);
```

#### Slave Management

```c
// Remove specific slave
bool espnow_easy_remove_slave(uint8_t slave_id);

// Remove all slaves
void espnow_easy_remove_all_slaves(void);

// Get slave information
slave_info_t* espnow_easy_get_slave_info(uint8_t slave_id);

// Get list of paired slave IDs
uint8_t espnow_easy_get_paired_slaves(uint8_t *slave_ids, uint8_t max_count);

// Check if slave is active
bool espnow_easy_is_slave_active(uint8_t slave_id, uint64_t timeout_us);

// Print status of all slaves
void espnow_easy_print_slaves_status(void);

// Remove inactive slaves automatically
uint8_t espnow_easy_remove_inactive_slaves(uint64_t timeout_us);

// Get list of inactive slaves
uint8_t espnow_easy_get_inactive_slaves(uint8_t *slave_ids, uint8_t max_count, 
                                        uint64_t timeout_us);
```

### Common Functions

```c
// Check pairing status and handle timeouts
void espnow_easy_check_pairing_status(uint32_t wait_time_ms);

// Get current status
espnow_status_t espnow_easy_get_status(void);

// Enable/disable printing received messages
void espnow_easy_set_print_received(bool state);
```

### Slave Functions

```c
// Slaves use broadcast to send to master
void espnow_easy_broadcast(message_type_t msg_type, const char *data_text, 
                           uint8_t data_value);
```

## Usage Examples

### Master Example

```c
#include "espnow_easy.h"

void app_main(void) {
    // Initialize NVS
    nvs_flash_init();
    
    // Initialize ESP-NOW as master
    if (!espnow_easy_init(MASTER, DEBUG_ON)) {
        ESP_LOGE("MASTER", "Init failed!");
        return;
    }
    
    // Start pairing
    espnow_easy_start_pairing();
    
    while (1) {
        // Check pairing status
        espnow_easy_check_pairing_status(5000);
        
        espnow_status_t status = espnow_easy_get_status();
        
        // Broadcast to all slaves
        if (status.slavesCount > 0) {
            espnow_easy_broadcast(DATA, "Hello slaves!", 42);
        }
        
        // Send to specific slave
        espnow_easy_send_to_slave(0, DATA, "Private msg", 99);
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### Slave Example

```c
#include "espnow_easy.h"

void app_main(void) {
    // Initialize NVS
    nvs_flash_init();
    
    // Initialize ESP-NOW as slave
    if (!espnow_easy_init(SLAVE, DEBUG_ON)) {
        ESP_LOGE("SLAVE", "Init failed!");
        return;
    }
    
    espnow_easy_set_print_received(true);
    
    while (1) {
        // Check pairing status
        espnow_easy_check_pairing_status(5000);
        
        espnow_status_t status = espnow_easy_get_status();
        
        // Send response to master if paired
        if (status.isPaired) {
            espnow_easy_broadcast(DATA, "Response", 123);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### Multi-Slave with Timeout Handling

```c
void app_main(void) {
    nvs_flash_init();
    espnow_easy_init(MASTER, DEBUG_ON);
    espnow_easy_start_pairing();
    
    uint64_t last_timeout_check = 0;
    
    while (1) {
        espnow_easy_check_pairing_status(5000);
        
        uint64_t current_time = esp_timer_get_time();
        
        // Check for inactive slaves every 15 seconds
        if (current_time - last_timeout_check > 15000000) {
            uint8_t removed = espnow_easy_remove_inactive_slaves(30000000);
            if (removed > 0) {
                ESP_LOGW("MASTER", "Removed %d inactive slaves", removed);
            }
            last_timeout_check = current_time;
        }
        
        // Continue pairing if needed
        espnow_status_t status = espnow_easy_get_status();
        if (status.slavesCount < 10 && !status.isPairingActive) {
            espnow_easy_start_pairing();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## LED Examples

The LED examples use ESP-IDF's native RMT driver for WS2812B control.

### Master LED Example

- Displays color gradient based on number of paired slaves
- 0 slaves = Red
- 3 slaves = Yellow/Green
- 6 slaves = Cyan
- 10 slaves = Purple/Pink

### Slave LED Example

- Red: Not paired
- Yellow (blinking): Pairing in progress
- Green: Successfully paired

### Hardware Connection

```
ESP32 GPIO5 -----> WS2812B Data In
              |
              +--- 5V Power
              |
              +--- GND
```

## Configuration

### sdkconfig.defaults

Key settings:
- `CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM=6` - Max encrypted peers
- `CONFIG_RMT_ISR_IRAM_SAFE=y` - RMT driver in IRAM for LEDs
- `CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200` - Serial baud rate

### Component Kconfig

Edit `components/espnow_easy/Kconfig` (if created) to expose configuration options.

## Performance

- **Message Rate**: ~250 packets/sec total (ESP-NOW limit)
- **Per Slave**: ~25 messages/sec with 10 slaves
- **Latency**: 1-5ms typical
- **Range**: 100-200m line of sight
- **Memory**: ~300 bytes additional RAM for multi-slave tracking

## Troubleshooting

### Build Errors

```bash
# Clean and rebuild
idf.py fullclean
idf.py build
```

### Pairing Issues

- Ensure both devices are on the same WiFi channel (default: 1)
- Check that broadcast peer is not deleted
- Verify serial output for debug messages

### LED Issues

- Check GPIO pin configuration (default: GPIO5)
- Verify WS2812B power supply (5V, sufficient current)
- Check data line connection
- Enable RMT driver in sdkconfig

## Migration from Arduino

Key differences:
- Use `esp_timer_get_time()` instead of `millis()` (returns microseconds)
- Use `vTaskDelay(pdMS_TO_TICKS(ms))` instead of `delay(ms)`
- Use `ESP_LOGx()` macros instead of `Serial.print()`
- No Arduino.h dependency

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

[Add your license here]

## Credits

Developed by ItsNotDaan
