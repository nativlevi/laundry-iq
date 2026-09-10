#pragma once
#include <vector>
#include "IObserver.h"

/**
 * @brief Subject in the Observer pattern — manages observers and notifies them.
 *
 * The AlertManager doesn't know WHAT each observer does.
 * It just calls onStateChange() on each one.
 *
 * Today:  TelegramNotifier listens → sends Telegram message
 * Tomorrow: add SMSNotifier, EmailNotifier — zero changes to AlertManager.
 *
 * Interview Q: "How is this different from a simple callback?"
 * A: "Callbacks are 1-to-1. Observer is 1-to-many. AlertManager
 *     can notify 5 different systems with one notify() call.
 *     Also, observers can subscribe/unsubscribe at runtime —
 *     you could add a 'quiet hours' feature that unsubscribes
 *     the Telegram notifier at night."
 */
class AlertManager {
public:
    /**
     * @brief Register an observer to receive state change notifications.
     * @param observer Pointer to an IObserver implementation.
     *
     * We store raw pointers because the observers outlive the AlertManager
     * (they're created in main and never deleted). In production code,
     * you might use std::shared_ptr for safety.
     */
    void subscribe(IObserver* observer);

    /**
     * @brief Notify all registered observers of a state change.
     * @param newState The new machine state.
     *
     * Iterates through all observers and calls onStateChange().
     * If one observer throws/crashes, the rest still get called?
     * → Not in this simple implementation. Production code would
     *   wrap each call in try/catch.
     */
    void notify(MachineState newState);

private:
    std::vector<IObserver*> _observers;
};
