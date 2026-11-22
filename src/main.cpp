#include <Arduino.h>
#include "MPU6050Sensor.h"
#include "VibrationAnalyzer.h"
#include "WiFiManager.h"
#include "AlertManager.h"
#include "TelegramNotifier.h"
#include "secrets.h"  // WiFi + Telegram credentials (not in Git)

// ============================================================
// Laundry-IQ — Full System with FreeRTOS
// ============================================================

static constexpr float    VIBRATION_THRESHOLD  = 0.05f;   // Low enough to catch gentle wash programs
static constexpr uint16_t START_COUNT_REQUIRED = 10;
static constexpr uint16_t STOP_COUNT_REQUIRED  = 1200;    // 60 seconds of quiet before declaring DONE

static constexpr uint32_t SAMPLING_INTERVAL_MS = 50;
static constexpr uint32_t NETWORK_CHECK_MS     = 1000;
// =============================================================

MPU6050Sensor sensor;
VibrationAnalyzer analyzer(VIBRATION_THRESHOLD, START_COUNT_REQUIRED, STOP_COUNT_REQUIRED);
AlertManager alertManager;
TelegramNotifier telegramNotifier(BOT_TOKEN, CHAT_ID);

SemaphoreHandle_t stateMutex = nullptr;
MachineState sharedState = MachineState::IDLE;

void samplingTask(void* pvParameters) {
    Serial.println("[SamplingTask] Started on core " + String(xPortGetCoreID()));

    while (true) {
        float magnitude = sensor.calcMagnitude();

        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            analyzer.feedSample(magnitude);
            sharedState = analyzer.getState();
            xSemaphoreGive(stateMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLING_INTERVAL_MS));
    }
}

void networkTask(void* pvParameters) {
    Serial.println("[NetworkTask] Started on core " + String(xPortGetCoreID()));

    MachineState localState = MachineState::IDLE;
    MachineState previousState = MachineState::IDLE;

    while (true) {
        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            localState = sharedState;
            xSemaphoreGive(stateMutex);
        }

        if (localState != previousState) {
            Serial.printf("[NetworkTask] State changed: %s → %s\n",
                          stateToString(previousState),
                          stateToString(localState));

            alertManager.notify(localState);

            if (localState == MachineState::DONE) {
                vTaskDelay(pdMS_TO_TICKS(2000));

                if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    analyzer.reset();
                    sharedState = MachineState::IDLE;
                    xSemaphoreGive(stateMutex);
                }
            }

            previousState = localState;
        }

        vTaskDelay(pdMS_TO_TICKS(NETWORK_CHECK_MS));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("╔══════════════════════════════════════╗");
    Serial.println("║       Laundry-IQ v1.0                ║");
    Serial.println("║  Washing Machine Completion Alert     ║");
    Serial.println("╚══════════════════════════════════════╝\n");

    if (!sensor.init()) {
        Serial.println("FATAL: Sensor init failed. Halting.");
        while (true) { delay(1000); }
    }

    WiFiManager& wifi = WiFiManager::getInstance();
    if (!wifi.connect(WIFI_SSID, WIFI_PASSWORD, 20000)) {
        Serial.println("WARNING: WiFi failed. Alerts won't work.");
    }

    alertManager.subscribe(&telegramNotifier);

    stateMutex = xSemaphoreCreateMutex();
    if (stateMutex == nullptr) {
        Serial.println("FATAL: Mutex creation failed. Halting.");
        while (true) { delay(1000); }
    }

    xTaskCreatePinnedToCore(samplingTask, "SamplingTask", 4096, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(networkTask, "NetworkTask", 8192, nullptr, 1, nullptr, 1);

    Serial.println("\n[Main] All tasks created. System running.\n");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
