#pragma once
#include <Arduino.h>

/**
 * @brief Possible states of the washing machine.
 *
 * Using enum class (not plain enum) for type safety —
 * you can't accidentally compare MachineState with an int.
 *
 * Interview Q: "Why enum class instead of enum?"
 * A: "enum class is scoped and strongly typed. With plain enum,
 *     IDLE is just an int — you could write 'if (state == 42)'
 *     and the compiler won't complain. enum class catches that."
 */
enum class MachineState {
    IDLE,     // No vibration detected — machine is off
    RUNNING,  // Vibration above threshold — machine is working
    DONE      // Vibration stopped after running — cycle complete
};

/**
 * @brief Converts MachineState to a human-readable string.
 *
 * Useful for logging/debugging. Defined inline in the header
 * so any file that includes this header can use it.
 */
inline const char* stateToString(MachineState state) {
    switch (state) {
        case MachineState::IDLE:    return "IDLE";
        case MachineState::RUNNING: return "RUNNING";
        case MachineState::DONE:    return "DONE";
        default:                    return "UNKNOWN";
    }
}

/**
 * @brief State machine that determines washing machine status from vibration data.
 *
 * Receives magnitude readings from the sensor and tracks state transitions.
 * Uses debouncing (consecutive sample counts) to avoid false triggers.
 *
 * Interview concepts:
 * - State Machine pattern: explicit states with defined transitions
 * - Debouncing / Hysteresis: prevents noise from causing false transitions
 * - Single Responsibility: this class ONLY decides state, doesn't read sensors
 *
 * Transition rules:
 *   IDLE → RUNNING:  magnitude deviation > threshold for N consecutive samples
 *   RUNNING → DONE:  magnitude deviation < threshold for M consecutive samples
 *   DONE → IDLE:     explicit reset() call (after alert is sent)
 */
class VibrationAnalyzer {
public:
    /**
     * @param vibrationThreshold  How far magnitude must deviate from 1.0g
     *                            to count as "vibrating". Default 0.15g.
     *                            Too low = false positives from noise.
     *                            Too high = misses gentle wash cycles.
     *
     * @param startCountRequired  How many consecutive "vibrating" samples
     *                            needed to transition IDLE → RUNNING.
     *                            At 50ms sampling, 10 = 500ms of vibration.
     *
     * @param stopCountRequired   How many consecutive "quiet" samples
     *                            needed to transition RUNNING → DONE.
     *                            At 50ms sampling, 100 = 5 seconds of quiet.
     *                            Must be high enough to survive pauses
     *                            between wash/rinse/spin cycles.
     */
    VibrationAnalyzer(float vibrationThreshold = 0.15f,
                      uint16_t startCountRequired = 10,
                      uint16_t stopCountRequired = 100);

    /**
     * @brief Feed a new magnitude sample into the state machine.
     * @param magnitude Current acceleration magnitude from sensor.
     *
     * Call this at a consistent rate (e.g., every 50ms).
     * The analyzer tracks consecutive samples above/below threshold
     * and transitions state accordingly.
     *
     * Interview Q: "Why pass magnitude instead of reading the sensor directly?"
     * A: "Separation of Concerns. The analyzer doesn't know what sensor
     *     exists. This makes it testable — I can feed it fake data in
     *     unit tests without needing real hardware."
     */
    void feedSample(float magnitude);

    /**
     * @brief Get current machine state.
     * @return Current MachineState (IDLE, RUNNING, or DONE).
     *
     * Thread safety note: in FreeRTOS, this will be called from
     * the Network Task while feedSample() is called from the
     * Sampling Task. We'll protect this with a Mutex in Phase 3.3.
     */
    MachineState getState() const;

    /**
     * @brief Reset state back to IDLE.
     *
     * Called after the DONE alert has been sent.
     * Resets all counters so the system is ready for the next cycle.
     */
    void reset();

private:
    MachineState _state;

    float _vibrationThreshold;
    uint16_t _startCountRequired;
    uint16_t _stopCountRequired;

    uint16_t _consecutiveActiveCount;   // Samples above threshold in a row
    uint16_t _consecutiveQuietCount;    // Samples below threshold in a row
};
