// Copyright 2026 Room Critter contributors. SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace room_critter {
inline bool packDepth(const uint8_t* data, int bytes, int width, int height,
                      int pixelStride, int rowStride, std::vector<uint8_t>& packed) {
  if (!data || width <= 0 || height <= 0 || pixelStride < 2 || bytes < 0 ||
      rowStride < int64_t(width) * pixelStride ||
      int64_t(bytes) < int64_t(height - 1) * rowStride +
          int64_t(width - 1) * pixelStride + 2) return false;
  packed.resize(size_t(width) * height * 2);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      std::memcpy(&packed[(size_t(y) * width + x) * 2],
                  data + size_t(y) * rowStride + size_t(x) * pixelStride, 2);
  return true;
}
}
