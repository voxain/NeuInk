# Open items

Things deliberately left for later. Each one says when to pick it up.

- **Host unit tests.** Run the full suite (`test/`) once there are features to
  show. Only the KOReader trust test has been run so far.
- **Firmware download certificates.** The OTA firmware download still runs the
  wolfSSL transport with certificate checks off (`src/network/HttpDownloader.cpp`).
  It is protected by the release's SHA-256 digest. Pin GitHub's release host
  when the release process is in use.
- **First release.** Publish tag `0.1.0` on `voxain/NeuInk` with
  `firmware-x4-pro.bin` attached, since the update check looks for that name.
  Language packs and updates need the release to exist.
- **Final logo.** Replace `src/images/neuink-logo.png` with the hand-made logo,
  then run `scripts/generate_logo.py` to refresh the boot, sleep and web logos.
- **Check the sleep wordmark on the device.** Its placement was worked out from
  the display code, not measured. Adjust the offsets in
  `src/activities/boot_sleep/SleepActivity.cpp` if it's off.
- **Merge.** Merge `feature/neuink-setup` into `development`, then `development`
  into `main`, once the setup work is finished.
