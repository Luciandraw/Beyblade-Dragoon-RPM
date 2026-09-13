import test from 'node:test';
import assert from 'node:assert/strict';
import { validateImage, validateVersion, prepare } from './prepare-web-release.mjs';
import { mkdtemp, mkdir, writeFile, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { runInNewContext } from 'node:vm';
import { webcrypto } from 'node:crypto';

function fixture() {
  const data = Buffer.alloc(0x11000, 0xff);
  for (const offset of [0, 0x10000]) { data[offset] = 0xe9; data.writeUInt16LE(9, offset + 12); }
  data[2] = 2; data[3] = 0x2f; data.writeUInt16LE(0x50aa, 0x8000);
  return data;
}
test('version rejects paths, empty values and shell syntax', () => {
  for (const bad of ['', '../test', 'v1/2', 'x;echo', '<script>', 'x'.repeat(49)]) assert.throws(() => validateVersion(bad));
  validateVersion('v1.0.0-test');
});
test('reject incomplete images, other chips, bad flash headers and missing partitions', () => {
  validateImage(fixture());
  assert.throws(() => validateImage(Buffer.alloc(32)));
  assert.throws(() => validateImage(Buffer.alloc(4194305)));
  for (const offset of [0, 2, 3, 12, 0x8000, 0x10000, 0x1000c]) {
    const data = fixture(); data[offset] = 0; assert.throws(() => validateImage(data));
  }
});
test('package creates relative S3 manifest and matching metadata', async () => {
  const root = await mkdtemp(join(tmpdir(), 'beyblade-web-test-'));
  try {
    const build = join(root, 'build'); const output = join(root, 'site');
    await mkdir(build); await writeFile(join(build, 'main.ino.merged.bin'), fixture());
    const file = await prepare(build, 'test-1', output);
    const release = JSON.parse(await readFile(join(output, 'release.json')));
    const manifest = JSON.parse(await readFile(join(output, 'manifest.json')));
    assert.equal(release.file, file); assert.equal(release.ready, true);
    assert.equal(release.sha256.length, 64);
    assert.equal(manifest.builds[0].chipFamily, 'ESP32-S3');
    assert.deepEqual(manifest.builds[0].parts, [{ path: file, offset: 0 }]);
    assert.equal(manifest.new_install_prompt_erase, false);
  } finally { await rm(root, { recursive: true }); }
});

const appSource = await readFile(new URL('../docs/app.js', import.meta.url), 'utf8');
async function pageState(release, { secure = true, serial = true, fail = false } = {}) {
  const elements = Object.fromEntries(['#status', '#consent', '#install-host', '#version'].map(id => [id, { textContent: '', disabled: true, dataset: {} }]));
  const tools = [];
  const context = {
    document: { querySelector: id => elements[id], modelContext: { registerTool: tool => tools.push(tool) } },
    location: { href: 'https://example.github.io/BeybladeRPM/', origin: 'https://example.github.io' },
    window: { isSecureContext: secure, addEventListener() {} },
    navigator: serial ? { serial: {} } : {}, URL, AbortSignal, AbortController,
    crypto: webcrypto, Uint8Array,
    fetch: async () => {
      if (fail) throw new Error('offline');
      return { ok: true, json: async () => release, arrayBuffer: async () => new Uint8Array(65560).buffer };
    }
  };
  await runInNewContext(appSource, context);
  return { elements, tools };
}
test('empty release stays locked, and read-only tool validates arguments', async () => {
  const { elements, tools } = await pageState({ ready: false });
  assert.equal(elements['#consent'].disabled, true);
  assert.match(elements['#status'].textContent, /No firmware release yet/);
  assert.equal(tools[0].execute({}).canConfirm, false);
  assert.throws(() => tools[0].execute({ flash: true }));
});
test('unsupported, insecure and offline environments remain locked', async () => {
  for (const options of [{ secure: false }, { serial: false }, { fail: true }]) {
    const { elements } = await pageState({ ready: true, version: 'test' }, options);
    assert.equal(elements['#consent'].disabled, true);
    assert.equal(elements['#status'].dataset.error, 'true');
  }
});
test('corrupt firmware never enables USB installation', async () => {
  const { elements } = await pageState({ ready: true, version: 'test', file: 'test.bin', size: 65560, sha256: '0'.repeat(64) });
  assert.equal(elements['#consent'].disabled, true);
  assert.match(elements['#status'].textContent, /integrity check failed/);
});

test('public installer is English and retains safety controls', async () => {
  const html = await readFile(new URL('../docs/index.html', import.meta.url), 'utf8');
  assert.match(html, /<html lang="en">/);
  assert.doesNotMatch(html + appSource, /[\u0400-\u04ff]/);
  assert.match(html, /id="consent" type="checkbox" disabled/);
  assert.match(html, /Launch history, personal bests and saved settings will be erased/);
});
