#pragma once

#include <Epub/EpubRenderMode.h>
#include <Epub/Section.h>

#include <cstdint>
#include <memory>

// How one attempt at building a chapter section is laid out.
struct SectionBuildProfile {
  EpubRenderMode renderMode;
  bool embeddedStyle;
  bool focusReadingEnabled;
  bool guideReadingEnabled;
  const char* label;
  bool safeMode;
};

// What one load-or-build of the chapter section needs and learns along the way.
struct SectionLoad {
  uint16_t viewportWidth = 0;
  uint16_t viewportHeight = 0;
  int readerFontId = 0;
  EpubRenderMode selectedRenderMode = EpubRenderMode::CrossInkDefault;
  EpubRenderMode usedRenderMode = EpubRenderMode::CrossInkDefault;
  EpubRenderMode lastAttemptedRenderMode = EpubRenderMode::CrossInkDefault;
  bool fullSectionIndexing = false;
  bool buildingFootnotePreview = false;
  bool safeModeBuildSucceeded = false;
  bool layoutAbortedForLowMemory = false;
  bool lastAttemptUsedSafeMode = false;
  bool buildCancelledForBack = false;
  // A readable partial cache kept alive until its full replacement has been built.
  std::unique_ptr<Section> readablePartialFallback;
};
