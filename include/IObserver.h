#pragma once
#include "VibrationAnalyzer.h"  // For MachineState enum

/**
 * @brief Observer interface — any class that wants to be notified
 *        of state changes must implement this.
 *
 * This is the "O" in Observer Pattern:
 *   - Subject (AlertManager) maintains a list of observers
 *   - When state changes, Subject calls onStateChange() on each observer
 *   - Observers don't know about each other
 *
 * Interview Q: "Why use an interface (pure virtual) instead of
 *              a concrete class?"
 * A: "Open/Closed Principle. AlertManager is OPEN for extension
 *     (add new observers like SMS, Email) but CLOSED for modification
 *     (you never change AlertManager code to add a notification type).
 *     The interface is the contract — any class that implements
 *     onStateChange() can plug in."
 *
 * Interview Q: "Why virtual destructor?"
 * A: "If we delete an observer through a pointer to IObserver,
 *     without virtual destructor the derived class destructor
 *     won't be called → resource leak. It's a classic C++ pitfall."
 */
class IObserver {
public:
    virtual ~IObserver() = default;

    /**
     * @brief Called when the machine state changes.
     * @param newState The new state (IDLE, RUNNING, or DONE).
     */
    virtual void onStateChange(MachineState newState) = 0;
};
