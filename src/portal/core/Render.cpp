/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <cstdio>
#include <cstring>

#include "Csrf.h"
#include "Render.h"

namespace portal {

namespace {

// Icons as drawn in the design: size, stroke and colors in attributes, so they
// keep their look whatever the stylesheet; the stylesheet only adapts the
// colors to the dark theme
const char ICON_HOME[] PROGMEM =
    "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M3 11l9-7 9 7\">"
    "</path>"
    "<path d=\"M5 10v10h14V10\">"
    "</path>"
    "</svg>";
const char ICON_SETTINGS[] PROGMEM =
    "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M4 6h10M18 6h2M4 12h4M12 12h8M4 18h12\">"
    "</path>"
    "<circle cx=\"16\" cy=\"6\" r=\"2\">"
    "</circle>"
    "<circle cx=\"10\" cy=\"12\" r=\"2\">"
    "</circle>"
    "<circle cx=\"18\" cy=\"18\" r=\"2\">"
    "</circle>"
    "</svg>";
const char ICON_WIFI[] PROGMEM =
    "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M2 9a15 15 0 0 1 20 0\">"
    "</path>"
    "<path d=\"M5.5 12.5a10 10 0 0 1 13 0\">"
    "</path>"
    "<path d=\"M9 16a5 5 0 0 1 6 0\">"
    "</path>"
    "<circle cx=\"12\" cy=\"19.5\" r=\"1\">"
    "</circle>"
    "</svg>";
const char ICON_INFO[] PROGMEM =
    "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<circle cx=\"12\" cy=\"12\" r=\"9\">"
    "</circle>"
    "<path d=\"M12 11v6\">"
    "</path>"
    "<circle cx=\"12\" cy=\"7.5\" r=\"0.6\">"
    "</circle>"
    "</svg>";
const char ICON_UPDATE[] PROGMEM =
    "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M12 16V4\">"
    "</path>"
    "<path d=\"M7 9l5-5 5 5\">"
    "</path>"
    "<path d=\"M4 16v4h16v-4\">"
    "</path>"
    "</svg>";
const char ICON_DOT[] PROGMEM =
    "<svg width=\"8\" height=\"8\" viewBox=\"0 0 8 8\" aria-hidden=\"true\">"
    "<circle cx=\"4\" cy=\"4\" r=\"4\" fill=\"#1A7F37\">"
    "</circle>"
    "</svg>";
const char ICON_EYE[] PROGMEM =
    "<svg width=\"14\" height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12z\">"
    "</path>"
    "<circle cx=\"12\" cy=\"12\" r=\"3\">"
    "</circle>"
    "</svg>";
const char ICON_CHECK[] PROGMEM =
    "<svg width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2.2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M5 12.5l4.5 4.5L19 7.5\">"
    "</path>"
    "</svg>";
const char ICON_WIFI_STATUS[] PROGMEM =
    "<svg width=\"22\" height=\"22\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#1A7F37\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M2 9a15 15 0 0 1 20 0\">"
    "</path>"
    "<path d=\"M5.5 12.5a10 10 0 0 1 13 0\">"
    "</path>"
    "<path d=\"M9 16a5 5 0 0 1 6 0\">"
    "</path>"
    "<circle cx=\"12\" cy=\"19.5\" r=\"1\">"
    "</circle>"
    "</svg>";
const char ICON_WIFI_OFF[] PROGMEM =
    "<svg width=\"22\" height=\"22\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#57606A\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M2 9a15 15 0 0 1 20 0\"></path><path d=\"M5.5 12.5a10 10 0 0 1 13 0\"></path>"
    "<path d=\"M9 16a5 5 0 0 1 6 0\"></path><circle cx=\"12\" cy=\"19.5\" r=\"1\"></circle></svg>";
const char ICON_SCAN[] PROGMEM =
    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M20 11a8 8 0 1 0-2.34 5.66\"></path><path d=\"M20 4v7h-7\"></path></svg>";
const char ICON_LOCK[] PROGMEM =
    "<svg width=\"14\" height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-label=\"Secured\" role=\"img\">"
    "<rect x=\"5\" y=\"11\" width=\"14\" height=\"10\" rx=\"2\">"
    "</rect>"
    "<path d=\"M8 11V8a4 4 0 0 1 8 0v3\">"
    "</path>"
    "</svg>";
const char ICON_FILE[] PROGMEM =
    "<svg width=\"32\" height=\"32\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#0969DA\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z\">"
    "</path>"
    "<path d=\"M14 3v5h5\">"
    "</path>"
    "<path d=\"M12 17v-6\">"
    "</path>"
    "<path d=\"M9.5 13.5L12 11l2.5 2.5\">"
    "</path>"
    "</svg>";
const char ICON_WARN[] PROGMEM =
    "<svg width=\"20\" height=\"20\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
    "<path d=\"M12 3l10 18H2z\">"
    "</path>"
    "<path d=\"M12 10v5\">"
    "</path>"
    "<circle cx=\"12\" cy=\"18\" r=\"0.6\">"
    "</circle>"
    "</svg>";

struct MenuItem {
    const char *path;
    const char *title;
    PGM_P icon;
};

const MenuItem MENU[] = {
    {"/", "Home", ICON_HOME},
    {"/edit", "Settings", ICON_SETTINGS},
    {"/wifi", "WiFi", ICON_WIFI},
    {"/info", "Info", ICON_INFO},
    {"/update", "Update", ICON_UPDATE},
};

void icon(Writer &w, PGM_P svg)
{
    w.printP(svg);
}

void head(Writer &w, const Layout &layout, unsigned refresh, const char *target = "/")
{
    // No translation: the browser would try it through the setup access
    // point without internet and report an error
    w.printP(PSTR("<!DOCTYPE html><html lang=\"en\" translate=\"no\"><head><meta charset=\"utf-8\">"
                  "<meta name=\"google\" content=\"notranslate\">"
                  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\">"
                  "<meta name=\"format-detection\" content=\"telephone=no\">"));

    if (refresh)
    {
        char buffer[80];
        snprintf(buffer, sizeof(buffer), "<meta http-equiv=\"refresh\" content=\"%u;url=%s\">", refresh, target);
        w.print(buffer);
    }

    w.printP(PSTR("<title>"));
    w.html(layout.name);
    // The version changes with the files: a browser doesn't keep the
    // stylesheet of the previous firmware
    w.printP(PSTR("</title><link rel=\"stylesheet\" href=\"/style.css?v="));
    w.print(assetsVersion());
    w.printP(PSTR("\"></head>"));

    // The local pages have the menu fixed at the bottom
    w.print(layout.access == Access::Local ? "<body class=\"nav\">" : "<body>");
}

// Header of a page: its title and the device name
void pageBegin(Writer &w, const Layout &layout, const char *heading, unsigned refresh = 0,
               const char *target = "/")
{
    head(w, layout, refresh, target);

    w.printP(PSTR("<header><h1>"));
    w.html(heading);
    w.printP(PSTR("</h1><span class=\"dev\">"));
    w.html(layout.name);
    w.printP(PSTR("</span></header><main>"));
}

void pageEnd(Writer &w, const Layout &layout)
{
    w.printP(PSTR("</main>"));

    // The public dashboard has no menu: the other pages don't exist there
    if (layout.access == Access::Local)
    {
        w.printP(PSTR("<nav aria-label=\"Pages\">"));
        for (const auto &item : MENU)
        {
            bool current = strcmp(item.path, layout.path) == 0;

            w.printP(PSTR("<a href=\""));
            w.print(item.path);
            w.print(current ? "\" class=\"on\" aria-current=\"page\">" : "\">");
            icon(w, item.icon);
            w.print(item.title);
            w.printP(PSTR("</a>"));
        }
        w.printP(PSTR("</nav>"));
    }

    w.printP(PSTR("<script src=\"/app.js?v="));
    w.print(assetsVersion());
    w.printP(PSTR("\"></script></body></html>"));
}

void message(Writer &w, const char *text, bool error)
{
    if (!text || !*text)
        return;

    if (error)
    {
        w.printP(PSTR("<p class=\"msg bad\" role=\"alert\">"));
    }
    else
    {
        w.printP(PSTR("<p class=\"msg\" role=\"status\">"));
        icon(w, ICON_CHECK);
    }
    w.html(text);
    w.printP(PSTR("</p>"));
}

void tokenInput(Writer &w, const std::string &token)
{
    w.printP(PSTR("<input type=\"hidden\" name=\""));
    w.print(Csrf::ARG);
    w.printP(PSTR("\" value=\""));
    w.html(token);
    w.printP(PSTR("\">"));
}

void row(Writer &w, const std::string &label, const std::string &value)
{
    w.printP(PSTR("<div class=\"row\"><span>"));
    w.html(label);
    w.printP(PSTR("</span><span>"));
    w.html(value);
    w.printP(PSTR("</span></div>"));
}

// Group heading when a field starts a new section
void section(Writer &w, const Field &field, const char *&current)
{
    const char *heading = field.heading();
    if (!heading || (current && strcmp(heading, current) == 0))
        return;

    current = heading;
    w.printP(PSTR("<h2>"));
    w.html(heading);
    w.printP(PSTR("</h2>"));
}

void signal(Writer &w, int32_t rssi, bool selected)
{
    int level = signalLevel(rssi);

    char buffer[160];
    snprintf(buffer, sizeof(buffer),
             "<svg class=\"sig\" width=\"20\" height=\"14\" viewBox=\"0 0 20 14\" role=\"img\" "
             "aria-label=\"Signal %d of 4, %ld dBm\">",
             level, static_cast<long>(rssi));
    w.print(buffer);

    static const char *const ON = " class=\"on\"";
    for (int bar = 0; bar < 4; ++bar)
    {
        bool on = bar < level;
        snprintf(buffer, sizeof(buffer), "<rect%s x=\"%d\" y=\"%d\" width=\"3\" height=\"%d\" rx=\"1\" fill=\"%s\"></rect>",
                 on ? ON : "", bar * 5, 10 - bar * 3 - (bar == 3 ? 1 : 0), 4 + bar * 3 + (bar == 3 ? 1 : 0),
                 !on ? "#D0D7DE" : (selected ? "#0969DA" : "#57606A"));
        w.print(buffer);
    }

    w.printP(PSTR("</svg>"));
}

} // namespace

const char *assetsVersion()
{
    static char version[9] = {};

    if (!version[0])
    {
        // FNV-1a of the static files
        uint32_t hash = 2166136261u;
        for (PGM_P file : {STYLE_CSS, APP_JS})
        {
            for (size_t i = 0, size = strlen_P(file); i < size; ++i)
            {
                char c;
                memcpy_P(&c, file + i, 1);
                hash = (hash ^ static_cast<unsigned char>(c)) * 16777619u;
            }
        }
        snprintf(version, sizeof(version), "%08lx", static_cast<unsigned long>(hash));
    }

    return version;
}

int signalLevel(int32_t rssi)
{
    if (rssi >= -55)
        return 4;
    if (rssi >= -65)
        return 3;
    if (rssi >= -75)
        return 2;
    return 1;
}

void renderDashboard(Writer &w, const Layout &layout, const Fields &fields, const Items &items,
                     const Actions &actions, const std::string &token, const std::string &done)
{
    head(w, layout, 0);

    w.printP(PSTR("<header><div><h1>"));
    w.html(layout.name);
    w.printP(PSTR("</h1>"));
    if (layout.access == Access::Local && layout.subtitle && *layout.subtitle)
    {
        w.printP(PSTR("<small>"));
        w.html(layout.subtitle);
        w.printP(PSTR("</small>"));
    }
    w.printP(PSTR("</div>"));

    if (layout.access == Access::Public)
    {
        w.printP(PSTR("<span class=\"pill\">"));
        icon(w, ICON_EYE);
        w.printP(PSTR("View only</span>"));
    }
    else if (layout.badge && *layout.badge)
    {
        w.printP(PSTR("<span class=\"pill ok\">"));
        icon(w, ICON_DOT);
        w.html(layout.badge);
        w.printP(PSTR("</span>"));
    }
    w.printP(PSTR("</header><main>"));

    bool values = false;
    const char *current = nullptr;
    for (const auto &field : fields)
    {
        if (!field->shown(layout.access))
            continue;

        values = true;
        section(w, *field, current);

        w.printP(PSTR("<div class=\"card\"><span class=\"l\">"));
        w.html(field->label());
        w.printP(PSTR("</span><span class=\"v\"><span id=\"v-"));
        w.html(field->key());
        w.printP(PSTR("\">"));
        w.html(field->text());
        w.printP(PSTR("</span> <small>"));
        w.html(field->unit());
        w.printP(PSTR("</small></span></div>"));
    }

    bool list = false;
    for (const auto &item : items)
    {
        if (!serves(layout.access, item.access))
            continue;

        if (!list)
            w.printP(PSTR("<div class=\"list\">"));
        list = true;
        row(w, item.label, item.value ? item.value() : std::string());
    }
    if (layout.access == Access::Local && layout.version && *layout.version)
    {
        if (!list)
            w.printP(PSTR("<div class=\"list\">"));
        list = true;
        row(w, "Firmware", layout.version);
    }
    if (list)
        w.printP(PSTR("</div>"));

    // An action changes the device state: not on the public port
    if (layout.access == Access::Local)
    {
        for (const auto &action : actions)
        {
            // The script adds the time and drops ?done from the address
            if (done == action.path)
            {
                w.printP(PSTR("<p class=\"msg\" role=\"status\" data-at>"));
                icon(w, ICON_CHECK);
                w.html(action.done);
                w.printP(PSTR("</p>"));
            }

            w.printP(PSTR("<form class=\"act\" method=\"post\" action=\""));
            w.html(action.path);
            w.printP(PSTR("\">"));
            tokenInput(w, token);
            w.printP(PSTR("<button class=\"action\" data-busy=\""));
            w.html(action.busy);
            w.printP(PSTR("\">"));
            icon(w, ICON_SCAN);
            w.html(action.label);
            w.printP(PSTR("</button></form>"));
        }
    }

    if (values)
        w.printP(PSTR("<p class=\"note\">Values refresh every 30 s</p>"));

    pageEnd(w, layout);
}

void renderValues(Writer &w, const Fields &fields, Access access)
{
    bool first = true;

    w.print("{");
    for (const auto &field : fields)
    {
        if (!field->shown(access))
            continue;

        w.print(first ? "\"" : ",\"");
        w.json(field->key());
        w.print("\":\"");
        w.json(field->text());
        w.print("\"");
        first = false;
    }
    w.print("}");
}

void renderSettings(Writer &w, const Layout &layout, const Fields &fields,
                    const std::string &token, const FormResult *result, const char *text)
{
    pageBegin(w, layout, "Settings");

    if (result && !result->ok())
        message(w, "Invalid values, nothing is saved", true);
    else
        message(w, text, false);

    bool any = false;
    const char *current = nullptr;
    w.printP(PSTR("<form method=\"post\" action=\"/edit\">"));
    tokenInput(w, token);

    for (const auto &field : fields)
    {
        if (!field->editable())
            continue;

        any = true;
        section(w, *field, current);
        const std::string *error = result ? result->error(field.get()) : nullptr;

        w.printP(PSTR("<div class=\"f\"><label for=\"f-"));
        w.html(field->key());
        w.printP(PSTR("\">"));
        w.html(field->label());
        w.print(error ? "</label><div class=\"in bad\"><input id=\"f-" : "</label><div class=\"in\"><input id=\"f-");
        w.html(field->key());
        w.printP(PSTR("\" name=\""));
        w.html(field->key());
        w.printP(PSTR("\" value=\""));
        w.html(error ? *error : field->text());

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "\" maxlength=\"%u\" inputmode=\"%s\" autocomplete=\"off\"",
                 static_cast<unsigned>(field->maxLength()), field->inputMode());
        w.print(buffer);
        if (error)
        {
            w.printP(PSTR(" aria-invalid=\"true\" aria-describedby=\"e-"));
            w.html(field->key());
            w.print("\"");
        }
        w.print(">");

        if (*field->unit())
        {
            w.printP(PSTR("<span>"));
            w.html(field->unit());
            w.printP(PSTR("</span>"));
        }
        w.printP(PSTR("</div>"));

        if (error)
        {
            w.printP(PSTR("<span class=\"err\" id=\"e-"));
            w.html(field->key());
            w.printP(PSTR("\">Invalid value</span>"));
        }
        w.printP(PSTR("</div>"));
    }

