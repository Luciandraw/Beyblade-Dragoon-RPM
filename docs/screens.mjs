// Pixel-exact JavaScript mirror of main/display.cpp for the screen emulator.
// The primitives follow Adafruit GFX (classic 5x7 font, Bresenham lines, circle
// helpers, round rects, bitmaps); each draw* function copies its C++ twin line
// for line. Keep both files in step when changing a layout.
import {FONT, FONT_FIRST, BOOT_LOGO, BLADE_ICON} from './screen-assets.mjs';

export const WIDTH = 200, HEIGHT = 200;
const BLACK = 1, WHITE = 0;
const FULL_REFRESH_AFTER_PARTIAL = 6;
const decode = text => Uint8Array.from(atob(text), c => c.charCodeAt(0));
const font = decode(FONT), logo = decode(BOOT_LOGO.data), blade = decode(BLADE_ICON.data);

// C/C++ helpers: integer division truncates; lroundf rounds half away from zero.
const div = (a, b) => Math.trunc(a / b);
const f = Math.fround;
const lround = value => value < 0 ? -Math.round(-value) : Math.round(value);
const cosf = radians => f(Math.cos(f(radians))), sinf = radians => f(Math.sin(f(radians)));
const pad = (value, width) => String(value).padStart(width, '0');

export class Canvas {
  constructor() { this.pixels = new Uint8Array(WIDTH * HEIGHT); this.size = 1; this.color = BLACK; this.x = 0; this.y = 0; }
  drawPixel(x, y, c) { if (x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT) this.pixels[y * WIDTH + x] = c; }
  fillScreen(c) { this.pixels.fill(c); }
  drawFastVLine(x, y, h, c) { for (let i = 0; i < h; ++i) this.drawPixel(x, y + i, c); }
  drawFastHLine(x, y, w, c) { for (let i = 0; i < w; ++i) this.drawPixel(x + i, y, c); }
  fillRect(x, y, w, h, c) { for (let i = x; i < x + w; ++i) this.drawFastVLine(i, y, h, c); }
  drawRect(x, y, w, h, c) {
    this.drawFastHLine(x, y, w, c); this.drawFastHLine(x, y + h - 1, w, c);
    this.drawFastVLine(x, y, h, c); this.drawFastVLine(x + w - 1, y, h, c);
  }
  drawLine(x0, y0, x1, y1, c) {
    const steep = Math.abs(y1 - y0) > Math.abs(x1 - x0);
    if (steep) { [x0, y0] = [y0, x0]; [x1, y1] = [y1, x1]; }
    if (x0 > x1) { [x0, x1] = [x1, x0]; [y0, y1] = [y1, y0]; }
    const dx = x1 - x0, dy = Math.abs(y1 - y0), ystep = y0 < y1 ? 1 : -1;
    let err = div(dx, 2);
    for (; x0 <= x1; ++x0) {
      if (steep) this.drawPixel(y0, x0, c); else this.drawPixel(x0, y0, c);
      err -= dy;
      if (err < 0) { y0 += ystep; err += dx; }
    }
  }
  circleHelper(x0, y0, r, corner, c) {
    let f1 = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
    while (x < y) {
      if (f1 >= 0) { --y; ddy += 2; f1 += ddy; }
      ++x; ddx += 2; f1 += ddx;
      if (corner & 4) { this.drawPixel(x0 + x, y0 + y, c); this.drawPixel(x0 + y, y0 + x, c); }
      if (corner & 2) { this.drawPixel(x0 + x, y0 - y, c); this.drawPixel(x0 + y, y0 - x, c); }
      if (corner & 8) { this.drawPixel(x0 - y, y0 + x, c); this.drawPixel(x0 - x, y0 + y, c); }
      if (corner & 1) { this.drawPixel(x0 - y, y0 - x, c); this.drawPixel(x0 - x, y0 - y, c); }
    }
  }
  fillCircleHelper(x0, y0, r, corners, delta, c) {
    let f1 = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r, px = x, py = y;
    ++delta;
    while (x < y) {
      if (f1 >= 0) { --y; ddy += 2; f1 += ddy; }
      ++x; ddx += 2; f1 += ddx;
      if (x < y + 1) {
        if (corners & 1) this.drawFastVLine(x0 + x, y0 - y, 2 * y + delta, c);
        if (corners & 2) this.drawFastVLine(x0 - x, y0 - y, 2 * y + delta, c);
      }
      if (y !== py) {
        if (corners & 1) this.drawFastVLine(x0 + py, y0 - px, 2 * px + delta, c);
        if (corners & 2) this.drawFastVLine(x0 - py, y0 - px, 2 * px + delta, c);
        py = y;
      }
      px = x;
    }
  }
  fillCircle(x0, y0, r, c) { this.drawFastVLine(x0, y0 - r, 2 * r + 1, c); this.fillCircleHelper(x0, y0, r, 3, 0, c); }
  fillRoundRect(x, y, w, h, r, c) {
    r = Math.min(r, div(w < h ? w : h, 2));
    this.fillRect(x + r, y, w - 2 * r, h, c);
    this.fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, c);
    this.fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, c);
  }
  drawRoundRect(x, y, w, h, r, c) {
    r = Math.min(r, div(w < h ? w : h, 2));
    this.drawFastHLine(x + r, y, w - 2 * r, c); this.drawFastHLine(x + r, y + h - 1, w - 2 * r, c);
    this.drawFastVLine(x, y + r, h - 2 * r, c); this.drawFastVLine(x + w - 1, y + r, h - 2 * r, c);
    this.circleHelper(x + r, y + r, r, 1, c); this.circleHelper(x + w - r - 1, y + r, r, 2, c);
    this.circleHelper(x + w - r - 1, y + h - r - 1, r, 4, c); this.circleHelper(x + r, y + h - r - 1, r, 8, c);
  }
  drawBitmap(x, y, bitmap, w, h, c) {
    const byteWidth = (w + 7) >> 3;
    let byte = 0;
    for (let j = 0; j < h; ++j) for (let i = 0; i < w; ++i) {
      if (i & 7) byte <<= 1; else byte = bitmap[j * byteWidth + (i >> 3)];
      if (byte & 0x80) this.drawPixel(x + i, y + j, c);
    }
  }
  drawChar(x, y, ch, c, size) {
    const code = ch.charCodeAt(0);
    if (code < FONT_FIRST || code > 126) return;
    for (let i = 0; i < 5; ++i) {
      let line = font[(code - FONT_FIRST) * 5 + i];
      for (let j = 0; j < 8; ++j, line >>= 1) {
        if (line & 1) size === 1 ? this.drawPixel(x + i, y + j, c) : this.fillRect(x + i * size, y + j * size, size, size, c);
      }
    }
  }
  print(text) { for (const ch of text) { this.drawChar(this.x, this.y, ch, this.color, this.size); this.x += 6 * this.size; } }
}

