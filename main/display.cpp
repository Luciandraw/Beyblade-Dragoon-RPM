#include "display.h"

#include <GxEPD2_BW.h>
#include <math.h>
#include <SPI.h>
#include <vector>

#include "config.h"
#include "blade_icon.h"
#include "logo_bitmap.h"

// Game Boy styling for a 1-bit panel, matching the project website:
// dotted ground, black header/footer bars, Pokemon-style double-framed boxes
// and a triangle cursor. Only Adafruit GFX built-ins (5x7 font, rects, round
// rects, circles, bitmaps) are used so docs/screens.html can mirror every
// screen pixel for pixel. Keep that emulator in step when changing layouts.

using Panel = GxEPD2_154_D67;
static GxEPD2_BW<Panel, Panel::HEIGHT> epd(
    Panel(Config::PIN_EPD_CS, Config::PIN_EPD_DC,
          Config::PIN_EPD_RST, Config::PIN_EPD_BUSY));

namespace {
constexpr uint16_t BLACK = GxEPD_BLACK;
constexpr uint16_t WHITE = GxEPD_WHITE;
uint8_t partialCount = 0;

void startFrame(bool partial = false) {
  if (partial && partialCount < Config::FULL_REFRESH_AFTER_PARTIAL) {
    epd.setPartialWindow(0, 0, epd.width(), epd.height());
    ++partialCount;
  } else {
    epd.setFullWindow();
    partialCount = 0;
  }
  epd.firstPage();
}

int16_t textWidth(const char* text, uint8_t size) {
  return static_cast<int16_t>(strlen(text)) * 6 * size;
}

void text(const char* value, int16_t x, int16_t y, uint8_t size = 1,
          uint16_t color = BLACK) {
  epd.setTextSize(size);
  epd.setTextColor(color);
  epd.setCursor(x, y);
  epd.print(value);
}

void centered(const char* value, int16_t y, uint8_t size = 1,
              uint16_t color = BLACK) {
  text(value, (200 - textWidth(value, size)) / 2, y, size, color);
}

void rightAligned(const char* value, int16_t right, int16_t y, uint8_t size = 1,
                  uint16_t color = BLACK) {
  text(value, right - textWidth(value, size), y, size, color);
}

// Sparse two-dot pattern, like the ground tiles on the website.
void ground() {
  epd.fillScreen(WHITE);
  for (int16_t y = 0; y < 200; ++y) {
    for (int16_t x = 0; x < 200; ++x) {
      if ((x % 8 == 1 && y % 8 == 2) || (x % 8 == 5 && y % 8 == 6)) {
        epd.drawPixel(x, y, BLACK);
      }
    }
  }
}

// Pokemon text box: thick rounded outline with a thin inner rule.
void frame(int16_t x, int16_t y, int16_t w, int16_t h) {
  epd.fillRoundRect(x, y, w, h, 4, BLACK);
  epd.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 3, WHITE);
  epd.drawRoundRect(x + 4, y + 4, w - 8, h - 8, 2, BLACK);
}

// Right-pointing cursor, 7 px tall per size step.
void cursor(int16_t x, int16_t y, uint8_t size = 1, uint16_t color = BLACK) {
  for (int16_t i = 0; i < 4 * size; ++i) {
    epd.drawFastVLine(x + i, y + i, 8 * size - 1 - 2 * i, color);
  }
}

// "More text" prompt in the corner of a dialog box.
void prompt(int16_t x, int16_t y) {
  for (int16_t i = 0; i < 4; ++i) epd.drawFastHLine(x + i, y + i, 7 - 2 * i, BLACK);
}

void header(const char* title, const char* right = nullptr) {
  epd.fillRect(0, 0, 200, 20, BLACK);
  epd.fillRect(4, 3, 14, 14, WHITE);
  text("R", 8, 6, 1, BLACK);
  text(title, 24, 6, 1, WHITE);
  if (right) rightAligned(right, 195, 6, 1, WHITE);
}

