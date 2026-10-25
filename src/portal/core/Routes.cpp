/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <algorithm>
#include <cctype>

#include "Render.h"
#include "Routes.h"

namespace portal {

const std::vector<Route> &routes()
{
    static const std::vector<Route> all = {
        {"/", Method::Get, Page::Dashboard, Access::Public},
        {"/api/values", Method::Get, Page::Values, Access::Public},
        {"/style.css", Method::Get, Page::Style, Access::Public},
        {"/app.js", Method::Get, Page::Script, Access::Public},
        {"/edit", Method::Get, Page::Settings, Access::Local},
        {"/edit", Method::Post, Page::SettingsSave, Access::Local},
        {"/wifi", Method::Get, Page::Wifi, Access::Local},
        {"/wifi", Method::Post, Page::WifiSave, Access::Local},
        {"/wifi/scan", Method::Post, Page::WifiScan, Access::Local},
        {"/info", Method::Get, Page::Info, Access::Local},
        {"/restart", Method::Post, Page::Restart, Access::Local},
        {"/update", Method::Get, Page::Update, Access::Local},
        {"/update", Method::Post, Page::UpdateUpload, Access::Local},
    };

    return all;
}

std::vector<Route> routes(Access server)
{
    std::vector<Route> result;

    for (const auto &route : routes())
    {
        if (serves(server, route.access))
            result.push_back(route);
    }

    return result;
}

std::vector<Network> networks(std::vector<Network> scan, size_t limit)
{
    std::erase_if(scan, [](const Network &network) { return network.ssid.empty(); });

    std::stable_sort(scan.begin(), scan.end(), [](const Network &a, const Network &b) {
        return a.rssi > b.rssi;
    });

    std::vector<Network> result;
    for (auto &network : scan)
    {
        if (result.size() >= limit)
            break;

        bool known = std::any_of(result.begin(), result.end(), [&](const Network &other) {
            return other.ssid == network.ssid;
        });
        if (!known)
            result.push_back(std::move(network));
    }

    return result;
}

bool validSsid(const std::string &ssid)
{
    return !ssid.empty() && ssid.size() <= 32;
}

bool validPassword(const std::string &password)
{
    if (password.empty())
        return true;

    if (password.size() == 64)
    {
        return std::all_of(password.begin(), password.end(), [](char c) {
            return std::isxdigit(static_cast<unsigned char>(c));
        });
    }

    return password.size() >= 8 && password.size() <= 63;
}

} // namespace portal
