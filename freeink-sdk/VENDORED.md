# Vendored FreeInk SDK

This directory is a copy of the `freeink-sdk` library, committed as plain files
(not a git submodule), so no upstream change can reach the firmware without a
commit in this repository.

- **Source:** `varagh-v1.3.2-full-source.zip`, the full-source asset of the VARAGH
  1.3.2 release (`https://github.com/PewPewzxc/VARAGH/releases/tag/1.3.2`), folder
  `varagh-1.3.2/freeink-sdk`.
- **SHA-256 of the zip:** `b95aeaab809469a83beceba92ac31f9bebac4455b132c033a784110107d545f5`
- **Licence:** MIT (`LICENSE`), with OpenX4 attribution in `NOTICE`.
- **Base:** this copy differs from `Free-Ink/freeink-sdk` at `6993701`; it is the
  VARAGH fork, which the firmware was built against.

## Audit (see the commit message for scope)

- Licence and NOTICE checked; the MIT text matches the Free-Ink base.
- No private keys, certificates or credential literals in the SDK source.
- Linked modules: display, hardware, UI, font, BLE keyboard host, UsbMassStorage,
  SecureNet and NearbyTransfer. `NearbyTransfer` is not in the firmware binary.
  `ContentProtection` (credentials, rights, licence keys) is not linked.
- **Open issue in the calling code:** `SecureHttpClient::setInsecure()` disables
  certificate verification. NeuInk calls it for KOReader sync (which sends
  credentials) and for downloads (`lib/KOReaderSync/`, `src/network/HttpDownloader.cpp`).
  The SDK offers `setCACert()`, but no CA bundle is shipped.

## NeuInk changes to the SDK

- `libs/assets/Icons/tools/gen_icons.py` writes `inline constexpr` icon arrays and `Icon` structs instead of
  `static const`, so an icon used in several files is in the firmware once (0.4.0; it was in it once per file).
- `components/controls/button.h`: `paintButton()` draws a button on any `DrawTarget` without a hit area (`button()` now
  calls it), `ButtonProps::iconPlacement` puts the label under the icon, and `ButtonProps::shrinkFont` sets a label too
  wide for the button in a smaller font (0.4.1).