// ---- Helpers (display.cpp, anonymous namespace) ----
let g, partialCount = 0;
const textWidth = (value, size) => value.length * 6 * size;
function text(value, x, y, size = 1, color = BLACK) { g.size = size; g.color = color; g.x = x; g.y = y; g.print(value); }
const centered = (value, y, size = 1, color = BLACK) => text(value, div(200 - textWidth(value, size), 2), y, size, color);
const rightAligned = (value, right, y, size = 1, color = BLACK) => text(value, right - textWidth(value, size), y, size, color);
function ground() {
  g.fillScreen(WHITE);
  for (let y = 0; y < 200; ++y) for (let x = 0; x < 200; ++x) {
    if ((x % 8 === 1 && y % 8 === 2) || (x % 8 === 5 && y % 8 === 6)) g.drawPixel(x, y, BLACK);
  }
}
function frame(x, y, w, h) {
  g.fillRoundRect(x, y, w, h, 4, BLACK);
  g.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 3, WHITE);
  g.drawRoundRect(x + 4, y + 4, w - 8, h - 8, 2, BLACK);
}
function cursor(x, y, size = 1, color = BLACK) { for (let i = 0; i < 4 * size; ++i) g.drawFastVLine(x + i, y + i, 8 * size - 1 - 2 * i, color); }
function prompt(x, y) { for (let i = 0; i < 4; ++i) g.drawFastHLine(x + i, y + i, 7 - 2 * i, BLACK); }
function header(title, right) {
  g.fillRect(0, 0, 200, 20, BLACK); g.fillRect(4, 3, 14, 14, WHITE);
  text('R', 8, 6, 1, BLACK); text(title, 24, 6, 1, WHITE);
  if (right) rightAligned(right, 195, 6, 1, WHITE);
}
function footer(left, right) {
  g.fillRect(0, 184, 200, 16, BLACK); text(left, 6, 188, 1, WHITE);
  if (right) rightAligned(right, 195, 188, 1, WHITE);
}
const statsFooter = (bestRpm, launchCount) => footer(`BEST ${bestRpm}`, `#${pad(launchCount + 1, 3)}`);
const drawBlade = (x, y) => g.drawBitmap(x, y, blade, BLADE_ICON.width, BLADE_ICON.height, BLACK);

