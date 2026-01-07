# ESPNOW-EASY

ESPNOW-EASY is a library for easy communication between ESP8266 and ESP32 devices using the ESPNOW protocol.
Using this library it is easy to establish a connection between Master and Slave.

## Available Implementations

This repository contains two implementations:

### 1. Arduino Implementation (`Arduino/`)
- Arduino-based library with FastLED support
- Compatible with PlatformIO and Arduino IDE
- See [Arduino/examples/](Arduino/examples/) for usage examples

### 2. ESP-IDF Implementation (`ESP-IDF/`)
- Native ESP-IDF v5.0+ implementation (no Arduino dependencies)
- Hardware-accelerated WS2812B LED control via RMT driver
- See [ESP-IDF/ESP-IDF-README.md](ESP-IDF/ESP-IDF-README.md) for complete documentation

## Features

- Simple and intuitive API for sending and receiving data
- Automatic pairing and device discovery
- Multi-slave support (up to 10 devices)
- Reliable and efficient communication over a local network
- Support for both ESP8266 and ESP32 platforms
- LED status indicators (optional)

## Quick Start

### Arduino/PlatformIO

1. Clone the repository: `git clone https://github.com/ItsNotDaan/ESPNOW-EASY.git`
2. Copy the `Arduino/` library folder to your project
3. Include the library: `#include <ESPNOW-EASY.h>`

See [Arduino README](Arduino/) for detailed Arduino usage.

### ESP-IDF

1. Clone the repository: `git clone https://github.com/ItsNotDaan/ESPNOW-EASY.git`
2. Navigate to `ESP-IDF/` directory
3. Build with: `idf.py build`

See [ESP-IDF/BUILD-INSTRUCTIONS.md](ESP-IDF/BUILD-INSTRUCTIONS.md) for detailed build instructions.

## Contributing

Contributions are welcome! If you have any ideas, bug reports, or feature requests, please open an issue or submit a pull request.

