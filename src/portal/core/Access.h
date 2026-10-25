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

namespace portal {

// The access level is the port a request came to, there are no passwords:
// - Local: the main port, for the LAN; everything is available.
// - Public: the port published to the internet; only the dashboard and
//   its data are readable, nothing can be changed.
enum class Access : uint8_t {
    Public,
    Local,
};

// Whether a server of the given level serves a route or shows an element
// of the given level
constexpr bool serves(Access server, Access element)
{
    return server == Access::Local || element == Access::Public;
}

} // namespace portal
