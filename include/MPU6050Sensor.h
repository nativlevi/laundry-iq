#pragma once
#include <Arduino.h>
#include <Wire.h>

/**
 * @brief Raw accelerometer data from MPU-6050
 *
 * Groups X/Y/Z into a single struct so related data travels together.
 * Interview note: This follows the "Parameter Object" pattern —
 * if we later add gyroscope data, we extend the struct,
 * not every function signature.
 */
struct AccelData {
    float x;
    float y;
    float z;
};

/**
 * @brief Hardware Abstraction Layer for the MPU-6050 sensor.
 *
 * Encapsulates all I2C communication with the MPU-6050.
 * The rest of the system only sees init(), readRawAccel(), and calcMagnitude().
 * If we swap the sensor for an ADXL345 tomorrow, only this class changes.
 *
 * Interview concepts:
 * - Encapsulation: I2C register details are private
 * - Dependency Inversion: upper layers depend on this interface, not hardware
 * - Single Responsibility: this class does ONE thing — talk to the MPU-6050
 */
class MPU6050Sensor {
public:
    /**
     * @param address I2C address of the MPU-6050.
     *        0x68 when AD0 → GND (default)
     *        0x69 when AD0 → 3.3V
     */
    MPU6050Sensor(uint8_t address = 0x68);

    /**
     * @brief Initialize the sensor — wake it up from sleep mode.
     * @return true if sensor responded on I2C bus, false otherwise.
     *
     * Why bool return? — So the caller can handle failure gracefully
     * instead of crashing. Defensive programming.
     */
    bool init();

    /**
     * @brief Read raw accelerometer values from the sensor.
     * @return AccelData with X, Y, Z in g-force units.
     *
     * Reads 6 bytes (2 per axis) starting from register 0x3B.
     * Raw 16-bit values are divided by 16384 to convert to g-force
     * (at ±2g sensitivity, which is the default).
     */
    AccelData readRawAccel();

    /**
     * @brief Calculate vibration magnitude from accelerometer data.
     * @return sqrt(x² + y² + z²) — total acceleration magnitude.
     *
     * At rest this returns ~1.0 (gravity). During vibration it fluctuates.
     * The VibrationAnalyzer uses this to detect washing machine state.
     */
    float calcMagnitude();

private:
    uint8_t _address;

    /**
     * @brief Read a 16-bit register from the MPU-6050.
     * @param reg Starting register address.
     * @return Signed 16-bit value (high byte first — Big Endian).
     *
     * Private because external code should never touch registers directly.
     * This is the essence of encapsulation.
     */
    int16_t readRegister16(uint8_t reg);
};
