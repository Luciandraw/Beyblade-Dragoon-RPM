#pragma once

#include <Arduino.h>
#include <esp_adc/adc_continuous.h>
#include <soc/soc_caps.h>

struct SensorReading {
  uint16_t raw = 0;
  uint16_t filtered = 0;
  bool comparatorHigh = false;
  bool risingEdge = false;
  bool fallingEdge = false;
  bool discontinuity = false;
  uint32_t timestampUs = 0;
};

class Sensor {
 public:
  void begin();
  bool sample(SensorReading& reading);
  void beginCalibration();
  bool calibrationFinished() const;
  bool finishCalibration();
  bool calibrated() const { return calibrated_; }
  uint16_t minimum() const { return minimum_; }
  uint16_t maximum() const { return maximum_; }
  uint16_t range() const { return maximum_ - minimum_; }
  uint16_t thresholdLow() const { return thresholdLow_; }
  uint16_t thresholdHigh() const { return thresholdHigh_; }
  uint16_t lastRaw() const { return lastRaw_; }
  uint16_t lastFiltered() const { return lastFiltered_; }
  uint32_t sampleRateHz() const { return sampleRateHz_; }
  bool comparatorState() const { return comparatorHigh_; }

 private:
  bool beginContinuous();
  bool readContinuous(uint16_t& raw, uint32_t& timestampUs);
  void resetContinuousStream(uint32_t nowUs);
  static bool IRAM_ATTR onOverflow(adc_continuous_handle_t,
      const adc_continuous_evt_data_t*, void* user);

  static constexpr uint16_t ADC_FRAME_SAMPLES = 64;
  static constexpr uint16_t ADC_FRAME_BYTES =
      ADC_FRAME_SAMPLES * SOC_ADC_DIGI_RESULT_BYTES;

  adc_continuous_handle_t adcHandle_ = nullptr;
  alignas(4) uint8_t adcFrame_[ADC_FRAME_BYTES]{};
  adc_continuous_data_t adcSamples_[ADC_FRAME_SAMPLES]{};
  uint16_t adcSampleCount_ = 0;
  uint16_t adcSamplePosition_ = 0;
  uint32_t streamBaseUs_ = 0;
  uint64_t streamSampleOrdinal_ = 0;
  uint32_t lastPollUs_ = 0;
  bool continuous_ = false;
  bool streamRestartPending_ = false;
  portMUX_TYPE overflowMux_ = portMUX_INITIALIZER_UNLOCKED;
  bool overflow_ = false;
  bool discontinuityPending_ = false;
  bool streamInitialized_ = false;
  bool edgePending_ = false;
  uint32_t edgeCandidateUs_ = 0;

  uint32_t nextSampleUs_ = 0;
  uint32_t calibrationStartedMs_ = 0;
  uint32_t filteredAccumulator_ = 0;
  uint32_t previousSampleUs_ = 0;
  uint32_t sampleIntervalSumUs_ = 0;
  uint16_t sampleIntervalCount_ = 0;
  uint32_t sampleRateHz_ = 0;
  uint16_t minimum_ = 0;
  uint16_t maximum_ = 0;
  uint16_t thresholdLow_ = 0;
  uint16_t thresholdHigh_ = 0;
  uint16_t lastRaw_ = 0;
  uint16_t lastFiltered_ = 0;
  bool comparatorHigh_ = false;
  bool comparatorInitialized_ = false;
  bool calibrating_ = false;
  bool calibrated_ = false;
};
