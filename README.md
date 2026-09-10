# 🧺 Laundry-IQ

**Smart washing machine completion alert system built on ESP32 + FreeRTOS.**

Laundry-IQ uses an MPU-6050 accelerometer to detect vibration patterns on a washing machine. When the cycle finishes, it sends a Telegram notification to your phone — no more forgotten laundry.

## Features

- **Real-time vibration monitoring** — MPU-6050 accelerometer over I2C, sampled at 20 Hz
- **Intelligent state detection** — State machine with hysteresis/debouncing to avoid false triggers
- **Dual-core FreeRTOS** — Sensor sampling on Core 0, network communication on Core 1
- **Telegram alerts** — Notifications when the machine starts running and when the cycle completes
- **Thread-safe design** — Mutex-protected shared state between concurrent tasks

## Architecture

```
┌─────────────┐     ┌───────────────────┐     ┌──────────────┐     ┌───────────────────┐
│ MPU-6050     │────→│ VibrationAnalyzer │────→│ AlertManager │────→│ TelegramNotifier  │
│ (I2C HAL)   │     │ (State Machine)   │     │ (Subject)    │     │ (Observer)        │
└─────────────┘     └───────────────────┘     └──────────────┘     └───────────────────┘
   Core 0                Core 0                   Core 1                 Core 1
   Sampling Task         Sampling Task            Network Task           Network Task
                    ↕ Mutex ↕
               ┌──────────────┐
               │ sharedState  │
               └──────────────┘
```

### State Machine

```
IDLE ──(10 active samples)──→ RUNNING ──(100 quiet samples)──→ DONE
  ↑                                                              │
  └───────────────────── reset() ←───────────────────────────────┘
```

- **IDLE → RUNNING**: 10 consecutive samples with vibration deviation > 0.15g (500ms)
- **RUNNING → DONE**: 100 consecutive quiet samples (5 seconds of no vibration)
- **DONE → IDLE**: Automatic reset after alert is sent

## Design Patterns

| Pattern | Implementation | Purpose |
|---------|---------------|---------|
| **HAL (Hardware Abstraction)** | `MPU6050Sensor` | Encapsulates I2C communication. Swapping sensors requires changing only this class |
| **Singleton** | `WiFiManager` | Single WiFi radio = single instance. Meyer's Singleton with C++11 thread safety |
| **Observer** | `IObserver` → `AlertManager` → `TelegramNotifier` | Decoupled notification system. Add SMS/Email observers without modifying AlertManager |
| **State Machine** | `VibrationAnalyzer` | Explicit states (IDLE/RUNNING/DONE) with debounced transitions |

## FreeRTOS Task Design

| Task | Core | Priority | Stack | Interval | Responsibility |
|------|------|----------|-------|----------|----------------|
| `samplingTask` | 0 | 2 (high) | 4 KB | 50 ms | Read sensor, feed state machine |
| `networkTask` | 1 | 1 (low) | 8 KB | 1000 ms | Detect state changes, send alerts |

- **Why two tasks?** HTTP requests take hundreds of milliseconds — running them in the sampling loop would cause missed sensor readings
- **Why different priorities?** Sensor sampling is time-critical; network can wait
- **Why 8 KB stack for network?** HTTPS + TLS requires large buffers for encryption

### Concurrency

Shared state between tasks is protected by a FreeRTOS Mutex (`xSemaphoreCreateMutex`):

```cpp
// Writer (Sampling Task)
if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    sharedState = analyzer.getState();
    xSemaphoreGive(stateMutex);
}

// Reader (Network Task)
if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    localState = sharedState;
    xSemaphoreGive(stateMutex);
}
```

## Hardware

### Components

| Component | Description |
|-----------|-------------|
| ESP32 DevKit V1 | Dual-core MCU with WiFi |
| MPU-6050 (GY-521) | 3-axis accelerometer + gyroscope |
| Jumper wires | 4× Female-to-Female |
| USB cable | Micro-USB for power + programming |

### Wiring

| MPU-6050 Pin | ESP32 Pin | Wire Color (convention) |
|-------------|-----------|------------------------|
| VCC | 3.3V | Red |
| GND | GND | Black |
| SDA | GPIO 21 | Blue |
| SCL | GPIO 22 | Yellow |
| AD0 | GND | (tied low for address 0x68) |

> **Note:** AD0 must be connected to GND even with a single sensor to avoid a floating pin, which could cause intermittent address mismatches.

## Project Structure

```
laundry-iq/
├── include/
│   ├── MPU6050Sensor.h       # HAL — I2C sensor abstraction
│   ├── VibrationAnalyzer.h   # State machine + MachineState enum
│   ├── WiFiManager.h         # Singleton WiFi manager
│   ├── IObserver.h           # Observer interface (pure virtual)
│   ├── AlertManager.h        # Subject — manages observers
│   ├── TelegramNotifier.h    # Concrete observer — Telegram API
│   ├── secrets.h.example     # Template for credentials
│   └── secrets.h             # Your credentials (gitignored)
├── src/
│   ├── main.cpp              # FreeRTOS tasks + system setup
│   ├── MPU6050Sensor.cpp
│   ├── VibrationAnalyzer.cpp
│   ├── WiFiManager.cpp
│   ├── AlertManager.cpp
│   └── TelegramNotifier.cpp
├── platformio.ini
├── .gitignore
└── README.md
```

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- ESP32 DevKit board
- MPU-6050 sensor module

### Setup

1. Clone this repository
2. Copy the credentials template:
   ```bash
   cp include/secrets.h.example include/secrets.h
   ```
3. Edit `include/secrets.h` with your WiFi and Telegram Bot credentials
4. Connect the hardware as shown in the wiring table above
5. Build and upload:
   ```bash
   pio run --target upload
   ```
6. Open Serial Monitor at 115200 baud to see system logs

### Setting Up Telegram Bot

1. Message [@BotFather](https://t.me/BotFather) on Telegram → `/newbot`
2. Copy the bot token to `secrets.h`
3. Message your new bot, then visit:
   ```
   https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates
   ```
4. Find your `chat_id` in the JSON response

## Key Interview Concepts

This project demonstrates:

- **OOP & Encapsulation** — Private members, clean public interfaces, `enum class`
- **Design Patterns** — Singleton (WiFiManager), Observer (AlertManager + IObserver)
- **FreeRTOS** — Task creation, core pinning, priorities, `vTaskDelay`
- **Concurrency** — Mutex, race conditions, priority inversion awareness
- **I2C Protocol** — Register reads, burst read, repeated start, WHO_AM_I validation
- **Embedded Best Practices** — HAL, defensive init, debouncing, error handling

## License

MIT