    if (any)
        w.printP(PSTR("<button>Save</button>"));
    else
        w.printP(PSTR("<p class=\"note\">No settings</p>"));

    w.printP(PSTR("</form>"));
    pageEnd(w, layout);
}

std::string formatAge(long seconds)
{
    char buffer[24];

    if (seconds < 60)
        return "just now";
    if (seconds < 3600)
        snprintf(buffer, sizeof(buffer), "%ld min ago", seconds / 60);
    else
        snprintf(buffer, sizeof(buffer), "%ld h ago", seconds / 3600);

    return buffer;
}

void renderWifi(Writer &w, const Layout &layout, const Wifi &wifi, const std::string &token, const char *text)
{
    // A scan takes a few seconds: the page reloads until it is done
    pageBegin(w, layout, "WiFi", wifi.scanning ? 4 : 0, "/wifi");
    message(w, text, false);

    bool connected = !wifi.connected.empty();
    w.print(connected ? "<div class=\"card st\">" : "<div class=\"card st off\">");
    icon(w, connected ? ICON_WIFI_STATUS : ICON_WIFI_OFF);
    w.printP(PSTR("<div>"));
    if (connected)
    {
        w.printP(PSTR("<span>Connected to <b>"));
        w.html(wifi.connected);
        w.printP(PSTR("</b></span>"));
    }
    else
    {
        w.printP(PSTR("<span>Not connected</span>"));
    }
    if (!wifi.detail.empty())
    {
        w.printP(PSTR("<span class=\"l\">"));
        w.html(wifi.detail);
        w.printP(PSTR("</span>"));
    }
    w.printP(PSTR("</div></div>"));

    // Heading with the age of the list and the Scan button
    w.printP(PSTR("<div class=\"nh\"><h2>Networks"));
    if (!wifi.scanning && wifi.scanAge >= 0)
    {
        w.printP(PSTR(" <span>· "));
        w.print(formatAge(wifi.scanAge));
        w.printP(PSTR("</span>"));
    }
    w.printP(PSTR("</h2><form method=\"post\" action=\"/wifi/scan\">"));
    tokenInput(w, token);
    if (wifi.scanning)
    {
        w.printP(PSTR("<button class=\"scan\" disabled aria-disabled=\"true\">"));
        icon(w, ICON_SCAN);
        w.printP(PSTR("Scanning…</button>"));
    }
    else
    {
        w.printP(PSTR("<button class=\"scan\">"));
        icon(w, ICON_SCAN);
        w.printP(PSTR("Scan</button>"));
    }
    w.printP(PSTR("</form></div>"));

    if (wifi.scanning)
    {
        w.printP(PSTR("<div class=\"card scanning\" role=\"status\"><b>Searching for networks</b>"
                      "<progress></progress><span class=\"l\">The list updates in a few seconds. "
                      "A phone connected to the device&#39;s access point may drop for a moment.</span></div>"));
    }
    else if (wifi.scanAge < 0)
    {
        w.printP(PSTR("<div class=\"card empty\"><b>No scan yet</b><span class=\"l\">Tap Scan to find networks "
                      "nearby, or type the network name below</span></div>"));
    }
    else if (wifi.networks.empty())
    {
        w.printP(PSTR("<div class=\"card empty\"><b>No networks found</b><span class=\"l\">Scan again "
                      "or type the network name below</span></div>"));
    }
    else
    {
        // A tap fills the network field (app.js), the datalist works without it
        w.printP(PSTR("<div class=\"list nets\">"));
        for (const auto &network : wifi.networks)
        {
            bool current = network.ssid == wifi.connected;

            w.printP(PSTR("<button type=\"button\" data-s=\""));
            w.html(network.ssid);
            w.print(current ? "\" class=\"on\"><span>" : "\"><span>");
            w.html(network.ssid);
            w.printP(PSTR("</span><i>"));
            if (network.open)
                w.printP(PSTR("open"));
            else
                icon(w, ICON_LOCK);
            signal(w, network.rssi, current);
            w.printP(PSTR("</i></button>"));
        }
        w.printP(PSTR("</div>"));
    }

    w.printP(PSTR("<form method=\"post\" action=\"/wifi\">"));
    tokenInput(w, token);
    w.printP(PSTR("<div class=\"f\"><label for=\"s\">Network</label><div class=\"in\">"
                  "<input id=\"s\" name=\"s\" list=\"n\" maxlength=\"32\" required autocomplete=\"off\" value=\""));
    w.html(wifi.connected);
    w.printP(PSTR("\"></div></div><datalist id=\"n\">"));
    for (const auto &network : wifi.networks)
    {
        w.printP(PSTR("<option value=\""));
        w.html(network.ssid);
        w.printP(PSTR("\">"));
    }
    w.printP(PSTR("</datalist><div class=\"f\"><label for=\"p\">Password</label><div class=\"in\">"
                  "<input id=\"p\" name=\"p\" type=\"password\" maxlength=\"64\" autocomplete=\"off\"></div></div>"
                  "<button>Connect</button><p class=\"note\">The device joins the network and restarts</p></form>"));

    pageEnd(w, layout);
}

