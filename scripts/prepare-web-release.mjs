import { readFile, writeFile, mkdir } from 'node:fs/promises';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';

export function validateVersion(version) {
  if (!/^[A-Za-z0-9][A-Za-z0-9._-]{0,47}$/.test(version ?? '')) throw new Error('Version must be 1–48 ASCII letters, numbers, dots, underscores or hyphens.');
}

export function validateImage(data) {
  if (data.length < 0x10018 || data.length > 4 * 1024 * 1024) throw new Error('Expected a complete merged image, at most 4 MB.');
  for (const offset of [0, 0x10000]) {
    if (data[offset] !== 0xe9 || data.readUInt16LE(offset + 12) !== 9) throw new Error('Expected ESP32-S3 bootloader at 0 and application at 0x10000.');
  }
  if (data[2] !== 2 || data[3] !== 0x2f) throw new Error('Expected boot header DIO, 80 MHz, 4 MB (Arduino QIO bootloader build).');
  if (data.readUInt16LE(0x8000) !== 0x50aa) throw new Error('Partition table is missing at 0x8000.');
}

export async function prepare(buildDir, version, outputDir) {
  validateVersion(version);
  // Never package a diagnostic binary or an application-only export.
  const data = await readFile(join(buildDir, 'main.ino.merged.bin'));
  validateImage(data);
  const sha256 = createHash('sha256').update(data).digest('hex');
  const file = `beyblade-rpm-${version}-${sha256.slice(0, 12)}.bin`;
  await mkdir(outputDir, { recursive: true });
  await writeFile(join(outputDir, file), data);
  const manifest = {
    name: 'Beyblade RPM Analyzer', version,
    new_install_prompt_erase: false, new_install_improv_wait_time: 0,
    builds: [{ chipFamily: 'ESP32-S3', parts: [{ path: file, offset: 0 }] }]
  };
  await writeFile(join(outputDir, 'manifest.json'), JSON.stringify(manifest, null, 2) + '\n');
  // Publish readiness last, after both binary and manifest are written.
  await writeFile(join(outputDir, 'release.json'), JSON.stringify({ ready: true, version, file, size: data.length, sha256 }, null, 2) + '\n');
  return file;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const [buildDir, version, outputDir = 'docs/firmware'] = process.argv.slice(2);
  if (!buildDir || !version) {
    console.error('Usage: node scripts/prepare-web-release.mjs BUILD_DIRECTORY VERSION [OUTPUT_DIRECTORY]');
    process.exitCode = 1;
  } else {
    try { console.log(await prepare(resolve(buildDir), version, resolve(outputDir))); }
    catch (error) { console.error(error.message); process.exitCode = 1; }
  }
}
