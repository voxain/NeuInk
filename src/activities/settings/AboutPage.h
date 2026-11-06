#pragma once

#include <memory>

#include "activities/Activity.h"

// Settings > About: the build, the device, memory and storage, and a few reading totals - read-only rows.
std::unique_ptr<Activity> makeAboutPage(GfxRenderer& renderer, MappedInputManager& mappedInput);
