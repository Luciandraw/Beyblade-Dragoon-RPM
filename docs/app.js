const status = document.querySelector('#status');
const consent = document.querySelector('#consent');
const host = document.querySelector('#install-host');
const urls = [];

function message(text, error = false) {
  status.textContent = text;
  status.dataset.error = String(error);
}

async function get(url) {
  const response = await fetch(url, { cache: 'no-store', signal: AbortSignal.timeout(30000) });
  if (!response.ok) throw new Error(`File unavailable (HTTP ${response.status}).`);
  return response;
}

async function prepare() {
  try {
    const releaseUrl = new URL('./firmware/release.json', location.href);
    const release = await (await get(releaseUrl)).json();
    if (release.ready !== true) {
      message('No firmware release yet. Installation will become available once a build is published.');
      return;
    }
    document.querySelector('#version').textContent = release.version;
    if (!window.isSecureContext) throw new Error('Open this site over HTTPS, or use localhost for a local preview.');
    if (!('serial' in navigator)) throw new Error('USB access is not supported in this browser. Use Chrome or Edge on desktop.');
    if (!/^[a-f0-9]{64}$/.test(release.sha256) || !Number.isInteger(release.size) || release.size > 4194304 || release.size < 65560) throw new Error('Invalid release metadata.');
    const binaryUrl = new URL(release.file, releaseUrl);
    if (binaryUrl.origin !== location.origin || !binaryUrl.pathname.startsWith(new URL('./', releaseUrl).pathname)) throw new Error('Invalid firmware location.');
    message('Verifying the firmware download…');
    const bytes = await (await get(binaryUrl)).arrayBuffer();
    const hash = [...new Uint8Array(await crypto.subtle.digest('SHA-256', bytes))].map(x => x.toString(16).padStart(2, '0')).join('');
    if (bytes.byteLength !== release.size || hash !== release.sha256) throw new Error('Firmware integrity check failed. The file may be damaged or publication may be incomplete. Try again later.');
    const firmwareBlob = URL.createObjectURL(new Blob([bytes]));
    urls.push(firmwareBlob);
    const manifest = {
      name: 'Beyblade RPM Analyzer', version: release.version,
      new_install_prompt_erase: false, new_install_improv_wait_time: 0,
      builds: [{ chipFamily: 'ESP32-S3', parts: [{ path: firmwareBlob, offset: 0 }] }]
    };
    const manifestBlob = URL.createObjectURL(new Blob([JSON.stringify(manifest)], { type: 'application/json' }));
    urls.push(manifestBlob);
    // Pinned library; the USB chooser is opened by its button, never on page load.
    let loadTimeout;
    try {
      await Promise.race([
        import('https://unpkg.com/esp-web-tools@10.1.1/dist/web/install-button.js?module'),
        new Promise((_, reject) => { loadTimeout = setTimeout(() => reject(new Error('The installer library did not load within 30 seconds.')), 30000); })
      ]);
    } finally { clearTimeout(loadTimeout); }
    await customElements.whenDefined('esp-web-install-button');
    const installer = document.createElement('esp-web-install-button');
    installer.manifest = manifestBlob;
    const button = document.createElement('button');
    button.slot = 'activate';
    button.textContent = 'Erase & install firmware ↗';
    button.disabled = true;
    installer.append(button);
    host.replaceChildren(installer);
    consent.disabled = false;
    consent.addEventListener('change', () => { button.disabled = !consent.checked; });
    message('Firmware verified. Confirm your board and data erasure below, then select your USB port.');
  } catch (error) {
    message(`Installer unavailable. ${error.message} Check your connection and reload this page.`, true);
    consent.disabled = true;
  }
}

window.addEventListener('beforeunload', () => urls.forEach(url => URL.revokeObjectURL(url)));
// Optional read-only browser-agent integration. Never exposes USB or erase actions.
if (document.modelContext?.registerTool) {
  const lifecycle = new AbortController();
  const tool = {
    name: 'read_installer_status', title: 'Read USB installer readiness',
    description: 'Read the visible release and readiness status. Does not connect or flash a device.',
    inputSchema: { type: 'object', properties: {}, additionalProperties: false },
    annotations: { readOnlyHint: true, untrustedContentHint: false },
    execute(input) {
      if (!input || typeof input !== 'object' || Array.isArray(input) || Object.keys(input).length) throw new Error('Expected an empty object.');
      return { version: document.querySelector('#version').textContent, status: status.textContent, canConfirm: !consent.disabled };
    }
  };
  try { Promise.resolve(document.modelContext.registerTool(tool, { signal: lifecycle.signal })).catch(() => {}); } catch { /* Optional API must not block the installer. */ }
  window.addEventListener('pagehide', () => lifecycle.abort());
}
prepare();
