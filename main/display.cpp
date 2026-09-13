#include "display.h"

#include <GxEPD2_BW.h>
#include <math.h>
#include <SPI.h>

#include "config.h"
#include "blade_icon.h"
#include "logo_bitmap.h"

using Panel = GxEPD2_154_D67;
static GxEPD2_BW<Panel, Panel::HEIGHT> epd(
    Panel(Config::PIN_EPD_CS, Config::PIN_EPD_DC,
          Config::PIN_EPD_RST, Config::PIN_EPD_BUSY));

bool Display::begin() {
  SPI.begin(Config::PIN_EPD_SCK, -1, Config::PIN_EPD_MOSI, Config::PIN_EPD_CS);
  // WeAct's official ESP32-C3 example uses a 50 ms reset pulse.
  // Shorter pulses leave some SSD1681 revisions unresponsive.
  epd.init(Config::SERIAL_BAUD, true, 50, false);
  epd.setRotation(0);
  epd.setTextColor(GxEPD_BLACK);
  available_ = true;
  return true;
}

void Display::hibernate() {
  epd.hibernate();
}

void Display::startFrame(bool partial) {
  partial_ = partial && partialCount_ < Config::FULL_REFRESH_AFTER_PARTIAL;
  if (partial_) {
    epd.setPartialWindow(0, 0, epd.width(), epd.height());
    ++partialCount_;
  } else {
    epd.setFullWindow();
    partialCount_ = 0;
  }
  epd.firstPage();
}

void Display::finishFrame() {
  while (epd.nextPage()) {}
}

void Display::title(const char* text) {
  epd.setTextSize(1);
  epd.setCursor(8, 15);
  epd.print(text);
  epd.drawLine(5, 21, 194, 21, GxEPD_BLACK);
}

void Display::centered(const char* text, int16_t y, uint8_t size) {
  epd.setTextSize(size);
  int16_t x1, y1;
  uint16_t w, h;
  epd.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  epd.setCursor((200 - static_cast<int16_t>(w)) / 2, y);
  epd.print(text);
}

void Display::showBoot() {
  startFrame();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.drawBitmap(0, 0, BOOT_LOGO_BITMAP, BOOT_LOGO_WIDTH,
                   BOOT_LOGO_HEIGHT, GxEPD_BLACK);
  } while (epd.nextPage());
}

void Display::showCalibrating(uint16_t minimum, uint16_t maximum) {
  startFrame(true);
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("SENSOR CALIBRATION");
    centered("PULL LAUNCHER", 70, 2);
    centered("SMOOTHLY", 98, 2);
    epd.setTextSize(1);
    epd.setCursor(25, 145); epd.printf("MIN %u  MAX %u", minimum, maximum);
  } while (epd.nextPage());
}

void Display::showInsertBlade(uint32_t bestRpm, uint32_t launchCount) {
  startFrame();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("BEY ANALYZER");
    centered("INSERT", 72, 3);
    centered("BLADE", 106, 3);
    epd.setTextSize(1); epd.setCursor(16, 177);
    epd.printf("BEST %lu  #%lu", static_cast<unsigned long>(bestRpm),
               static_cast<unsigned long>(launchCount + 1));
  } while (epd.nextPage());
}

void Display::showReady(uint32_t bestRpm, uint32_t launchCount) {
  startFrame();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("BEY ANALYZER");
    epd.drawBitmap(155, 28, BLADE_ICON_BITMAP, BLADE_ICON_WIDTH,
                   BLADE_ICON_HEIGHT, GxEPD_BLACK);
    centered("READY", 78, 3);
    centered("PULL TO START", 122, 1);
    epd.setTextSize(1); epd.setCursor(16, 177);
    epd.printf("BEST %lu  #%lu", static_cast<unsigned long>(bestRpm),
               static_cast<unsigned long>(launchCount + 1));
  } while (epd.nextPage());
}

