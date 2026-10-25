/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Field.h"
#include "Format.h"

namespace portal {

bool Field::shown(Access access) const
{
    switch (mShow)
    {
    case Show::Public: return true;
    case Show::Local: return access == Access::Local;
    default: return false;
    }
}

CounterField::CounterField(const char *key, const char *label, const char *unit,
                           uint32_t scale, uint8_t decimals, Getter get, Setter store)
    : ValueField(key, label, unit, std::move(get), std::move(store)),
      mScale(scale), mDecimals(decimals) {}

std::string CounterField::text() const
{
    return formatScaled(mGet(), mScale, mDecimals);
}

bool CounterField::stage(const std::string &text)
{
    uint32_t value = 0;
    if (!parseScaled(text, mScale, value))
        return false;

    staged(value, text);
    return true;
}

size_t CounterField::maxLength() const
{
    // uint32_t digits, the separator and decimals with some spare for spaces
    return 10 + 1 + mDecimals + 4;
}

FloatField::FloatField(const char *key, const char *label, const char *unit,
                       uint8_t decimals, Getter get, Setter store)
    : ValueField(key, label, unit, std::move(get), std::move(store)), mDecimals(decimals) {}

std::string FloatField::text() const
{
    return formatFloat(mGet(), mDecimals);
}

bool FloatField::stage(const std::string &text)
{
    float value = 0;
    if (!parseFloat(text, value))
        return false;

    staged(value, text);
    return true;
}

TextField::TextField(const char *key, const char *label, size_t maxLength, Getter get, Setter store)
    : ValueField(key, label, nullptr, std::move(get), std::move(store)), mMaxLength(maxLength) {}

bool TextField::stage(const std::string &text)
{
    if (!validText(text, mMaxLength))
        return false;

    staged(text, text);
    return true;
}

} // namespace portal
