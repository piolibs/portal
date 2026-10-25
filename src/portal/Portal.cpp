/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#undef LOG_MODULE
#define LOG_MODULE "PRTL"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <console.h>

#include "Portal.h"
#include "ServerWriter.h"
#include "core/Form.h"
#include "core/Format.h"

extern "C" char _heap_start[];

namespace portal {

namespace {

constexpr uint32_t CONNECT_TIMEOUT_MS = 30'000;
constexpr uint32_t RESTART_DELAY_MS = 1'000;
constexpr uint32_t WIFI_DELAY_MS = 500;
constexpr size_t NETWORKS_LIMIT = 12;

template <size_t size = 32>
std::string format(const char *fmt, ...)
{
    char buffer[size];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    return std::string(buffer);
}

std::string arg(ESP8266WebServer &server, const char *name)
{
    return server.arg(name).c_str();
}

void logRequest(ESP8266WebServer &server, Access access)
{
    LOGD("%s %s %s from %s", (access == Access::Local) ? "local" : "public",
         (server.method() == HTTP_POST) ? "POST" : "GET", server.uri().c_str(),
         server.client().remoteIP().toString().c_str());
}

// The heap of the core: from the end of the static data to 0x3FFFC000
uint32_t heapSize()
{
    return 0x3FFFC000UL - reinterpret_cast<uint32_t>(&_heap_start[0]);
}

// Free sketch space a new firmware is written to, sector aligned
uint32_t updateSpace()
{
    return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
}

bool elapsed(uint32_t since, uint32_t ms)
{
    return (millis() - since) >= ms;
}

} // namespace

Portal &Portal::name(const std::string &name)
{
    mName = name;
    return *this;
}

Portal &Portal::version(const char *version)
{
    mVersion = version;
    return *this;
}

Portal &Portal::ports(uint16_t local, uint16_t pub)
{
    mLocalPort = local;
    mPublicPort = pub;
    return *this;
}

Portal &Portal::setupTimeout(uint16_t seconds)
{
    mSetupTimeout = seconds;
    return *this;
}

Portal &Portal::setupOnBoot(uint16_t seconds)
{
    mSetupOnBoot = seconds;
    return *this;
}

Portal &Portal::setupPassword(const std::string &password)
{
    mSetupPassword = password;
    return *this;
}

Portal &Portal::storage(config::Journal &journal)
{
    mJournal = &journal;
    return *this;
}

CounterField &Portal::counter(unsigned char id, const char *key, const char *label, const char *unit,
                              uint32_t scale, uint8_t decimals)
{
    auto field = std::make_unique<CounterField>(
        key, label, unit, scale, decimals,
        [id] {
            return static_cast<uint32_t>(config::Config::getInstance().getOr<config::PersistCounter>(id, 0));
        },
        [id](const uint32_t &value) {
            config::Config::getInstance().set<config::PersistCounter>(id, value);
        });

    auto &result = *field;
    mFields.push_back(std::move(field));
    return result;
}

FloatField &Portal::number(unsigned char id, const char *key, const char *label, const char *unit,
                           uint8_t decimals)
{
    auto field = std::make_unique<FloatField>(
        key, label, unit, decimals,
        [id] { return config::Config::getInstance().getOr<float>(id, 0.0f); },
        [id](const float &value) { config::Config::getInstance().set<float>(id, value); });

    auto &result = *field;
    mFields.push_back(std::move(field));
    return result;
}

TextField &Portal::text(unsigned char id, const char *key, const char *label, size_t maxLength)
{
    auto field = std::make_unique<TextField>(
        key, label, maxLength,
        [id] { return config::Config::getInstance().getOr<std::string>(id, ""); },
        [id](const std::string &value) { config::Config::getInstance().set<std::string>(id, value); });

    auto &result = *field;
    mFields.push_back(std::move(field));
    return result;
}

Portal &Portal::item(const char *label, std::function<std::string()> value, Access access)
{
    mItems.push_back({label, std::move(value), access});
    return *this;
}

Portal &Portal::on(const char *path, Handler handler)
{
    mHandlers.emplace_back(path, std::move(handler));
    return *this;
}

Portal &Portal::onSave(std::function<void()> callback)
{
    mOnSave = std::move(callback);
    return *this;
}

bool Portal::connect()
{
    // Setup access point on every boot, e.g. to test the setup or to move
    // the device to another network
    if (mSetupOnBoot && setup(mSetupOnBoot, true))
    {
        LOGI("connected to %s, ip=%s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        return true;
    }

    WiFi.mode(WIFI_STA);

    // The credentials are kept by the SDK, as WiFiManager kept them
    if (WiFi.SSID().length() && (WiFi.begin() != WL_CONNECT_FAILED) && waitConnected(CONNECT_TIMEOUT_MS))
    {
        LOGI("connected to %s, ip=%s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        return true;
    }

    return setup(mSetupTimeout, false);
}

bool Portal::setup(uint16_t seconds, bool onBoot)
{
    LOGW("setup access point %s, %u s%s", mName.c_str(), seconds, onBoot ? " on boot" : "");

    // The station keeps connecting meanwhile, e.g. to a router booting slower
    // after a power outage
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(mName.c_str(), mSetupPassword.empty() ? nullptr : mSetupPassword.c_str());
    if (onBoot && WiFi.SSID().length())
        WiFi.begin();

    // Any name resolves to the portal, phones open it as a captive portal
    mDns = std::make_unique<DNSServer>();
    mDns->setErrorReplyCode(DNSReplyCode::NoError);
    mDns->start(53, "*", WiFi.softAPIP());

    mSetup = true;
    mWifiApplied = false;
    begin();

    LOGI("setup access point ip=%s", WiFi.softAPIP().toString().c_str());

    // Requests extend the time. On boot the saved network may connect in
    // the background: the access point stays for the whole time, unless a
    // network is set by the WiFi form.
    mActivity = millis();
    while (!elapsed(mActivity, seconds * 1000UL))
    {
        process();

        if (WiFi.status() == WL_CONNECTED && (!onBoot || mWifiApplied))
            break;

        delay(10);
    }

    bool connected = (WiFi.status() == WL_CONNECTED);
    LOGI("setup access point is done, connected=%d", connected);

    mDns->stop();
    mDns.reset();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    mSetup = false;

    return connected;
}

void Portal::begin()
{
    if (mCsrf.token().empty())
        mCsrf.init(ESP.random(), ESP.random());

    if (!mLocal)
    {
        mLocal = std::make_unique<ESP8266WebServer>(mLocalPort);
        registerRoutes(*mLocal, Access::Local);
        mLocal->begin();
        LOGI("local server, port=%u", mLocalPort);
    }

    if (mSetup || mPublic || !mPublicPort)
        return;

    // The same port would serve the public requests with the local routes
    if (mPublicPort == mLocalPort)
    {
        LOGE("public port %u is the local one, the public server is not started", mPublicPort);
        return;
    }

    mPublic = std::make_unique<ESP8266WebServer>(mPublicPort);
    registerRoutes(*mPublic, Access::Public);
    mPublic->begin();
    LOGI("public server, port=%u", mPublicPort);
}

void Portal::process()
{
    if (mDns)
        mDns->processNextRequest();
    if (mLocal)
        mLocal->handleClient();
    if (mPublic)
        mPublic->handleClient();

    if (mScanning)
    {
        int count = WiFi.scanComplete();
        if (count >= 0)
        {
            std::vector<Network> found;
            for (int i = 0; i < count; ++i)
                found.push_back({WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE});
            WiFi.scanDelete();

            mNetworks = networks(std::move(found), NETWORKS_LIMIT);
            mScanning = false;
            mScanAt = millis() | 1;
            LOGD("scan is done, networks=%d", count);
        }
        else if (count != WIFI_SCAN_RUNNING)
        {
            mScanning = false;
        }
    }

    // Delayed to let the response go out first
    if (!mWifiSsid.empty() && elapsed(mWifiAt, WIFI_DELAY_MS))
    {
        LOGW("connect to %s", mWifiSsid.c_str());

        WiFi.persistent(true);
        WiFi.begin(mWifiSsid.c_str(), mWifiPassword.c_str());

        mWifiSsid.clear();
        mWifiPassword.clear();

        // Out of the setup the new network is used after a restart
        if (mSetup)
            mWifiApplied = true;
        else
            restartLater();
    }

    if (mRestartAt && static_cast<int32_t>(millis() - mRestartAt) >= 0)
    {
        LOGW("restart");
        delay(100);
        ESP.restart();
    }
}

void Portal::registerRoutes(ESP8266WebServer &server, Access access)
{
    for (const auto &route : routes(access))
    {
        HTTPMethod method = (route.method == Method::Get) ? HTTP_GET : HTTP_POST;

        if (route.page == Page::UpdateUpload)
        {
            server.on(route.path, method,
                      [this, &server, route] { handle(server, Access::Local, route); },
                      [this, &server] { upload(server); });
        }
        else
        {
            server.on(route.path, method, [this, &server, access, route] { handle(server, access, route); });
        }
    }

    if (access == Access::Local)
    {
        for (const auto &[path, handler] : mHandlers)
        {
            server.on(path, HTTP_GET, [this, &server, handler = handler] {
                touch(server);
                logRequest(server, Access::Local);
                sendHeaders(server, false);
                handler(server);
            });
        }
    }

    server.onNotFound([this, &server, access] {
        touch(server);
        logRequest(server, access);

        // Captive portal: any page of the setup access point is the WiFi page
        if (mSetup && access == Access::Local && server.uri() != F("/favicon.ico"))
        {
            server.sendHeader(F("Location"), String(F("http://")) + WiFi.softAPIP().toString() + F("/wifi"));
            server.send(302, F("text/plain"), F(""));
            return;
        }

        sendHeaders(server, false);
        server.send(404, F("text/plain"), F("Not found"));
    });
}

void Portal::handle(ESP8266WebServer &server, Access access, const Route &route)
{
    touch(server);
    logRequest(server, access);

    // Never happens: a server registers the routes of its level only
    if (!serves(access, route.access))
    {
        server.send(404, F("text/plain"), F("Not found"));
        return;
    }

    switch (route.page)
    {
    case Page::Style:
        sendStatic(server, PSTR("text/css"), STYLE_CSS);
        return;

    case Page::Script:
        sendStatic(server, PSTR("application/javascript"), APP_JS);
        return;

    default:
        break;
    }

    sendHeaders(server, false);

    switch (route.page)
    {
    case Page::Dashboard:
    {
        // Network and address in the header of the local dashboard
        std::string subtitle;
        Layout frame = layout(access, route.path);
        if (access == Access::Local)
        {
            subtitle = mSetup ? "Setup · " + std::string(WiFi.softAPIP().toString().c_str())
                              : std::string(WiFi.SSID().c_str()) + " · " + WiFi.localIP().toString().c_str();
            frame.subtitle = subtitle.c_str();
            frame.badge = mSetup ? "Setup" : "Online";
        }

        ServerWriter w(server, 200, PSTR("text/html; charset=utf-8"));
        renderDashboard(w, frame, mFields, mItems);
        break;
    }

    case Page::Values:
    {
        ServerWriter w(server, 200, PSTR("application/json"));
        renderValues(w, mFields, access);
        break;
    }

    case Page::Settings:
    case Page::SettingsSave:
        pageSettings(server, route.page == Page::SettingsSave);
        break;

    case Page::Wifi:
    case Page::WifiSave:
        pageWifi(server, route.page == Page::WifiSave);
        break;

    case Page::WifiScan:
        if (!checkToken(server))
            break;
        scan();
        server.sendHeader(F("Location"), F("/wifi"));
        server.send(303, F("text/plain"), F(""));
        break;

    case Page::Info:
        pageInfo(server);
        break;

    case Page::Restart:
        if (!checkToken(server))
            break;
        sendMessage(server, 200, "Restart", "The device restarts", 15);
        restartLater();
        break;

    case Page::Update:
    {
        ServerWriter w(server, 200, PSTR("text/html; charset=utf-8"));
        renderUpdate(w, layout(Access::Local, "/update"), mCsrf.token(), mVersion, nullptr);
        break;
    }

    case Page::UpdateUpload:
        pageUpdate(server);
        break;

    default:
        break;
    }
}

void Portal::sendHeaders(ESP8266WebServer &server, bool cache)
{
    server.sendHeader(F("X-Content-Type-Options"), F("nosniff"));
    server.sendHeader(F("X-Frame-Options"), F("DENY"));
    server.sendHeader(F("Referrer-Policy"), F("no-referrer"));
    server.sendHeader(F("Content-Security-Policy"),
                      F("default-src 'self'; form-action 'self'; frame-ancestors 'none'; base-uri 'none'"));
    server.sendHeader(F("Cache-Control"), cache ? F("max-age=86400") : F("no-store"));
}

void Portal::sendStatic(ESP8266WebServer &server, PGM_P type, PGM_P content)
{
    sendHeaders(server, true);
    server.send_P(200, type, content);
}

bool Portal::checkToken(ESP8266WebServer &server)
{
    if (mCsrf.check(arg(server, Csrf::ARG)))
        return true;

    LOGW("invalid form token, uri=%s", server.uri().c_str());
    sendMessage(server, 403, "Forbidden", "The page is outdated, open it again");
    return false;
}

void Portal::sendMessage(ESP8266WebServer &server, int code, const char *heading, const std::string &text,
                         unsigned refresh)
{
    ServerWriter w(server, code, PSTR("text/html; charset=utf-8"));
    renderMessage(w, layout(Access::Local, ""), heading, text, refresh);
}

void Portal::pageSettings(ESP8266WebServer &server, bool save)
{
    if (!save)
    {
        ServerWriter w(server, 200, PSTR("text/html; charset=utf-8"));
        renderSettings(w, layout(Access::Local, "/edit"), mFields, mCsrf.token(), nullptr,
                       server.hasArg(F("saved")) ? "Saved" : nullptr);
        return;
    }

    if (!checkToken(server))
        return;

    FormResult result = submit(mFields, [&server](const char *name, std::string &value) {
        if (!server.hasArg(name))
            return false;
        value = arg(server, name);
        return true;
    });

    if (!result.ok())
    {
        LOGW("settings rejected, invalid=%u", static_cast<unsigned>(result.errors.size()));
        ServerWriter w(server, 400, PSTR("text/html; charset=utf-8"));
        renderSettings(w, layout(Access::Local, "/edit"), mFields, mCsrf.token(), &result, nullptr);
        return;
    }

    if (result.changed)
    {
        LOGI("settings saved, changed=%u", static_cast<unsigned>(result.changed));

        if (mJournal)
            config::Config::getInstance().write(*mJournal);
        if (mOnSave)
            mOnSave();
    }

    // Reloading the result page doesn't post the form again
    server.sendHeader(F("Location"), F("/edit?saved=1"));
    server.send(303, F("text/plain"), F(""));
}

Wifi Portal::wifiState() const
{
    Wifi wifi;

    if (WiFi.status() == WL_CONNECTED)
    {
        wifi.connected = WiFi.SSID().c_str();
        wifi.detail = format<64>("%s · %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
    else if (mSetup)
    {
        wifi.detail = "Setup access point · " + std::string(WiFi.softAPIP().toString().c_str());
    }

    wifi.networks = mNetworks;
    wifi.scanning = mScanning;
    wifi.scanAge = mScanAt ? static_cast<long>((millis() - mScanAt) / 1000) : -1;
    return wifi;
}

void Portal::pageWifi(ESP8266WebServer &server, bool save)
{
    if (!save)
    {
        ServerWriter w(server, 200, PSTR("text/html; charset=utf-8"));
        renderWifi(w, layout(Access::Local, "/wifi"), wifiState(), mCsrf.token(), nullptr);
        return;
    }

    if (!checkToken(server))
        return;

    std::string ssid = arg(server, "s");
    std::string password = arg(server, "p");

    if (!validSsid(ssid) || !validPassword(password))
    {
        ServerWriter w(server, 400, PSTR("text/html; charset=utf-8"));
        renderWifi(w, layout(Access::Local, "/wifi"), wifiState(), mCsrf.token(),
                   "Invalid network name or password");
        return;
    }

    mWifiSsid = ssid;
    mWifiPassword = password;
    mWifiAt = millis();

    sendMessage(server, 200, "WiFi",
                "Connecting to " + ssid + ". The device is available in that network after a restart.");
}

void Portal::pageInfo(ESP8266WebServer &server)
{
    uint32_t uptime = static_cast<uint32_t>(micros64() / 1'000'000);

    uint32_t sketch = ESP.getSketchSize();

    Sections sections = {
        {"System", {
            {"Firmware", mVersion},
            {"Uptime", format("%ud %02u:%02u:%02u", uptime / 86400, uptime / 3600 % 24, uptime / 60 % 60, uptime % 60)},
            {"Reset reason", ESP.getResetReason().c_str()},
            {"Core · CPU", format("%s · %u MHz", ESP.getCoreVersion().c_str(), ESP.getCpuFreqMHz())},
            {"Chip ID", format("%06x", ESP.getChipId())},
        }},
        {"Memory", {
            {"Heap used", formatUsage(heapSize() - ESP.getFreeHeap(), heapSize())},
            {"Max block · fragmentation", format("%u B · %u%%", ESP.getMaxFreeBlockSize(), ESP.getHeapFragmentation())},
            {"Sketch", format("%u B", sketch)},
            // Room for a new firmware uploaded by the update page
            {"OTA space", format("%.1f MB free", updateSpace() / 1048576.0)},
            // All but the free space for the sketch and its update: the
            // sketch, the config area and the SDK data
            {"Flash used", formatUsage(ESP.getFlashChipSize() - ESP.getFreeSketchSpace(), ESP.getFlashChipSize())},
        }},
        {"Network", {
            {"SSID · RSSI", format<64>("%s · %d dBm", WiFi.SSID().c_str(), WiFi.RSSI())},
            {"IP", WiFi.localIP().toString().c_str()},
            {"Gateway", WiFi.gatewayIP().toString().c_str()},
            {"MAC", WiFi.macAddress().c_str()},
            {"Ports", mPublic ? format("%u local · %u public", mLocalPort, mPublicPort)
                              : format("%u local · public off", mLocalPort)},
        }},
    };

    ServerWriter w(server, 200, PSTR("text/html; charset=utf-8"));
    renderInfo(w, layout(Access::Local, "/info"), sections, mCsrf.token());
}

void Portal::upload(ESP8266WebServer &server)
{
    HTTPUpload &file = server.upload();

    switch (file.status)
    {
    case UPLOAD_FILE_START:
    {
        touch(server);
        mUpdateError.clear();
        mUpdateWritten = false;

        // The token is in the query, checked before anything is flashed
        mUpdateAllowed = mCsrf.check(arg(server, Csrf::ARG));
        if (!mUpdateAllowed)
        {
            LOGW("update rejected, invalid form token");
            return;
        }

        LOGW("update start, file=%s", file.filename.c_str());

        mImage.begin();
        if (!Update.begin(updateSpace(), U_FLASH))
            updateFailed(Update.getErrorString().c_str());
        break;
    }

    case UPLOAD_FILE_WRITE:
        if (!mUpdateAllowed || !mUpdateError.empty())
            break;

        // The data goes to the free flash, the running firmware is kept
        // until the update is committed by Update.end(true)
        if (!mImage.feed(file.buf, file.currentSize))
            updateFailed(mImage.error());
        else if (Update.write(file.buf, file.currentSize) != file.currentSize)
            updateFailed(Update.getErrorString().c_str());
        break;

    case UPLOAD_FILE_END:
        if (!mUpdateAllowed || !mUpdateError.empty())
            break;

        // Committed only when the whole firmware is there and its CRC matches
        if (!mImage.valid())
        {
            updateFailed(mImage.error());
        }
        else if (Update.end(true))
        {
            mUpdateWritten = true;
            LOGW("update is written, size=%u", static_cast<unsigned>(file.totalSize));
        }
        else
        {
            updateFailed(Update.getErrorString().c_str());
        }
        break;

    case UPLOAD_FILE_ABORTED:
        // The connection is lost: no response, nothing is left for the
        // next request
        if (mUpdateAllowed && mUpdateError.empty())
            Update.end();
        LOGW("update aborted");
        mUpdateError.clear();
        mUpdateAllowed = false;
        mUpdateWritten = false;
        break;
    }
}

void Portal::updateFailed(const char *error)
{
    mUpdateError = error ? error : "Update failed";

    // Not committed: the running firmware stays. A reset updater also takes
    // the next upload, else it is still running.
    Update.end();
}

void Portal::pageUpdate(ESP8266WebServer &server)
{
    // The state of this upload only: nothing is left for the next request
    bool written = mUpdateWritten;
    std::string failure = std::move(mUpdateError);
    mUpdateError.clear();
    mUpdateWritten = false;
    mUpdateAllowed = false;

    // Checked again: an upload interrupted before leaves its state behind
    if (!checkToken(server))
        return;

    if (!failure.empty() || !written)
    {
        const char *error = failure.empty() ? "No firmware file" : failure.c_str();
        LOGW("update failed: %s", error);

        ServerWriter w(server, failure.empty() ? 400 : 500, PSTR("text/html; charset=utf-8"));
        renderUpdate(w, layout(Access::Local, "/update"), mCsrf.token(), mVersion, error);
        return;
    }

    sendMessage(server, 200, "Update", "The firmware is updated, the device restarts", 20);
    restartLater();
}

Layout Portal::layout(Access access, const char *path) const
{
    return {mName.c_str(), access, path};
}

void Portal::scan()
{
    // By the Scan button only: a scan switches the radio channels for
    // seconds, the setup access point drops its clients meanwhile. In the
    // background, so the page shows the progress.
    if (mScanning)
        return;

    LOGD("scan networks");
    mScanning = true;
    WiFi.scanNetworks(true);
}

void Portal::touch(ESP8266WebServer &server)
{
    // Only the clients of the setup access point extend its time, not the
    // requests from the saved network connected meanwhile
    if (server.client().localIP() == WiFi.softAPIP())
        mActivity = millis();
}

void Portal::restartLater()
{
    mRestartAt = (millis() + RESTART_DELAY_MS) | 1;
}

bool Portal::waitConnected(uint32_t timeoutMs)
{
    uint32_t start = millis();

    while (WiFi.status() != WL_CONNECTED && !elapsed(start, timeoutMs))
        delay(100);

    return WiFi.status() == WL_CONNECTED;
}

} // namespace portal
