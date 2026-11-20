// Copyright 2026 Room Critter contributors. SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cmath>
namespace room_critter {
struct Motion { float x, y, heading; };
inline Motion motion(float seconds) {
  const float phase = seconds * .7f;
  return {.35f * std::sin(phase), .015f * std::abs(std::sin(seconds * 6.f)),
          std::cos(phase) >= 0 ? 1.5708f : -1.5708f};
}
}