void footer(const char* left, const char* right = nullptr) {
  epd.fillRect(0, 184, 200, 16, BLACK);
  text(left, 6, 188, 1, WHITE);
  if (right) rightAligned(right, 195, 188, 1, WHITE);
}

void statsFooter(uint32_t bestRpm, uint32_t launchCount) {
  char best[20], next[12];
  snprintf(best, sizeof(best), "BEST %lu", static_cast<unsigned long>(bestRpm));
  snprintf(next, sizeof(next), "#%03lu", static_cast<unsigned long>(launchCount + 1));
  footer(best, next);
}

void drawBoot() {
  startFrame();
  do {
    epd.fillScreen(WHITE);
    epd.drawBitmap(0, 0, BOOT_LOGO_BITMAP, BOOT_LOGO_WIDTH, BOOT_LOGO_HEIGHT, BLACK);
  } while (epd.nextPage());
}

void drawCalibrating(uint16_t minimum, uint16_t maximum) {
  char line[24];
  startFrame(true);
  do {
    ground();
    header("CALIBRATION");
    frame(4, 26, 192, 98);
    centered("PULL", 42, 3);
    centered("SMOOTHLY", 74, 2);
    centered("FOR 3 SECONDS", 100, 1);
    frame(4, 128, 192, 52);
    snprintf(line, sizeof(line), "MIN %u", minimum);
    text(line, 16, 142);
    snprintf(line, sizeof(line), "MAX %u", maximum);
    text(line, 16, 158);
    prompt(178, 164);
    footer("SENSOR SETUP");
  } while (epd.nextPage());
}

void drawInsertBlade(uint32_t bestRpm, uint32_t launchCount) {
  startFrame();
  do {
    ground();
    header("BEYBLADE RPM");
    frame(60, 28, 80, 80);
    epd.drawBitmap(81, 49, BLADE_ICON_BITMAP, BLADE_ICON_WIDTH, BLADE_ICON_HEIGHT, BLACK);
    frame(4, 114, 192, 66);
    text("INSERT", 16, 128, 2);
    text("BLADE", 16, 148, 2);
    text("TO ARM", 112, 134);
    text("LAUNCHER", 112, 148);
    prompt(178, 164);
    statsFooter(bestRpm, launchCount);
  } while (epd.nextPage());
}

void drawReady(uint32_t bestRpm, uint32_t launchCount) {
  startFrame();
  do {
    ground();
    header("BEYBLADE RPM", "ARMED");
    frame(4, 26, 192, 84);
    epd.drawBitmap(16, 49, BLADE_ICON_BITMAP, BLADE_ICON_WIDTH, BLADE_ICON_HEIGHT, BLACK);
    text("READY", 64, 52, 4);
    frame(4, 114, 192, 66);
    cursor(14, 130, 1);
    text("PULL THE", 26, 130, 1);
    text("STRING!", 26, 146, 2);
    prompt(178, 164);
    statsFooter(bestRpm, launchCount);
  } while (epd.nextPage());
}

