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

/*
 * Token of the local forms, so another site opened in a LAN browser can't
 * post them (the local port has no password). It is random per boot and
 * every change (settings, WiFi, update, restart) must send it back.
 */
class Csrf {
public:
    static constexpr const char *ARG = "t";

    // Random words, e.g. from the hardware generator
    void init(uint32_t high, uint32_t low);

    const std::string &token() const { return mToken; }

    // Constant time compare, always false before init()
    bool check(const std::string &value) const;

private:
    std::string mToken;
};

} // namespace portal
