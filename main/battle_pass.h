#pragma once

#include <Arduino.h>

#include "launch_record.h"

// Emulates the companion pass over BLE so the official
// Beyblade X app can register the device and read launches. Protocol from
// community captures; see docs in battle_pass.cpp.
class BattlePass {
 public:
  // Starts the BLE stack only when enabled; it stays down otherwise.
  void begin(bool enabled);
  // Starts or stops advertising and drops the app's connection when off.
  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }
  // Handles commands queued by the app and status changes. Call from loop();
  // readyToLaunch is reported to the app as the pass's "bey attached" state.
  // Returns true when the app talked to the device (counts as activity).
  bool update(bool readyToLaunch);
  // Records a finished launch for the app and notifies it if connected.
  void publishLaunch(const LaunchRecord& record,
                     const RpmSample* profile, uint16_t profileCount);
  bool connected() const;
  // True once after the app received the history; count is how many launches it got.
  bool takeSync(uint8_t& count);
  // Tells the pass whether its button is down. While held, the status page
  // reads "button pressed" (also when the app polls it); the real status
  // returns on release.
  void setButtonHeld(bool held);

 private:
  void sendHeader();
  void sendHeader(uint8_t status);
  void sendHistory();
  void sendCurve();
  void notify(const uint8_t* page);

  uint8_t status_ = 0;
  uint8_t lastStatus_ = 0xFF;
  bool enabled_ = false;
  bool initialized_ = false;
  bool buttonHeld_ = false;
  uint32_t dropAtMs_ = 0;  // Non-zero: drop the app's connection at this time.
  bool syncPending_ = false;
  uint8_t syncCount_ = 0;
  uint8_t readCount_ = 0;  // Launches in the last history sent to the app.
};
