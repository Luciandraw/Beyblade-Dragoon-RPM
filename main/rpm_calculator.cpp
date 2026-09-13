#include "rpm_calculator.h"

void RpmCalculator::reset() {
  candidateCount_ = periodCount_ = periodPosition_ = 0;
  profileCount_ = profileInputCount_ = 0;
  profileDecimation_ = 1;
  startedAtUs_ = lastEdgeUs_ = 0;
  shortestValidPeriodUs_ = UINT32_MAX;
  currentRpm_ = peakRpm_ = 0;
  fastestRpm_[0] = fastestRpm_[1] = fastestRpm_[2] = 0;
  validPeriodTimeUs_ = 0;
  validPeriodCount_ = rejectedPeriodCount_ = transitionCount_ = 0;
  rejectedTooFastCount_ = rejectedTooSlowCount_ = rejectedJumpCount_ = 0;
  started_ = false;
}

uint32_t RpmCalculator::medianPeriod() const {
  // With only one optical period per revolution, the first two samples may
  // contain most of a short launcher's acceleration. Do not flatten them by
  // choosing the slower half of an incomplete median window.
  if (periodCount_ < 3) {
    const uint8_t latest =
        (periodPosition_ + Config::FILTER_WINDOW - 1) % Config::FILTER_WINDOW;
    return periodWindow_[latest];
  }
  uint32_t values[Config::FILTER_WINDOW];
  for (uint8_t i = 0; i < periodCount_; ++i) values[i] = periodWindow_[i];
  for (uint8_t i = 1; i < periodCount_; ++i) {
    const uint32_t value = values[i];
    uint8_t position = i;
    while (position > 0 && values[position - 1] > value) {
      values[position] = values[position - 1];
      --position;
    }
    values[position] = value;
  }
  return values[periodCount_ / 2];
}

void RpmCalculator::appendProfile(uint32_t timestampUs, uint32_t rpm) {
  ++profileInputCount_;
  if (profileInputCount_ % profileDecimation_ != 0) return;
  if (profileCount_ < Config::PROFILE_SIZE) {
    profile_[profileCount_++] = {timestampUs - startedAtUs_, rpm};
    return;
  }
  for (uint16_t i = 0; i < Config::PROFILE_SIZE / 2; ++i) {
    profile_[i] = profile_[i * 2];
  }
  profileCount_ = Config::PROFILE_SIZE / 2;
  profileDecimation_ *= 2;
}

bool RpmCalculator::acceptPeriod(uint32_t period, uint32_t timestampUs) {
  if (period < Config::MIN_PERIOD_US || period > Config::MAX_PERIOD_US) {
    ++rejectedPeriodCount_;
    if (period < Config::MIN_PERIOD_US) {
      ++rejectedTooFastCount_;
    } else {
      ++rejectedTooSlowCount_;
    }
    return false;
  }

  periodWindow_[periodPosition_] = period;
  periodPosition_ = (periodPosition_ + 1) % Config::FILTER_WINDOW;
  if (periodCount_ < Config::FILTER_WINDOW) ++periodCount_;
  const uint32_t filteredPeriod = medianPeriod();
  const uint32_t rpm = static_cast<uint32_t>(
      (60000000.0f * Config::DRIVE_RATIO) /
      (static_cast<float>(filteredPeriod) * Config::ENCODER_PERIODS_PER_REV));
  currentRpm_ = rpm;
  // Rank actual revolutions, not repeated outputs of the rolling median.
  // A hard percentage-step gate can lock onto a slow initial speed forever.
  const uint32_t measuredRpm = static_cast<uint32_t>(
      60000000.0 * Config::DRIVE_RATIO /
      (static_cast<double>(period) * Config::ENCODER_PERIODS_PER_REV));
  if (measuredRpm >= fastestRpm_[0]) {
    fastestRpm_[2] = fastestRpm_[1];
    fastestRpm_[1] = fastestRpm_[0];
    fastestRpm_[0] = measuredRpm;
  } else if (measuredRpm >= fastestRpm_[1]) {
    fastestRpm_[2] = fastestRpm_[1];
    fastestRpm_[1] = measuredRpm;
  } else if (measuredRpm > fastestRpm_[2]) {
    fastestRpm_[2] = measuredRpm;
  }
  // The median of the three fastest values is their middle (second-highest)
  // value. Until three samples exist, retain the measured maximum so short
  // string-launcher pulls can still produce a result.
  peakRpm_ = validPeriodCount_ >= 2 ? fastestRpm_[1] : fastestRpm_[0];
  validPeriodTimeUs_ += period;
  ++validPeriodCount_;
  shortestValidPeriodUs_ = min(shortestValidPeriodUs_, period);
  appendProfile(timestampUs, rpm);
  return true;
}

