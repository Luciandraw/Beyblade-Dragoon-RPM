#pragma once

#include <Arduino.h>

namespace Config {
// ESP32-S3-Zero pinout. GPIO0/3/45/46 are strapping pins and are avoided.
// GPIO19/20 are native USB D-/D+ and must remain untouched.
constexpr uint8_t PIN_SENSOR = 1;       // ADC1_CH0
constexpr uint8_t PIN_EPD_RST = 4;
constexpr uint8_t PIN_EPD_DC = 5;
constexpr uint8_t PIN_EPD_SCK = 6;
constexpr uint8_t PIN_EPD_BUSY = 7;
constexpr uint8_t PIN_EPD_MOSI = 8;
constexpr uint8_t PIN_EPD_CS = 9;
constexpr uint8_t PIN_BLADE_PRESENT = 10;
constexpr uint8_t PIN_BUTTON_CENTER = 11;

// The real encoder disc has one white half and one black half. Only the more
// stable LOW -> HIGH edge is counted: one measured period per revolution.
constexpr uint16_t ENCODER_PERIODS_PER_REV = 1;
constexpr uint16_t ENCODER_EDGES_PER_REV = 2;
constexpr float DRIVE_RATIO = 1.0f;
constexpr bool SENSOR_INVERTED = false;

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint16_t ADC_MAX_VALUE = 4095;
constexpr uint32_t ADC_SAMPLE_RATE_HZ = 80000;
constexpr uint32_t ADC_SAMPLE_INTERVAL_US = 1000000UL / ADC_SAMPLE_RATE_HZ;
constexpr uint32_t SENSOR_STABILIZE_MS = 700;
constexpr uint32_t CALIBRATION_TIME_MS = 3000;
constexpr uint16_t SENSOR_MIN_CONTRAST = 300;
// The encoder has two broad half-disc areas, so filtering and a wide
// hysteresis reject ADC noise without hiding real black/white transitions.
constexpr uint8_t SENSOR_HYSTERESIS_PERCENT = 10;
constexpr uint32_t SENSOR_EDGE_CONFIRM_US = 75;
// Fixed thresholds measured on the assembled device. Startup calibration is
// skipped; manual calibration remains available from the menu if optics move.
constexpr uint16_t SENSOR_FIXED_MIN = 2000;
constexpr uint16_t SENSOR_FIXED_MAX = 4095;
constexpr uint16_t SENSOR_FIXED_THRESHOLD_LOW = 3548;
constexpr uint16_t SENSOR_FIXED_THRESHOLD_HIGH = 3730;

constexpr uint8_t START_MIN_VALID_EDGES = 3;
constexpr uint32_t START_WINDOW_US = 200000;
constexpr uint32_t STOP_TIMEOUT_US = 250000;
constexpr uint32_t MIN_VALID_RPM = 10;
constexpr uint32_t MAX_VALID_RPM = 30000;
constexpr uint8_t FILTER_WINDOW = 5;
// Three start edges contain two complete periods, enough for a short
// string-launcher pull after period validation.
constexpr uint8_t MIN_VALID_PERIODS = 2;
constexpr uint8_t MAX_REJECT_PERCENT = 60;
// Short string-launcher pulls can contain several clean revolutions in less
// than 40 ms. Valid periods and RPM bounds provide the rejection criteria.
constexpr uint16_t MIN_LAUNCH_DURATION_MS = 0;

constexpr uint32_t MIN_PERIOD_US =
    60000000UL / (MAX_VALID_RPM * ENCODER_PERIODS_PER_REV);
constexpr uint32_t MAX_PERIOD_US =
    60000000UL / (MIN_VALID_RPM * ENCODER_PERIODS_PER_REV);
constexpr uint32_t MIN_EDGE_INTERVAL_US = MIN_PERIOD_US / 2;

constexpr uint16_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint16_t DOUBLE_CLICK_MS = 350;
constexpr uint16_t CENTER_LONG_PRESS_MS = 900;
constexpr uint16_t SERVICE_HOLD_MS = 3000;

constexpr uint8_t HISTORY_SIZE = 20;
constexpr uint16_t PROFILE_SIZE = 192;
constexpr uint8_t FULL_REFRESH_AFTER_PARTIAL = 6;
constexpr uint32_t RESULT_SCREEN_MS = 5000;
constexpr uint32_t INACTIVITY_SLEEP_MS = 60000;

constexpr bool DEBUG_SENSOR = false;
constexpr bool DEBUG_RPM = false;
constexpr bool DEBUG_BUTTONS = false;

// Hardware assumption: WeAct 1.54 inch, 200x200, SSD1681/GDEY0154D67.
// Change the panel type in display.cpp if the label on the flex cable differs.
constexpr const char* EPD_DRIVER_NAME = "GxEPD2_154_D67";
}  // namespace Config
