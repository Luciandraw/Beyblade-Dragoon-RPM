#pragma once

#include <Arduino.h>

#include "config.h"
#include "launch_record.h"

class RpmCalculator {
 public:
  void reset();
  bool acceptRisingEdge(uint32_t timestampUs);
  bool stopped(uint32_t nowUs) const;
  bool hasStarted() const { return started_; }
  bool validLaunch(uint32_t durationMs) const;
  LaunchRecord makeRecord(uint32_t launchIndex, uint16_t sensorRange) const;

  uint32_t currentRpm() const { return currentRpm_; }
  uint32_t peakRpm() const { return peakRpm_; }
  uint32_t averageRpm() const;
  uint32_t transitionCount() const { return transitionCount_; }
  uint32_t validPeriodCount() const { return validPeriodCount_; }
  uint32_t rejectedPeriodCount() const { return rejectedPeriodCount_; }
  uint32_t rejectedTooFastCount() const { return rejectedTooFastCount_; }
  uint32_t rejectedTooSlowCount() const { return rejectedTooSlowCount_; }
  uint32_t rejectedJumpCount() const { return rejectedJumpCount_; }
  uint32_t startedAtUs() const { return startedAtUs_; }
  uint32_t lastEdgeUs() const { return lastEdgeUs_; }
  uint32_t shortestValidPeriodUs() const { return shortestValidPeriodUs_; }
  const RpmSample* profile() const { return profile_; }
  uint16_t profileCount() const { return profileCount_; }

 private:
  uint32_t medianPeriod() const;
  bool acceptPeriod(uint32_t periodUs, uint32_t timestampUs);
  void appendProfile(uint32_t timestampUs, uint32_t rpm);

  uint32_t candidateEdges_[Config::START_MIN_VALID_EDGES]{};
  uint8_t candidateCount_ = 0;
  uint32_t periodWindow_[Config::FILTER_WINDOW]{};
  uint8_t periodCount_ = 0;
  uint8_t periodPosition_ = 0;
  RpmSample profile_[Config::PROFILE_SIZE]{};
  uint16_t profileCount_ = 0;
  uint16_t profileDecimation_ = 1;
  uint16_t profileInputCount_ = 0;
  uint32_t startedAtUs_ = 0;
  uint32_t lastEdgeUs_ = 0;
  uint32_t shortestValidPeriodUs_ = UINT32_MAX;
  uint32_t currentRpm_ = 0;
  uint32_t peakRpm_ = 0;
  uint32_t fastestRpm_[3]{};
  uint64_t validPeriodTimeUs_ = 0;
  uint32_t validPeriodCount_ = 0;
  uint32_t rejectedPeriodCount_ = 0;
  uint32_t rejectedTooFastCount_ = 0;
  uint32_t rejectedTooSlowCount_ = 0;
  uint32_t rejectedJumpCount_ = 0;
  uint32_t transitionCount_ = 0;
  bool started_ = false;
};
