#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"
#include "launch_record.h"

class Storage {
 public:
  bool begin();
  void end();
  bool add(const LaunchRecord& record);
  bool getNewest(uint8_t offset, LaunchRecord& record) const;
  bool last(LaunchRecord& record) const { return getNewest(0, record); }
  uint8_t count() const { return data_.count; }
  uint32_t launchCounter() const { return data_.launchCounter; }
  uint32_t nextLaunchIndex() const { return data_.launchCounter + 1; }
  uint32_t bestRpm() const { return data_.bestRpm; }
  bool clear();
  bool healthy() const { return healthy_; }

 private:
  static constexpr uint32_t MAGIC = 0x4252504D;
  static constexpr uint16_t VERSION = 1;
  struct PersistedData {
    uint32_t magic;
    uint16_t version;
    uint8_t count;
    uint8_t writeIndex;
    uint32_t launchCounter;
    uint32_t bestRpm;
    LaunchRecord records[Config::HISTORY_SIZE];
  };
  bool save();

  mutable Preferences preferences_;
  PersistedData data_{};
  bool healthy_ = false;
};

