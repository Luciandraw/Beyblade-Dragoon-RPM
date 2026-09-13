#include "app_state.h"
#include "buttons.h"
#include "config.h"
#include "display.h"
#include "launch_record.h"
#include "rpm_calculator.h"
#include "sensor.h"
#include "storage.h"

#include <driver/rtc_io.h>
#include <esp_sleep.h>

AppStateMachine app;
Buttons buttons;
Display display;
Sensor sensor;
Storage storage;
RpmCalculator rpm;

static LaunchRecord lastResult;
static RpmSample lastProfile[Config::PROFILE_SIZE];
static uint16_t lastProfileCount = 0;
static uint8_t menuItem = 0;
static uint8_t historyOffset = 0;
static bool historyView = false;
static bool graphView = false;
static bool clearConfirmation = false;
static bool serviceView = false;
static bool bladeArmed = false;
static bool suppressWakeButton = false;
static uint32_t serviceTransitions = 0;
static uint32_t lastActivityMs = 0;

void publishLaunchResult(const LaunchRecord& record) {
  (void)record;
}

void enterCalibration() {
  bladeArmed = false;
  app.set(AppState::Calibrating);
  display.showCalibrating(Config::ADC_MAX_VALUE, 0);
  sensor.beginCalibration();
  Serial.println("CALIBRATION: pull launcher smoothly for 3 seconds");
}

void enterReady() {
  rpm.reset();
  if (buttons.bladePresent()) bladeArmed = true;
  app.set(AppState::Ready);
  if (bladeArmed) {
    display.showReady(storage.bestRpm(), storage.launchCounter());
  } else {
    display.showInsertBlade(storage.bestRpm(), storage.launchCounter());
  }
  lastActivityMs = millis();
  Serial.println("READY");
}

void enterDeepSleep() {
  Serial.println("SLEEP: logo displayed; waiting for center button");
  display.showBoot();
  display.hibernate();
  Serial.flush();

  const gpio_num_t wakePin = static_cast<gpio_num_t>(Config::PIN_BUTTON_CENTER);
  rtc_gpio_pulldown_dis(wakePin);
  rtc_gpio_pullup_en(wakePin);
  esp_sleep_enable_ext0_wakeup(wakePin, 0);
  esp_deep_sleep_start();
}

void showCurrentMenu() {
  historyView = graphView = clearConfirmation = false;
  display.showMenu(menuItem);
}

void openHistory() {
  historyView = true;
  historyOffset = min<uint8_t>(historyOffset, storage.count() ? storage.count() - 1 : 0);
  LaunchRecord record;
  const bool found = storage.getNewest(historyOffset, record);
  display.showHistory(found ? &record : nullptr, historyOffset, storage.count());
}

void finishLaunch() {
  const uint32_t durationMs = (rpm.lastEdgeUs() - rpm.startedAtUs()) / 1000UL;
  bladeArmed = false;
  if (!rpm.validLaunch(durationMs)) {
    Serial.printf("REJECTED valid=%lu rejected=%lu fast=%lu slow=%lu jump=%lu duration=%lums adc=%luHz\n",
                  static_cast<unsigned long>(rpm.validPeriodCount()),
                  static_cast<unsigned long>(rpm.rejectedPeriodCount()),
                  static_cast<unsigned long>(rpm.rejectedTooFastCount()),
                  static_cast<unsigned long>(rpm.rejectedTooSlowCount()),
                  static_cast<unsigned long>(rpm.rejectedJumpCount()),
                  static_cast<unsigned long>(durationMs),
                  static_cast<unsigned long>(sensor.sampleRateHz()));
    // Keep rejected attempts out of the user interface. They remain visible
    // in Serial diagnostics, then the device immediately rearms.
    enterReady();
    return;
  }
  app.set(AppState::ShowingResult);
  lastResult = rpm.makeRecord(storage.nextLaunchIndex(), sensor.range());
  lastProfileCount = rpm.profileCount();
  for (uint16_t i = 0; i < lastProfileCount; ++i) lastProfile[i] = rpm.profile()[i];
  const bool newBest = lastResult.peakRpm > storage.bestRpm();
  storage.add(lastResult);
  publishLaunchResult(lastResult);
  display.showResult(lastResult, storage.bestRpm(), newBest);
  Serial.printf("RESULT peak=%lu avg=%lu duration=%lums samples=%lu edges=%lu rejected=%lu adc=%luHz\n",
                static_cast<unsigned long>(lastResult.peakRpm),
                static_cast<unsigned long>(lastResult.averageRpm),
                static_cast<unsigned long>(lastResult.durationMs),
                static_cast<unsigned long>(lastResult.validSampleCount),
                static_cast<unsigned long>(rpm.transitionCount()),
                static_cast<unsigned long>(rpm.rejectedPeriodCount()),
                static_cast<unsigned long>(sensor.sampleRateHz()));
}