void drawResult(const LaunchRecord& record, uint32_t bestRpm, bool newBest) {
  char rpm[12], line[24], title[16];
  snprintf(rpm, sizeof(rpm), "%lu", static_cast<unsigned long>(record.peakRpm));
  snprintf(title, sizeof(title), "LAUNCH %03lu", static_cast<unsigned long>(record.timestamp));
  startFrame();
  do {
    ground();
    header(title, newBest ? "NEW BEST!" : "RESULT");
    frame(4, 24, 192, 112);

    // Tachometer arc from 0 to 15K RPM.
    constexpr int16_t gaugeX = 100;
    constexpr int16_t gaugeY = 126;
    constexpr int16_t gaugeRadius = 78;
    constexpr uint32_t gaugeMaxRpm = 15000;
    int16_t previousX = gaugeX - gaugeRadius;
    int16_t previousY = gaugeY;
    for (int16_t degrees = 185; degrees <= 360; degrees += 5) {
      const float radians = degrees * PI / 180.0f;
      const int16_t x = gaugeX + lroundf(cosf(radians) * gaugeRadius);
      const int16_t y = gaugeY + lroundf(sinf(radians) * gaugeRadius);
      epd.drawLine(previousX, previousY, x, y, BLACK);
      previousX = x;
      previousY = y;
    }
    for (uint8_t tick = 0; tick <= 10; ++tick) {
      const float radians = (180.0f + tick * 18.0f) * PI / 180.0f;
      const int16_t inner = tick % 5 == 0 ? 66 : 71;
      epd.drawLine(gaugeX + lroundf(cosf(radians) * inner),
                   gaugeY + lroundf(sinf(radians) * inner),
                   gaugeX + lroundf(cosf(radians) * gaugeRadius),
                   gaugeY + lroundf(sinf(radians) * gaugeRadius), BLACK);
    }
    const uint32_t gaugeRpm = min<uint32_t>(record.peakRpm, gaugeMaxRpm);
    const float marker = (180.0f + 180.0f * gaugeRpm / gaugeMaxRpm) * PI / 180.0f;
    epd.fillCircle(gaugeX + lroundf(cosf(marker) * 72),
                   gaugeY + lroundf(sinf(marker) * 72), 3, BLACK);
    centered(rpm, 80, 4);
    centered("RPM", 112, 1);
    text("0", 26, 118);
    text("15K", 156, 118);

    // HP-style power bar, saturated at the gauge maximum.
    frame(4, 138, 192, 46);
    text("PWR", 12, 150);
    epd.drawRoundRect(34, 149, 154, 9, 3, BLACK);
    const int16_t fill = static_cast<int16_t>(150UL * gaugeRpm / gaugeMaxRpm);
    if (fill > 0) epd.fillRect(36, 151, fill, 5, BLACK);
    snprintf(line, sizeof(line), "AVG %lu", static_cast<unsigned long>(record.averageRpm));
    text(line, 12, 166);
    snprintf(line, sizeof(line), "%lums", static_cast<unsigned long>(record.durationMs));
    rightAligned(line, 188, 166);
    snprintf(line, sizeof(line), "BEST %lu", static_cast<unsigned long>(bestRpm));
    footer(line, "CLICK: OK");
  } while (epd.nextPage());
}

void drawInvalidLaunch() {
  startFrame();
  do {
    ground();
    header("LAUNCH");
    frame(4, 40, 192, 100);
    centered("NO VALID", 62, 2);
    centered("DATA", 84, 2);
    centered("TRY AGAIN", 114, 1);
    footer("REJECTED");
  } while (epd.nextPage());
}

void drawSensorError(uint16_t minimum, uint16_t maximum) {
  char line[24];
  startFrame();
  do {
    ground();
    header("SENSOR", "ERROR");
    frame(4, 26, 192, 98);
    centered("RANGE", 42, 2);
    centered("TOO LOW", 62, 2);
    snprintf(line, sizeof(line), "MIN %u  MAX %u", minimum, maximum);
    centered(line, 88);
    centered("CHECK DISC + SENSOR", 104);
    frame(4, 128, 192, 52);
    cursor(14, 146, 1);
    text("CLICK: RETRY", 26, 146);
    footer("SENSOR ALIGN");
  } while (epd.nextPage());
}

void drawHistory(bool hasRecord, const LaunchRecord& record, uint8_t position, uint8_t count) {
  char line[24], page[12];
  snprintf(page, sizeof(page), "%u/%u", hasRecord ? position + 1 : 0, count);
  startFrame(true);
  do {
    ground();
    header("HISTORY", page);
    frame(4, 26, 192, 112);
    if (!hasRecord) {
      centered("NO LAUNCHES", 66, 2);
      centered("YET", 88, 2);
    } else {
      snprintf(line, sizeof(line), "#%03lu", static_cast<unsigned long>(record.timestamp));
      text(line, 16, 38, 2);
      epd.drawBitmap(146, 36, BLADE_ICON_BITMAP, BLADE_ICON_WIDTH, BLADE_ICON_HEIGHT, BLACK);
      snprintf(line, sizeof(line), "%lu", static_cast<unsigned long>(record.peakRpm));
      text(line, 16, 74, 4);
      text("RPM PEAK", 16, 112);
    }
    frame(4, 140, 192, 44);
    if (hasRecord) {
      snprintf(line, sizeof(line), "AVG %lu", static_cast<unsigned long>(record.averageRpm));
      text(line, 14, 152);
      snprintf(line, sizeof(line), "%lums", static_cast<unsigned long>(record.durationMs));
      rightAligned(line, 186, 152);
    }
    text("CLICK NEXT", 14, 166);
    rightAligned("HOLD BACK", 186, 166);
    footer("LAUNCH LOG");
  } while (epd.nextPage());
}

