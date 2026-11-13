import test from 'node:test';
import assert from 'node:assert/strict';
import {parseHeader, parseHistory, parseCurve, isHeader, isHistoryPage} from '../docs/battle-pass.mjs';

// Builds pages exactly like main/battle_pass.cpp does.
function headerPage(status, best, total, address) {
  const page = new Uint8Array(17);
  page[0] = 0xa0; page[1] = 0x3a; page[3] = status; page[4] = 0xff;
  page[7] = best & 255; page[8] = best >> 8; page[9] = total & 255; page[10] = total >> 8;
  page.set(address, 11);
  return page;
}

function historyPages(slots, best, total) {
  const data = new Uint8Array(112);
  slots.forEach((value, i) => { data[i * 2] = value & 255; data[i * 2 + 1] = value >> 8; });
  const put = (offset, value) => { data[96 + offset] = value & 255; data[96 + offset + 1] = value >> 8; };
  put(6, best); put(8, total); put(10, slots.length);
  const pages = [];
  for (let p = 0; p < 7; p += 1) pages.push(Uint8Array.of(0xb0 + p, ...data.slice(p * 16, p * 16 + 16)));
  const last = new Uint8Array(17);
  last[0] = 0xb7;
  last[16] = data.reduce((sum, byte) => (sum + byte) & 255, 0);
  pages.push(last);
  return pages;
}

test('header page decodes status, summary and address', () => {
  const header = parseHeader(headerPage(0x04, 9999, 7, [1, 2, 3, 4, 5, 6]));
  assert.equal(header.ready, true);
  assert.equal(header.bestRpm, 9999);
  assert.equal(header.launches, 7);
  assert.equal(header.address, '06:05:04:03:02:01');
  assert.equal(parseHeader(headerPage(0x10, 0, 0, [0, 0, 0, 0, 0, 0])).button, true);
});

test('history decodes slots oldest first and checks the checksum', () => {
  const slots = [8000, 12345, 9999];
  const pages = historyPages(slots, 12345, 3);
  const history = parseHistory(pages);
  assert.deepEqual(history.launches, slots);
  assert.equal(history.best, 12345);
  assert.equal(history.total, 3);
  pages[7][16] ^= 1;
  assert.throws(() => parseHistory(pages), /checksum/);
});

test('history with all 50 slots keeps slots 48 and 49 apart from the summary', () => {
  const slots = Array.from({length: 50}, (_, i) => 1000 + i * 100);
  const history = parseHistory(historyPages(slots, 5900, 60));
  assert.deepEqual(history.launches, slots);
  assert.equal(history.total, 60);
});

test('curve pages decode to RPM, zero stays zero', () => {
  const pages = [];
  for (let p = 0; p < 4; p += 1) {
    const page = new Uint8Array(17);
    page[0] = 0x70 + p;
    for (let i = 0; i < 8; i += 1) {
      const rpm = p === 3 && i === 7 ? 0 : 5000 + p * 100 + i;
      const raw = rpm ? Math.floor(7500000 / rpm) : 0;
      page[1 + i * 2] = raw & 255; page[2 + i * 2] = raw >> 8;
    }
    pages.push(page);
  }
  const curve = parseCurve(pages);
  assert.equal(curve.length, 32);
  assert.ok(Math.abs(curve[0] - 5000) <= 2);
  assert.equal(curve[31], 0);
});

test('page tag helpers', () => {
  assert.ok(isHeader(Uint8Array.of(0xa0)));
  assert.ok(isHistoryPage(Uint8Array.of(0xb7)));
  assert.ok(!isHistoryPage(Uint8Array.of(0xa0)));
});
