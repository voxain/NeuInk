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
- **Boot screen text.** The boot screen still shows the name as text. Replace it
  with the wordmark if the sleep screen's look is the one to keep.
- **Sync server.** If custom sync servers are wanted back, add their roots to
  `lib/KOReaderSync/KOReaderTrust.h`. Custom servers are removed for now.
