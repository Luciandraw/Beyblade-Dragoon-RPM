// Screen emulator: a small copy of the state machine in main/main.ino driving
// the pixel-exact screen renderer. Launch data is simulated with the same
// rules as rpm_calculator.cpp (one period per revolution, peak = second-fastest
// revolution, average = revolutions / time).
import {render, WIDTH, HEIGHT} from './screens.mjs';

const PAPER = [227, 228, 220], INK = [31, 32, 28];
const RESULT_SCREEN_MS = 5000, BOOT_MS = 700, CALIBRATION_MS = 3000, SLEEP_AFTER_MS = 60000;

function paint(canvas, frame) {
  const context = canvas.getContext('2d'), image = context.createImageData(WIDTH, HEIGHT);
  frame.pixels.forEach((value, i) => image.data.set([...(value ? INK : PAPER), 255], i * 4));
  context.putImageData(image, 0, 0);
}

// ---- Simulated launch, following rpm_calculator.cpp ----
function simulateLaunch(targetPeak) {
  const periods = [];
  let time = 0, rpm = targetPeak * .55;
  // Pull: quick spin-up, then the bey leaves and the disc coasts down.
  for (let turn = 0; turn < 400; ++turn) {
    rpm = turn < 6 ? rpm + (targetPeak - rpm) * .55 : rpm * .965 - 30;
    if (rpm < 600) break;
    const jitter = 1 + (Math.random() - .5) * .01;
    const period = Math.round(60e6 / (rpm * jitter));
    time += period; periods.push({period, time});
  }
  const measured = periods.map(p => Math.floor(60e6 / p.period));
  const fastest = [...measured].sort((a, b) => b - a);
  const peakRpm = measured.length >= 3 ? fastest[1] : fastest[0];
  const total = periods.reduce((sum, p) => sum + p.period, 0);
  const samples = [];
  const window = [];
  for (const {period, time: t} of periods) {
    window.push(period); if (window.length > 5) window.shift();
    const sorted = [...window].sort((a, b) => a - b);
    const filtered = window.length < 3 ? period : sorted[Math.floor(sorted.length / 2)];
    samples.push({timestampUs: t, rpm: Math.floor(60e6 / filtered)});
  }
  // The firmware keeps at most 192 profile points, halving resolution when full.
  const step = Math.ceil(samples.length / 192);
  const profile = samples.filter((_, i) => i % step === 0);
  return {peakRpm, averageRpm: Math.floor(60e6 * periods.length / total), durationMs: Math.floor(time / 1000), samples: profile};
}

// ---- Device state (mirrors main.ino) ----
const device = {
  state: 'Boot', enteredAt: performance.now(), bladePresent: false, bladeArmed: false,
  history: [], best: 0, counter: 0, menuItem: 0, historyOffset: 0,
  historyView: false, graphView: false, clearConfirmation: false, serviceView: false,
  lastResult: null, lastProfile: [], sleeping: false, lastActivity: performance.now(),
  calibration: {minimum: 2000, maximum: 4095}
};
const screen = document.querySelector('#epd'), panel = screen.parentElement;
const statusLine = document.querySelector('#emulator-status');
let current = null;

function show(name, data, note) {
  current = {name, data};
  const {canvas, partial} = render(name, data);
  paint(screen, canvas);
  if (!partial) { panel.classList.remove('refresh'); void panel.offsetWidth; panel.classList.add('refresh'); }
  if (note) statusLine.textContent = note;
}
const setState = state => { device.state = state; device.enteredAt = performance.now(); };
const stats = () => ({bestRpm: device.best, launchCount: device.counter});

function enterReady() {
  if (device.bladePresent) device.bladeArmed = true;
  setState('Ready');
  if (device.bladeArmed) show('ready', stats(), 'READY · blade armed. Pull the launcher.');
  else show('insertBlade', stats(), 'Insert the blade to arm the launcher. Sensor is off to save power.');
}
function showMenu() {
  device.historyView = device.graphView = device.clearConfirmation = false;
  show('menu', {item: device.menuItem}, 'Menu: click moves, double-click selects, hold goes back.');
}
function openHistory() {
  device.historyView = true;
  const count = device.history.length;
  device.historyOffset = Math.min(device.historyOffset, count ? count - 1 : 0);
  const record = device.history[count - 1 - device.historyOffset] || null;
  show('history', {record, position: device.historyOffset, count}, 'History: click for the next launch, hold to go back.');
}
function enterCalibration() {
  device.bladeArmed = false; setState('Calibrating');
  show('calibrating', {minimum: 4095, maximum: 0}, 'Calibrating: pull the launcher smoothly for 3 seconds.');
}
function enterSleep() {
  device.sleeping = true; device.serviceView = false;
  show('boot', {}, device.bladePresent
    ? 'Asleep. Press the center button to wake.'
    : 'Asleep. Press the center button or insert a blade to wake.');
}
function wake(reason) {
  device.sleeping = false; device.lastActivity = performance.now();
  setState('Boot');
  statusLine.textContent = reason === 'blade' ? 'Woken by the blade; arming…' : 'Woken by the center button.';
}

