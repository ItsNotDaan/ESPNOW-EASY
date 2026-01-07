# ESPNOW-EASY - ESP-IDF Implementation

This directory contains the native ESP-IDF implementation of ESPNOW-EASY with no Arduino dependencies.

## Quick Links

- **[ESP-IDF-README.md](ESP-IDF-README.md)** - Complete API documentation and usage guide
- **[BUILD-INSTRUCTIONS.md](BUILD-INSTRUCTIONS.md)** - Step-by-step build and flash instructions
- **[ESP-IDF-MIGRATION-SUMMARY.md](ESP-IDF-MIGRATION-SUMMARY.md)** - Arduino to ESP-IDF migration details

## Quick Start

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Project Structure

```
ESP-IDF/
├── components/
│   └── espnow_easy/         # Core ESP-NOW library
├── examples/
│   ├── master/              # Basic master node
│   ├── slave/               # Basic slave node
│   ├── multi_slave/         # Multi-slave demo
│   ├── master_led/          # Master with LED
│   └── slave_led/           # Slave with LED
├── main/                    # Default application
├── CMakeLists.txt           # Build configuration
└── sdkconfig.defaults       # Default ESP-IDF config
```

## Switching Examples

Edit `CMakeLists.txt` and uncomment the desired example:

```cmake
set(EXTRA_COMPONENT_DIRS 
    components
    # main                  # Default placeholder
    examples/master       # Uncomment to build master
    # examples/slave        # Uncomment to build slave
    # ...
)
```

## Features

- ✅ Pure C implementation (no Arduino)
- ✅ Multi-slave support (up to 10 devices)
- ✅ Native RMT driver for WS2812B LEDs
- ✅ Hardware-accelerated timing
- ✅ Menuconfig integration
- ✅ Production-ready

For detailed information, see [ESP-IDF-README.md](ESP-IDF-README.md).
