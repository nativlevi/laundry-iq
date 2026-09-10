#include "MPU6050Sensor.h"

// ---- MPU-6050 Register Map ----
// Full datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
static constexpr uint8_t REG_PWR_MGMT_1  = 0x6B;  // Power management — write 0 to wake up
static constexpr uint8_t REG_WHO_AM_I    = 0x75;  // Identity register — should return 0x68
static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B; // First of 6 accel data registers

// At ±2g range (default), raw value of 16384 = 1g
static constexpr float ACCEL_SCALE = 16384.0f;

// ---------------------------------------------------------------
// Constructor — just stores config, does NOT touch hardware.
// Why? — Hardware init should be explicit (init()), not hidden
// in a constructor. This lets us control WHEN init happens
// and handle failure (constructors can't return error codes).
// ---------------------------------------------------------------
MPU6050Sensor::MPU6050Sensor(uint8_t address)
    : _address(address)
{
}

// ---------------------------------------------------------------
// init() — Wake up the sensor and verify it's alive.
//
// I2C flow:
// 1. Start I2C bus (Wire.begin)
// 2. Write 0 to PWR_MGMT_1 register to exit sleep mode
// 3. Read WHO_AM_I register to verify we're talking to an MPU-6050
//
// Interview Q: "Why not init in the constructor?"
// A: "Constructors can't return error codes. If init fails,
//     we need a way to tell the caller. Also, we may want to
//     construct the object early but init the hardware later,
//     when the system is ready."
// ---------------------------------------------------------------
bool MPU6050Sensor::init() {
    Wire.begin();  // Start I2C as master on default pins (21, 22)

    // Wake up MPU-6050 — it starts in sleep mode
    Wire.beginTransmission(_address);
    Wire.write(REG_PWR_MGMT_1);
    Wire.write(0x00);  // 0 = wake up, use internal 8MHz oscillator
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        // I2C transmission failed — sensor not connected or wrong address
        // Error codes: 1=data too long, 2=NACK on address, 3=NACK on data, 4=other
        Serial.printf("[MPU6050] I2C error: %d. Check wiring!\n", error);
        return false;
    }

    // Verify identity — WHO_AM_I should return 0x68
    Wire.beginTransmission(_address);
    Wire.write(REG_WHO_AM_I);
    Wire.endTransmission(false);  // false = repeated start (don't release bus)
    Wire.requestFrom(_address, (uint8_t)1);

    uint8_t whoAmI = Wire.read();
    // Accept both 0x68 (MPU-6050) and 0x70 (MPU-6500 variant found on some GY-521 boards)
    if (whoAmI != 0x68 && whoAmI != 0x70) {
        Serial.printf("[MPU6050] WHO_AM_I = 0x%02X, expected 0x68 or 0x70\n", whoAmI);
        return false;
    }

    Serial.println("[MPU6050] Initialized successfully.");
    return true;
}

// ---------------------------------------------------------------
// readRawAccel() — Read X, Y, Z acceleration in g-force.
//
// The MPU-6050 stores accel data in 6 consecutive registers:
//   0x3B: ACCEL_XOUT_H  (X high byte)
//   0x3C: ACCEL_XOUT_L  (X low byte)
//   0x3D: ACCEL_YOUT_H  (Y high byte)
//   0x3E: ACCEL_YOUT_L  (Y low byte)
//   0x3F: ACCEL_ZOUT_H  (Z high byte)
//   0x40: ACCEL_ZOUT_L  (Z low byte)
//
// Interview Q: "Why read all 6 bytes at once instead of 3 separate reads?"
// A: "Burst read guarantees all axes are sampled at the same moment.
//     Separate reads could give X from one sample and Y from the next,
//     causing data inconsistency."
// ---------------------------------------------------------------
AccelData MPU6050Sensor::readRawAccel() {
    int16_t rawX = readRegister16(REG_ACCEL_XOUT_H);
    int16_t rawY = readRegister16(REG_ACCEL_XOUT_H + 2);
    int16_t rawZ = readRegister16(REG_ACCEL_XOUT_H + 4);

    AccelData data;
    data.x = rawX / ACCEL_SCALE;
    data.y = rawY / ACCEL_SCALE;
    data.z = rawZ / ACCEL_SCALE;

    return data;
}

// ---------------------------------------------------------------
// calcMagnitude() — Total acceleration vector length.
//
// sqrt(x² + y² + z²)
//
// At rest: ~1.0g (just gravity pulling down on one axis)
// Vibrating: fluctuates above/below 1.0
//
// The VibrationAnalyzer will use abs(magnitude - 1.0) as the
// "vibration intensity" — how far we are from rest.
// ---------------------------------------------------------------
float MPU6050Sensor::calcMagnitude() {
    AccelData data = readRawAccel();
    return sqrt(data.x * data.x + data.y * data.y + data.z * data.z);
}

// ---------------------------------------------------------------
// readRegister16() — Private helper to read a 16-bit value.
//
// I2C protocol step by step:
// 1. beginTransmission() — START condition + slave address + WRITE bit
// 2. write(reg) — Tell sensor which register we want
// 3. endTransmission(false) — Repeated START (keep bus ownership)
// 4. requestFrom() — START + slave address + READ bit, read N bytes
// 5. Read high byte, then low byte → combine into 16-bit signed int
//
// Interview Q: "What is a repeated start and why use it?"
// A: "A repeated start lets us switch from write to read without
//     releasing the bus. This prevents another master from
//     hijacking the bus between our register-select write and
//     our data read. It's an atomic operation."
// ---------------------------------------------------------------
int16_t MPU6050Sensor::readRegister16(uint8_t reg) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.endTransmission(false);  // Repeated start

    Wire.requestFrom(_address, (uint8_t)2);
    int16_t value = (Wire.read() << 8) | Wire.read();  // Big Endian

    return value;
}
