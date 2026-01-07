# Quick Start Guide - ESP-IDF Build Instructions

## Prerequisites

1. **Install ESP-IDF v5.0+**
   ```bash
   # Follow official guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
   # Or quick install:
   mkdir -p ~/esp
   cd ~/esp
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh esp32
   ```

2. **Set up ESP-IDF environment**
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```
   
   Add this to your `~/.bashrc` or `~/.zshrc` for convenience.

## Building Examples

### 1. Master Example (Basic)

```bash
cd ESPNOW-EASY

# Edit CMakeLists.txt - uncomment the master example line:
# main                  # Default placeholder application
examples/master       # Uncomment to build master example

# Build and flash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 2. Slave Example (Basic)

```bash
# Edit CMakeLists.txt - uncomment the slave example line:
# main                  # Default placeholder application
examples/slave        # Uncomment to build slave example

# Build and flash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3. Multi-Slave Example

```bash
# Edit CMakeLists.txt:
examples/multi_slave  # Uncomment to build multi-slave example

# Build and flash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 4. Master with LED

```bash
# Edit CMakeLists.txt:
examples/master_led   # Uncomment to build master with LED

# Build and flash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

**Hardware Requirements:**
- WS2812B LED strip connected to GPIO 5
- 5V power supply for LEDs

### 5. Slave with LED

```bash
# Edit CMakeLists.txt:
examples/slave_led    # Uncomment to build slave with LED

# Build and flash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Quick Edit CMakeLists.txt

Instead of manually editing, you can use sed:

```bash
# Switch to master example
sed -i 's|    main|    # main|' CMakeLists.txt
sed -i 's|    # examples/master|    examples/master|' CMakeLists.txt

# Switch to slave example
sed -i 's|    main|    # main|' CMakeLists.txt
sed -i 's|    # examples/slave|    examples/slave|' CMakeLists.txt

# Switch back to default
sed -i 's|    # main|    main|' CMakeLists.txt
sed -i 's|    examples/.*|    # &|' CMakeLists.txt
```

## Common Build Commands

```bash
# Full build
idf.py build

# Clean build
idf.py fullclean
idf.py build

# Flash only (no build)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Build, flash, and monitor in one command
idf.py -p /dev/ttyUSB0 flash monitor

# Configure project
idf.py menuconfig

# Check partition table
idf.py partition-table

# Erase flash completely
idf.py -p /dev/ttyUSB0 erase-flash
```

## Serial Port Selection

### Linux
- Usually `/dev/ttyUSB0` or `/dev/ttyACM0`
- Check with: `ls /dev/tty*`
- Add user to dialout group: `sudo usermod -a -G dialout $USER`

### macOS
- Usually `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`
- Check with: `ls /dev/cu.*`

### Windows
- Usually `COM3`, `COM4`, etc.
- Check in Device Manager

## Configuration Options

Edit `sdkconfig.defaults` to change default settings:

```ini
# WiFi settings
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32

# ESP-NOW
CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM=6

# RMT (for LEDs)
CONFIG_RMT_ISR_IRAM_SAFE=y

# Serial
CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200
```

Or use menuconfig:
```bash
idf.py menuconfig
# Navigate: Component config -> ESP-NETIF Adapter -> ESP-NOW
```

## Troubleshooting

### Build Errors

```bash
# Clean everything
idf.py fullclean
rm -rf build sdkconfig

# Rebuild
idf.py build
```

### Flash Errors

```bash
# Try different baud rate
idf.py -p /dev/ttyUSB0 -b 115200 flash

# Or force flash mode
idf.py -p /dev/ttyUSB0 flash --force
```

### Permission Denied (Linux)

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Or use sudo
sudo idf.py -p /dev/ttyUSB0 flash monitor
```

### Monitor Exit

- Press `Ctrl+]` to exit monitor
- Or `Ctrl+T` then `Ctrl+X` for older IDF versions

## Testing Multi-Device Setup

### Setup 1: One Master, Multiple Slaves

**Device 1 (Master):**
```bash
# Edit CMakeLists.txt for master or multi_slave
idf.py -p /dev/ttyUSB0 flash monitor
```

**Device 2-4 (Slaves):**
```bash
# Edit CMakeLists.txt for slave
idf.py -p /dev/ttyUSB1 flash monitor  # Change port for each device
```

### Setup 2: Master with LED + Slaves with LED

**Master:**
```bash
# Connect WS2812B to GPIO 5
# Edit CMakeLists.txt for master_led
idf.py -p /dev/ttyUSB0 flash monitor
```

**Slaves:**
```bash
# Connect WS2812B to GPIO 5 on each slave
# Edit CMakeLists.txt for slave_led
idf.py -p /dev/ttyUSB1 flash monitor
```

## Expected Output

### Master
```
I (xxx) MASTER: ========================================
I (xxx) MASTER: ESP-NOW Master - Multi-Slave Example
I (xxx) MASTER: ========================================
I (xxx) ESPNOW_EASY: Master MAC: XX:XX:XX:XX:XX:XX
I (xxx) ESPNOW_EASY: ESP-NOW Easy initialized successfully
I (xxx) MASTER: Master initialized successfully
I (xxx) MASTER: Ready to pair slaves...
I (xxx) ESPNOW_EASY: Starting pairing for new slave...
I (xxx) ESPNOW_EASY: Pairing broadcast sent
```

### Slave
```
I (xxx) SLAVE: ========================================
I (xxx) SLAVE: ESP-NOW Slave Example
I (xxx) SLAVE: ========================================
I (xxx) ESPNOW_EASY: Slave MAC: XX:XX:XX:XX:XX:XX
I (xxx) ESPNOW_EASY: ESP-NOW Easy initialized successfully
I (xxx) ESPNOW_EASY: Slave ready for pairing
I (xxx) SLAVE: Slave initialized successfully
I (xxx) SLAVE: Waiting for master to pair...
```

## Next Steps

After successful pairing, you should see:
- Master: "Pairing complete! Total slaves paired: 1"
- Slave: "Pairing complete! Paired with master"
- Data exchange messages appearing on both devices

Refer to `ESP-IDF-README.md` for detailed API documentation and advanced usage.
