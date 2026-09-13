#include "sensor.h"

#include "config.h"

void Sensor::begin() {
  pinMode(Config::PIN_SENSOR, INPUT);
  minimum_ = Config::SENSOR_FIXED_MIN;
  maximum_ = Config::SENSOR_FIXED_MAX;
  thresholdLow_ = Config::SENSOR_FIXED_THRESHOLD_LOW;
  thresholdHigh_ = Config::SENSOR_FIXED_THRESHOLD_HIGH;
  calibrated_ = true;
  nextSampleUs_ = micros();
  continuous_ = beginContinuous();
  if (continuous_) {
    sampleRateHz_ = Config::ADC_SAMPLE_RATE_HZ;
    Serial.printf("ADC continuous: %luHz\n",
                  static_cast<unsigned long>(sampleRateHz_));
  } else {
    analogReadResolution(12);
    analogSetPinAttenuation(Config::PIN_SENSOR, ADC_11db);
    Serial.println("ADC continuous failed; using analogRead fallback");
  }
}

bool Sensor::beginContinuous() {
  adc_unit_t unit;
  adc_channel_t channel;
  if (adc_continuous_io_to_channel(Config::PIN_SENSOR, &unit, &channel) != ESP_OK ||
      unit != ADC_UNIT_1) {
    return false;
  }

  adc_continuous_handle_cfg_t handleConfig{};
  handleConfig.max_store_buf_size = ADC_FRAME_BYTES * 64;
  handleConfig.conv_frame_size = ADC_FRAME_BYTES;
  handleConfig.flags.flush_pool = 0;
  if (adc_continuous_new_handle(&handleConfig, &adcHandle_) != ESP_OK) {
    adcHandle_ = nullptr;
    return false;
  }

  adc_digi_pattern_config_t pattern{};
  pattern.atten = ADC_ATTEN_DB_12;
  pattern.channel = channel;
  pattern.unit = unit;
  pattern.bit_width = ADC_BITWIDTH_12;

  adc_continuous_config_t config{};
  config.pattern_num = 1;
  config.adc_pattern = &pattern;
  config.sample_freq_hz = Config::ADC_SAMPLE_RATE_HZ;
  config.conv_mode = ADC_CONV_SINGLE_UNIT_1;
  config.format = ADC_DIGI_OUTPUT_FORMAT_TYPE2;
  adc_continuous_evt_cbs_t callbacks{};
  callbacks.on_pool_ovf = onOverflow;
  if (adc_continuous_config(adcHandle_, &config) != ESP_OK ||
      adc_continuous_register_event_callbacks(adcHandle_, &callbacks, this) != ESP_OK ||
      adc_continuous_start(adcHandle_) != ESP_OK) {
    adc_continuous_deinit(adcHandle_);
    adcHandle_ = nullptr;
    return false;
  }

  lastPollUs_ = micros();
  return true;
}

bool IRAM_ATTR Sensor::onOverflow(adc_continuous_handle_t,
    const adc_continuous_evt_data_t*, void* user) {
  Sensor* self = static_cast<Sensor*>(user);
  portENTER_CRITICAL_ISR(&self->overflowMux_);
  self->overflow_ = true;
  portEXIT_CRITICAL_ISR(&self->overflowMux_);
  return false;
}

void Sensor::resetContinuousStream(uint32_t nowUs) {
  adc_continuous_stop(adcHandle_);
  adc_continuous_flush_pool(adcHandle_);
  portENTER_CRITICAL(&overflowMux_);
  overflow_ = false;
  portEXIT_CRITICAL(&overflowMux_);
  // Keep continuous mode selected even if restart fails: analogRead cannot
  // safely take over while this driver's ADC handle is still allocated.
  streamRestartPending_ = adc_continuous_start(adcHandle_) != ESP_OK;
  adcSampleCount_ = adcSamplePosition_ = 0;
  streamSampleOrdinal_ = 0;
  streamBaseUs_ = 0;
  streamInitialized_ = false;
  discontinuityPending_ = true;
  edgePending_ = false;
  comparatorInitialized_ = false;
  lastPollUs_ = nowUs;
}

