/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Image.h"

namespace portal {

namespace {

constexpr uint8_t IMAGE_MAGIC = 0xe9;

// CRC of eboot and elf2bin.py: CRC-32 polynomial, MSB first, no reflection
uint32_t crcUpdate(uint32_t crc, uint8_t byte)
{
    for (uint8_t mask = 0x80; mask; mask >>= 1)
    {
        bool bit = crc & 0x80000000;
        if (byte & mask)
            bit = !bit;

        crc <<= 1;
        if (bit)
            crc ^= 0x04c11db7;
    }

    return crc;
}

} // namespace

void ImageCheck::begin()
{
    *this = ImageCheck();
}

bool ImageCheck::feed(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size && !mError; ++i, ++mOffset)
    {
        uint8_t byte = data[i];

        if (mOffset == 0 && byte != IMAGE_MAGIC)
            mError = "Not an ESP8266 firmware";
        else if (mOffset == SKETCH_OFFSET && byte != IMAGE_MAGIC)
            mError = "No sketch in the firmware";
        else if (mOffset >= HEADER_END && mOffset >= mSize)
            mError = "The firmware is longer than its header says";

        // The size and the CRC are taken as zeros for the CRC
        if (mOffset >= SIZE_OFFSET && mOffset < CRC_OFFSET)
        {
            mSize |= uint32_t(byte) << (8 * (mOffset - SIZE_OFFSET));
            byte = 0;
        }
        else if (mOffset >= CRC_OFFSET && mOffset < HEADER_END)
        {
            mStoredCrc |= uint32_t(byte) << (8 * (mOffset - CRC_OFFSET));
            byte = 0;
        }

        mCrc = crcUpdate(mCrc, byte);
    }

    return !mError;
}

bool ImageCheck::valid() const
{
    return !error();
}

const char *ImageCheck::error() const
{
    if (mError)
        return mError;
    if (mOffset < HEADER_END || mOffset != mSize)
        return "The firmware is incomplete";
    if (mCrc != mStoredCrc)
        return "The firmware is damaged, its CRC doesn't match";

    return nullptr;
}

} // namespace portal
