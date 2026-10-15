#pragma once

#include <Arduino.h>
#include <functional>

#include "launch_record.h"

// Screens are drawn by a FreeRTOS task on core 0, so an e-paper refresh never
// stalls loop() and the ADC stream. Each show*() call only queues the screen;
// a newer request replaces one that has not started drawing yet.
class Display {
 public:
  bool begin();
  // Waits for queued drawing, then puts the panel into deep sleep.
  void hibernate();
  void waitIdle();
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
  void post(std::function<void()> job);
  static void renderTask(void* self);

  bool available_ = false;
  SemaphoreHandle_t lock_ = nullptr;
  TaskHandle_t task_ = nullptr;
  std::function<void()> pending_;
  bool drawing_ = false;
};
