#pragma once

#include <Arduino.h>

struct LaunchRecord {
  uint32_t timestamp = 0;  // Monotonic launch index; the device has no RTC.
  uint32_t peakRpm = 0;
  uint32_t averageRpm = 0;
  uint32_t durationMs = 0;
  uint32_t pulseCount = 0;
  uint32_t validSampleCount = 0;
  uint16_t sensorRange = 0;
  uint16_t reserved = 0;
};

// Sensor thresholds from a manual calibration, kept across sleep and reboot.
struct SensorCalibration {
  uint16_t minimum = 0;
  uint16_t maximum = 0;
  uint16_t thresholdLow = 0;
  uint16_t thresholdHigh = 0;
};

struct RpmSample {
  uint32_t timestampUs;
  uint32_t rpm;
};

