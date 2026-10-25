/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

// A water meter on the portal: the local port 80 for the LAN and the public
// port 8080 to publish the dashboard to the internet.
//
//   pio run -e example -t upload

#undef LOG_MODULE
#define LOG_MODULE "MAIN"

#include <Arduino.h>
#include <flash_hal.h>

#include <console.h>
#include <config.h>
#include <portal.h>

using CONFIG = config::Config;

constexpr unsigned char WATER = CONFIG::ID::CUSTOM_START + 1;
constexpr unsigned char CORRECTION = CONFIG::ID::CUSTOM_START + 2;
constexpr unsigned char MEASUREMENT = CONFIG::ID::CUSTOM_START + 3;

// Config journal in the FS area, no filesystem is mounted
config::FlashEsp configFlash(FS_PHYS_ADDR, FS_PHYS_SIZE);
config::Journal configJournal(configFlash);

portal::Portal web;

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    auto &config = CONFIG::getInstance()
        .add<config::PersistCounter>(WATER, 0)
        .add<float>(CORRECTION, 1.0f)
        .add<std::string>(MEASUREMENT, "test");

    config.read(configJournal);

    std::string name = "portal-" + std::to_string(ESP.getChipId());

    web.name(name)
        .version("1.0.0")
        .ports(80, 8080)
        .storage(configJournal);

    // On the public dashboard; liters shown as m3
    web.counter(WATER, "water", "Water", "m³", 1'000, 3)
        .section("Meters")
        .publish()
        .editable([](const uint32_t &value) { LOGI("water=%u", value); });

    // Settings: on the settings page only
    web.number(CORRECTION, "correction", "Correction", "Wh/kWh", 2).section("Reporting").hide().editable();
    web.text(MEASUREMENT, "measurement", "Measurement", 16).hide().editable();

    web.item("Uptime, s", [] { return std::to_string(millis() / 1000); }, portal::Access::Public);
    web.item("Free heap, B", [] { return std::to_string(ESP.getFreeHeap()); });

    web.on("/request", [](ESP8266WebServer &server) {
        server.send(200, "text/plain", "Measurements are requested");
    });

    // The same as a button on the local dashboard
    web.action("/request", "Request measurements", "Requesting…", "Measurements are requested",
               [] { LOGI("measurements are requested"); });

    if (!web.connect())
    {
        LOGW("WiFi is not connected, restart");
        delay(1000);
        ESP.restart();
    }

    web.begin();
}

void loop()
{
    // A liter every 10 seconds
    static uint32_t last = millis();
    if (millis() - last >= 10'000)
    {
        last = millis();

        auto &config = CONFIG::getInstance();
        config.set<config::PersistCounter>(WATER, config.getOr<config::PersistCounter>(WATER, 0) + 1);
        config.write(configJournal);
    }

    web.process();
    delay(10);
}
