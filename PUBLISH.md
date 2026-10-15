# Publishing Beyblade RPM

## Current package

- English landing page, interactive wiring diagram and 3D assembly viewer.
- Static files use relative paths for GitHub Pages project URLs.
- The full public model is included in `docs/models/LauncherV2.glb` and can be downloaded by visitors.
- Firmware 1.0.0 is included from the user's hardware-tested Arduino export, with PSRAM disabled. The original merged binary is preserved byte-for-byte; its structure and SHA-256 were checked locally.
- Browser visual QA and an end-to-end USB installation test of the published package remain outstanding.

## Publish the website without building firmware

1. Extract `BeybladeRPM-GitHubPages.zip` and upload its contents to your repository's default branch. Include the hidden `.github` directory and `docs/.nojekyll`. Do not upload the ZIP itself.
2. In repository **Settings → Pages**, select **Deploy from a branch**, the default branch and **/docs**, then save.
3. Open the HTTPS address shown by GitHub Pages. The files are also provided separately in `BeybladeRPM-SiteOnly.zip`; that archive contains the `docs` folder for the same publishing configuration.
4. Verify the homepage, 3D loading, selection, separation, Wireframe mode, and wiring contact details on desktop and mobile. Check browser console/network errors. Firmware installation requires a successful integrity check and explicit erasure consent in a supported browser.

The installer uses ESP Web Tools from unpkg and the viewer uses Three.js from jsDelivr. These features require access to those CDNs. The homepage background is a static image and does not download the 12.1 MB model.

## Publish with installable firmware

Follow `WEB_INSTALLER.md` to export a production merged binary from Arduino IDE or use the existing manually triggered **Build and publish USB installer** workflow. Test the firmware on your actual hardware first. If publishing through that workflow, switch the Pages source to **GitHub Actions** instead of branch deployment. Never substitute a diagnostic binary.

## Local checks

```powershell
node --test scripts/*.test.mjs
python -m http.server 8080 --bind 127.0.0.1 --directory docs
```

No credentials or personal tokens are required in the website. Source `Meshes/`, local build directories and release ZIPs are excluded from version control; the public model copy under `docs/models/` is intentionally included.
