// Protocol of the companion pass, as emulated by main/battle_pass.cpp.
// Pure parsing/encoding functions plus a small Web Bluetooth client.
//
// Device: BLE name BEYBLADE_TOOL01, one service; the host writes a command byte
// to WRITE_UUID and receives 17-byte pages (tag + 16 data bytes) as
// notifications on NOTIFY_UUID.

export const DEVICE_NAME = 'BEYBLADE_TOOL01';
export const SERVICE_UUID = '55c40000-f8eb-11ec-b939-0242ac120002';
export const WRITE_UUID = '55c4f001-f8eb-11ec-b939-0242ac120002';
export const NOTIFY_UUID = '55c4f002-f8eb-11ec-b939-0242ac120002';

export const PAGE_SIZE = 17;
export const SLOTS = 50;
export const CURVE_SAMPLES = 32;
export const COMMANDS = {header: 0x51, history: 0x74, clear: 0x75};
export const STATUS = {idle: 0x00, ready: 0x04, button: 0x10};

const TAG_HEADER = 0xa0;
const TAG_CLEAR_ACK = 0xa1;
const TAG_HISTORY = 0xb0; // b0..b7
const TAG_CURVE = 0x70; // 70..73
const CURVE_NUMERATOR = 7500000;

const le16 = (bytes, offset) => bytes[offset] | (bytes[offset + 1] << 8);

// a0 page -> {status, ready, button, battery, bestRpm, launches, address}.
export function parseHeader(page) {
  if (page.length !== PAGE_SIZE || page[0] !== TAG_HEADER) throw new Error('not a header page');
  const status = page[3];
  return {
    status,
    ready: status === STATUS.ready,
    button: status === STATUS.button,
    battery: page[4],
    bestRpm: le16(page, 7),
    launches: le16(page, 9),
    // Sent least significant byte first; shown most significant first.
    address: Array.from(page.slice(11, 17)).reverse()
      .map(byte => byte.toString(16).padStart(2, '0')).join(':'),
  };
}

// b0..b7 pages (any order) -> {launches: [rpm oldest first], best, total, stored}.
export function parseHistory(pages) {
  const byTag = new Map(pages.map(page => [page[0], page]));
  const data = new Uint8Array(7 * 16);
  for (let i = 0; i < 7; i += 1) {
    const page = byTag.get(TAG_HISTORY + i);
    if (!page) throw new Error(`missing history page b${i}`);
    data.set(page.slice(1, 17), i * 16);
  }
  const last = byTag.get(TAG_HISTORY + 7);
  if (!last) throw new Error('missing history page b7');
  const checksum = data.reduce((sum, byte) => (sum + byte) & 0xff, 0);
  if (checksum !== last[16]) throw new Error('history checksum mismatch');
  const best = le16(data, 6 * 16 + 6);
  const total = le16(data, 6 * 16 + 8);
  const stored = Math.min(le16(data, 6 * 16 + 10), SLOTS);
  const launches = [];
  for (let i = 0; i < stored; i += 1) launches.push(le16(data, i * 2));
  return {launches, best, total, stored};
}

// 70..73 pages -> RPM per curve sample (0 = no sample).
export function parseCurve(pages) {
  const byTag = new Map(pages.map(page => [page[0], page]));
  const rpm = [];
  for (let p = 0; p < 4; p += 1) {
    const page = byTag.get(TAG_CURVE + p);
    if (!page) throw new Error(`missing curve page ${(TAG_CURVE + p).toString(16)}`);
    for (let i = 0; i < 8; i += 1) {
      const raw = le16(page, 1 + i * 2);
      rpm.push(raw ? Math.round(CURVE_NUMERATOR / raw) : 0);
    }
  }
  return rpm;
}

export const isHeader = page => page[0] === TAG_HEADER;
export const isClearAck = page => page[0] === TAG_CLEAR_ACK;
export const isHistoryPage = page => page[0] >= TAG_HISTORY && page[0] <= TAG_HISTORY + 7;
export const isCurvePage = page => page[0] >= TAG_CURVE && page[0] <= TAG_CURVE + 3;

// Web Bluetooth client. `bluetooth` defaults to navigator.bluetooth.
export class BattlePassClient extends EventTarget {
  constructor(bluetooth = globalThis.navigator?.bluetooth) {
    super();
    this.bluetooth = bluetooth;
    this.device = null;
    this.writeChar = null;
    this.pages = [];
    this.onNotify = event => this.#handle(new Uint8Array(event.target.value.buffer.slice(
      event.target.value.byteOffset, event.target.value.byteOffset + event.target.value.byteLength)));
  }

  get connected() { return Boolean(this.device?.gatt?.connected); }

  // Must be called from a user gesture (click handler).
  async connect() {
    if (!this.bluetooth) throw new Error('Web Bluetooth is not available in this browser');
    this.device = await this.bluetooth.requestDevice({
      filters: [{name: DEVICE_NAME}, {services: [SERVICE_UUID]}],
      optionalServices: [SERVICE_UUID],
    });
    this.device.addEventListener('gattserverdisconnected', () => this.dispatchEvent(new Event('disconnect')));
    const server = await this.device.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    this.writeChar = await service.getCharacteristic(WRITE_UUID);
    const notifyChar = await service.getCharacteristic(NOTIFY_UUID);
    notifyChar.addEventListener('characteristicvaluechanged', this.onNotify);
    await notifyChar.startNotifications();
  }

  disconnect() { if (this.connected) this.device.gatt.disconnect(); }

  async #send(command) {
    this.pages = [];
    await this.writeChar.writeValue(Uint8Array.of(command));
  }

  // Resolves with the pages received until `done(pages)` is true.
  #collect(command, done, timeoutMs = 4000) {
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.removeEventListener('page', check);
        reject(new Error('The device did not answer in time'));
      }, timeoutMs);
      const check = () => {
        if (!done(this.pages)) return;
        clearTimeout(timer);
        this.removeEventListener('page', check);
        resolve(this.pages.slice());
      };
      this.addEventListener('page', check);
      this.#send(command).catch(error => { clearTimeout(timer); reject(error); });
    });
  }

  async readHeader() {
    const pages = await this.#collect(COMMANDS.header, all => all.some(isHeader));
    return parseHeader(pages.find(isHeader));
  }

  // The device sends a header, then b0..b7 (about 130 ms of paced notifications).
  async readHistory() {
    const pages = await this.#collect(COMMANDS.history,
      all => all.some(isHeader) && all.filter(isHistoryPage).length >= 8);
    return {header: parseHeader(pages.find(isHeader)), ...parseHistory(pages.filter(isHistoryPage))};
  }

  // Erases the history on the device (the app's own action, like the real pass).
  async clearHistory() {
    await this.#collect(COMMANDS.clear,
      all => all.some(isClearAck) && all.filter(isHistoryPage).length >= 8);
  }

  #handle(page) {
    if (page.length !== PAGE_SIZE) return;
    this.pages.push(page);
    this.dispatchEvent(new Event('page'));
    // Unsolicited pushes: a launch (header + history + curve) or a button press.
    if (isHeader(page)) {
      const header = parseHeader(page);
      this.dispatchEvent(new CustomEvent(header.button ? 'button' : 'status', {detail: header}));
    }
    if (isCurvePage(page) && this.pages.filter(isCurvePage).length >= 4) {
      this.dispatchEvent(new CustomEvent('curve', {detail: parseCurve(this.pages.filter(isCurvePage))}));
    }
  }
}