void drawMenu(uint8_t item) {
  static const char* const items[] = {"HISTORY", "GRAPH", "CALIBRATE", "CLEAR LOG", "BACK"};
  startFrame(true);
  do {
    ground();
    header("MENU");
    frame(4, 26, 192, 140);
    for (uint8_t i = 0; i < 5; ++i) {
      const int16_t y = 38 + i * 25;
      if (i == item) cursor(16, y, 2);
      text(items[i], 34, y, 2);
    }
    footer("2X SELECT", "HOLD BACK");
  } while (epd.nextPage());
}

void drawClearConfirmation() {
  startFrame(true);
  do {
    ground();
    header("CLEAR LOG?");
    frame(4, 26, 192, 92);
    centered("ERASE ALL", 42, 2);
    centered("LAUNCHES?", 62, 2);
    centered("CANNOT BE UNDONE", 92);
    frame(100, 120, 96, 62);
    cursor(112, 134, 2);
    text("YES", 130, 134, 2);
    text("NO", 130, 156, 2);
    frame(4, 120, 94, 62);
    text("DOUBLE", 14, 134);
    text("= YES", 14, 144);
    text("CLICK", 14, 158);
    text("= NO", 14, 168);
    footer("HISTORY ONLY", "KEEPS CAL");
  } while (epd.nextPage());
}

void drawService(uint16_t raw, uint16_t minimum, uint16_t maximum, uint16_t low,
                 uint16_t high, uint32_t transitions, bool state) {
  char line[32];
  startFrame(true);
  do {
    ground();
    header("SERVICE", state ? "HIGH" : "LOW");
    frame(4, 26, 192, 134);
    const char* labels[] = {"RAW", "MIN/MAX", "LOW/HIGH", "RANGE", "EDGES"};
    for (uint8_t i = 0; i < 5; ++i) {
      const int16_t y = 40 + i * 22;
      text(labels[i], 14, y);
      switch (i) {
        case 0: snprintf(line, sizeof(line), "%u", raw); break;
        case 1: snprintf(line, sizeof(line), "%u/%u", minimum, maximum); break;
        case 2: snprintf(line, sizeof(line), "%u/%u", low, high); break;
        case 3: snprintf(line, sizeof(line), "%u", maximum - minimum); break;
        default: snprintf(line, sizeof(line), "%lu", static_cast<unsigned long>(transitions)); break;
      }
      rightAligned(line, 186, y);
      if (i < 4) {
        for (int16_t x = 14; x < 186; x += 3) epd.drawPixel(x, y + 13, BLACK);
      }
    }
    frame(4, 162, 192, 22);
    centered("CLICK REFRESH  HOLD EXIT", 169);
    footer("SENSOR DIAGNOSTICS");
  } while (epd.nextPage());
}