void Display::showResult(const LaunchRecord& record, uint32_t bestRpm,
                         bool newBest) {
  char rpm[12];
  snprintf(rpm, sizeof(rpm), "%lu", static_cast<unsigned long>(record.peakRpm));
  startFrame();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);

    // Inverted status header.
    epd.fillRect(4, 4, 192, 20, GxEPD_BLACK);
    epd.setTextColor(GxEPD_WHITE);
    epd.setTextSize(1);
    epd.setCursor(9, 17);
    epd.printf("LAUNCH %03lu", static_cast<unsigned long>(record.timestamp));
    epd.setCursor(newBest ? 143 : 158, 17);
    epd.print(newBest ? "NEW BEST" : "RESULT");
    epd.setTextColor(GxEPD_BLACK);

    // Retro tachometer arc from 0 to 15K RPM.
    constexpr int16_t gaugeX = 100;
    constexpr int16_t gaugeY = 126;
    constexpr int16_t gaugeRadius = 68;
    constexpr uint32_t gaugeMaxRpm = 15000;
    int16_t previousX = gaugeX - gaugeRadius;
    int16_t previousY = gaugeY;
    for (int16_t degrees = 185; degrees <= 355; degrees += 5) {
      const float radians = degrees * PI / 180.0f;
      const int16_t x = gaugeX + cosf(radians) * gaugeRadius;
      const int16_t y = gaugeY + sinf(radians) * gaugeRadius;
      epd.drawLine(previousX, previousY, x, y, GxEPD_BLACK);
      previousX = x;
      previousY = y;
    }
    epd.drawLine(previousX, previousY, gaugeX + gaugeRadius, gaugeY,
                 GxEPD_BLACK);
    for (uint8_t tick = 0; tick <= 10; ++tick) {
      const float radians = (180.0f + tick * 18.0f) * PI / 180.0f;
      const int16_t innerRadius = tick % 5 == 0 ? 57 : 62;
      epd.drawLine(gaugeX + cosf(radians) * innerRadius,
                   gaugeY + sinf(radians) * innerRadius,
                   gaugeX + cosf(radians) * gaugeRadius,
                   gaugeY + sinf(radians) * gaugeRadius, GxEPD_BLACK);
    }

    const uint32_t gaugeRpm = min<uint32_t>(record.peakRpm, gaugeMaxRpm);
    const float markerRadians =
        (180.0f + 180.0f * gaugeRpm / gaugeMaxRpm) * PI / 180.0f;
    const int16_t markerX = gaugeX + cosf(markerRadians) * 63;
    const int16_t markerY = gaugeY + sinf(markerRadians) * 63;
    epd.fillCircle(markerX, markerY, 3, GxEPD_BLACK);

    centered(rpm, 73, 4);
    centered("RPM", 109, 1);
    epd.setTextSize(1);
    epd.setCursor(25, 132); epd.print("0");
    epd.setCursor(157, 132); epd.print("15K");

    // Ten-segment launch power bar, saturated at the gauge maximum.
    constexpr int16_t barLeft = 15;
    constexpr int16_t barTop = 143;
    constexpr int16_t segmentStep = 17;
    const uint8_t filledSegments = min<uint32_t>(
        10, (gaugeRpm * 10 + gaugeMaxRpm - 1) / gaugeMaxRpm);
    for (uint8_t segment = 0; segment < 10; ++segment) {
      const int16_t x = barLeft + segment * segmentStep;
      epd.drawRect(x, barTop, 14, 13, GxEPD_BLACK);
      if (segment < filledSegments) {
        epd.fillRect(x + 2, barTop + 2, 10, 9, GxEPD_BLACK);
      }
    }

    epd.setCursor(9, 171);
    epd.printf("AVG %lu", static_cast<unsigned long>(record.averageRpm));
    epd.setCursor(116, 171);
    epd.printf("%lums", static_cast<unsigned long>(record.durationMs));
    epd.setCursor(9, 188);
    epd.printf("BEST %lu", static_cast<unsigned long>(bestRpm));
  } while (epd.nextPage());
}

void Display::showInvalidLaunch() {
  startFrame();
  do {
    epd.fillScreen(GxEPD_WHITE); epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("LAUNCH REJECTED"); centered("NO VALID DATA", 92, 2);
    centered("TRY AGAIN", 128, 1);
  } while (epd.nextPage());
}

