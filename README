# ESPNOW-EASY

ESPNOW-EASY is a library for easy communication between ESP8266 and ESP32 devices using the ESPNOW protocol.
Using this library it is easy to establish a connection between Master and Slave.

## Features

- Simple and intuitive API for sending and receiving data.
- Automatic pairing and device discovery.
- Reliable and efficient communication over a local network.
- Support for both ESP8266 and ESP32 platforms.

## Installation

1. Clone the repository to test out the program using platformio: `git clone https://github.com/your-username/ESPNOW-EASY.git`
2. Copy the library folder and include it in your own project.
2. Include the library in your Arduino project using: `#include <ESPNOW-EASY.h>`

## Usage
1. Include the library using `#include <ESPNOW-EASY.h>`
2. Define a device type using: `#define DEVICE_TYPE MASTER` MASTER/SLAVE
3. Define a debug setting using: `#define DEBUG_SETTING DEBUG_ON` DEBUG_ON/DEBUG_OFF
4. Init the ESPNOW using the following:
```
  if (initESPNOW(DEVICE_TYPE, DEBUG_SETTING) == false)
    {
      Serial.println("ESP-NOW initialization failed");
      ESP.restart();
    }
```
5. Start the pairing process: `startPairingProcess();`
6. Set setReceivedMessageOnMonitor to true to print the received message on the monitor: `setReceivedMessageOnMonitor(true);` 
7. Check the pairing mode status every 5 seconds if pairing mode is active: `checkPairingModeStatus(5000);`
8. To send data use the function: `sendData(DATA, "Hello, Slave!", 42);`

## Contributing

Contributions are welcome! If you have any ideas, bug reports, or feature requests, please open an issue or submit a pull request.