void drawGraph(const std::vector<RpmSample>& samples, uint32_t peakRpm) {
  const uint16_t count = samples.size();
  startFrame();
  do {
    ground();
    header("LAUNCH PROFILE");
    frame(0, 22, 200, 160);

    const int16_t left = 27;
    const int16_t width = 163;
    const int16_t lineTop = 32;
    const int16_t lineHeight = 60;
    const int16_t barsTop = 102;
    const int16_t barsHeight = 70;
    const int16_t right = left + width - 1;

    epd.drawRect(left, lineTop, width, lineHeight, BLACK);
    epd.drawRect(left, barsTop, width, barsHeight, BLACK);
    // Dotted oscilloscope-style grid.
    for (uint8_t division = 1; division < 4; ++division) {
      const int16_t x = left + division * (width - 1) / 4;
      for (int16_t y = lineTop + 2; y < lineTop + lineHeight - 1; y += 4) epd.drawPixel(x, y, BLACK);
      for (int16_t y = barsTop + 2; y < barsTop + barsHeight - 1; y += 4) epd.drawPixel(x, y, BLACK);
    }
    const int16_t lineMid = lineTop + (lineHeight - 1) / 2;
    const int16_t barsMid = barsTop + (barsHeight - 1) / 2;
    for (int16_t x = left + 2; x < right; x += 4) {
      epd.drawPixel(x, lineMid, BLACK);
      epd.drawPixel(x, barsMid, BLACK);
    }
    text("100", 8, lineTop + 1);
    text("50", 14, lineMid - 3);
    text("0", 20, lineTop + lineHeight - 8);
    text("100", 8, barsTop + 1);
    text("50", 14, barsMid - 3);
    text("0", 20, barsTop + barsHeight - 8);

    uint32_t durationUs = 0;
    if (count > 1 && peakRpm) {
      const uint32_t firstTime = samples[0].timestampUs;
      durationUs = samples[count - 1].timestampUs - firstTime;
      if (durationUs == 0) durationUs = 1;
      const auto yFor = [&](uint32_t rpm) {
        return static_cast<int16_t>(lineTop + lineHeight - 2 -
            static_cast<uint64_t>(min(rpm, peakRpm)) * (lineHeight - 3) / peakRpm);
      };
      int16_t previousX = left + 1;
      int16_t previousY = yFor(samples[0].rpm);
      const int16_t pointRadius = count <= 12 ? 2 : 1;
      if (count <= 40) epd.fillCircle(previousX, previousY, pointRadius, BLACK);
      int16_t lastHatchX = -10;
      for (uint16_t i = 1; i < count; ++i) {
        const int16_t x = left + 1 +
            static_cast<uint64_t>(samples[i].timestampUs - firstTime) * (width - 3) / durationUs;
        const int16_t y = yFor(samples[i].rpm);
        epd.drawLine(previousX, previousY, x, y, BLACK);
        if (count <= 40) epd.fillCircle(x, y, pointRadius, BLACK);
        // Sparse fill keeps the curve readable without a solid black block.
        if (x - lastHatchX >= 4) {
          epd.drawFastVLine(x, y, lineTop + lineHeight - 2 - y, BLACK);
          lastHatchX = x;
        }
        previousX = x;
        previousY = y;
      }
      constexpr uint8_t barCount = 18;
      const int16_t barStep = (width - 3) / barCount;
      const int16_t barWidth = barStep > 2 ? barStep - 2 : 1;
      for (uint8_t bar = 0; bar < barCount; ++bar) {
        const uint16_t begin = static_cast<uint32_t>(bar) * count / barCount;
        uint16_t end = static_cast<uint32_t>(bar + 1) * count / barCount;
        if (end <= begin) end = begin + 1;
        if (end > count) end = count;
        uint32_t binPeak = 0;
        for (uint16_t i = begin; i < end; ++i) binPeak = max(binPeak, samples[i].rpm);
        const int16_t barHeight = static_cast<uint64_t>(min(binPeak, peakRpm)) * (barsHeight - 4) / peakRpm;
        if (barHeight > 0) {
          epd.fillRect(left + 2 + bar * barStep, barsTop + barsHeight - 2 - barHeight,
                       barWidth, barHeight, BLACK);
        }
      }
    } else {
      epd.fillRect(left + 20, 130, width - 40, 14, WHITE);
      centered("NO PROFILE DATA", 133);
    }
    char time[20], peak[20];
    snprintf(time, sizeof(time), "TIME %lums", static_cast<unsigned long>(durationUs / 1000));
    snprintf(peak, sizeof(peak), "PEAK %lu", static_cast<unsigned long>(peakRpm));
    footer(time, peak);
  } while (epd.nextPage());
}
}  // namespace