void handleMenu(const ButtonEvents& events) {
  if (clearConfirmation) {
    if (events.centerDouble) {
      storage.clear(); menuItem = 0; showCurrentMenu();
    } else if (events.centerPressed || events.centerLong) {
      showCurrentMenu();
    }
    return;
  }
  if (graphView) {
    if (events.centerPressed || events.centerDouble || events.centerLong) showCurrentMenu();
    return;
  }
  if (historyView) {
    if (events.centerPressed && storage.count()) {
      historyOffset = (historyOffset + 1) % storage.count();
      openHistory();
    } else if (events.centerLong) {
      showCurrentMenu();
    }
    return;
  }
  if (events.centerPressed) {
    menuItem = (menuItem + 1) % 5; display.showMenu(menuItem);
  } else if (events.centerDouble) {
    switch (menuItem) {
      case 0: openHistory(); break;
      case 1:
        graphView = true;
        display.showGraph(lastProfileCount ? lastProfile : nullptr,
                          lastProfileCount, lastResult.peakRpm);
        break;
      case 2: enterCalibration(); break;
      case 3: clearConfirmation = true; display.showClearConfirmation(); break;
      default: enterReady(); break;
    }
  } else if (events.centerLong) {
    enterReady();
  }
}

void setup() {
  const bool wokeFromCenter =
      esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
  Serial.begin(Config::SERIAL_BAUD);
  delay(250);
  Serial.println("BOOT: serial OK");
  if (wokeFromCenter) {
    rtc_gpio_deinit(static_cast<gpio_num_t>(Config::PIN_BUTTON_CENTER));
  }
  buttons.begin();
  sensor.begin();
  storage.begin();
  Serial.println("EPD: initializing");
  display.begin();
  Serial.println("EPD: initialization returned");
  app.set(AppState::Boot);
  if (!wokeFromCenter) {
    display.showBoot();
  } else {
    suppressWakeButton = true;
    Serial.println("WAKE: center button");
  }
  lastActivityMs = millis();
  Serial.printf("Beyblade RPM Analyzer; panel=%s\n", Config::EPD_DRIVER_NAME);
}