void renderInfo(Writer &w, const Layout &layout, const Sections &sections, const std::string &token)
{
    pageBegin(w, layout, "Info");

    for (const auto &[heading, rows] : sections)
    {
        w.printP(PSTR("<h2>"));
        w.html(heading);
        w.printP(PSTR("</h2><div class=\"list\">"));
        for (const auto &[label, value] : rows)
            row(w, label, value);
        w.printP(PSTR("</div>"));
    }

    w.printP(PSTR("<form method=\"post\" action=\"/restart\">"));
    tokenInput(w, token);
    w.printP(PSTR("<button class=\"danger\">Restart device</button></form>"));

    pageEnd(w, layout);
}

void renderUpdate(Writer &w, const Layout &layout, const std::string &token, const char *version,
                  const char *text)
{
    pageBegin(w, layout, "Update");
    message(w, text, true);

    w.printP(PSTR("<div class=\"list\">"));
    row(w, "Installed firmware", version ? version : "");
    w.printP(PSTR("</div>"));

    // A multipart body is uploaded before the handler runs, the token goes
    // in the query to be checked before the first chunk is flashed
    w.printP(PSTR("<form id=\"fw\" method=\"post\" enctype=\"multipart/form-data\" action=\"/update?"));
    w.print(Csrf::ARG);
    w.print("=");
    w.html(token);
    w.printP(PSTR("\"><label class=\"drop\" for=\"f\">"));
    icon(w, ICON_FILE);
    w.printP(PSTR("<span id=\"fn\">Choose firmware (.bin)</span><small>Tap to choose a file</small>"
                  "<input id=\"f\" name=\"firmware\" type=\"file\" accept=\".bin\" required></label>"
                  "<progress id=\"pr\" max=\"100\" value=\"0\" hidden></progress>"
                  "<div class=\"warn\">"));
    icon(w, ICON_WARN);
    w.printP(PSTR("<span>Keep the device powered. It restarts when the update is written, "
                  "the stored values are kept.</span></div><button>Update firmware</button></form>"));

    pageEnd(w, layout);
}

void renderMessage(Writer &w, const Layout &layout, const char *heading, const std::string &text,
                   unsigned refresh)
{
    pageBegin(w, layout, heading, refresh);
    w.printP(PSTR("<div class=\"card\"><span>"));
    w.html(text);
    w.printP(PSTR("</span></div><p class=\"note\"><a href=\"/\">Home</a></p>"));
    pageEnd(w, layout);
}

} // namespace portal