// Returns whether the panel would use a partial refresh (no full-screen flash).
function startFrame(partial = false) {
  if (partial && partialCount < FULL_REFRESH_AFTER_PARTIAL) { ++partialCount; return true; }
  partialCount = 0; return false;
}

// ---- Screens ----
const screens = {
  boot() {
    const partial = startFrame();
    g.fillScreen(WHITE); g.drawBitmap(0, 0, logo, BOOT_LOGO.width, BOOT_LOGO.height, BLACK);
    return partial;
  },
  calibrating({minimum = 4095, maximum = 0} = {}) {
    const partial = startFrame(true);
    ground(); header('CALIBRATION');
    frame(4, 26, 192, 98);
    centered('PULL', 42, 3); centered('SMOOTHLY', 74, 2); centered('FOR 3 SECONDS', 100, 1);
    frame(4, 128, 192, 52);
    text(`MIN ${minimum}`, 16, 142); text(`MAX ${maximum}`, 16, 158);
    prompt(178, 164); footer('SENSOR SETUP');
    return partial;
  },
  insertBlade({bestRpm = 0, launchCount = 0} = {}) {
    const partial = startFrame();
    ground(); header('BEYBLADE RPM');
    frame(60, 28, 80, 80); drawBlade(81, 49);
    frame(4, 114, 192, 66);
    text('INSERT', 16, 128, 2); text('BLADE', 16, 148, 2);
    text('TO ARM', 112, 134); text('LAUNCHER', 112, 148);
    prompt(178, 164); statsFooter(bestRpm, launchCount);
    return partial;
  },
  ready({bestRpm = 0, launchCount = 0} = {}) {
    const partial = startFrame();
    ground(); header('BEYBLADE RPM', 'ARMED');
    frame(4, 26, 192, 84); drawBlade(16, 49); text('READY', 64, 52, 4);
    frame(4, 114, 192, 66);
    cursor(14, 130, 1); text('PULL THE', 26, 130, 1); text('STRING!', 26, 146, 2);
    prompt(178, 164); statsFooter(bestRpm, launchCount);
    return partial;
  },
  result({record, bestRpm = 0, newBest = false}) {
    const partial = startFrame();
    ground(); header(`LAUNCH ${pad(record.timestamp, 3)}`, newBest ? 'NEW BEST!' : 'RESULT');
    frame(4, 24, 192, 112);
    const gaugeX = 100, gaugeY = 126, gaugeRadius = 78, gaugeMaxRpm = 15000;
    let previousX = gaugeX - gaugeRadius, previousY = gaugeY;
    for (let degrees = 185; degrees <= 360; degrees += 5) {
      const radians = degrees * Math.PI / 180;
      const x = gaugeX + lround(f(cosf(radians) * gaugeRadius)), y = gaugeY + lround(f(sinf(radians) * gaugeRadius));
      g.drawLine(previousX, previousY, x, y, BLACK); previousX = x; previousY = y;
    }
    for (let tick = 0; tick <= 10; ++tick) {
      const radians = f(180 + f(tick * 18)) * Math.PI / 180, inner = tick % 5 === 0 ? 66 : 71;
      g.drawLine(gaugeX + lround(f(cosf(radians) * inner)), gaugeY + lround(f(sinf(radians) * inner)),
                 gaugeX + lround(f(cosf(radians) * gaugeRadius)), gaugeY + lround(f(sinf(radians) * gaugeRadius)), BLACK);
    }
    const gaugeRpm = Math.min(record.peakRpm, gaugeMaxRpm);
    const marker = f(180 + f(f(180 * gaugeRpm) / gaugeMaxRpm)) * Math.PI / 180;
    g.fillCircle(gaugeX + lround(f(cosf(marker) * 72)), gaugeY + lround(f(sinf(marker) * 72)), 3, BLACK);
    centered(String(record.peakRpm), 80, 4); centered('RPM', 112, 1);
    text('0', 26, 118); text('15K', 156, 118);
    frame(4, 138, 192, 46);
    text('PWR', 12, 150); g.drawRoundRect(34, 149, 154, 9, 3, BLACK);
    const fill = div(150 * gaugeRpm, gaugeMaxRpm);
    if (fill > 0) g.fillRect(36, 151, fill, 5, BLACK);
    text(`AVG ${record.averageRpm}`, 12, 166); rightAligned(`${record.durationMs}ms`, 188, 166);
    footer(`BEST ${bestRpm}`, 'CLICK: OK');
    return partial;
  },
  invalidLaunch() {
    const partial = startFrame();
    ground(); header('LAUNCH');
    frame(4, 40, 192, 100);
    centered('NO VALID', 62, 2); centered('DATA', 84, 2); centered('TRY AGAIN', 114, 1);
    footer('REJECTED');
    return partial;
  },
  sensorError({minimum = 2100, maximum = 2250} = {}) {
    const partial = startFrame();
    ground(); header('SENSOR', 'ERROR');
    frame(4, 26, 192, 98);
    centered('RANGE', 42, 2); centered('TOO LOW', 62, 2);
    centered(`MIN ${minimum}  MAX ${maximum}`, 88); centered('CHECK DISC + SENSOR', 104);
    frame(4, 128, 192, 52); cursor(14, 146, 1); text('CLICK: RETRY', 26, 146);
    footer('SENSOR ALIGN');
    return partial;
  },
  history({record = null, position = 0, count = 0} = {}) {
    const partial = startFrame(true);
    ground(); header('HISTORY', `${record ? position + 1 : 0}/${count}`);
    frame(4, 26, 192, 112);
    if (!record) { centered('NO LAUNCHES', 66, 2); centered('YET', 88, 2); }
    else {
      text(`#${pad(record.timestamp, 3)}`, 16, 38, 2); drawBlade(146, 36);
      text(String(record.peakRpm), 16, 74, 4); text('RPM PEAK', 16, 112);
    }
    frame(4, 140, 192, 44);
    if (record) { text(`AVG ${record.averageRpm}`, 14, 152); rightAligned(`${record.durationMs}ms`, 186, 152); }
    text('CLICK NEXT', 14, 166); rightAligned('HOLD BACK', 186, 166);
    footer('LAUNCH LOG');
    return partial;
  },
  menu({item = 0} = {}) {
    const items = ['HISTORY', 'GRAPH', 'CALIBRATE', 'CLEAR LOG', 'BACK'];
    const partial = startFrame(true);
    ground(); header('MENU'); frame(4, 26, 192, 140);
    items.forEach((label, i) => { const y = 38 + i * 25; if (i === item) cursor(16, y, 2); text(label, 34, y, 2); });
    footer('2X SELECT', 'HOLD BACK');
    return partial;
  },
  clearConfirmation() {
    const partial = startFrame(true);
    ground(); header('CLEAR LOG?');
    frame(4, 26, 192, 92);
    centered('ERASE ALL', 42, 2); centered('LAUNCHES?', 62, 2); centered('CANNOT BE UNDONE', 92);
    frame(100, 120, 96, 62); cursor(112, 134, 2); text('YES', 130, 134, 2); text('NO', 130, 156, 2);
    frame(4, 120, 94, 62);
    text('DOUBLE', 14, 134); text('= YES', 14, 144); text('CLICK', 14, 158); text('= NO', 14, 168);
    footer('HISTORY ONLY', 'KEEPS CAL');
    return partial;
  },
  service({raw = 3612, minimum = 2000, maximum = 4095, low = 3548, high = 3730, transitions = 0, state = false} = {}) {
    const partial = startFrame(true);
    ground(); header('SERVICE', state ? 'HIGH' : 'LOW'); frame(4, 26, 192, 134);
    const labels = ['RAW', 'MIN/MAX', 'LOW/HIGH', 'RANGE', 'EDGES'];
    const values = [`${raw}`, `${minimum}/${maximum}`, `${low}/${high}`, `${maximum - minimum}`, `${transitions}`];
    for (let i = 0; i < 5; ++i) {
      const y = 40 + i * 22;
      text(labels[i], 14, y); rightAligned(values[i], 186, y);
      if (i < 4) for (let x = 14; x < 186; x += 3) g.drawPixel(x, y + 13, BLACK);
    }
    frame(4, 162, 192, 22); centered('CLICK REFRESH  HOLD EXIT', 169);
    footer('SENSOR DIAGNOSTICS');
    return partial;
  },
  graph({samples = [], peakRpm = 0} = {}) {
    const count = samples.length;
    const partial = startFrame();
    ground(); header('LAUNCH PROFILE'); frame(0, 22, 200, 160);
    const left = 27, width = 163, lineTop = 32, lineHeight = 60, barsTop = 102, barsHeight = 70, right = left + width - 1;
    g.drawRect(left, lineTop, width, lineHeight, BLACK); g.drawRect(left, barsTop, width, barsHeight, BLACK);
    for (let division = 1; division < 4; ++division) {
      const x = left + div(division * (width - 1), 4);
      for (let y = lineTop + 2; y < lineTop + lineHeight - 1; y += 4) g.drawPixel(x, y, BLACK);
      for (let y = barsTop + 2; y < barsTop + barsHeight - 1; y += 4) g.drawPixel(x, y, BLACK);
    }
    const lineMid = lineTop + div(lineHeight - 1, 2), barsMid = barsTop + div(barsHeight - 1, 2);
    for (let x = left + 2; x < right; x += 4) { g.drawPixel(x, lineMid, BLACK); g.drawPixel(x, barsMid, BLACK); }
    text('100', 8, lineTop + 1); text('50', 14, lineMid - 3); text('0', 20, lineTop + lineHeight - 8);
    text('100', 8, barsTop + 1); text('50', 14, barsMid - 3); text('0', 20, barsTop + barsHeight - 8);
    let durationUs = 0;
    if (count > 1 && peakRpm) {
      const firstTime = samples[0].timestampUs;
      durationUs = samples[count - 1].timestampUs - firstTime || 1;
      const yFor = rpm => lineTop + lineHeight - 2 - div(Math.min(rpm, peakRpm) * (lineHeight - 3), peakRpm);
      let previousX = left + 1, previousY = yFor(samples[0].rpm);
      const pointRadius = count <= 12 ? 2 : 1;
      if (count <= 40) g.fillCircle(previousX, previousY, pointRadius, BLACK);
      let lastHatchX = -10;
      for (let i = 1; i < count; ++i) {
        const x = left + 1 + div((samples[i].timestampUs - firstTime) * (width - 3), durationUs), y = yFor(samples[i].rpm);
        g.drawLine(previousX, previousY, x, y, BLACK);
        if (count <= 40) g.fillCircle(x, y, pointRadius, BLACK);
        if (x - lastHatchX >= 4) { g.drawFastVLine(x, y, lineTop + lineHeight - 2 - y, BLACK); lastHatchX = x; }
        previousX = x; previousY = y;
      }
      const barCount = 18, barStep = div(width - 3, barCount), barWidth = barStep > 2 ? barStep - 2 : 1;
      for (let bar = 0; bar < barCount; ++bar) {
        const begin = div(bar * count, barCount);
        let end = div((bar + 1) * count, barCount);
        if (end <= begin) end = begin + 1;
        if (end > count) end = count;
        let binPeak = 0;
        for (let i = begin; i < end; ++i) binPeak = Math.max(binPeak, samples[i].rpm);
        const barHeight = div(Math.min(binPeak, peakRpm) * (barsHeight - 4), peakRpm);
        if (barHeight > 0) g.fillRect(left + 2 + bar * barStep, barsTop + barsHeight - 2 - barHeight, barWidth, barHeight, BLACK);
      }
    } else {
      g.fillRect(left + 20, 130, width - 40, 14, WHITE);
      centered('NO PROFILE DATA', 133);
    }
    footer(`TIME ${div(durationUs, 1000)}ms`, `PEAK ${peakRpm}`);
    return partial;
  }
};
export const SCREEN_NAMES = Object.keys(screens);

// Draws a screen into a fresh canvas. Returns {canvas, partial}.
export function render(name, data) {
  g = new Canvas();
  const partial = screens[name](data);
  return {canvas: g, partial};
}
