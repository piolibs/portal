/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <functional>
#include <memory>
#include <string>

#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <config.h>

#include "core/Csrf.h"
#include "core/Field.h"
#include "core/Image.h"
#include "core/Render.h"
#include "core/Routes.h"

// Seconds of the setup access point on every boot before the saved network
// is used, 0: only when there is no network. E.g. -D PORTAL_SETUP_ON_BOOT=60
#ifndef PORTAL_SETUP_ON_BOOT
#define PORTAL_SETUP_ON_BOOT 0
#endif

namespace portal {

/*
 * Web portal with two ports:
 * - the local port (80) for the LAN: dashboard, settings, WiFi, info, update;
 * - the public port, to publish to the internet: the dashboard and its
 *   values only, nothing else exists there.
 *
 * Fields are bound to config parameters: the pages read the current values
 * from config, a saved form stores the changed ones and writes the journal.
 */
class Portal {
public:
    using Handler = std::function<void(ESP8266WebServer &server)>;

    // Name of the setup access point and the page title
    Portal &name(const std::string &name);
    // Firmware version on the info page
    Portal &version(const char *version);
    // Public port 0 disables the public server
    Portal &ports(uint16_t local, uint16_t pub = 0);
    // Seconds without requests in the setup access point before connect()
    // gives up
    Portal &setupTimeout(uint16_t seconds);
    // Seconds of the setup access point on every boot, 0 disables
    // (PORTAL_SETUP_ON_BOOT by default)
    Portal &setupOnBoot(uint16_t seconds);
    // Password of the setup access point, open if empty
    Portal &setupPassword(const std::string &password);
    // Journal written after the settings are saved
    Portal &storage(config::Journal &journal);

    // Unsigned counter config parameter (PersistCounter) shown in scale units
    CounterField &counter(unsigned char id, const char *key, const char *label, const char *unit,
                          uint32_t scale = 1, uint8_t decimals = 0);
    // float config parameter
    FloatField &number(unsigned char id, const char *key, const char *label, const char *unit,
                       uint8_t decimals = 2);
    // std::string config parameter
    TextField &text(unsigned char id, const char *key, const char *label, size_t maxLength);

    // Dashboard line computed on each render
    Portal &item(const char *label, std::function<std::string()> value, Access access = Access::Local);

    // Extra GET route, on the local port only; added before connect()/begin()
    Portal &on(const char *path, Handler handler);
    // Called after a saved form changed some values
    Portal &onSave(std::function<void()> callback);

    // Connect with the stored WiFi credentials. Without them, or if the
    // network doesn't answer, start the setup access point with the local
    // server until WiFi is configured or the timeout (see setupOnBoot() too).
    // False if not connected.
    bool connect();

    // Start the servers, after connect()
    void begin();

    // Serve the requests, from loop()
    void process();

    // Fields and items, e.g. for tests
    const Fields &fields() const { return mFields; }
    const Items &items() const { return mItems; }

private:
    void registerRoutes(ESP8266WebServer &server, Access access);
    void handle(ESP8266WebServer &server, Access access, const Route &route);
    void upload(ESP8266WebServer &server);

    void sendHeaders(ESP8266WebServer &server, bool cache);
    void sendStatic(ESP8266WebServer &server, PGM_P type, PGM_P content);
    bool checkToken(ESP8266WebServer &server);
    void sendMessage(ESP8266WebServer &server, int code, const char *heading, const std::string &text,
                     unsigned refresh = 0);

    void pageSettings(ESP8266WebServer &server, bool save);
    void pageWifi(ESP8266WebServer &server, bool save);
    void pageInfo(ESP8266WebServer &server);
    void pageUpdate(ESP8266WebServer &server);
    void updateFailed(const char *error);

    Layout layout(Access access, const char *path) const;
    void scan();
    Wifi wifiState() const;
    void touch(ESP8266WebServer &server);
    void restartLater();
    bool waitConnected(uint32_t timeoutMs);
    // Setup access point with the local server for the seconds since the
    // last request, true if WiFi is connected
    bool setup(uint16_t seconds, bool onBoot);

    std::string mName = "esp8266";
    const char *mVersion = "";
    uint16_t mLocalPort = 80;
    uint16_t mPublicPort = 0;
    uint16_t mSetupTimeout = 180;
    uint16_t mSetupOnBoot = PORTAL_SETUP_ON_BOOT;
    std::string mSetupPassword;
    config::Journal *mJournal = nullptr;

    Fields mFields;
    Items mItems;
    std::vector<std::pair<const char *, Handler>> mHandlers;
    std::function<void()> mOnSave;

    std::unique_ptr<ESP8266WebServer> mLocal;
    std::unique_ptr<ESP8266WebServer> mPublic;
    std::unique_ptr<DNSServer> mDns;
    Csrf mCsrf;

    // Setup access point; its timeout restarts on every request
    bool mSetup = false;
    uint32_t mActivity = 0;

    // Networks of the last scan and its end, 0 if none
    std::vector<Network> mNetworks;
    uint32_t mScanAt = 0;
    bool mScanning = false;

    // Firmware upload: allowed by the token, written to the end or failed
    std::string mUpdateError;
    bool mUpdateAllowed = false;
    bool mUpdateWritten = false;
    ImageCheck mImage;

    // New WiFi credentials, applied after the response is sent
    std::string mWifiSsid;
    std::string mWifiPassword;
    uint32_t mWifiAt = 0;
    // A network is set by the WiFi form during the setup
    bool mWifiApplied = false;

    uint32_t mRestartAt = 0;
};

} // namespace portal
