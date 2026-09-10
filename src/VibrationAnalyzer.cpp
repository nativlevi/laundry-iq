#include "VibrationAnalyzer.h"
#include <cmath>  // for fabs()

// ---------------------------------------------------------------
// Constructor — set thresholds and start in IDLE state.
//
// All parameters have defaults in the header, so you can write:
//   VibrationAnalyzer analyzer;           // all defaults
//   VibrationAnalyzer analyzer(0.2, 15);  // custom threshold + start count
//
// Interview Q: "How did you choose the default values?"
// A: "Empirically. I ran the sensor on my washing machine, observed
//     the magnitude values, and picked thresholds that correctly
//     distinguished running from idle. The start count is low (10)
//     because we want quick detection. The stop count is high (100)
//     because washing machines pause between cycles — we don't want
//     a false DONE during a pause between wash and spin."
// ---------------------------------------------------------------
VibrationAnalyzer::VibrationAnalyzer(float vibrationThreshold,
                                     uint16_t startCountRequired,
                                     uint16_t stopCountRequired)
    : _state(MachineState::IDLE)
    , _vibrationThreshold(vibrationThreshold)
    , _startCountRequired(startCountRequired)
    , _stopCountRequired(stopCountRequired)
    , _consecutiveActiveCount(0)
    , _consecutiveQuietCount(0)
{
}

// ---------------------------------------------------------------
// feedSample() — The heart of the state machine.
//
// 1. Calculate how far we are from rest (1.0g = gravity only).
//    deviation = |magnitude - 1.0|
//
// 2. Based on current state, count consecutive active/quiet samples.
//
// 3. Transition when the count threshold is met.
//
// Key insight: we reset the OPPOSITE counter each time.
// If we get one quiet sample during vibration, the quiet counter
// starts from 0 again. This is the debouncing mechanism.
//
// Interview Q: "What is hysteresis and how does this implement it?"
// A: "Hysteresis means the transition threshold differs depending
//     on direction. Here, it takes 10 active samples to START
//     but 100 quiet samples to STOP. This asymmetry prevents
//     rapid toggling between states — a pause in the wash cycle
//     won't trigger DONE because 100 quiet samples = 5 seconds."
// ---------------------------------------------------------------
void VibrationAnalyzer::feedSample(float magnitude) {
    float deviation = fabs(magnitude - 1.0f);
    bool isActive = (deviation > _vibrationThreshold);

    switch (_state) {

        case MachineState::IDLE:
            if (isActive) {
                _consecutiveActiveCount++;
                // Enough consecutive vibration? → Machine started!
                if (_consecutiveActiveCount >= _startCountRequired) {
                    _state = MachineState::RUNNING;
                    _consecutiveQuietCount = 0;
                    Serial.println("[Analyzer] State: IDLE → RUNNING");
                }
            } else {
                // One quiet sample resets the active counter
                _consecutiveActiveCount = 0;
            }
            break;

        case MachineState::RUNNING:
            if (isActive) {
                // Still vibrating — reset quiet counter
                _consecutiveQuietCount = 0;
            } else {
                _consecutiveQuietCount++;
                // Enough consecutive quiet? → Machine finished!
                if (_consecutiveQuietCount >= _stopCountRequired) {
                    _state = MachineState::DONE;
                    _consecutiveActiveCount = 0;
                    Serial.println("[Analyzer] State: RUNNING → DONE");
                }
            }
            break;

        case MachineState::DONE:
            // Stay in DONE until explicitly reset.
            // This prevents re-triggering if someone bumps the machine.
            // The Network Task will call reset() after sending the alert.
            break;
    }
}

// ---------------------------------------------------------------
// getState() — Simple getter, but important for thread safety.
//
// In Phase 3.3, this will be called from a DIFFERENT FreeRTOS task
// than feedSample(). Without a Mutex, the Network Task might read
// a partially-written state. For now, it's a direct read.
//
// Interview Q: "Is reading an enum atomic on ESP32?"
// A: "On a 32-bit architecture like ESP32 (Xtensa), reading a
//     single 32-bit aligned value IS atomic. But relying on that
//     is fragile — it breaks if the struct grows, or on a different
//     architecture. Using a Mutex makes the intent explicit and
//     the code portable. We'll add it in Phase 3.3."
// ---------------------------------------------------------------
MachineState VibrationAnalyzer::getState() const {
    return _state;
}

// ---------------------------------------------------------------
// reset() — Return to IDLE, ready for the next wash cycle.
//
// Called by the Network Task after it has sent the "DONE" alert.
// Resets all counters so accidental bumps don't re-trigger.
// ---------------------------------------------------------------
void VibrationAnalyzer::reset() {
    _state = MachineState::IDLE;
    _consecutiveActiveCount = 0;
    _consecutiveQuietCount = 0;
    Serial.println("[Analyzer] State: DONE → IDLE (reset)");
}