void Display::showSensorError(uint16_t minimum, uint16_t maximum) {
  startFrame();
  do {
    epd.fillScreen(GxEPD_WHITE); epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("SENSOR ALIGN"); centered("RANGE TOO LOW", 78, 2);
    epd.setTextSize(1); epd.setCursor(31, 120);
    epd.printf("MIN %u MAX %u", minimum, maximum);
    centered("CENTER: RETRY", 162, 1);
  } while (epd.nextPage());
}

void Display::showHistory(const LaunchRecord* record, uint8_t position, uint8_t count) {
  startFrame(true);
  do {
    epd.fillScreen(GxEPD_WHITE); epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("LAUNCH HISTORY");
    if (!record) {
      centered("EMPTY", 98, 2);
    } else {
      epd.setTextSize(2); epd.setCursor(20, 65);
      epd.printf("#%03lu", static_cast<unsigned long>(record->timestamp));
      epd.setTextSize(3); epd.setCursor(20, 108);
      epd.printf("%lu", static_cast<unsigned long>(record->peakRpm));
      epd.setTextSize(1); epd.setCursor(20, 133); epd.print("RPM PEAK");
      epd.setCursor(20, 158);
      epd.printf("AVG %lu  %lums", static_cast<unsigned long>(record->averageRpm),
                 static_cast<unsigned long>(record->durationMs));
      epd.setCursor(75, 184); epd.printf("%u/%u", position + 1, count);
    }
  } while (epd.nextPage());
}

void Display::showMenu(uint8_t item) {
  static const char* const items[] = {"HISTORY", "LAST GRAPH", "CALIBRATE", "CLEAR HISTORY", "BACK"};
  startFrame(true);
  do {
    epd.fillScreen(GxEPD_WHITE); epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("MENU");
    for (uint8_t i = 0; i < 5; ++i) {
      const int16_t y = 50 + i * 29;
      if (i == item) { epd.fillRect(10, y - 15, 180, 22, GxEPD_BLACK); epd.setTextColor(GxEPD_WHITE); }
      epd.setTextSize(1); epd.setCursor(18, y); epd.print(items[i]);
      epd.setTextColor(GxEPD_BLACK);
    }
    epd.setTextSize(1); epd.setCursor(32, 190);
    epd.print("2X SELECT  HOLD BACK");
  } while (epd.nextPage());
}

void Display::showClearConfirmation() {
  startFrame(true);
  do {
    epd.fillScreen(GxEPD_WHITE); epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("CLEAR HISTORY?");
    centered("DOUBLE: YES", 86, 2);
    centered("CLICK: NO", 128, 1);
    centered("CANNOT UNDO", 166, 1);
  } while (epd.nextPage());
}

void Display::showService(uint16_t raw, uint16_t minimum, uint16_t maximum,
                          uint16_t low, uint16_t high, uint32_t transitions,
                          bool state) {
  startFrame(true);
  do {
    epd.fillScreen(GxEPD_WHITE); epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("SERVICE"); epd.setTextSize(1);
    epd.setCursor(15, 48); epd.printf("RAW       %u", raw);
    epd.setCursor(15, 70); epd.printf("MIN/MAX   %u/%u", minimum, maximum);
    epd.setCursor(15, 92); epd.printf("LOW/HIGH  %u/%u", low, high);
    epd.setCursor(15, 114); epd.printf("RANGE     %u", maximum - minimum);
    epd.setCursor(15, 136); epd.printf("EDGES     %lu", static_cast<unsigned long>(transitions));
    epd.setCursor(15, 158); epd.printf("STATE     %s", state ? "HIGH" : "LOW");
    epd.setCursor(15, 178); epd.print("CLICK: REFRESH");
    epd.setCursor(15, 190); epd.print("HOLD: EXIT");
  } while (epd.nextPage());
}