function handle(event) {
  device.lastActivity = performance.now();
  if (device.sleeping) { if (event === 'click' || event === 'double' || event === 'long' || event === 'service') wake('center'); return; }
  if (device.serviceView) {
    if (event === 'long' || event === 'service') { device.serviceView = false; enterReady(); }
    else if (event === 'click') show('service', serviceData(), 'Service: values refreshed.');
    return;
  }
  if (event === 'service' && device.state !== 'Measuring') {
    device.serviceView = true; show('service', serviceData(), 'Service screen: click refreshes, hold exits.');
    return;
  }
  switch (device.state) {
    case 'Ready':
      if (event === 'click') { setState('Menu'); device.menuItem = 0; showMenu(); }
      else if (event === 'long') enterCalibration();
      break;
    case 'ShowingResult':
      if (event !== 'service') enterReady();
      break;
    case 'SensorError':
      if (event !== 'service') enterCalibration();
      break;
    case 'Menu': handleMenu(event); break;
  }
}
function handleMenu(event) {
  if (device.clearConfirmation) {
    if (event === 'double') { device.history = []; device.best = 0; device.counter = 0; device.menuItem = 0; showMenu(); statusLine.textContent = 'History cleared. Calibration kept.'; }
    else if (event === 'click' || event === 'long') showMenu();
    return;
  }
  if (device.graphView) { if (event !== 'service') showMenu(); return; }
  if (device.historyView) {
    if (event === 'click' && device.history.length) { device.historyOffset = (device.historyOffset + 1) % device.history.length; openHistory(); }
    else if (event === 'long') showMenu();
    return;
  }
  if (event === 'click') { device.menuItem = (device.menuItem + 1) % 5; showMenu(); }
  else if (event === 'double') {
    if (device.menuItem === 0) openHistory();
    else if (device.menuItem === 1) { device.graphView = true; show('graph', {samples: device.lastProfile, peakRpm: device.lastResult?.peakRpm || 0}, 'Graph of the last launch. Any click returns.'); }
    else if (device.menuItem === 2) enterCalibration();
    else if (device.menuItem === 3) { device.clearConfirmation = true; show('clearConfirmation', {}, 'Double-click erases the history; click cancels.'); }
    else enterReady();
  } else if (event === 'long') enterReady();
}
const serviceData = () => ({raw: 3540 + Math.floor(Math.random() * 300), ...device.calibration, low: 3548, high: 3730, transitions: device.counter * 120, state: Math.random() > .5});

function setBlade(present) {
  device.bladePresent = present;
  bladeButton.setAttribute('aria-pressed', String(present));
  bladeButton.textContent = present ? 'Remove blade' : 'Insert blade';
  device.lastActivity = performance.now();
  if (present && device.sleeping) { wake('blade'); return; }
  if (present && device.state === 'Ready' && !device.bladeArmed) {
    device.bladeArmed = true; show('ready', stats(), 'BLADE PRESENT: armed. Sensor on.');
  }
}

function launch() {
  device.lastActivity = performance.now();
  if (device.sleeping || device.state !== 'Ready' || !device.bladeArmed || device.serviceView) {
    statusLine.textContent = 'Nothing measured: insert the blade on the READY screen first.';
    return;
  }
  setState('Measuring'); statusLine.textContent = 'Measuring…';
  const result = simulateLaunch(Number(rpmInput.value));
  // The blade leaves the launcher, releasing the BeySense button.
  setBlade(false);
  setTimeout(() => {
    device.bladeArmed = false;
    const record = {timestamp: ++device.counter, peakRpm: result.peakRpm, averageRpm: result.averageRpm, durationMs: result.durationMs};
    const newBest = record.peakRpm > device.best;
    device.best = Math.max(device.best, record.peakRpm);
    device.history.push(record); if (device.history.length > 20) device.history.shift();
    device.lastResult = record; device.lastProfile = result.samples;
    setState('ShowingResult');
    show('result', {record, bestRpm: device.best, newBest}, newBest ? 'New personal best!' : 'Result shown for 5 seconds.');
  }, Math.min(1200, result.durationMs + 250));
}

