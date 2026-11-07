---
title: Icon Libraries
nav_order: 4
---

# Icon Libraries

CrossInk rasterizes only the icons it uses into 1-bit C++ headers. This keeps
the firmware small enough for ESP32-C3 devices; adding an icon does not embed a
whole icon library.

The local manifests in `src/components/icons/` can use either source:

- **Lucide:** the SVGs of the `lucide-static` npm package (the version named at the top of
  `homeIcons.manifest`). The vendored SDK does not carry them: fetch them into a scratch folder, never into the
  repository.
- **Tabler outline:** `assets/tabler-icons/icons/outline`
- **Tabler filled:** `assets/tabler-icons/icons/filled`

## Lucide icons for the apps, Home, Settings and the reader panel

These all live in one manifest, `src/components/icons/homeIcons.manifest`, at 24 and 32 px, and one header,
`homeIcons.h`. To add one:

1. Add `alias = lucide-name` to the manifest. 24 px is Lucide's own grid, pixel-exact; 32 px is for squircles
   and tiles.
2. Put the Lucide SVGs (and `src/components/icons/lucide-snapped/*.svg`) in one scratch folder and generate:

   ```sh
   npm pack lucide-static@<version> && tar xzf lucide-static-*.tgz   # in the scratch folder
   python3 freeink-sdk/libs/assets/Icons/tools/gen_icons.py --svgdir <folder> \
     --manifest src/components/icons/homeIcons.manifest --sizes 24,32 \
     --out src/components/icons/homeIcons.h
   ```

   The generator calls `rsvg-convert`. Where only Inkscape is installed, a small `rsvg-convert` script in front of
   `PATH` that runs `inkscape --export-type=png --export-width=N --export-height=N --export-filename=- file.svg`
   does the same job.
3. Use the symbol `icon_<alias>_24` (or `_32`) through `freeink::ui::bitmapFromIcon()`, or `homeIcon(UIIcon, size)`
   for the ones that have a `UIIcon`.

The reader panel's tabs, its back and backspace buttons and the picture buttons of the alignment and orientation
choices (`align_*`, `orient_*`) come from here.

## Legacy "style B" headers

The other headers (`listIcons.h`, `keyboardIcons.h`, `touchHeaderIcons.h`, the frontlight and stats ones) were drawn
by `scripts/generate_neuink_icons.py`, which redraws them from vectors. They are still used by the other themes and
screens; when the last user of one of their icons is gone, remove the icon from the header, the script's `SYMBOLS`
and its manifest line.

NeuInk does not include the Tabler icon source (the icons it uses are already
converted). To generate a new Tabler icon, add it first:

```sh
git submodule add https://github.com/tabler/tabler-icons.git assets/tabler-icons
```

## Generate an icon header

Use one library per manifest and generated header. Each manifest maps a C++
alias to an SVG filename without `.svg`:

```ini
tabler-settings = settings
```

Generate it with the firmware wrapper, which selects the SVG source and calls
the existing FreeInk icon generator:

```sh
python3 scripts/generate_icons.py --library tabler \
  --manifest src/components/icons/tablerIcons.manifest \
  --sizes 28 --out src/components/icons/tablerIcons.h
clang-format -i src/components/icons/tablerIcons.h
```

Use `--library lucide` for existing Lucide manifests. The generated `Icon`
objects are library-neutral FreeInk assets, so use them with
`freeink::ui::bitmapFromIcon()` exactly like the existing Lucide icons:

Use `--library tabler-filled` for filled Tabler icons:

```cpp
target.bitmap(rect, freeink::ui::bitmapFromIcon(icon_tabler_settings_28));
```

Do not edit generated `.h` files by hand. Add the SVG alias to its manifest,
regenerate the header, and include that header only in the translation units
that use its icons. This avoids duplicating bitmap data across the firmware.
