#include "buttons.h"

#include "config.h"

void Buttons::begin() {
  pinMode(Config::PIN_BUTTON_CENTER, INPUT_PULLUP);
  pinMode(Config::PIN_BLADE_PRESENT, INPUT_PULLUP);
  raw_ = stable_ = digitalRead(Config::PIN_BUTTON_CENTER);
  bladeRaw_ = bladeStable_ = digitalRead(Config::PIN_BLADE_PRESENT);
}

ButtonEvents Buttons::update() {
  ButtonEvents events;
  const uint32_t now = millis();

  const bool bladeRaw = digitalRead(Config::PIN_BLADE_PRESENT);
  if (bladeRaw != bladeRaw_) {
    bladeRaw_ = bladeRaw;
    bladeChangedAt_ = now;
  }
  if (bladeRaw != bladeStable_ &&
      now - bladeChangedAt_ >= Config::BUTTON_DEBOUNCE_MS) {
    bladeStable_ = bladeRaw;
    events.bladeInserted = !bladeStable_;
    events.bladeRemoved = bladeStable_;
  }

  const bool raw = digitalRead(Config::PIN_BUTTON_CENTER);
  if (raw != raw_) {
    raw_ = raw;
    changedAt_ = now;
  }
  if (raw != stable_ && now - changedAt_ >= Config::BUTTON_DEBOUNCE_MS) {
    stable_ = raw;
    if (!stable_) {
      pressedAt_ = now;
    } else {
      const uint32_t heldMs = now - pressedAt_;
      if (heldMs >= Config::SERVICE_HOLD_MS) {
        pendingClick_ = false;
        events.serviceHold = true;
      } else if (heldMs >= Config::CENTER_LONG_PRESS_MS) {
        pendingClick_ = false;
        events.centerLong = true;
      } else if (pendingClick_ &&
                 now - pendingClickAt_ <= Config::DOUBLE_CLICK_MS) {
        pendingClick_ = false;
        events.centerDouble = true;
      } else {
        // If an older click has already expired, deliver it and keep this
        // release as the beginning of a new possible double-click.
        if (pendingClick_) events.centerPressed = true;
        pendingClick_ = true;
        pendingClickAt_ = now;
      }
    }
  }

  // A single click is emitted only after the double-click window closes.
  // Waiting until the button is released avoids turning a slow second press
  // into an accidental menu step.
  if (stable_ && pendingClick_ &&
      now - pendingClickAt_ > Config::DOUBLE_CLICK_MS) {
    pendingClick_ = false;
    events.centerPressed = true;
  }
  return events;
}
