/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace portal {

/*
 * Check of an uploaded ESP8266 Arduino firmware, fed while it is uploaded.
 *
 * The core's updater checks the first byte and the flash size only: a wrong
 * or damaged file is flashed, and the device boots garbage until it is
 * flashed by wire. elf2bin.py builds firmware.bin of the eboot image at 0,
 * the sketch image at 0x1000 and stores the file size and the CRC of the
 * whole file (with these 8 bytes zeroed) at 0x1010: the update is committed
 * only when they match.
 */
class ImageCheck {
public:
    void begin();

    // False as soon as the data can't be a firmware
    bool feed(const uint8_t *data, size_t size);

    // The whole firmware is fed and its size and CRC match
    bool valid() const;

    // Why the firmware is not valid, nullptr if it is
    const char *error() const;

private:
    static constexpr uint32_t SKETCH_OFFSET = 0x1000;
    static constexpr uint32_t SIZE_OFFSET = SKETCH_OFFSET + 16;
    static constexpr uint32_t CRC_OFFSET = SKETCH_OFFSET + 20;
    static constexpr uint32_t HEADER_END = CRC_OFFSET + 4;

    uint32_t mOffset = 0;
    uint32_t mCrc = 0xffffffff;
    uint32_t mSize = 0;
    uint32_t mStoredCrc = 0;
    const char *mError = nullptr;
};

} // namespace portal