bool Display::begin() {
  SPI.begin(Config::PIN_EPD_SCK, -1, Config::PIN_EPD_MOSI, Config::PIN_EPD_CS);
  // WeAct's official ESP32-C3 example uses a 50 ms reset pulse.
  // Shorter pulses leave some SSD1681 revisions unresponsive.
  epd.init(Config::SERIAL_BAUD, true, 50, false);
  epd.setRotation(0);
  epd.setTextColor(BLACK);
  epd.setTextWrap(false);
  lock_ = xSemaphoreCreateMutex();
  // Core 0: loop() and ADC processing stay on core 1.
  if (lock_ && xTaskCreatePinnedToCore(renderTask, "epd", 8192, this, 1, &task_, 0) != pdPASS) {
    task_ = nullptr;
  }
  available_ = true;
  return true;
}

void Display::renderTask(void* arg) {
  Display* self = static_cast<Display*>(arg);
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    for (;;) {
      std::function<void()> job;
      xSemaphoreTake(self->lock_, portMAX_DELAY);
      job.swap(self->pending_);
      self->drawing_ = static_cast<bool>(job);
      xSemaphoreGive(self->lock_);
      if (!job) break;
      job();
    }
  }
}

void Display::post(std::function<void()> job) {
  if (!task_) {  // No render task: draw synchronously, as before.
    job();
    return;
  }
  xSemaphoreTake(lock_, portMAX_DELAY);
  pending_ = std::move(job);
  xSemaphoreGive(lock_);
  xTaskNotifyGive(task_);
}

void Display::waitIdle() {
  if (!task_) return;
  for (;;) {
    xSemaphoreTake(lock_, portMAX_DELAY);
    const bool idle = !pending_ && !drawing_;
    xSemaphoreGive(lock_);
    if (idle) return;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void Display::hibernate() {
  waitIdle();
  epd.hibernate();
}

void Display::showBoot() { post([] { drawBoot(); }); }

void Display::showCalibrating(uint16_t minimum, uint16_t maximum) {
  post([=] { drawCalibrating(minimum, maximum); });
}

void Display::showInsertBlade(uint32_t bestRpm, uint32_t launchCount) {
  post([=] { drawInsertBlade(bestRpm, launchCount); });
}

void Display::showReady(uint32_t bestRpm, uint32_t launchCount) {
  post([=] { drawReady(bestRpm, launchCount); });
}

void Display::showResult(const LaunchRecord& record, uint32_t bestRpm, bool newBest) {
  post([=] { drawResult(record, bestRpm, newBest); });
}

void Display::showInvalidLaunch() { post([] { drawInvalidLaunch(); }); }

void Display::showSensorError(uint16_t minimum, uint16_t maximum) {
  post([=] { drawSensorError(minimum, maximum); });
}

void Display::showHistory(const LaunchRecord* record, uint8_t position, uint8_t count) {
  const LaunchRecord copy = record ? *record : LaunchRecord{};
  const bool hasRecord = record != nullptr;
  post([=] { drawHistory(hasRecord, copy, position, count); });
}

void Display::showMenu(uint8_t item) { post([=] { drawMenu(item); }); }

void Display::showClearConfirmation() { post([] { drawClearConfirmation(); }); }

void Display::showService(uint16_t raw, uint16_t minimum, uint16_t maximum,
                          uint16_t low, uint16_t high, uint32_t transitions,
                          bool state) {
  post([=] { drawService(raw, minimum, maximum, low, high, transitions, state); });
}

void Display::showGraph(const RpmSample* samples, uint16_t count, uint32_t peakRpm) {
  // Copy: the caller's buffer may be reused by the next launch while drawing.
  std::vector<RpmSample> copy;
  if (samples && count) copy.assign(samples, samples + count);
  post([copy = std::move(copy), peakRpm] { drawGraph(copy, peakRpm); });
}
