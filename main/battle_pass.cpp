#include "battle_pass.h"

#include <atomic>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <esp_mac.h>
#include <Preferences.h>

#include "config.h"

// BLE profile of the companion pass for the Beyblade X app (from community captures):
//   name BEYBLADE_TOOL01, service 55c40000-..., the app writes command bytes
//   to f001 and receives 17-byte pages (tag + 16 data bytes) on f002.
// Pages:
//   a0     header: [1]=0x3a [3]=status (0x04 blade ready, 0x00 not ready,
//          0x10 button pressed)
//          [4]=battery [7..8]=best RPM LE [9..10]=launch counter LE (same
//          values as the b6 summary)
//          [11..16]=BT address, least significant byte first.
//   b0..b6 50 launch values (uint16 LE, oldest first) from b0[1]; b6[5..6]
//          padding, b6[7..8] best, b6[9..10] total, b6[11..12] stored count.
//   b7     [16]=low byte of the sum of data bytes of b0..b6.
//   70..73 32 curve samples, uint16 LE, 7500000 / RPM.
// Commands: 0x51 header, 0x74 header + history, 0x75 clear history,
// 0x63 powers the pass off (the app sends it last when a sync is done).
namespace {
constexpr const char* DEVICE_NAME = "BEYBLADE_TOOL01";
constexpr const char* SERVICE_UUID = "55c40000-f8eb-11ec-b939-0242ac120002";
constexpr const char* WRITE_UUID = "55c4f001-f8eb-11ec-b939-0242ac120002";
constexpr const char* NOTIFY_UUID = "55c4f002-f8eb-11ec-b939-0242ac120002";
constexpr uint8_t PAGE_SIZE = 17;
constexpr uint8_t SLOTS = 50;
constexpr uint8_t CURVE_SAMPLES = 32;
constexpr uint8_t STATUS_READY = 0x04;
constexpr uint8_t STATUS_IDLE = 0x00;
// Sent once per press of the pass's button; the official app asks for a
// double press to register the pass.
constexpr uint8_t STATUS_BUTTON = 0x10;
// The emulator has no battery gauge; report a full cell.
constexpr uint8_t BATTERY_FULL = 0xFF;

constexpr uint8_t CMD_HEADER = 0x51;
constexpr uint8_t CMD_HISTORY = 0x74;
constexpr uint8_t CMD_CLEAR = 0x75;
constexpr uint8_t CMD_POWER_OFF = 0x63;

// The app's view of the history survives deep sleep, like the pass's own
// memory, and is cleared only by the app.
RTC_DATA_ATTR uint16_t appSlots[SLOTS];
RTC_DATA_ATTR uint8_t appCount = 0;
RTC_DATA_ATTR uint16_t appTotal = 0;
RTC_DATA_ATTR uint16_t appBest = 0;

// The history also lives in flash, so it survives a reset or power loss like
// the real pass's memory. The app compares the launch counter between syncs
// and must never see it go backwards.
constexpr uint32_t STORE_MAGIC = 0xB5A55002;
struct StoredState {
  uint32_t magic;
  uint16_t slots[SLOTS];
  uint16_t total;
  uint16_t best;
  uint8_t count;
};
Preferences passStore;
bool passStoreOpen = false;

void saveState() {
  if (!passStoreOpen) return;
  StoredState state{};
  state.magic = STORE_MAGIC;
  memcpy(state.slots, appSlots, sizeof(appSlots));
  state.total = appTotal;
  state.best = appBest;
  state.count = appCount;
  passStore.putBytes("state", &state, sizeof(state));
}

void loadState() {
  StoredState state{};
  if (passStore.getBytesLength("state") != sizeof(state)) return;
  if (passStore.getBytes("state", &state, sizeof(state)) != sizeof(state)) return;
  if (state.magic != STORE_MAGIC || state.count > SLOTS) return;
  memcpy(appSlots, state.slots, sizeof(appSlots));
  appTotal = state.total;
  appBest = state.best;
  appCount = state.count;
}
uint16_t curve[CURVE_SAMPLES];
bool curveValid = false;

// True once the app enabled notifications on f002. Only NimBLE reports this;
// on other stacks assume it, since the app cannot be told apart.
#if defined(CONFIG_NIMBLE_ENABLED)
constexpr bool TRACK_SUBSCRIPTION = true;
#else
constexpr bool TRACK_SUBSCRIPTION = false;
#endif
std::atomic<bool> subscribed{!TRACK_SUBSCRIPTION};

BLEServer* server = nullptr;
BLECharacteristic* notifyChar = nullptr;
uint8_t address[6];  // Least significant byte first.

// Written by the BLE host task, consumed in loop().
// The app can write several commands back to back (e.g. clear then power
// off), so they are queued instead of overwriting each other.
constexpr uint8_t QUEUE_SIZE = 8;
uint8_t commandQueue[QUEUE_SIZE];
std::atomic<uint8_t> queueHead{0};  // Written by the BLE task only.
std::atomic<uint8_t> queueTail{0};  // Written by loop() only.

void pushCommand(uint8_t command) {
  const uint8_t head = queueHead.load();
  const uint8_t next = (head + 1) % QUEUE_SIZE;
  if (next == queueTail.load()) return;  // Full: drop the newest.
  commandQueue[head] = command;
  queueHead.store(next);
}

bool popCommand(uint8_t& command) {
  const uint8_t tail = queueTail.load();
  if (tail == queueHead.load()) return false;
  command = commandQueue[tail];
  queueTail.store((tail + 1) % QUEUE_SIZE);
  return true;
}
volatile bool pendingConnect = false;
volatile bool advertisingWanted = false;

void putLe16(uint8_t* p, uint16_t value) {
  p[0] = value & 0xFF;
  p[1] = value >> 8;
}

uint16_t clamp16(uint32_t value) { return value > 0xFFFF ? 0xFFFF : value; }

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    pendingConnect = true;
    subscribed = !TRACK_SUBSCRIPTION;
    if (Config::DEBUG_APP_SYNC) Serial.println("SYNC: app connected");
  }
  void onDisconnect(BLEServer*) override {
    subscribed = !TRACK_SUBSCRIPTION;
    if (Config::DEBUG_APP_SYNC) Serial.println("SYNC: app disconnected");
    if (advertisingWanted) BLEDevice::startAdvertising();
  }
};

class WriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    // String on Arduino-ESP32 3.x, std::string on 2.x: both have length() and [].
    const auto value = characteristic->getValue();
    if (Config::DEBUG_APP_SYNC) {
      Serial.print("SYNC: app wrote");
      for (size_t i = 0; i < value.length(); ++i) Serial.printf(" %02x", static_cast<uint8_t>(value[i]));
      Serial.println();
    }
    if (value.length()) pushCommand(static_cast<uint8_t>(value[0]));
  }
};

class NotifyCallbacks : public BLECharacteristicCallbacks {
#if defined(CONFIG_NIMBLE_ENABLED)
  void onSubscribe(BLECharacteristic*, ble_gap_conn_desc*, uint16_t subValue) override {
    subscribed = (subValue & 1) != 0;
    if (Config::DEBUG_APP_SYNC) Serial.printf("SYNC: app subscribe=%u\n", subValue);
  }
#endif
};
}  // namespace

void BattlePass::begin(bool enabled) {
  passStoreOpen = passStore.begin("bey-pass", false);
  if (passStoreOpen) loadState();
  if (appCount > SLOTS) appCount = 0;
  setEnabled(enabled);
}

void BattlePass::setEnabled(bool enabled) {
  enabled_ = enabled;
  advertisingWanted = enabled;
  if (!enabled) {
    if (!initialized_) return;
    BLEDevice::stopAdvertising();
    for (const auto& peer : server->getPeerDevices(false)) server->disconnect(peer.first);
    return;
  }
  if (initialized_) {
    BLEDevice::startAdvertising();
    return;
  }
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  for (uint8_t i = 0; i < 6; ++i) address[i] = mac[5 - i];

  BLEDevice::init(DEVICE_NAME);
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService* service = server->createService(SERVICE_UUID);
  BLECharacteristic* writeChar = service->createCharacteristic(
      WRITE_UUID, BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_WRITE_NR);
  writeChar->setCallbacks(new WriteCallbacks());
  notifyChar = service->createCharacteristic(
      NOTIFY_UUID, BLECharacteristic::PROPERTY_READ |
                       BLECharacteristic::PROPERTY_NOTIFY);
  notifyChar->setCallbacks(new NotifyCallbacks());
  service->start();

  // Flags + name fill the advertisement; the 128-bit UUID goes to the scan
  // response so both name- and UUID-filtered scans find the device.
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  BLEAdvertisementData advert;
  advert.setFlags(0x06);
  advert.setName(DEVICE_NAME);
  BLEAdvertisementData response;
  response.setCompleteServices(BLEUUID(SERVICE_UUID));
  advertising->setAdvertisementData(advert);
  advertising->setScanResponseData(response);
  BLEDevice::startAdvertising();
  initialized_ = true;
}

bool BattlePass::connected() const {
  return initialized_ && enabled_ && server->getConnectedCount() > 0;
}

bool BattlePass::takeSync(uint8_t& count) {
  if (!syncPending_) return false;
  syncPending_ = false;
  count = syncCount_;
  return true;
}

