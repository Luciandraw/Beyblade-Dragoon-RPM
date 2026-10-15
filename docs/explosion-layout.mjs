// LauncherV2's thickness is world Y. Keep related pieces on the same layer.
// These are presentation layers, not a prescribed physical disassembly order.
const layers = [
  [-2, ['LauncherClip 1', 'LauncherClip 2']],
  [-1, ['SensorMount']],
  [0, ['Case']],
  [1, ['HeadBottomn']],
  [2, ['QRE1113', 'ButtonCenter', 'BeySenseButton',
    'Battery200mAh', 'ESP32-S3_Zero', 'LX-LCBST Boost Module', 'Switch', 'SwitchCover']],
  [3, ['E-ink_screen', 'HeadDecorConnector_1',
    'HeadDecorConnector_2', 'HeadDecorConnector_3']],
  [4, ['ScreenCover 1', 'BackPart', 'HeadTop', 'Eye1', 'Eye2']],
  [5, ['ButtonCoverCenter', 'MeshCover', 'HeadDecor 1', 'HeadDecor 2']]
];
// GLTFLoader normalizes spaces/punctuation in node names.
const key = name => name.replace(/[^a-z0-9]/gi, '').toLowerCase();
const byName = new Map(layers.flatMap(([layer,names]) => names.map(name => [key(name),layer])));
export function explosionLayer(name) { return byName.get(key(name)) ?? 0; }
export function explosionOffset(name, gap) { return [0, explosionLayer(name)*gap, 0]; }
// Stack layers by their real height so neighbouring layers never overlap.
// bounds: Map(layer → [minY, maxY]) of the assembled model; returns Map(layer → shiftY).
// Layer 0 (Case) stays put; higher layers rise, lower layers drop, each clearing the next by gap.
export function layerShifts(bounds, gap) {
  const layers = [...bounds.keys()].sort((a, b) => a - b);
  const base = layers.reduce((best, layer) => Math.abs(layer) < Math.abs(best) ? layer : best, layers[0]);
  const shifts = new Map([[base, 0]]);
  for (const [direction, list] of [[1, layers.filter(l => l > base)], [-1, layers.filter(l => l < base).reverse()]]) {
    let previous = 0, edge = bounds.get(base)[direction > 0 ? 1 : 0];
    for (const layer of list) {
      const [min, max] = bounds.get(layer);
      const shift = direction > 0 ? Math.max(previous, edge + gap - min) : Math.min(previous, edge - gap - max);
      shifts.set(layer, shift); previous = shift; edge = (direction > 0 ? max : min) + shift;
    }
  }
  return shifts;
}