bool Sensor::readContinuous(uint16_t& raw, uint32_t& timestampUs) {
  const uint32_t nowUs = micros();
  if (streamRestartPending_) {
    if (nowUs - lastPollUs_ >= 40000) resetContinuousStream(nowUs);
    return false;
  }
  // Check once per frame, not 80,000 critical sections per second.
  bool overflowed = false;
  if (adcSamplePosition_ >= adcSampleCount_) {
    portENTER_CRITICAL(&overflowMux_);
    overflowed = overflow_;
    portEXIT_CRITICAL(&overflowMux_);
  }
  if (nowUs - lastPollUs_ > 40000 || overflowed) {
    // A display refresh can pause loop() for more than a second. Discard its
    // stale DMA data so it cannot be mistaken for a new launcher pull.
    resetContinuousStream(nowUs);
    return false;
  }
  lastPollUs_ = nowUs;

  while (adcSamplePosition_ >= adcSampleCount_) {
    uint32_t bytesRead = 0;
    const esp_err_t readResult = adc_continuous_read(
        adcHandle_, adcFrame_, sizeof(adcFrame_), &bytesRead, 0);
    if (readResult != ESP_OK) {
      if (readResult != ESP_ERR_TIMEOUT) resetContinuousStream(nowUs);
      return false;
    }
    if (bytesRead == 0) return false;

    uint32_t parsedCount = 0;
    if (adc_continuous_parse_data(adcHandle_, adcFrame_, bytesRead,
                                  adcSamples_, &parsedCount) != ESP_OK) {
      resetContinuousStream(nowUs);
      return false;
    }
    adcSampleCount_ = min<uint32_t>(parsedCount, ADC_FRAME_SAMPLES);
    adcSamplePosition_ = 0;
    if (adcSampleCount_ == 0) return false;
    if (!streamInitialized_) {
      const uint32_t frameDurationUs = static_cast<uint64_t>(
          adcSampleCount_ - 1) * 1000000ULL / Config::ADC_SAMPLE_RATE_HZ;
      streamBaseUs_ = nowUs - frameDurationUs;
      streamSampleOrdinal_ = 0;
      streamInitialized_ = true;
    }
  }

  const adc_continuous_data_t& sample = adcSamples_[adcSamplePosition_++];
  if (!sample.valid) {
    resetContinuousStream(nowUs);
    return false;
  }
  raw = static_cast<uint16_t>(sample.raw_data);
  timestampUs = streamBaseUs_ + static_cast<uint64_t>(streamSampleOrdinal_++) *
      1000000ULL / Config::ADC_SAMPLE_RATE_HZ;
  return true;
}

void Sensor::beginCalibration() {
  minimum_ = Config::ADC_MAX_VALUE;
  maximum_ = 0;
  calibrationStartedMs_ = millis();
  calibrating_ = true;
  calibrated_ = false;
}

bool Sensor::calibrationFinished() const {
  return calibrating_ &&
         static_cast<uint32_t>(millis() - calibrationStartedMs_) >=
             Config::CALIBRATION_TIME_MS;
}

bool Sensor::finishCalibration() {
  calibrating_ = false;
  if (maximum_ <= minimum_ || range() < Config::SENSOR_MIN_CONTRAST) {
    calibrated_ = false;
    return false;
  }
  const uint16_t center = minimum_ + range() / 2;
  uint16_t hysteresis =
      static_cast<uint32_t>(range()) * Config::SENSOR_HYSTERESIS_PERCENT / 100;
  hysteresis = max<uint16_t>(hysteresis, 2);
  thresholdLow_ = center > hysteresis ? center - hysteresis : 0;
  thresholdHigh_ = min<uint16_t>(center + hysteresis, Config::ADC_MAX_VALUE);
  comparatorHigh_ = Config::SENSOR_INVERTED ? lastFiltered_ < center : lastFiltered_ >= center;
  edgePending_ = false;
  calibrated_ = true;
  return true;
}

