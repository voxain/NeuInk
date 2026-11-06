# Keeping NeuInk up to date with VARAGH, CrossInk and CrossPoint

NeuInk is a fork of [VARAGH](https://github.com/PewPewzxc/VARAGH), which is itself
based on CrossInk and CrossPoint. Updates arrive from those projects, in this order
of importance: VARAGH first, then CrossInk, then single CrossPoint fixes.

The repository has these remotes:

| Remote | Repository | Use |
|---|---|---|
| `origin` | github.com/voxain/NeuInk | your fork (push here) |
| `varagh` | github.com/PewPewzxc/VARAGH | the fork NeuInk comes from; merge its releases |
| `crossink` | github.com/uxjulia/crossink | CrossInk, the base of VARAGH |
| `crosspoint` | github.com/crosspoint-reader/crosspoint-reader | pick single fixes/features |

Add the last one once with `git remote add crosspoint https://github.com/crosspoint-reader/crosspoint-reader.git`.

The display/UI library is a submodule pointing at
github.com/Free-Ink/freeink-sdk, pinned to one commit. To move it, change the
commit in `freeink-sdk` and commit the new gitlink.

## Merge a new VARAGH release

```bash
git fetch varagh --tags
git checkout -b sync/varagh-vX.Y.Z development
git merge varagh/main       # or the release tag
```

Conflicts are most likely in files VARAGH changed (translations, the dictionary
and reader activities, settings). Keep both sides where you can; VARAGH's
changes are listed in the [changelog](../../CHANGELOG.md).

## Merge a new CrossInk release

```bash
git fetch crossink --tags
git checkout -b sync/crossink-vX.Y.Z development
git merge crossink/main     # or the release tag
```

Only do this when VARAGH has already taken the release, or when you want its
changes now; otherwise the two histories diverge further.

Then build, test on the device, and merge the feature branch into `development`,
and `development` into `main` once it is stable:

```bash
pio run -e x4-pro
git checkout development && git merge sync/varagh-vX.Y.Z && git push
```

After a merge, check that the OTA update URL in `src/network/OtaUpdater.cpp`
still points at `voxain/NeuInk`, that `version` in `platformio.ini` is
NeuInk's version, and that the `STR_NEUINK_` strings still say NeuInk.

## Take one feature or fix from CrossPoint

CrossPoint and CrossInk have drifted apart, so take single pull requests rather
than merging everything:

```bash
git fetch crosspoint
git cherry-pick -x <commit>          # a commit from a merged PR
```

If a cherry-pick does not apply, fetch the PR's diff
(`https://github.com/crosspoint-reader/crosspoint-reader/pull/<N>.diff`), apply
it by hand, and note "ported from CrossPoint #N" in the commit message and
[THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md). VARAGH 1.0.0 already
contains CrossPoint #3616 (shared font interval tables) and #3633 (faster glyph
drawing).

## What 1.2.0 touches in upstream files

Keep these in mind when merging a newer CrossInk; everything else of 1.2.0 is
in files of its own (`src/activities/apps`, `src/games`, `src/network/Online*`,
`src/util/CalendarMath.h`, `src/util/OnlineDictionaryText.*`).

- `src/activities/home/HomeActivity.*`: the Apps entry, the Carousel frame
  cache and touch handling, covers prepared before the first paint, a single
  paint for Cover Grid.
- `lib/hal/HalPowerManager.*`: `setWifiIdleLowPowerAllowed`.
- `src/SettingsList.h`, `src/CrossPointSettings.*`: "Online Lookup: Keep Wi-Fi
  On" and the Clock & Calendar sleep screen (`CLOCK_SLEEP`, also in the sleep
  screen storage order).
- `src/activities/boot_sleep/SleepActivity.*`: `renderClockSleepScreen`.
- `src/activities/reader/EpubReaderActivity.cpp`, the dictionary activities and
  `src/main.cpp`: the online dictionary hooks.

## What 1.2.1 touches in upstream files

- `src/CrossPointState.*` (`lastSleepApp`), `src/activities/ActivityManager.*`
  (`goToApps`, `sleepResumeApp`) and `src/main.cpp` (`enterDeepSleep` and the
  wake routing): waking returns to the app that was open at sleep.
- `src/network/HttpDownloader.*`: `DownloadOptions::timeoutMs` and
  `httpStatus`, and cancel checks before a connection is opened and before its
  headers are read.
- `lib/hal/HalPowerManager.*`: `beginBackgroundJob` / `endBackgroundJob`.
- `src/activities/reader/EpubReaderActivity.*` (Tap to Look Up, the release
  action passed to word selection, the rating tap at the end of a book),
  `DictionaryWordSelectActivity.*` and `src/util/WordSelectNavigator.*` (the
  Ask bubble), `DictionaryDefinitionActivity.*` (`addToCategoryOnEnter`).
- `src/activities/reader/EndOfBookOptions.*`, `XtcReaderActivity.cpp`,
  `BookStatsView.*`, `BookStatsActivity.*`: the rating on the end screen and on
  Reading Stats.
- `src/activities/home/FileBrowserActionActivity.*`, `BookActions.*`,
  `FileBrowserActivity.cpp`, `src/activities/library/LibraryActivity.*`,
  `src/activities/util/BmpViewerActivity.cpp`: the Rating row of the book menu
  and the stars in Library rows.
- `src/util/BookMoveUtils.cpp`: a moved book keeps its rating.
- `lib/EpdFont/builtinFonts/ui_symbols_10.h`, its generator script and
  `test/ui_symbol_fallback`: three star glyphs. `fontIds.h` was left as it is.

## Release

1. Raise `version` in `platformio.ini` (for example `1.0.1`) and add a
   changelog entry.
2. `pio run -e x4-pro`, test on the device, commit and push.
3. On GitHub: **Releases → Draft a new release**, tag `v1.0.1`, attach
   `.pio/build/x4-pro/firmware-x4-pro.bin` **with exactly that file name**
   (online updates look for it), the SD-card zip and a full-source zip.
