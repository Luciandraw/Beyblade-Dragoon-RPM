#pragma once

#include <Arduino.h>

struct ButtonEvents {
  bool centerPressed = false;
  bool centerDouble = false;
  bool centerLong = false;
  bool serviceHold = false;
  bool bladeInserted = false;
  bool bladeRemoved = false;
};

class Buttons {
 public:
  void begin();
  ButtonEvents update();
  bool bladePresent() const { return !bladeStable_; }
  bool centerDown() const { return !stable_; }

 private:
  bool raw_ = true;
  bool stable_ = true;
  uint32_t changedAt_ = 0;
  uint32_t pressedAt_ = 0;
  uint32_t pendingClickAt_ = 0;
  bool pendingClick_ = false;
  bool bladeRaw_ = true;
  bool bladeStable_ = true;
  uint32_t bladeChangedAt_ = 0;
};
