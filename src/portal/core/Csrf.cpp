/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <cstdio>

#include "Csrf.h"

namespace portal {

void Csrf::init(uint32_t high, uint32_t low)
{
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%08lx%08lx",
             static_cast<unsigned long>(high), static_cast<unsigned long>(low));
    mToken = buffer;
}

bool Csrf::check(const std::string &value) const
{
    if (mToken.empty() || value.size() != mToken.size())
        return false;

    unsigned char diff = 0;
    for (size_t i = 0; i < mToken.size(); ++i)
        diff |= static_cast<unsigned char>(mToken[i] ^ value[i]);

    return diff == 0;
}

} // namespace portal
