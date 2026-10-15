#include "storage.h"

bool Storage::begin() {
  if (!preferences_.begin("bey-rpm", false)) return false;
  const size_t size = preferences_.getBytesLength("history");
  if (size == sizeof(data_)) preferences_.getBytes("history", &data_, sizeof(data_));
  if (size != sizeof(data_) || data_.magic != MAGIC || data_.version != VERSION ||
      data_.count > Config::HISTORY_SIZE || data_.writeIndex >= Config::HISTORY_SIZE) {
    data_ = PersistedData{};
    data_.magic = MAGIC;
    data_.version = VERSION;
    save();
  }
  healthy_ = true;
  return true;
}

void Storage::end() { preferences_.end(); }

bool Storage::save() {
  return preferences_.putBytes("history", &data_, sizeof(data_)) == sizeof(data_);
}

bool Storage::add(const LaunchRecord& incoming) {
  LaunchRecord record = incoming;
  record.timestamp = ++data_.launchCounter;
  data_.records[data_.writeIndex] = record;
  data_.writeIndex = (data_.writeIndex + 1) % Config::HISTORY_SIZE;
  if (data_.count < Config::HISTORY_SIZE) ++data_.count;
  data_.bestRpm = max(data_.bestRpm, record.peakRpm);
  healthy_ = save();
  return healthy_;
}

bool Storage::getNewest(uint8_t offset, LaunchRecord& record) const {
  if (offset >= data_.count) return false;
  const uint8_t index =
      (data_.writeIndex + Config::HISTORY_SIZE - 1 - offset) % Config::HISTORY_SIZE;
  record = data_.records[index];
  return true;
}

bool Storage::clear() {
  data_ = PersistedData{};
  data_.magic = MAGIC;
  data_.version = VERSION;
  healthy_ = save();
  return healthy_;
}
