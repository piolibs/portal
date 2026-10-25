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
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Access.h"
#include "Field.h"
#include "Form.h"
#include "Writer.h"

namespace portal {

// Page frame: the device name, the access level and the current menu path.
// The dashboard header also shows the subtitle (e.g. network and address)
// and the badge (e.g. "Online") when they are set, the local dashboard the
// firmware version.
struct Layout {
    const char *name;
    Access access;
    const char *path;
    const char *subtitle = nullptr;
    const char *badge = nullptr;
    const char *version = nullptr;
};

// Extra dashboard line computed on each render, e.g. the device time
struct Item {
    const char *label;
    std::function<std::string()> value;
    Access access;
};

using Items = std::vector<Item>;

// Button on the local dashboard, posts the form token to the path. The
// button shows busy while the action runs, the dashboard shows done after it.
struct Action {
    const char *path;
    const char *label;
    const char *busy;
    const char *done;
    std::function<void()> run;
};

using Actions = std::vector<Action>;
using Rows = std::vector<std::pair<std::string, std::string>>;
// Info page groups: a heading and its rows
using Sections = std::vector<std::pair<const char *, Rows>>;

struct Network {
    std::string ssid;
    int32_t rssi;
    bool open;
};

// State of the WiFi page
struct Wifi {
    // Network name, empty if not connected; detail is shown under it
    std::string connected;
    std::string detail;
    // Networks of the last scan
    std::vector<Network> networks;
    bool scanning = false;
    // Seconds since the last scan, negative if there was none
    long scanAge = -1;
};

// Static files, served from flash
extern const char STYLE_CSS[] PROGMEM;
extern const char APP_JS[] PROGMEM;

// Actions are shown on the local dashboard only; done is the path of the
// action that has just run, its message is shown above its button
void renderDashboard(Writer &w, const Layout &layout, const Fields &fields, const Items &items,
                     const Actions &actions = {}, const std::string &token = {},
                     const std::string &done = {});

// {"key":"text",...} of the fields shown at the access level
void renderValues(Writer &w, const Fields &fields, Access access);

// Settings form; result is the rejected submit to show its values and errors
void renderSettings(Writer &w, const Layout &layout, const Fields &fields,
                    const std::string &token, const FormResult *result, const char *message);

// Networks are scanned by the Scan button only; a scanning page reloads
// itself until the scan is done
void renderWifi(Writer &w, const Layout &layout, const Wifi &wifi, const std::string &token,
                const char *message);

// "just now", "5 min ago", "2 h ago"
std::string formatAge(long seconds);

void renderInfo(Writer &w, const Layout &layout, const Sections &sections, const std::string &token);

void renderUpdate(Writer &w, const Layout &layout, const std::string &token, const char *version,
                  const char *message);

// Short page after an action; refresh > 0 goes to the dashboard after that many seconds
void renderMessage(Writer &w, const Layout &layout, const char *heading, const std::string &text,
                   unsigned refresh = 0);

// Version of the static files, added to their links
const char *assetsVersion();

// Signal bars, 1..4, of a network
int signalLevel(int32_t rssi);

} // namespace portal
