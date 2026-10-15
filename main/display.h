#pragma once

#include <Arduino.h>

#include "launch_record.h"

class Display {
 public:
  bool begin();
  void hibernate();
  bool available() const { return available_; }
  void showBoot();
  void showCalibrating(uint16_t minimum, uint16_t maximum);
  void showInsertBlade(uint32_t bestRpm, uint32_t launchCount);
  void showReady(uint32_t bestRpm, uint32_t launchCount);
  void showResult(const LaunchRecord& record, uint32_t bestRpm, bool newBest);
  void showInvalidLaunch();
  void showSensorError(uint16_t minimum, uint16_t maximum);
  void showHistory(const LaunchRecord* record, uint8_t position, uint8_t count);
  void showMenu(uint8_t item);
  void showClearConfirmation();
  void showService(uint16_t raw, uint16_t minimum, uint16_t maximum,
                   uint16_t thresholdLow, uint16_t thresholdHigh,
                   uint32_t transitions, bool comparatorHigh);
  void showGraph(const RpmSample* samples, uint16_t count, uint32_t peakRpm);

 private:
  void startFrame(bool partial = false);
  void finishFrame();
  void title(const char* text);
  void centered(const char* text, int16_t y, uint8_t size);
  bool available_ = false;
  bool partial_ = false;
  uint8_t partialCount_ = 0;
};