// Timers: boot delay, calibration, result timeout, inactivity sleep.
setInterval(() => {
  const now = performance.now(), elapsed = now - device.enteredAt;
  if (device.sleeping) return;
  if (device.state === 'Boot' && elapsed >= BOOT_MS) enterReady();
  else if (device.state === 'Calibrating' && elapsed >= CALIBRATION_MS) {
    statusLine.textContent = 'Calibration saved; it now survives sleep and reboot.'; enterReady();
  } else if (device.state === 'ShowingResult' && elapsed >= RESULT_SCREEN_MS) enterReady();
  if (!['Measuring', 'Calibrating'].includes(device.state) && now - device.lastActivity >= SLEEP_AFTER_MS) enterSleep();
}, 100);

// ---- Controls ----
const bladeButton = document.querySelector('#blade'), rpmInput = document.querySelector('#launch-rpm');
for (const button of document.querySelectorAll('[data-action]')) button.addEventListener('click', () => handle(button.dataset.action));
bladeButton.addEventListener('click', () => setBlade(!device.bladePresent));
document.querySelector('#sleep').addEventListener('click', enterSleep);
document.querySelector('#launch').addEventListener('click', launch);
rpmInput.addEventListener('input', () => { document.querySelector('#launch-rpm-value').textContent = rpmInput.value; });

// The round center button behaves like the real one: press length decides.
const center = document.querySelector('#center');
let pressedAt = 0, lastClickAt = 0, clickTimer = 0;
center.addEventListener('pointerdown', () => { pressedAt = performance.now(); });
center.addEventListener('pointerup', () => {
  if (!pressedAt) return;
  const held = performance.now() - pressedAt; pressedAt = 0;
  if (held >= 3000) return handle('service');
  if (held >= 900) return handle('long');
  const now = performance.now();
  if (clickTimer && now - lastClickAt <= 350) { clearTimeout(clickTimer); clickTimer = 0; return handle('double'); }
  lastClickAt = now;
  clickTimer = setTimeout(() => { clickTimer = 0; handle('click'); }, 350);
});
center.addEventListener('keydown', event => { if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); handle('click'); } });

// ---- Gallery of every screen with sample data ----
const sample = simulateLaunch(9800);
const sampleRecord = {timestamp: 14, peakRpm: sample.peakRpm, averageRpm: sample.averageRpm, durationMs: sample.durationMs};
const gallery = [
  ['boot', {}, 'Boot / sleep'],
  ['insertBlade', {bestRpm: 11240, launchCount: 13}, 'Insert blade'],
  ['ready', {bestRpm: 11240, launchCount: 13}, 'Ready'],
  ['result', {record: sampleRecord, bestRpm: 11240, newBest: false}, 'Result'],
  ['graph', {samples: sample.samples, peakRpm: sample.peakRpm}, 'Launch profile'],
  ['menu', {item: 0}, 'Menu'],
  ['history', {record: sampleRecord, position: 0, count: 14}, 'History'],
  ['clearConfirmation', {}, 'Clear history'],
  ['calibrating', {minimum: 2010, maximum: 4080}, 'Calibration'],
  ['sensorError', {minimum: 2100, maximum: 2250}, 'Sensor error'],
  ['service', {raw: 3612, minimum: 2000, maximum: 4095, low: 3548, high: 3730, transitions: 1536, state: true}, 'Service']
];
const galleryHost = document.querySelector('#gallery');
for (const [name, data, label] of gallery) {
  const figure = document.createElement('figure'), canvas = document.createElement('canvas'), caption = document.createElement('figcaption');
  canvas.width = WIDTH; canvas.height = HEIGHT; canvas.setAttribute('role', 'img'); canvas.setAttribute('aria-label', label);
  paint(canvas, render(name, data).canvas);
  caption.textContent = label;
  figure.append(canvas, caption); galleryHost.append(figure);
}

show('boot', {}, 'Booting…');
