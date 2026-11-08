# Open items

Things deliberately left for later. Each one says when to pick it up.

## Cleanup follow-ups

Left over from the cleanup sweep on `chore/cleanup-sweep` (done there: dead `ScalableGsub.h`, two dead reader helpers,
one calendar-math header instead of six copies, shared enum/swipe/byte-order/ASCII-text/xorshift/toast helpers, one
action-button drawer, 98 unused translation keys, eight dead functions). Pick
these up one at a time; each is a behaviour-neutral merge that needs a build, so batch them.

- Byte helpers still local: `Xtc.cpp` (pointer-style `writeLe16/32`), `Epub.cpp`, `SdCardFont.cpp`, `EpubReaderActivity.cpp`
  and `NearbyBookPositionSyncActivity.cpp` (file/stream readers) could use `lib/Serialization/ByteOrder.h`.
- `OtaUpdater.cpp` keeps a hex-only `asciiLower`; `WordForms.cpp` a lambda `endsWith`: both could use `lib/Utf8/AsciiText.h`.
- Web server `isTwoFingerSwipeSetting(SettingInfo)` and `enumDisplayIndexForWeb` duplicate the settings helpers
  (by name id instead of member pointer); derive one from the other.
- Hand-drawn buttons still left: `GameUi.h` `button`/`iconButton`/`cornerButton`, `BookStatsView` date fields and
  adjust buttons, `DictionaryWordSelectActivity` ask bar, `HighlightCategoryActivity` pill. Games and the stats card
  need their own look (white on a gray card), so only the plain ones should move to `TouchActionButtons::drawButton`
  or `fui::button` with `grayButtonStyles()`.
- `readerDarkModeEnabled()` is a stub that returns false, behind `readerBackgroundColor()`, `ThemeContent::darkMode`,
  `drawImageInverted` and the `darkMode` header parameters: remove the whole chain in one go (about 30 call sites).
- Unused Lucide icons: the generated tables (`homeIcons.h`, `homeGridIcons.h`) are only reached through macros, so a
  text search cannot say which manifest entries are unused. Compare `homeIcons.manifest` against `UIIcon` uses, drop
  the rest and regenerate.
- Test build: `UiSymbolFallbackTest` still includes the removed `inter_*` headers; the scalable-font tests need libasan.

## Battery statistics

Battery health and usage. The page exists (X4 Pro, 0.3.0); what is left is health and cycles.

- X3: the BQ27220 fuel gauge keeps its own counters. Read CycleCount (0x2A), StateOfHealth (0x2E, percent) and
  FullChargeCapacity (0x12, against DesignCapacity 0x3C) over I2C in `BatteryMonitor`, next to the voltage,
  current and charge reads it already does. Check the register addresses against the datasheet and a real X3
  before relying on them.
- X4 Pro: done in 0.3.0 (graph, runtimes, per-app charge; see Settings > Battery). Charge sessions and estimated full
  cycles from the voltage-based percentage are not counted yet.
- Shared page: show real gauge values where there are any, estimates elsewhere, and say which is which.

## Lexend Deca UI font and Vega greeting (experimental)

The menu font is Lexend Deca now, and the Vega Home greeting is set in Bitter. Both shipped in 0.4.0 without
measurements: nobody has measured their performance or RAM cost on a device yet.

- UI font: each face carries Hebrew, Arabic and IPA fallbacks, so the headers are larger than Inter's. Compare flash use,
  first-draw time and heap on the X4 Pro with the previous Inter build. The 8 px face is the one to check for legibility.
- Greeting: on scalable builds it opens one face, Bitter Bold at 22 pt (`scalableBuiltinDisplayFontId`). The whole
  family it loaded before took so much of the shared font arena that SD-card reader fonts failed to load. Check on
  a device that an installed font loads with the greeting on; if not, set the greeting in the largest built-in UI
  size instead (`greetingFontId()` in `VegaTheme.cpp`).
- If either is not worth it, `git revert` of the commit "Lexend Deca UI font, taller header status row, Vega greeting"
  brings back Inter (the Inter source fonts are still in `lib/EpdFont/builtinFonts/source/Inter`).

## Host tests after the font change

`UiSymbolFallbackTest` still includes `builtinFonts/inter_10_bold.h`, which the Lexend Deca change removed, so it no
longer builds; point it at the Lexend UI faces. (`font_catalog_index`, `scalable_fonts` and `scalable_font_adapter`
do not link on the Linux build machine, which lacks libasan.)
