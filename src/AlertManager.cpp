#include "AlertManager.h"

void AlertManager::subscribe(IObserver* observer) {
    if (observer != nullptr) {
        _observers.push_back(observer);
        Serial.printf("[AlertManager] Observer subscribed. Total: %d\n",
                      _observers.size());
    }
}

// ---------------------------------------------------------------
// notify() — Fan out the state change to all observers.
//
// This is where the decoupling magic happens:
// AlertManager has NO idea what TelegramNotifier does.
// It just calls the interface method.
//
// Interview Q: "What if you had 100 observers and notify() takes too long?"
// A: "In FreeRTOS, notify() runs in the Network Task.
//     If it blocks too long, other tasks starve. Solutions:
//     1) Queue the notification and process async
//     2) Use xTaskNotify() to wake each observer's own task
//     3) Set a timeout per observer
//     For our 1-2 observers, direct call is fine."
// ---------------------------------------------------------------
void AlertManager::notify(MachineState newState) {
    Serial.printf("[AlertManager] Notifying %d observer(s): state=%s\n",
                  _observers.size(), stateToString(newState));

    for (auto* observer : _observers) {
        observer->onStateChange(newState);
    }
}
