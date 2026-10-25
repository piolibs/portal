/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <cstdint>
#include <string>

namespace portal {

// Number of decimal digits of a power of ten scale (1000 -> 3), -1 otherwise
int scaleDigits(uint32_t scale);

// Counter in units shown in scale units, e.g. 385990 L with scale 1000 and
// 3 decimals is "385.990" m3. The value is rounded half up to the decimals.
std::string formatScaled(uint32_t value, uint32_t scale, uint8_t decimals);

// Reverse of formatScaled(): "385.99" -> 385990. Accepts '.' or ',' as the
// decimal separator, rounds extra decimals half up, rejects negative values,
// garbage and values over uint32_t. Integer arithmetic, so totals above 2^24
// don't lose units as they would through a float.
bool parseScaled(const std::string &text, uint32_t scale, uint32_t &value);

std::string formatFloat(float value, uint8_t decimals);

// Accepts '.' or ',' as the decimal separator, rejects garbage, inf and nan
bool parseFloat(const std::string &text, float &value);

// Part of a total in bytes with the percentage: "377088 / 3141888 B · 12%"
std::string formatUsage(uint32_t part, uint32_t total);

// Not longer than maxLength bytes and without control characters
bool validText(const std::string &text, size_t maxLength);

} // namespace portal
