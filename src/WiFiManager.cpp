#include "WiFiManager.h"

// ---------------------------------------------------------------
// getInstance() — Meyer's Singleton.
//
// How it works:
// 1. First call: creates the static WiFiManager instance
// 2. All subsequent calls: return a reference to the same instance
// 3. Thread-safe by C++11 standard (compiler guarantees it)
//
// Interview Q: "Why return a reference, not a pointer?"
// A: "A reference can't be null and can't be deleted.
//     With a pointer, someone could call 'delete getInstance()'
//     and crash the system. Reference makes misuse impossible."
// ---------------------------------------------------------------
WiFiManager& WiFiManager::getInstance() {
    static WiFiManager instance;
    return instance;
}

// ---------------------------------------------------------------
// connect() — Join a WiFi network with timeout.
//
// WiFi.mode(WIFI_STA) = Station mode (client), not Access Point.
// We poll WiFi.status() in a loop rather than blocking forever.
// The timeout prevents hanging if the network is unreachable.
//
// Interview Q: "Why not use WiFi events/callbacks instead of polling?"
// A: "For this use case, polling with timeout is simpler and more
//     predictable. Callbacks add complexity — you need to handle
//     re-entrancy and state management. For a production IoT device
//     I'd use event-driven with automatic reconnection, but polling
//     is adequate here and easier to debug."
// ---------------------------------------------------------------
bool WiFiManager::connect(const char* ssid, const char* password,
                          uint32_t timeoutMs) {
    Serial.printf("[WiFi] Connecting to '%s'...\n", ssid);

    WiFi.mode(WIFI_STA);  // Station mode — connect to existing network
    WiFi.begin(ssid, password);

    uint32_t startMs = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startMs >= timeoutMs) {
            Serial.println("[WiFi] Connection TIMEOUT.");
            return false;
        }
        Serial.print(".");
        delay(500);
    }

    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIP() const {
    return WiFi.localIP().toString();
}
