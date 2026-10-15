import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {BOOT_LOGO, BLADE_ICON} from '../docs/screen-assets.mjs';
import {render, SCREEN_NAMES} from '../docs/screens.mjs';

const firmware = name => readFileSync(new URL(`../main/${name}`, import.meta.url), 'utf8');
const bytes = (source, name) => Buffer.from(source.match(new RegExp(`${name}\\[\\]\\s*PROGMEM\\s*=\\s*\\{([\\s\\S]*?)\\};`))[1]
  .match(/0x[0-9A-Fa-f]{2}/g).map(value => parseInt(value, 16)));

test('Emulator bitmaps match the firmware headers', () => {
  assert.deepEqual(Buffer.from(BOOT_LOGO.data, 'base64'), bytes(firmware('logo_bitmap.h'), 'BOOT_LOGO_BITMAP'));
  assert.deepEqual(Buffer.from(BLADE_ICON.data, 'base64'), bytes(firmware('blade_icon.h'), 'BLADE_ICON_BITMAP'));
});

test('Every firmware screen has an emulator twin', () => {
  const drawn = [...firmware('display.cpp').matchAll(/^void draw(\w+)\(/gm)].map(([, name]) => name[0].toLowerCase() + name.slice(1));
  assert.deepEqual([...SCREEN_NAMES].sort(), drawn.sort());
});

test('All emulator screens render inside the 200 x 200 panel', () => {
  const record = {timestamp: 7, peakRpm: 9800, averageRpm: 6100, durationMs: 420};
  const samples = Array.from({length: 60}, (_, i) => ({timestampUs: i * 7000, rpm: 4000 + i * 90}));
  const data = {result: {record, bestRpm: 9900}, history: {record, position: 0, count: 3}, graph: {samples, peakRpm: 9400}};
  for (const name of SCREEN_NAMES) {
    const {canvas} = render(name, data[name]);
    const inked = canvas.pixels.reduce((sum, value) => sum + value, 0);
    assert.ok(inked > 200 && inked < 40000, `${name}: ${inked} black pixels`);
  }
});