bool Sensor::sample(SensorReading& reading) {
  uint32_t timestampUs = 0;
  if (continuous_) {
    if (!readContinuous(lastRaw_, timestampUs)) {
      if (!discontinuityPending_) return false;
      // Notify the calculator immediately, including if ADC restart failed.
      reading = SensorReading{};
      reading.discontinuity = true;
      reading.timestampUs = micros();
      discontinuityPending_ = false;
      return true;
    }
  } else {
    const uint32_t now = micros();
    if (static_cast<int32_t>(now - nextSampleUs_) < 0) return false;
    nextSampleUs_ = now + Config::ADC_SAMPLE_INTERVAL_US;
    timestampUs = now;
    if (previousSampleUs_) {
      const uint32_t intervalUs = now - previousSampleUs_;
      if (intervalUs < 1000) {
        sampleIntervalSumUs_ += intervalUs;
        if (++sampleIntervalCount_ >= 256) {
          if (sampleIntervalSumUs_) {
            sampleRateHz_ = static_cast<uint32_t>(
                (static_cast<uint64_t>(sampleIntervalCount_) * 1000000ULL) /
                sampleIntervalSumUs_);
          }
          sampleIntervalSumUs_ = 0;
          sampleIntervalCount_ = 0;
        }
      } else {
        sampleIntervalSumUs_ = 0;
        sampleIntervalCount_ = 0;
      }
    }
    previousSampleUs_ = now;
    lastRaw_ = analogRead(Config::PIN_SENSOR);
  }

  if (!comparatorInitialized_) {
    filteredAccumulator_ = static_cast<uint32_t>(lastRaw_) << 2;
    lastFiltered_ = lastRaw_;
    const uint16_t center = thresholdLow_ + (thresholdHigh_ - thresholdLow_) / 2;
    comparatorHigh_ = Config::SENSOR_INVERTED
                          ? lastRaw_ < center : lastRaw_ >= center;
    comparatorInitialized_ = true;
  }
  filteredAccumulator_ = filteredAccumulator_ - (filteredAccumulator_ >> 2) + lastRaw_;
  lastFiltered_ = filteredAccumulator_ >> 2;
  if (calibrating_) {
    minimum_ = min(minimum_, lastFiltered_);
    maximum_ = max(maximum_, lastFiltered_);
  }

  // The disc consists of one broad black half and one broad white half.
  // Detecting edges on the filtered DMA stream removes idle noise while the
  // filter still settles much faster than either half-revolution.
  const uint16_t comparatorInput = lastFiltered_;
  bool nextState = comparatorHigh_;
  if (calibrated_) {
    if (!Config::SENSOR_INVERTED) {
      if (!comparatorHigh_ && comparatorInput > thresholdHigh_) nextState = true;
      if (comparatorHigh_ && comparatorInput < thresholdLow_) nextState = false;
    } else {
      if (!comparatorHigh_ && comparatorInput < thresholdLow_) nextState = true;
      if (comparatorHigh_ && comparatorInput > thresholdHigh_) nextState = false;
    }
  }

  // Require a sustained crossing, but timestamp its beginning to avoid
  // adding confirmation latency to the measured period.
  uint32_t edgeTimestampUs = timestampUs;
  if (nextState != comparatorHigh_) {
    if (!edgePending_) {
      edgePending_ = true;
      edgeCandidateUs_ = timestampUs;
    }
    if (timestampUs - edgeCandidateUs_ < Config::SENSOR_EDGE_CONFIRM_US) {
      nextState = comparatorHigh_;
    } else {
      edgeTimestampUs = edgeCandidateUs_;
      edgePending_ = false;
    }
  } else {
    edgePending_ = false;
  }
  reading.discontinuity = discontinuityPending_;
  discontinuityPending_ = false;
  reading.raw = lastRaw_;
  reading.filtered = lastFiltered_;
  reading.risingEdge = !comparatorHigh_ && nextState;
  reading.fallingEdge = comparatorHigh_ && !nextState;
  reading.comparatorHigh = nextState;
  reading.timestampUs = edgeTimestampUs;
  comparatorHigh_ = nextState;
  return true;
}
