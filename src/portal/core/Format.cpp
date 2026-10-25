/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "Format.h"

namespace portal {

namespace {

uint64_t pow10(int digits)
{
    uint64_t value = 1;
    while (digits-- > 0)
        value *= 10;
    return value;
}

// Text without leading/trailing spaces, with ',' replaced by '.'
std::string normalize(const std::string &text)
{
    size_t first = text.find_first_not_of(" \t");
    if (first == std::string::npos)
        return {};

    size_t last = text.find_last_not_of(" \t");
    std::string result = text.substr(first, last - first + 1);

    for (auto &c : result)
    {
        if (c == ',')
            c = '.';
    }

    return result;
}

} // namespace

int scaleDigits(uint32_t scale)
{
    int digits = 0;

    for (uint32_t value = scale; value > 1; value /= 10, ++digits)
    {
        if (value % 10)
            return -1;
    }

    return (scale == 0) ? -1 : digits;
}

std::string formatScaled(uint32_t value, uint32_t scale, uint8_t decimals)
{
    int digits = scaleDigits(scale);
    if (digits < 0)
        return {};

    uint64_t units = value;
    int fraction = digits;

    if (decimals < digits)
    {
        uint64_t divider = pow10(digits - decimals);
        units = (units + divider / 2) / divider;
        fraction = decimals;
    }

    uint64_t divider = pow10(fraction);
    unsigned long whole = static_cast<unsigned long>(units / divider);
    unsigned long part = static_cast<unsigned long>(units % divider);

    char buffer[48];
    if (fraction == 0)
        snprintf(buffer, sizeof(buffer), "%lu", whole);
    else
        // A uint32_t scale has at most 9 digits
        snprintf(buffer, sizeof(buffer), "%lu.%0*lu", whole, std::min(fraction, 9), part);

    std::string result = buffer;

    // Decimals over the scale digits are zeros: 5 with scale 10 is "0.500"
    if (decimals > fraction)
    {
        if (fraction == 0)
            result += '.';
        result.append(decimals - fraction, '0');
    }

    return result;
}

bool parseScaled(const std::string &text, uint32_t scale, uint32_t &value)
{
    int digits = scaleDigits(scale);
    std::string input = normalize(text);

    if (digits < 0 || input.empty())
        return false;

    uint64_t whole = 0;
    uint64_t part = 0;
    int partDigits = 0;
    bool roundUp = false;
    bool point = false;
    bool any = false;

    for (char c : input)
    {
        if (c == '.')
        {
            if (point)
                return false;
            point = true;
            continue;
        }

        if (c < '0' || c > '9')
            return false;

        any = true;
        int digit = c - '0';

        if (!point)
        {
            whole = whole * 10 + digit;
            if (whole > std::numeric_limits<uint32_t>::max())
                return false;
        }
        else if (partDigits < digits)
        {
            part = part * 10 + digit;
            ++partDigits;
        }
        else if (partDigits == digits)
        {
            // The first dropped digit decides the rounding
            roundUp = (digit >= 5);
            ++partDigits;
        }
    }

    if (!any)
        return false;

    if (partDigits < digits)
        part *= pow10(digits - partDigits);

    uint64_t result = whole * scale + part + (roundUp ? 1 : 0);
    if (result > std::numeric_limits<uint32_t>::max())
        return false;

    value = static_cast<uint32_t>(result);
    return true;
}

std::string formatFloat(float value, uint8_t decimals)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", decimals, static_cast<double>(value));
    return buffer;
}

bool parseFloat(const std::string &text, float &value)
{
    std::string input = normalize(text);
    if (input.empty())
        return false;

    // strtof() also takes "inf", "nan" and hex, the form needs plain numbers
    for (char c : input)
    {
        if (!((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E'))
            return false;
    }

    char *end = nullptr;
    float result = strtof(input.c_str(), &end);
    if (end == input.c_str() || *end != '\0' || !std::isfinite(result))
        return false;

    value = result;
    return true;
}

std::string formatUsage(uint32_t part, uint32_t total)
{
    unsigned long percent = total ? static_cast<unsigned long>((uint64_t(part) * 100 + total / 2) / total) : 0;

    char buffer[48];
    snprintf(buffer, sizeof(buffer), "%lu / %lu B · %lu%%", static_cast<unsigned long>(part),
             static_cast<unsigned long>(total), percent);
    return buffer;
}

bool validText(const std::string &text, size_t maxLength)
{
    if (text.size() > maxLength)
        return false;

    for (char c : text)
    {
        unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7f)
            return false;
    }

    return true;
}

} // namespace portal
