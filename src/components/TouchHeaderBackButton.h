#pragma once

#include <FreeInkUIGfxRenderer.h>

#include "GfxRenderer.h"
#include "MappedInputManager.h"
#include "themes/BaseTheme.h"

namespace TouchHeaderBackButton {

constexpr int ICON_SIZE = 32;
// Keep navigation controls close to the divider while leaving the status row unchanged.
constexpr int TITLE_VERTICAL_OFFSET = 11;

struct Layout {
  Rect iconRect;
  Rect touchRect;
  int titleX;
};

Layout layout(const Rect& header);
int height(const ThemeMetrics& metrics, const MappedInputManager& input);
Rect headerRect(const GfxRenderer& renderer, const MappedInputManager& input);
// The y of the middle of the title row of a header drawn by draw()/drawCompact(): where the back arrow and the
// title are centred. Icons at the right end of the bar use it to sit on the same line.
int titleCenterY(const Rect& header, int verticalOffset = TITLE_VERTICAL_OFFSET);
Rect standardHeaderRect(const GfxRenderer& renderer);
Rect compactHeaderRect(const GfxRenderer& renderer);
bool wasTapped(const MappedInputManager& input, const Rect& header);
bool wasTapped(const MappedInputManager& input, const GfxRenderer& renderer);
void draw(GfxRenderer& renderer, const Rect& header, const char* title, bool readerContext, int rightReserve = 0,
          const char* subtitle = nullptr, int verticalOffset = TITLE_VERTICAL_OFFSET, bool showStatus = true);
void draw(GfxRenderer& renderer, freeink::ui::GfxRendererTarget& target, const Rect& header, const char* title,
          bool readerContext, int rightReserve = 0, const char* subtitle = nullptr,
          int verticalOffset = TITLE_VERTICAL_OFFSET, bool showStatus = true);
// Draws a reader-context list header: with the back arrow on a touch screen, plain otherwise.
void drawReaderHeader(GfxRenderer& renderer, freeink::ui::GfxRendererTarget& target, const MappedInputManager& input,
                      const Rect& header, const char* title);
// The standard page header, as the settings pages draw it: with the back arrow on a touch screen. Returns it.
Rect drawPageHeader(GfxRenderer& renderer, const MappedInputManager& input, const char* title);
void drawCompact(GfxRenderer& renderer, const char* title, bool readerContext = false, bool showDate = false,
                 int verticalOffset = TITLE_VERTICAL_OFFSET);

}  // namespace TouchHeaderBackButton
