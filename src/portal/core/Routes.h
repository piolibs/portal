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
#include <string>
#include <vector>

#include "Access.h"

namespace portal {

enum class Page : uint8_t {
    Dashboard,
    Values,
    Style,
    Script,
    Settings,
    SettingsSave,
    Wifi,
    WifiSave,
    WifiScan,
    Info,
    Restart,
    Update,
    UpdateUpload,
};

enum class Method : uint8_t {
    Get,
    Post,
};

struct Route {
    const char *path;
    Method method;
    Page page;
    Access access;
};

// All built-in routes. A server registers only the routes of its level, so
// on the public port the other pages don't exist (404) rather than being
// refused, and nothing there accepts a POST.
const std::vector<Route> &routes();

// Routes registered on a server of the given level
std::vector<Route> routes(Access server);

struct Network;

// Networks for the WiFi page: without hidden ones and duplicates (the
// strongest access point is kept), the strongest first, at most limit
std::vector<Network> networks(std::vector<Network> scan, size_t limit);

// SSID is 1..32 bytes, a WPA password is 8..63 characters or 64 hex digits,
// an empty password is an open network
bool validSsid(const std::string &ssid);
bool validPassword(const std::string &password);

} // namespace portal
