#pragma once
#include <Arduino.h>
#include <WiFi.h>

/**
 * @brief Singleton WiFi connection manager.
 *
 * Why Singleton?
 * - There is exactly ONE WiFi radio on the ESP32.
 * - If two threads both call WiFi.begin() simultaneously → crash.
 * - Singleton guarantees a single instance and a global access point.
 *
 * Interview Q: "What are the downsides of Singleton?"
 * A: "1) Hard to unit test — you can't inject a mock.
 *     2) Hidden dependency — any code can call getInstance().
 *     3) Thread safety of getInstance() itself needs care.
 *     In production I might use Dependency Injection instead,
 *     but for a resource like WiFi hardware that truly has one
 *     instance, Singleton is appropriate."
 *
 * Interview Q: "Is this Singleton thread-safe?"
 * A: "The Meyer's Singleton (static local variable) is guaranteed
 *     thread-safe since C++11. The compiler inserts the synchronization.
 *     But the connect() and isConnected() methods still need external
 *     synchronization if called from multiple tasks — in our design,
 *     only the Network Task touches WiFi, so that's not an issue."
 */
class WiFiManager {
public:
    // Delete copy/move — Singleton must not be copied
    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;

    /**
     * @brief Get the single instance (Meyer's Singleton).
     *
     * The static local variable is initialized exactly once,
     * even if called from multiple threads (C++11 guarantee).
     */
    static WiFiManager& getInstance();

    /**
     * @brief Connect to a WiFi network.
     * @param ssid Network name.
     * @param password Network password.
     * @param timeoutMs Max time to wait for connection.
     * @return true if connected, false if timeout.
     */
    bool connect(const char* ssid, const char* password,
                 uint32_t timeoutMs = 10000);

    /**
     * @brief Check if WiFi is currently connected.
     */
    bool isConnected() const;

    /**
     * @brief Get the local IP address (for debug logging).
     */
    String getIP() const;

private:
    // Private constructor — only getInstance() can create
    WiFiManager() = default;
};
