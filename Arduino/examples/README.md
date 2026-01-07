# ESP-NOW Examples

Clean, simple examples demonstrating the ESPNOW-EASY library functionality.

## Basic Examples

### 📄 **main.cpp**

**Multi-slave master demonstration**

- Auto-pairs up to 3 slaves
- Broadcasts messages to all slaves
- Sends targeted messages to specific slaves
- Displays slave status periodically

**Use this as**: The main reference example for multi-slave functionality.

---

### 📄 **main_master.cpp**

**Clean master example**

- Auto-pairs multiple slaves (configurable target)
- Demonstrates broadcasting to all slaves
- Demonstrates targeted messaging to specific slaves
- Periodic status monitoring

**Best for**: Understanding basic master functionality without extra complexity.

---

### 📄 **main_slave.cpp**

**Clean slave example**

- Waits for master pairing
- Sends periodic responses to master
- Status monitoring
- Simple and straightforward

**Best for**: Understanding basic slave functionality.

---

## LED Examples

### 💡 **main_master_led.cpp**

**Master with visual LED feedback**

**LED Colors based on paired slave count:**

- Smooth color gradient that transitions as slaves are added
- **0 slaves** → Red
- **3 slaves** → Yellow/Green
- **6 slaves** → Cyan
- **10 slaves** → Purple/Pink
- Each additional slave shifts the hue by 25 units, creating a rainbow effect

**Features:**

- Visual confirmation of slave count
- Auto-pairs up to 4 slaves
- Broadcasts heartbeat messages

**Pin Configuration:**

```cpp
#define LED_R_PIN 25  // Red LED
#define LED_G_PIN 26  // Green LED
#define LED_B_PIN 27  // Blue LED
```

---

### 💡 **main_slave_led.cpp**

**Slave with visual pairing status**

**LED States:**

- 🔴 **Red (solid)** = Not paired
- 🟡 **Yellow (blinking)** = Pairing in progress
- 🟢 **Green (solid)** = Successfully paired

**Features:**

- Visual pairing feedback
- Blinking animation during pairing
- Connection status monitoring
- Sends heartbeat when paired
- Uses FastLED library for easy control

**Hardware Setup:**

- Addressable RGB LED (WS2812B/NeoPixel)
- Single data wire to ESP32

**Configuration:**

```cpp
#define LED_PIN 5        // Data pin for LED
#define NUM_LEDS 1       // Number of LEDs
#define LED_TYPE WS2812B // LED type
```

---

## Quick Start

### Running Master

1. Open `main_master.cpp` or `main.cpp`
2. Verify `#define DEVICE_TYPE MASTER`
3. Upload to your ESP32
4. Open Serial Monitor (115200 baud)
5. Watch as it discovers and pairs slaves

### Running Slave

1. Open `main_slave.cpp`
2. Verify `#define DEVICE_TYPE SLAVE`
3. Upload to your ESP32
4. Open Serial Monitor (115200 baud)
5. It will automatically pair when master broadcasts

### LED Examples Setup

1. **Install FastLED library** in Arduino IDE (Sketch → Include Library → Manage Libraries → Search "FastLED")
2. Connect an addressable RGB LED (WS2812B/NeoPixel) to pin 5 (or modify `LED_PIN`)
3. Upload the LED example (master or slave)
4. Watch the LED change based on status!

---

## Key Functions Demonstrated

### Master Functions

```cpp
startPairingForNewSlave();              // Start pairing one new slave
broadcastData(DATA, "message", value);  // Send to all slaves
sendDataToSlave(id, DATA, "msg", val);  // Send to specific slave
printSlavesStatus();                    // Print all slave info
getESPNOWStatus();                      // Get current status
```

### Slave Functions

```cpp
broadcastData(DATA, "message", value);  // Send to master
getESPNOWStatus();                      // Get pairing status
```

### Common Functions

```cpp
initESPNOW(DEVICE_TYPE, DEBUG_SETTING); // Initialize
checkPairingModeStatus(timeout);        // Monitor pairing
setReceivedMessageOnMonitor(true);      // Print received data
```

---

## Hardware Requirements

- **ESP32** (any variant with WiFi)
- **For LED examples**:
  - Addressable RGB LED (WS2812B, WS2811, NeoPixel, etc.)
  - FastLED library (install via Arduino Library Manager)
  - No resistors needed for data line (optional for longer cables)

## Pin Modifications

If your board uses a different pin for the LED data line:

```cpp
#define LED_PIN 5  // Change to your data pin
```

For different LED types, modify:

```cpp
#define LED_TYPE WS2812B  // Options: WS2812B, WS2811, NEOPIXEL, etc.
#define COLOR_ORDER GRB   // Options: GRB, RGB, BRG (depends on your LED)
```

---

## Tips

- **Serial Monitor**: Always open Serial Monitor (115200 baud) to see status messages
- **Pairing**: Master must be running before slaves start
- **Multiple Slaves**: Use different ESP32 boards as slaves, they'll pair automatically
- **Testing**: Start with basic examples (`main_master.cpp` + `main_slave.cpp`) before trying LED examples

---

**Happy coding!** 🚀
