#include "app_state.h"

void AppStateMachine::set(AppState state) {
  state_ = state;
  enteredAtMs_ = millis();
}

bool AppStateMachine::elapsed(uint32_t durationMs) const {
  return static_cast<uint32_t>(millis() - enteredAtMs_) >= durationMs;
}