void loop() {
  ButtonEvents events = buttons.update();
  if (suppressWakeButton) {
    events.centerPressed = false;
    events.centerDouble = false;
    events.centerLong = false;
    events.serviceHold = false;
    if (!buttons.centerDown()) suppressWakeButton = false;
  }
  if (buttons.centerDown() || events.centerPressed || events.centerDouble ||
      events.centerLong || events.serviceHold ||
      events.bladeInserted || events.bladeRemoved) {
    lastActivityMs = millis();
  }
  if (events.bladeInserted && app.get() == AppState::Ready && !bladeArmed) {
    bladeArmed = true;
    rpm.reset();
    display.showReady(storage.bestRpm(), storage.launchCounter());
    Serial.println("BLADE PRESENT: armed");
  }
  SensorReading reading;
  while (sensor.sample(reading)) {
    if (reading.discontinuity) {
      // Never calculate a turn across missing ADC data or a display pause.
      rpm.reset();
      if (app.get() == AppState::Measuring) app.set(AppState::Ready);
    }
    if (reading.risingEdge || reading.fallingEdge) ++serviceTransitions;
    if (reading.risingEdge &&
        ((app.get() == AppState::Ready && bladeArmed) ||
         app.get() == AppState::Measuring)) {
      const bool startedNow = rpm.acceptRisingEdge(reading.timestampUs);
      if (app.get() == AppState::Ready && startedNow && rpm.hasStarted()) {
        app.set(AppState::Measuring);
        Serial.println("START");
      }
    }
    if (Config::DEBUG_SENSOR) {
      static uint32_t lastDebugMs = 0;
      if (millis() - lastDebugMs >= 100) {
        lastDebugMs = millis();
        Serial.printf("ADC raw=%u filtered=%u state=%u\n",
                      reading.raw, reading.filtered, reading.comparatorHigh);
      }
    }
  }

  const AppState currentState = app.get();
  if (currentState != AppState::Measuring &&
      currentState != AppState::Calibrating &&
      static_cast<uint32_t>(millis() - lastActivityMs) >=
          Config::INACTIVITY_SLEEP_MS) {
    enterDeepSleep();
  }

  if (serviceView) {
    if (events.centerLong || events.serviceHold) {
      serviceView = false; enterReady();
    } else if (events.centerPressed) {
      display.showService(sensor.lastRaw(), sensor.minimum(), sensor.maximum(),
                          sensor.thresholdLow(), sensor.thresholdHigh(),
                          serviceTransitions, sensor.comparatorState());
      Serial.printf("SERVICE raw=%u min=%u max=%u low=%u high=%u edges=%lu state=%u\n",
                    sensor.lastRaw(), sensor.minimum(), sensor.maximum(),
                    sensor.thresholdLow(), sensor.thresholdHigh(),
                    static_cast<unsigned long>(serviceTransitions),
                    sensor.comparatorState());
    }
    return;
  }
  if (events.serviceHold && app.get() != AppState::Measuring) {
    serviceView = true;
    display.showService(sensor.lastRaw(), sensor.minimum(), sensor.maximum(),
                        sensor.thresholdLow(), sensor.thresholdHigh(),
                        serviceTransitions, sensor.comparatorState());
    return;
  }

  switch (app.get()) {
    case AppState::Boot:
      if (app.elapsed(Config::SENSOR_STABILIZE_MS)) enterReady();
      break;
    case AppState::Calibrating:
      if (sensor.calibrationFinished()) {
        if (sensor.finishCalibration()) {
          Serial.printf("CAL OK min=%u max=%u low=%u high=%u\n", sensor.minimum(),
                        sensor.maximum(), sensor.thresholdLow(), sensor.thresholdHigh());
          enterReady();
        } else {
          app.set(AppState::SensorError);
          display.showSensorError(sensor.minimum(), sensor.maximum());
          Serial.printf("SENSOR ERROR min=%u max=%u\n", sensor.minimum(), sensor.maximum());
        }
      }
      break;
    case AppState::Ready:
      if (events.centerPressed) {
        app.set(AppState::Menu); menuItem = 0; showCurrentMenu();
      } else if (events.centerLong) {
        enterCalibration();
      }
      break;
    case AppState::Measuring:
      if (rpm.stopped(micros())) finishLaunch();
      if (events.centerLong) enterReady();
      break;
    case AppState::ShowingResult:
      if (events.centerPressed || events.centerDouble || events.centerLong ||
          app.elapsed(Config::RESULT_SCREEN_MS)) enterReady();
      break;
    case AppState::Menu:
      handleMenu(events);
      break;
    case AppState::SensorError:
      if (events.centerPressed || events.centerDouble || events.centerLong) enterCalibration();
      break;
  }
  yield();
}