bool BattlePass::update(bool readyToLaunch) {
  if (!initialized_ || !enabled_) return false;
  bool activity = false;
  if (dropAtMs_ && static_cast<int32_t>(millis() - dropAtMs_) >= 0) {
    dropAtMs_ = 0;
    for (const auto& peer : server->getPeerDevices(false)) server->disconnect(peer.first);
  }
  status_ = buttonHeld_ ? STATUS_BUTTON : readyToLaunch ? STATUS_READY : STATUS_IDLE;
  if (pendingConnect) {
    pendingConnect = false;
    lastStatus_ = 0xFF;  // Force a fresh header for the new client.
    activity = true;
  }
  uint8_t command = 0;
  while (popCommand(command)) {
    activity = true;
    switch (command) {
      case CMD_HEADER:
        sendHeader();
        break;
      case CMD_HISTORY:
        sendHeader();
        sendHistory();
        // Only remember what was sent: the notice appears when the app
        // confirms with a clear, not on every (possibly repeated) request.
        readCount_ = appCount;
        break;
      case CMD_CLEAR: {
        if (readCount_ && subscribed) {
          syncCount_ = readCount_;
          syncPending_ = true;
        }
        readCount_ = 0;
        appCount = 0;
        appTotal = 0;
        appBest = 0;
        saveState();
        uint8_t page[PAGE_SIZE] = {0xA1};
        notify(page);
        sendHeader();
        sendHistory();
        break;
      }
      case CMD_POWER_OFF:
        // The real pass switches off now and the app reconnects at the next
        // sync. Drop the link shortly so it does.
        dropAtMs_ = (millis() + 300) | 1;
        break;
      default:
        break;
    }
  }
  if (status_ != lastStatus_ && connected()) sendHeader();
  return activity;
}

void BattlePass::publishLaunch(const LaunchRecord& record,
                               const RpmSample* profile,
                               uint16_t profileCount) {
  // Launches are kept for the app even while Bluetooth is off, like the
  // pass's own memory; they are sent on the next sync.
  const uint16_t value = clamp16(record.peakRpm);
  if (appCount == SLOTS) {
    memmove(appSlots, appSlots + 1, (SLOTS - 1) * sizeof(appSlots[0]));
    --appCount;
  }
  appSlots[appCount++] = value;
  if (appTotal < 0xFFFF) ++appTotal;
  appBest = max(appBest, value);
  saveState();

  curveValid = profileCount > 0;
  for (uint8_t i = 0; i < CURVE_SAMPLES; ++i) {
    const uint32_t rpm =
        curveValid ? profile[static_cast<uint32_t>(i) * profileCount / CURVE_SAMPLES].rpm : 0;
    curve[i] = rpm ? clamp16(7500000UL / rpm) : 0;
  }
  // The bey has left the launcher; the real pass reports 0x00 after a launch.
  status_ = STATUS_IDLE;
  if (!connected()) return;
  sendHeader();
  sendHistory();
  sendCurve();
}

void BattlePass::setButtonHeld(bool held) {
  if (held == buttonHeld_) return;
  buttonHeld_ = held;
}

void BattlePass::sendHeader() { sendHeader(status_); }

void BattlePass::sendHeader(uint8_t status) {
  uint8_t page[PAGE_SIZE] = {0xA0, 0x3A};
  lastStatus_ = page[3] = status;
  page[4] = BATTERY_FULL;
  // The pass mirrors its b6 summary here; the app cross-checks the two.
  putLe16(page + 7, appBest);
  putLe16(page + 9, appTotal);
  memcpy(page + 11, address, sizeof(address));
  notify(page);
}

void BattlePass::sendHistory() {
  // 7 pages x 16 data bytes = 112 bytes: 50 slots, then the summary in b6.
  uint8_t data[7 * 16] = {};
  for (uint8_t i = 0; i < appCount; ++i) putLe16(data + i * 2, appSlots[i]);
  uint8_t* summary = data + 6 * 16;
  putLe16(summary + 6, appBest);
  putLe16(summary + 8, appTotal);
  putLe16(summary + 10, appCount);

  uint8_t checksum = 0;
  for (const uint8_t byte : data) checksum += byte;
  uint8_t page[PAGE_SIZE];
  for (uint8_t p = 0; p < 7; ++p) {
    page[0] = 0xB0 + p;
    memcpy(page + 1, data + p * 16, 16);
    notify(page);
  }
  memset(page, 0, sizeof(page));
  page[0] = 0xB7;
  page[16] = checksum;
  notify(page);
}

void BattlePass::sendCurve() {
  if (!curveValid) return;
  uint8_t page[PAGE_SIZE];
  for (uint8_t p = 0; p < 4; ++p) {
    page[0] = 0x70 + p;
    for (uint8_t i = 0; i < 8; ++i) putLe16(page + 1 + i * 2, curve[p * 8 + i]);
    notify(page);
  }
}

void BattlePass::notify(const uint8_t* page) {
  if (Config::DEBUG_APP_SYNC) {
    Serial.print("SYNC: sent");
    for (uint8_t i = 0; i < PAGE_SIZE; ++i) Serial.printf(" %02x", page[i]);
    Serial.println();
  }
  notifyChar->setValue(const_cast<uint8_t*>(page), PAGE_SIZE);
  notifyChar->notify();
  // Pace the burst so the controller's notification buffers do not overflow.
  delay(8);
}