bool RpmCalculator::acceptRisingEdge(uint32_t timestampUs) {
  // An impossible early edge must not move the reference for the next turn.
  if ((candidateCount_ || started_) &&
      timestampUs - lastEdgeUs_ < Config::MIN_PERIOD_US) {
    ++rejectedPeriodCount_;
    ++rejectedTooFastCount_;
    return false;
  }
  ++transitionCount_;

  if (!started_) {
    if (candidateCount_ == 0 ||
        timestampUs - candidateEdges_[0] <= Config::START_WINDOW_US) {
      candidateEdges_[candidateCount_++] = timestampUs;
    } else {
      candidateEdges_[0] = timestampUs;
      candidateCount_ = 1;
    }
    lastEdgeUs_ = timestampUs;
    if (candidateCount_ >= Config::START_MIN_VALID_EDGES) {
      started_ = true;
      startedAtUs_ = candidateEdges_[0];
      for (uint8_t i = 1; i < candidateCount_; ++i) {
        const uint32_t period = candidateEdges_[i] - candidateEdges_[i - 1];
        acceptPeriod(period, candidateEdges_[i]);
      }
    }
    return started_;
  }

  const uint32_t period = timestampUs - lastEdgeUs_;
  lastEdgeUs_ = timestampUs;
  return acceptPeriod(period, timestampUs);
}

bool RpmCalculator::stopped(uint32_t nowUs) const {
  // DMA sample timestamps can slightly lead the polling clock. Unsigned
  // underflow would otherwise turn that small lead into an immediate stop.
  return started_ && static_cast<int32_t>(nowUs - lastEdgeUs_) >=
                         static_cast<int32_t>(Config::STOP_TIMEOUT_US);
}

uint32_t RpmCalculator::averageRpm() const {
  if (!validPeriodCount_ || !validPeriodTimeUs_) return 0;
  return static_cast<uint32_t>(
      (60000000.0 * Config::DRIVE_RATIO * validPeriodCount_) /
      (static_cast<double>(validPeriodTimeUs_) *
       Config::ENCODER_PERIODS_PER_REV));
}

bool RpmCalculator::validLaunch(uint32_t durationMs) const {
  const uint32_t allPeriods = validPeriodCount_ + rejectedPeriodCount_;
  const bool rejectsOk = !allPeriods ||
      rejectedPeriodCount_ * 100UL <= allPeriods * Config::MAX_REJECT_PERCENT;
  return validPeriodCount_ >= Config::MIN_VALID_PERIODS &&
         durationMs >= Config::MIN_LAUNCH_DURATION_MS && rejectsOk &&
         peakRpm_ >= Config::MIN_VALID_RPM && peakRpm_ <= Config::MAX_VALID_RPM;
}

LaunchRecord RpmCalculator::makeRecord(uint32_t launchIndex, uint16_t sensorRange) const {
  LaunchRecord record;
  record.timestamp = launchIndex;
  record.peakRpm = peakRpm_;
  record.averageRpm = averageRpm();
  record.durationMs = (lastEdgeUs_ - startedAtUs_) / 1000UL;
  record.pulseCount = transitionCount_;
  record.validSampleCount = validPeriodCount_;
  record.sensorRange = sensorRange;
  return record;
}