void Display::showGraph(const RpmSample* samples, uint16_t count, uint32_t peakRpm) {
  startFrame();
  do {
    epd.fillScreen(GxEPD_WHITE); epd.drawRect(3, 3, 194, 194, GxEPD_BLACK);
    title("LAUNCH PROFILE");

    const int16_t left = 27;
    const int16_t width = 163;
    const int16_t lineTop = 29;
    const int16_t lineHeight = 56;
    const int16_t barsTop = 94;
    const int16_t barsHeight = 67;
    const int16_t right = left + width - 1;

    epd.drawRect(left, lineTop, width, lineHeight, GxEPD_BLACK);
    epd.drawRect(left, barsTop, width, barsHeight, GxEPD_BLACK);

    // Dotted oscilloscope-style grid.
    for (uint8_t division = 1; division < 4; ++division) {
      const int16_t x = left + division * (width - 1) / 4;
      for (int16_t y = lineTop + 2; y < lineTop + lineHeight - 1; y += 4) {
        epd.drawPixel(x, y, GxEPD_BLACK);
      }
      for (int16_t y = barsTop + 2; y < barsTop + barsHeight - 1; y += 4) {
        epd.drawPixel(x, y, GxEPD_BLACK);
      }
    }
    for (uint8_t division = 1; division < 2; ++division) {
      const int16_t lineY = lineTop + division * (lineHeight - 1) / 2;
      const int16_t barsY = barsTop + division * (barsHeight - 1) / 2;
      for (int16_t x = left + 2; x < right; x += 4) {
        epd.drawPixel(x, lineY, GxEPD_BLACK);
        epd.drawPixel(x, barsY, GxEPD_BLACK);
      }
    }

    epd.setTextSize(1);
    epd.setCursor(5, 36); epd.print("100");
    epd.setCursor(11, 61); epd.print("50");
    epd.setCursor(17, 84); epd.print("0");
    epd.setCursor(5, 102); epd.print("100");
    epd.setCursor(11, 132); epd.print("50");
    epd.setCursor(17, 160); epd.print("0");

    uint32_t durationUs = 0;
    if (samples && count > 1 && peakRpm) {
      const uint32_t firstTime = samples[0].timestampUs;
      durationUs = samples[count - 1].timestampUs - firstTime;
      if (durationUs == 0) durationUs = 1;

      const uint32_t firstRpm = samples[0].rpm > peakRpm
                                    ? peakRpm : samples[0].rpm;
      int16_t previousX = left + 1;
      int16_t previousY = lineTop + lineHeight - 2 -
          static_cast<uint64_t>(firstRpm) * (lineHeight - 3) / peakRpm;
      const int16_t pointRadius = count <= 12 ? 2 : 1;
      if (count <= 40) {
        epd.fillCircle(previousX, previousY, pointRadius, GxEPD_BLACK);
      }
      int16_t lastHatchX = -10;
      for (uint16_t i = 1; i < count; ++i) {
        const int16_t x = left + 1 +
            static_cast<uint64_t>(samples[i].timestampUs - firstTime) *
            (width - 3) / durationUs;
        const uint32_t rpm = samples[i].rpm > peakRpm
                                 ? peakRpm : samples[i].rpm;
        const int16_t y = lineTop + lineHeight - 2 -
            static_cast<uint64_t>(rpm) * (lineHeight - 3) / peakRpm;
        epd.drawLine(previousX, previousY, x, y, GxEPD_BLACK);
        if (count <= 40) epd.fillCircle(x, y, pointRadius, GxEPD_BLACK);
        // Sparse fill makes the curve readable without creating a large black block.
        if (x - lastHatchX >= 4) {
          epd.drawFastVLine(x, y, lineTop + lineHeight - 2 - y, GxEPD_BLACK);
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
        for (uint16_t i = begin; i < end; ++i) {
          if (samples[i].rpm > binPeak) binPeak = samples[i].rpm;
        }
        if (binPeak > peakRpm) binPeak = peakRpm;
        const int16_t barHeight = static_cast<uint64_t>(binPeak) *
            (barsHeight - 4) / peakRpm;
        if (barHeight > 0) {
          epd.fillRect(left + 2 + bar * barStep,
                       barsTop + barsHeight - 2 - barHeight,
                       barWidth, barHeight, GxEPD_BLACK);
        }
      }
    } else {
      centered("NO PROFILE DATA", 127, 1);
    }

    epd.setTextSize(1);
    epd.setCursor(7, 174);
    epd.printf("TIME %lums", static_cast<unsigned long>(durationUs / 1000));
    epd.setCursor(96, 174);
    epd.printf("PEAK %lu", static_cast<unsigned long>(peakRpm));
    epd.setCursor(67, 189); epd.print("RPM / TIME");
  } while (epd.nextPage());
}
