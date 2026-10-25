/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

// Device side of the portal: fields bound to config, flash strings and heap.
// HTTP is checked from the host by test/smoke.sh against examples/basic.

#include <map>
#include <string>

#include <Arduino.h>
#include <unity.h>

#include <config.h>
#include <portal.h>

using namespace portal;

namespace {

constexpr unsigned char COUNTER = config::Config::ID::CUSTOM_START + 1;
constexpr unsigned char NUMBER = config::Config::ID::CUSTOM_START + 2;
constexpr unsigned char TEXT = config::Config::ID::CUSTOM_START + 3;

FormArgs args(const std::map<std::string, std::string> &values)
{
    return [values](const char *name, std::string &value) {
        auto it = values.find(name);
        if (it == values.end())
            return false;
        value = it->second;
        return true;
    };
}

bool contains(const std::string &text, const char *part)
{
    return text.find(part) != std::string::npos;
}

} // namespace

void setUp()
{
    config::Config::getInstance()
        .add<config::PersistCounter>(COUNTER, 385990)
        .add<float>(NUMBER, 1.0f)
        .add<std::string>(TEXT, "sensor");

    auto &config = config::Config::getInstance();
    config.set<config::PersistCounter>(COUNTER, 385990);
    config.set<float>(NUMBER, 1.0f);
    config.set<std::string>(TEXT, "sensor");
}

void tearDown() {}

void test_fields_read_config()
{
    Portal web;
    web.counter(COUNTER, "cold", "Cold water", "m3", 1'000, 3);
    web.number(NUMBER, "correction", "Correction", "", 2);
    web.text(TEXT, "measurement", "Measurement", 16);

    TEST_ASSERT_EQUAL_STRING("385.990", web.fields()[0]->text().c_str());
    TEST_ASSERT_EQUAL_STRING("1.00", web.fields()[1]->text().c_str());
    TEST_ASSERT_EQUAL_STRING("sensor", web.fields()[2]->text().c_str());

    // A change of the parameter is shown without updating the field
    config::Config::getInstance().set<config::PersistCounter>(COUNTER, 386001);
    TEST_ASSERT_EQUAL_STRING("386.001", web.fields()[0]->text().c_str());
}

void test_form_writes_config()
{
    uint32_t applied = 0;
    std::string measurement;

    Portal web;
    web.counter(COUNTER, "cold", "Cold water", "m3", 1'000, 3)
        .editable([&applied](const uint32_t &value) { applied = value; });
    web.number(NUMBER, "correction", "Correction", "", 2).editable();
    web.text(TEXT, "measurement", "Measurement", 16)
        .editable([&measurement](const std::string &value) { measurement = value; });

    FormResult result = submit(web.fields(), args({
        {"cold", "16844.469"},
        {"correction", "1.00"},
        {"measurement", "flow"},
    }));

    auto &config = config::Config::getInstance();

    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL(2, result.changed);
    TEST_ASSERT_EQUAL_UINT32(16844469, config.getOr<config::PersistCounter>(COUNTER, 0));
    TEST_ASSERT_EQUAL_UINT32(16844469, applied);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, config.getOr<float>(NUMBER, 0.0f));
    TEST_ASSERT_EQUAL_STRING("flow", config.getOr<std::string>(TEXT, "").c_str());
    TEST_ASSERT_EQUAL_STRING("flow", measurement.c_str());
}

void test_invalid_form_keeps_config()
{
    Portal web;
    web.counter(COUNTER, "cold", "Cold water", "m3", 1'000, 3).editable();
    web.text(TEXT, "measurement", "Measurement", 16).editable();

    FormResult result = submit(web.fields(), args({
        {"cold", "-1"},
        {"measurement", "flow"},
    }));

    auto &config = config::Config::getInstance();

    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(385990, config.getOr<config::PersistCounter>(COUNTER, 0));
    TEST_ASSERT_EQUAL_STRING("sensor", config.getOr<std::string>(TEXT, "").c_str());
}

void test_flash_strings()
{
    // PROGMEM is read by words on the ESP8266, printP must copy it right
    StringWriter css;
    css.printP(STYLE_CSS);
    TEST_ASSERT_EQUAL(strlen_P(STYLE_CSS), css.str().size());
    TEST_ASSERT_TRUE(contains(css.str(), "prefers-color-scheme:dark"));
    TEST_ASSERT_EQUAL('}', css.str().back());

    StringWriter js;
    js.printP(APP_JS);
    TEST_ASSERT_EQUAL(strlen_P(APP_JS), js.str().size());
    TEST_ASSERT_TRUE(contains(js.str(), "/api/values"));
}

void test_render_doesnt_leak()
{
    Portal web;
    web.counter(COUNTER, "cold", "Cold water", "m3", 1'000, 3).publish().editable();
    web.text(TEXT, "measurement", "Measurement", 16).hide().editable();
    web.item("Time", [] { return std::string("12:00"); }, Access::Public);

    Layout layout = {"test", Access::Local, "/"};

    // The first render allocates the lazy statics
    {
        StringWriter w;
        renderDashboard(w, layout, web.fields(), web.items());
    }

    uint32_t heap = ESP.getFreeHeap();
    for (int i = 0; i < 20; ++i)
    {
        StringWriter dashboard;
        renderDashboard(dashboard, layout, web.fields(), web.items());
        TEST_ASSERT_TRUE(contains(dashboard.str(), "385.990"));

        StringWriter settings;
        renderSettings(settings, layout, web.fields(), "token", nullptr, nullptr);
        TEST_ASSERT_TRUE(contains(settings.str(), "name=\"measurement\""));
    }

    TEST_ASSERT_EQUAL_UINT32(heap, ESP.getFreeHeap());
}

void test_csrf_token_is_random()
{
    Csrf a;
    Csrf b;
    a.init(ESP.random(), ESP.random());
    b.init(ESP.random(), ESP.random());

    TEST_ASSERT_EQUAL(16, a.token().size());
    TEST_ASSERT_TRUE(a.token() != b.token());
    TEST_ASSERT_TRUE(a.check(a.token()));
    TEST_ASSERT_FALSE(a.check(b.token()));
}

void setup()
{
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_fields_read_config);
    RUN_TEST(test_form_writes_config);
    RUN_TEST(test_invalid_form_keeps_config);
    RUN_TEST(test_flash_strings);
    RUN_TEST(test_render_doesnt_leak);
    RUN_TEST(test_csrf_token_is_random);

    UNITY_END();
}

void loop() {}
