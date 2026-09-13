#pragma once

#include <Arduino.h>

enum class AppState : uint8_t {
  Boot,
  Calibrating,
  Ready,
  Measuring,
  ShowingResult,
  Menu,
  SensorError
};

class AppStateMachine {
 public:
  void set(AppState state);
  AppState get() const { return state_; }
  uint32_t enteredAtMs() const { return enteredAtMs_; }
  bool elapsed(uint32_t durationMs) const;

 private:
  AppState state_ = AppState::Boot;
  uint32_t enteredAtMs_ = 0;
};

