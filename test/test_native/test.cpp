/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <cstring>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>
#include <string>

#include <unity.h>

#include <portal/core/Csrf.h>
#include <portal/core/Field.h>
#include <portal/core/Form.h>
#include <portal/core/Format.h>
#include <portal/core/Image.h>
#include <portal/core/Render.h>
#include <portal/core/Routes.h>

using namespace portal;

namespace {

int count(const std::string &text, const char *part)
{
    int result = 0;
    for (size_t at = text.find(part); at != std::string::npos; at = text.find(part, at + 1))
        ++result;
    return result;
}

bool contains(const std::string &text, const char *part)
{
    return text.find(part) != std::string::npos;
}

#define ASSERT_CONTAINS(text, part) TEST_ASSERT_TRUE_MESSAGE(contains(text, part), part)
#define ASSERT_NOT_CONTAINS(text, part) TEST_ASSERT_FALSE_MESSAGE(contains(text, part), part)

// Values behind the fields, as the config parameters on the device
struct Store {
    uint32_t cold = 385990;
    uint32_t heat = 38596706;
    float correction = 1.0f;
    std::string measurement = "sensor";

    int coldStores = 0;
    int coldChanges = 0;
    int correctionStores = 0;
    int measurementStores = 0;
};

Fields makeFields(Store &store)
{
    Fields fields;

    auto cold = std::make_unique<CounterField>(
        "cold", "Cold water", "m3", 1'000, 3,
        [&store] { return store.cold; },
        [&store](const uint32_t &v) { store.cold = v; ++store.coldStores; });
    cold->section("Meters").publish().editable([&store](const uint32_t &) { ++store.coldChanges; });
    fields.push_back(std::move(cold));

    // Shown on the local dashboard only, read only
    fields.push_back(std::make_unique<CounterField>(
        "heat", "Heat", "Gcal", 1'000'000, 6,
        [&store] { return store.heat; }, nullptr));

    auto correction = std::make_unique<FloatField>(
        "correction", "Correction", "Wh/kWh", 2,
        [&store] { return store.correction; },
        [&store](const float &v) { store.correction = v; ++store.correctionStores; });
    correction->section("Reporting").hide().editable();
    fields.push_back(std::move(correction));

    auto measurement = std::make_unique<TextField>(
        "measurement", "Measurement", 16,
        [&store] { return store.measurement; },
        [&store](const std::string &v) { store.measurement = v; ++store.measurementStores; });
    measurement->hide().editable();
    fields.push_back(std::move(measurement));

    return fields;
}

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

const Layout LOCAL = {"Flow <meter>", Access::Local, "/"};
const Layout PUBLIC = {"Flow <meter>", Access::Public, "/"};

} // namespace

void setUp() {}
void tearDown() {}

//--- Format ---

void test_scale_digits()
{
    TEST_ASSERT_EQUAL(0, scaleDigits(1));
    TEST_ASSERT_EQUAL(3, scaleDigits(1'000));
    TEST_ASSERT_EQUAL(6, scaleDigits(1'000'000));
    TEST_ASSERT_EQUAL(-1, scaleDigits(0));
    TEST_ASSERT_EQUAL(-1, scaleDigits(1'024));
}

void test_format_scaled()
{
    TEST_ASSERT_EQUAL_STRING("385.990", formatScaled(385990, 1'000, 3).c_str());
    TEST_ASSERT_EQUAL_STRING("0.005", formatScaled(5, 1'000, 3).c_str());
    TEST_ASSERT_EQUAL_STRING("38.596706", formatScaled(38596706, 1'000'000, 6).c_str());
    TEST_ASSERT_EQUAL_STRING("4294967.295", formatScaled(4294967295u, 1'000, 3).c_str());
    TEST_ASSERT_EQUAL_STRING("42", formatScaled(42, 1, 0).c_str());
    TEST_ASSERT_EQUAL_STRING("0.000", formatScaled(0, 1'000, 3).c_str());
}

void test_format_scaled_rounds_and_pads()
{
    TEST_ASSERT_EQUAL_STRING("386.0", formatScaled(385990, 1'000, 1).c_str());
    TEST_ASSERT_EQUAL_STRING("1.24", formatScaled(1235, 1'000, 2).c_str());
    TEST_ASSERT_EQUAL_STRING("1.23", formatScaled(1234, 1'000, 2).c_str());
    TEST_ASSERT_EQUAL_STRING("4294967", formatScaled(4294967295u, 1'000, 0).c_str());
    TEST_ASSERT_EQUAL_STRING("0.500", formatScaled(5, 10, 3).c_str());
    TEST_ASSERT_EQUAL_STRING("5.00", formatScaled(5, 1, 2).c_str());
    TEST_ASSERT_EQUAL_STRING("", formatScaled(5, 3, 2).c_str());
}

void test_parse_scaled()
{
    uint32_t value = 0;

    TEST_ASSERT_TRUE(parseScaled("385.990", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(385990, value);
    TEST_ASSERT_TRUE(parseScaled("385.99", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(385990, value);
    TEST_ASSERT_TRUE(parseScaled("385", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(385000, value);
    TEST_ASSERT_TRUE(parseScaled("  385,5 ", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(385500, value);
    TEST_ASSERT_TRUE(parseScaled(".5", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(500, value);
    TEST_ASSERT_TRUE(parseScaled("7.", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(7000, value);
    TEST_ASSERT_TRUE(parseScaled("38.596706", 1'000'000, value));
    TEST_ASSERT_EQUAL_UINT32(38596706, value);
    TEST_ASSERT_TRUE(parseScaled("0", 1, value));
    TEST_ASSERT_EQUAL_UINT32(0, value);
}

void test_parse_scaled_large_values_are_exact()
{
    uint32_t value = 0;

    // Above 2^24 a float would lose units
    TEST_ASSERT_TRUE(parseScaled("16844.469", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(16844469, value);
    TEST_ASSERT_TRUE(parseScaled("4294967.295", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(4294967295u, value);
    TEST_ASSERT_TRUE(parseScaled("4294967295", 1, value));
    TEST_ASSERT_EQUAL_UINT32(4294967295u, value);
}

void test_parse_scaled_rounds_half_up()
{
    uint32_t value = 0;

    TEST_ASSERT_TRUE(parseScaled("1.2345", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(1235, value);
    TEST_ASSERT_TRUE(parseScaled("1.2344999", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(1234, value);
    TEST_ASSERT_TRUE(parseScaled("0.0005", 1'000, value));
    TEST_ASSERT_EQUAL_UINT32(1, value);
    TEST_ASSERT_TRUE(parseScaled("2.5", 1, value));
    TEST_ASSERT_EQUAL_UINT32(3, value);
}

void test_parse_scaled_rejects()
{
    uint32_t value = 123;

    const char *invalid[] = {
        "", "  ", ".", "-1", "+1", "1e3", "1.2.3", "1,2.3", "abc", "12a", "1 2",
        "0x10", "4294967.296", "4294967296", "99999999999999999999",
    };

    for (const char *text : invalid)
    {
        TEST_ASSERT_FALSE_MESSAGE(parseScaled(text, 1'000, value), text);
        TEST_ASSERT_EQUAL_UINT32(123, value);
    }

    TEST_ASSERT_FALSE(parseScaled("1", 0, value));
    TEST_ASSERT_FALSE(parseScaled("1", 1'024, value));
    TEST_ASSERT_FALSE(parseScaled("4294967296", 1, value));
}

void test_scaled_round_trip()
{
    const uint32_t values[] = {0, 1, 999, 1'000, 385990, 16844469, 4294967295u};

    for (uint32_t expected : values)
    {
        uint32_t value = 0;
        TEST_ASSERT_TRUE(parseScaled(formatScaled(expected, 1'000, 3), 1'000, value));
        TEST_ASSERT_EQUAL_UINT32(expected, value);
    }
}

void test_float()
{
    float value = 0;

    TEST_ASSERT_EQUAL_STRING("1.00", formatFloat(1.0f, 2).c_str());
    TEST_ASSERT_EQUAL_STRING("-0.125", formatFloat(-0.125f, 3).c_str());

    TEST_ASSERT_TRUE(parseFloat("1.5", value));
    TEST_ASSERT_EQUAL_FLOAT(1.5f, value);
    TEST_ASSERT_TRUE(parseFloat(" -0,25 ", value));
    TEST_ASSERT_EQUAL_FLOAT(-0.25f, value);
    TEST_ASSERT_TRUE(parseFloat("1e2", value));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, value);

    const char *invalid[] = {"", "abc", "1.5x", "inf", "nan", "0x10", "1e99", "--1", "1 2"};
    for (const char *text : invalid)
    {
        value = 7.0f;
        TEST_ASSERT_FALSE_MESSAGE(parseFloat(text, value), text);
        TEST_ASSERT_EQUAL_FLOAT(7.0f, value);
    }
}

void test_format_usage()
{
    TEST_ASSERT_EQUAL_STRING("377088 / 3141888 B · 12%", formatUsage(377088, 3141888).c_str());
    TEST_ASSERT_EQUAL_STRING("38424 / 52128 B · 74%", formatUsage(38424, 52128).c_str());
    TEST_ASSERT_EQUAL_STRING("0 / 100 B · 0%", formatUsage(0, 100).c_str());
    TEST_ASSERT_EQUAL_STRING("100 / 100 B · 100%", formatUsage(100, 100).c_str());
    TEST_ASSERT_EQUAL_STRING("1 / 0 B · 0%", formatUsage(1, 0).c_str());
    TEST_ASSERT_EQUAL_STRING("4194304 / 4194304 B · 100%", formatUsage(4194304, 4194304).c_str());
    TEST_ASSERT_EQUAL_STRING("4294967295 / 4294967295 B · 100%", formatUsage(4294967295u, 4294967295u).c_str());
}

void test_valid_text()
{
    TEST_ASSERT_TRUE(validText("", 4));
    TEST_ASSERT_TRUE(validText("abcd", 4));
    TEST_ASSERT_TRUE(validText("\xd0\xb4\xd0\xbe", 4)); // UTF-8
    TEST_ASSERT_FALSE(validText("abcde", 4));
    TEST_ASSERT_FALSE(validText("a\nb", 4));
    TEST_ASSERT_FALSE(validText(std::string("a\0b", 3), 4));
    TEST_ASSERT_FALSE(validText("a\x7f", 4));
}

//--- Writer ---

void test_writer_escapes_html()
{
    StringWriter w;
    w.html("<a href=\"x\" title='y'>&</a>");
    TEST_ASSERT_EQUAL_STRING("&lt;a href=&quot;x&quot; title=&#39;y&#39;&gt;&amp;&lt;/a&gt;", w.str().c_str());
}

void test_writer_escapes_json()
{
    StringWriter w;
    w.json("a\"b\\c\n</script>");
    TEST_ASSERT_EQUAL_STRING("a\\\"b\\\\c\\u000a\\u003c/script>", w.str().c_str());
}

void test_writer_prints_long_flash_text()
{
    std::string text(200, 'x');
    text += "end";

    StringWriter w;
    w.printP(text.c_str());
    TEST_ASSERT_EQUAL_STRING(text.c_str(), w.str().c_str());
}

//--- Field and form ---

void test_field_visibility()
{
    Store store;
    Fields fields = makeFields(store);

    TEST_ASSERT_TRUE(fields[0]->shown(Access::Public));
    TEST_ASSERT_TRUE(fields[0]->shown(Access::Local));
    TEST_ASSERT_FALSE(fields[1]->shown(Access::Public));
    TEST_ASSERT_TRUE(fields[1]->shown(Access::Local));
    TEST_ASSERT_FALSE(fields[2]->shown(Access::Local));

    TEST_ASSERT_TRUE(fields[0]->editable());
    TEST_ASSERT_FALSE(fields[1]->editable());
}

void test_form_stores_changed_fields_only()
{
    Store store;
    Fields fields = makeFields(store);

    FormResult result = submit(fields, args({
        {"cold", "386"},
        {"correction", "1.00"},     // same as shown
        {"measurement", "sensor"},  // same
    }));

    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL(1, result.changed);
    TEST_ASSERT_EQUAL_UINT32(386000, store.cold);
    TEST_ASSERT_EQUAL(1, store.coldStores);
    TEST_ASSERT_EQUAL(1, store.coldChanges);
    TEST_ASSERT_EQUAL(0, store.correctionStores);
    TEST_ASSERT_EQUAL(0, store.measurementStores);
}

void test_form_unchanged_text_keeps_rounded_value()
{
    Store store;
    store.correction = 1.005f;
    Fields fields = makeFields(store);

    // The form shows "1.00" for 1.005, sending it back changes nothing
    FormResult result = submit(fields, args({{"correction", "1.00"}}));

    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL(0, result.changed);
    TEST_ASSERT_EQUAL_FLOAT(1.005f, store.correction);

    result = submit(fields, args({{"correction", "1.01"}}));
    TEST_ASSERT_EQUAL(1, result.changed);
    TEST_ASSERT_EQUAL_FLOAT(1.01f, store.correction);
}

void test_form_same_value_in_other_text_is_unchanged()
{
    Store store;
    Fields fields = makeFields(store);

    FormResult result = submit(fields, args({{"cold", "385,99"}}));

    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL(0, result.changed);
    TEST_ASSERT_EQUAL(0, store.coldStores);
}

void test_form_invalid_field_rejects_all()
{
    Store store;
    Fields fields = makeFields(store);

    FormResult result = submit(fields, args({
        {"cold", "400"},
        {"correction", "abc"},
        {"measurement", "this is longer than 16"},
    }));

    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(0, result.changed);
    TEST_ASSERT_EQUAL(2, result.errors.size());
    TEST_ASSERT_NOT_NULL(result.error(fields[2].get()));
    TEST_ASSERT_EQUAL_STRING("abc", result.error(fields[2].get())->c_str());
    TEST_ASSERT_NULL(result.error(fields[0].get()));

    TEST_ASSERT_EQUAL_UINT32(385990, store.cold);
    TEST_ASSERT_EQUAL(0, store.coldStores);
    TEST_ASSERT_EQUAL_STRING("sensor", store.measurement.c_str());

    // Nothing stays staged for the next submit
    result = submit(fields, args({}));
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL(0, result.changed);
    TEST_ASSERT_EQUAL_UINT32(385990, store.cold);
}

void test_form_ignores_read_only_and_unknown()
{
    Store store;
    Fields fields = makeFields(store);

    FormResult result = submit(fields, args({
        {"heat", "1"},
        {"unknown", "1"},
    }));

    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL(0, result.changed);
    TEST_ASSERT_EQUAL_UINT32(38596706, store.heat);
}

//--- Csrf ---

void test_csrf()
{
    Csrf csrf;
    TEST_ASSERT_FALSE(csrf.check(""));

    csrf.init(0x12345678, 0x9abcdef0);
    TEST_ASSERT_EQUAL_STRING("123456789abcdef0", csrf.token().c_str());
    TEST_ASSERT_TRUE(csrf.check("123456789abcdef0"));
    TEST_ASSERT_FALSE(csrf.check("123456789abcdef1"));
    TEST_ASSERT_FALSE(csrf.check("123456789abcdef"));
    TEST_ASSERT_FALSE(csrf.check(""));
}

//--- Routes ---

void test_public_routes_are_read_only()
{
    auto publicRoutes = routes(Access::Public);

    TEST_ASSERT_EQUAL(4, publicRoutes.size());
    for (const auto &route : publicRoutes)
    {
        TEST_ASSERT_TRUE(route.method == Method::Get);
        TEST_ASSERT_TRUE(route.access == Access::Public);
        TEST_ASSERT_TRUE(route.page == Page::Dashboard || route.page == Page::Values ||
                         route.page == Page::Style || route.page == Page::Script);
    }
}

void test_local_routes_are_all()
{
    TEST_ASSERT_EQUAL(routes().size(), routes(Access::Local).size());

    // Every change is a POST
    for (const auto &route : routes())
    {
        bool change = route.page == Page::SettingsSave || route.page == Page::WifiSave ||
                      route.page == Page::WifiScan ||
                      route.page == Page::Restart || route.page == Page::UpdateUpload;
        TEST_ASSERT_EQUAL(change, route.method == Method::Post);
    }
}

void test_networks()
{
    auto result = networks({
        {"home", -70, false},
        {"", -30, true},
        {"cafe", -50, true},
        {"home", -40, false},
        {"far", -90, false},
    }, 2);

    TEST_ASSERT_EQUAL(2, result.size());
    TEST_ASSERT_EQUAL_STRING("home", result[0].ssid.c_str());
    TEST_ASSERT_EQUAL(-40, result[0].rssi);
    TEST_ASSERT_EQUAL_STRING("cafe", result[1].ssid.c_str());
}

void test_wifi_validation()
{
    TEST_ASSERT_TRUE(validSsid("home"));
    TEST_ASSERT_TRUE(validSsid(std::string(32, 'a')));
    TEST_ASSERT_FALSE(validSsid(""));
    TEST_ASSERT_FALSE(validSsid(std::string(33, 'a')));

    TEST_ASSERT_TRUE(validPassword(""));
    TEST_ASSERT_TRUE(validPassword("12345678"));
    TEST_ASSERT_TRUE(validPassword(std::string(63, 'a')));
    TEST_ASSERT_TRUE(validPassword(std::string(64, 'f')));
    TEST_ASSERT_FALSE(validPassword("1234567"));
    TEST_ASSERT_FALSE(validPassword(std::string(64, 'g')));
    TEST_ASSERT_FALSE(validPassword(std::string(65, 'a')));
}

//--- Render ---

void test_public_dashboard_shows_published_only()
{
    Store store;
    Fields fields = makeFields(store);
    Items items = {
        {"Time", [] { return std::string("12:00"); }, Access::Public},
        {"SSID", [] { return std::string("home-wlan"); }, Access::Local},
    };

    StringWriter w;
    renderDashboard(w, PUBLIC, fields, items);
    const std::string &page = w.str();

    ASSERT_CONTAINS(page, "<span id=\"v-cold\">385.990</span>");
    ASSERT_CONTAINS(page, "12:00");
    ASSERT_CONTAINS(page, "/app.js");
    ASSERT_NOT_CONTAINS(page, "38.596706");
    ASSERT_NOT_CONTAINS(page, "home-wlan");
    ASSERT_NOT_CONTAINS(page, "sensor");
    // No links to the pages that don't exist on the public port
    ASSERT_CONTAINS(page, "View only");
    ASSERT_NOT_CONTAINS(page, "<nav");
    ASSERT_NOT_CONTAINS(page, "/edit");
    ASSERT_NOT_CONTAINS(page, "/wifi");
    ASSERT_NOT_CONTAINS(page, "<form");
}

void test_local_dashboard()
{
    Store store;
    Fields fields = makeFields(store);
    Items items = {{"SSID", [] { return std::string("home-wlan"); }, Access::Local}};

    StringWriter w;
    renderDashboard(w, LOCAL, fields, items);
    const std::string &page = w.str();

    ASSERT_CONTAINS(page, "<nav aria-label=\"Pages\">");
    ASSERT_CONTAINS(page, "<a href=\"/\" class=\"on\" aria-current=\"page\">");
    ASSERT_CONTAINS(page, "<a href=\"/edit\">");
    ASSERT_CONTAINS(page, "</svg>Settings</a>");
    ASSERT_CONTAINS(page, "<body class=\"nav\">");
    ASSERT_CONTAINS(page, "385.990");
    ASSERT_CONTAINS(page, "38.596706");
    ASSERT_CONTAINS(page, "home-wlan");
    // Hidden settings are not on the dashboard
    ASSERT_NOT_CONTAINS(page, "Measurement");
}

void test_render_escapes()
{
    Store store;
    store.measurement = "<script>x</script>";
    Fields fields = makeFields(store);
    Items items = {{"<b>", [] { return std::string("\"&'"); }, Access::Local}};

    StringWriter w;
    renderDashboard(w, LOCAL, fields, items);
    renderSettings(w, LOCAL, fields, "tok", nullptr, "<i>");
    const std::string &page = w.str();

    ASSERT_CONTAINS(page, "Flow &lt;meter&gt;");
    ASSERT_CONTAINS(page, "&lt;b&gt;");
    ASSERT_CONTAINS(page, "&quot;&amp;&#39;");
    ASSERT_CONTAINS(page, "&lt;script&gt;x&lt;/script&gt;");
    ASSERT_CONTAINS(page, "&lt;i&gt;");
    ASSERT_NOT_CONTAINS(page, "<script>x");
    ASSERT_NOT_CONTAINS(page, "<i>");
    ASSERT_NOT_CONTAINS(page, "<meter>");
}

void test_values_json()
{
    Store store;
    store.measurement = "a\"b";
    Fields fields = makeFields(store);

    StringWriter pub;
    renderValues(pub, fields, Access::Public);
    TEST_ASSERT_EQUAL_STRING("{\"cold\":\"385.990\"}", pub.str().c_str());

    StringWriter local;
    renderValues(local, fields, Access::Local);
    TEST_ASSERT_EQUAL_STRING("{\"cold\":\"385.990\",\"heat\":\"38.596706\"}", local.str().c_str());

    Fields none;
    StringWriter empty;
    renderValues(empty, none, Access::Local);
    TEST_ASSERT_EQUAL_STRING("{}", empty.str().c_str());
}

void test_settings_form()
{
    Store store;
    Fields fields = makeFields(store);

    StringWriter w;
    renderSettings(w, LOCAL, fields, "tok123", nullptr, "Saved");
    const std::string &page = w.str();

    ASSERT_CONTAINS(page, "<form method=\"post\" action=\"/edit\">");
    ASSERT_CONTAINS(page, "<input type=\"hidden\" name=\"t\" value=\"tok123\">");
    ASSERT_CONTAINS(page, "name=\"cold\" value=\"385.990\" maxlength=\"18\" inputmode=\"decimal\"");
    ASSERT_CONTAINS(page, "name=\"correction\" value=\"1.00\"");
    ASSERT_CONTAINS(page, "name=\"measurement\" value=\"sensor\" maxlength=\"16\"");
    ASSERT_CONTAINS(page, "role=\"status\">");
    ASSERT_CONTAINS(page, "</svg>Saved</p>");
    // The unit is shown in the field
    ASSERT_CONTAINS(page, "autocomplete=\"off\"><span>m3</span></div>");
    // Read only values are not in the form
    ASSERT_NOT_CONTAINS(page, "name=\"heat\"");
}

void test_settings_form_keeps_rejected_values()
{
    Store store;
    Fields fields = makeFields(store);
    FormResult result = submit(fields, args({{"cold", "12x"}, {"measurement", "new"}}));

    StringWriter w;
    renderSettings(w, LOCAL, fields, "tok", &result, nullptr);
    const std::string &page = w.str();

    ASSERT_CONTAINS(page, "msg bad");
    ASSERT_CONTAINS(page, "value=\"12x\"");
    ASSERT_CONTAINS(page, "<div class=\"in bad\"><input id=\"f-cold\"");
    ASSERT_CONTAINS(page, "aria-describedby=\"e-cold\"");
    ASSERT_CONTAINS(page, "<span class=\"err\" id=\"e-cold\">Invalid value</span>");
    // A valid field shows the stored value: nothing was saved
    ASSERT_CONTAINS(page, "name=\"measurement\" value=\"sensor\"");
}

void test_forms_carry_token()
{
    StringWriter wifi;
    Wifi state;
    state.connected = "home";
    state.detail = "10.0.0.2 · -40 dBm";
    state.networks = {{"home <1>", -40, false}, {"cafe", -80, true}};
    state.scanAge = 70;
    renderWifi(wifi, LOCAL, state, "tok", nullptr);
    ASSERT_CONTAINS(wifi.str(), "name=\"t\" value=\"tok\"");
    ASSERT_CONTAINS(wifi.str(), "<option value=\"home &lt;1&gt;\">");
    ASSERT_CONTAINS(wifi.str(), "Connected to <b>home</b>");
    ASSERT_CONTAINS(wifi.str(), "10.0.0.2 · -40 dBm");
    ASSERT_CONTAINS(wifi.str(), "<h2>Networks <span>· 1 min ago</span></h2>");
    // A tap on a network fills the form
    ASSERT_CONTAINS(wifi.str(), "<button type=\"button\" data-s=\"home &lt;1&gt;\">");
    ASSERT_CONTAINS(wifi.str(), "aria-label=\"Signal 4 of 4, -40 dBm\"");
    ASSERT_CONTAINS(wifi.str(), "<i>open<svg class=\"sig\" width=\"20\" height=\"14\"");
    ASSERT_CONTAINS(wifi.str(), "aria-label=\"Signal 1 of 4, -80 dBm\"");

    StringWriter info;
    renderInfo(info, LOCAL, {{"Memory", {{"Free heap", "24704"}}}}, "tok");
    ASSERT_CONTAINS(info.str(), "action=\"/restart\"");
    ASSERT_CONTAINS(info.str(), "name=\"t\" value=\"tok\"");
    ASSERT_CONTAINS(info.str(), "<h2>Memory</h2>");
    ASSERT_CONTAINS(info.str(), "<span>Free heap</span><span>24704</span>");
    ASSERT_CONTAINS(info.str(), "<button class=\"danger\">Restart device</button>");

    StringWriter update;
    renderUpdate(update, LOCAL, "tok", "1.2.0", nullptr);
    ASSERT_CONTAINS(update.str(), "<span>Installed firmware</span><span>1.2.0</span>");
    ASSERT_CONTAINS(update.str(), "action=\"/update?t=tok\"");
    ASSERT_CONTAINS(update.str(), "enctype=\"multipart/form-data\"");
    // Warning triangle of the design
    ASSERT_CONTAINS(update.str(), "<div class=\"warn\"><svg width=\"20\" height=\"20\"");
    ASSERT_CONTAINS(update.str(), "<path d=\"M12 3l10 18H2z\"></path>");
    // The WiFi status card has the WiFi icon, muted when not connected
    StringWriter off;
    renderWifi(off, LOCAL, Wifi(), "tok", nullptr);
    ASSERT_CONTAINS(off.str(), "<div class=\"card st off\"><svg width=\"22\" height=\"22\"");
    ASSERT_CONTAINS(wifi.str(), "<div class=\"card st\"><svg width=\"22\" height=\"22\"");
    // Signal bars as in the design: the connected network in the accent color
    StringWriter selected;
    Wifi current;
    current.connected = "home";
    current.networks = {{"home", -40, false}};
    current.scanAge = 0;
    renderWifi(selected, LOCAL, current, "tok", nullptr);
    ASSERT_CONTAINS(selected.str(), "data-s=\"home\" class=\"on\">");
    ASSERT_CONTAINS(selected.str(), "<rect class=\"on\" x=\"15\" y=\"0\" width=\"3\" height=\"14\" rx=\"1\" fill=\"#0969DA\">");
    ASSERT_CONTAINS(wifi.str(), "<rect class=\"on\" x=\"0\" y=\"10\" width=\"3\" height=\"4\" rx=\"1\" fill=\"#57606A\">");
    ASSERT_CONTAINS(wifi.str(), "<rect x=\"5\" y=\"7\" width=\"3\" height=\"7\" rx=\"1\" fill=\"#D0D7DE\">");
}

void test_sections()
{
    Store store;
    Fields fields = makeFields(store);

    // A second field of the same section doesn't repeat the heading
    auto hot = std::make_unique<CounterField>("hot", "Hot water", "m3", 1'000, 3,
                                              [] { return 1u; }, nullptr);
    hot->section("Meters").publish();
    fields.insert(fields.begin() + 1, std::move(hot));

    StringWriter dashboard;
    renderDashboard(dashboard, PUBLIC, fields, {});
    TEST_ASSERT_EQUAL(1, count(dashboard.str(), "<h2>Meters</h2>"));
    ASSERT_NOT_CONTAINS(dashboard.str(), "Reporting");

    StringWriter settings;
    renderSettings(settings, LOCAL, fields, "tok", nullptr, nullptr);
    TEST_ASSERT_EQUAL(1, count(settings.str(), "<h2>Meters</h2>"));
    TEST_ASSERT_EQUAL(1, count(settings.str(), "<h2>Reporting</h2>"));
    TEST_ASSERT_TRUE(settings.str().find("<h2>Reporting</h2>") < settings.str().find("name=\"correction\""));
}

void test_dashboard_header()
{
    Store store;
    Fields fields = makeFields(store);

    Layout local = LOCAL;
    local.subtitle = "home · 10.0.0.2";
    local.badge = "Online";

    StringWriter w;
    renderDashboard(w, local, fields, {});
    ASSERT_CONTAINS(w.str(), "<small>home · 10.0.0.2</small>");
    ASSERT_CONTAINS(w.str(), "<span class=\"pill ok\"><svg width=\"8\" height=\"8\"");
    ASSERT_CONTAINS(w.str(), "</svg>Online</span>");

    // The public dashboard never shows the network
    Layout pub = PUBLIC;
    pub.subtitle = "home · 10.0.0.2";
    pub.badge = "Online";

    StringWriter p;
    renderDashboard(p, pub, fields, {});
    ASSERT_NOT_CONTAINS(p.str(), "10.0.0.2");
    ASSERT_NOT_CONTAINS(p.str(), "Online");
    // The eye icon of the design in the view only badge
    ASSERT_CONTAINS(p.str(), "<circle cx=\"12\" cy=\"12\" r=\"3\"></circle></svg>View only</span>");
}

void test_signal_level()
{
    TEST_ASSERT_EQUAL(4, signalLevel(-40));
    TEST_ASSERT_EQUAL(4, signalLevel(-55));
    TEST_ASSERT_EQUAL(3, signalLevel(-60));
    TEST_ASSERT_EQUAL(2, signalLevel(-70));
    TEST_ASSERT_EQUAL(1, signalLevel(-76));
    TEST_ASSERT_EQUAL(1, signalLevel(-95));
}

void test_icons_have_size()
{
    // Every icon has its size and stroke in attributes: a page looks right
    // even with a stylesheet missing or cached from another version
    StringWriter w;
    renderDashboard(w, LOCAL, {}, {});
    const std::string &page = w.str();

    for (size_t at = page.find("<svg"); at != std::string::npos; at = page.find("<svg", at + 1))
    {
        std::string tag = page.substr(at, page.find('>', at) - at);
        TEST_ASSERT_TRUE_MESSAGE(contains(tag, "width=\""), tag.c_str());
        TEST_ASSERT_TRUE_MESSAGE(contains(tag, "height=\""), tag.c_str());
    }
    TEST_ASSERT_EQUAL(5, count(page, "<svg width=\"24\" height=\"24\""));
    TEST_ASSERT_EQUAL(5, count(page, "stroke-width=\"1.8\""));
}

void test_assets_version()
{
    const char *version = assetsVersion();
    TEST_ASSERT_EQUAL(8, strlen(version));
    TEST_ASSERT_EQUAL_STRING(version, assetsVersion());

    StringWriter w;
    renderDashboard(w, PUBLIC, {}, {});
    ASSERT_CONTAINS(w.str(), (std::string("href=\"/style.css?v=") + version + "\"").c_str());
    ASSERT_CONTAINS(w.str(), (std::string("src=\"/app.js?v=") + version + "\"").c_str());
}

void test_wifi_scan()
{
    // Before a scan: no list, the Scan button and a hint
    Wifi none;
    none.detail = "Setup access point · 192.168.4.1";
    StringWriter empty;
    renderWifi(empty, LOCAL, none, "tok", nullptr);
    ASSERT_CONTAINS(empty.str(), "<h2>Networks</h2>");
    ASSERT_CONTAINS(empty.str(), "<form method=\"post\" action=\"/wifi/scan\"><input type=\"hidden\" name=\"t\" value=\"tok\">");
    ASSERT_CONTAINS(empty.str(), "</svg>Scan</button>");
    ASSERT_CONTAINS(empty.str(), "No scan yet");
    ASSERT_CONTAINS(empty.str(), "Setup access point · 192.168.4.1");
    ASSERT_NOT_CONTAINS(empty.str(), "http-equiv=\"refresh\"");
    ASSERT_NOT_CONTAINS(empty.str(), "data-s=");

    // Scanning: the button is disabled and the page reloads itself
    Wifi scanning;
    scanning.scanning = true;
    scanning.scanAge = 300;
    StringWriter busy;
    renderWifi(busy, LOCAL, scanning, "tok", nullptr);
    ASSERT_CONTAINS(busy.str(), "<meta http-equiv=\"refresh\" content=\"4;url=/wifi\">");
    ASSERT_CONTAINS(busy.str(), "<button class=\"scan\" disabled aria-disabled=\"true\">");
    ASSERT_CONTAINS(busy.str(), "Scanning…</button>");
    ASSERT_CONTAINS(busy.str(), "Searching for networks");
    ASSERT_NOT_CONTAINS(busy.str(), "min ago");

    // Done without networks
    Wifi nothing;
    nothing.scanAge = 5;
    StringWriter found;
    renderWifi(found, LOCAL, nothing, "tok", nullptr);
    ASSERT_CONTAINS(found.str(), "No networks found");
    ASSERT_CONTAINS(found.str(), "<h2>Networks <span>· just now</span></h2>");
}

void test_format_age()
{
    TEST_ASSERT_EQUAL_STRING("just now", formatAge(0).c_str());
    TEST_ASSERT_EQUAL_STRING("just now", formatAge(59).c_str());
    TEST_ASSERT_EQUAL_STRING("1 min ago", formatAge(60).c_str());
    TEST_ASSERT_EQUAL_STRING("59 min ago", formatAge(3599).c_str());
    TEST_ASSERT_EQUAL_STRING("1 h ago", formatAge(3600).c_str());
    TEST_ASSERT_EQUAL_STRING("49 h ago", formatAge(49 * 3600L).c_str());
}

// Firmware as elf2bin.py builds it: eboot at 0 padded to 0x1000, the sketch
// image, the file size and the CRC at 0x1010
std::vector<uint8_t> makeImage(size_t size)
{
    std::vector<uint8_t> image(size);
    for (size_t i = 0; i < size; ++i)
        image[i] = static_cast<uint8_t>(i * 7 + 3);
    image[0] = 0xe9;
    image[0x1000] = 0xe9;

    for (size_t i = 0x1010; i < 0x1018; ++i)
        image[i] = 0;

    // Reference CRC, the algorithm of elf2bin.py
    uint32_t crc = 0xffffffff;
    for (uint8_t byte : image)
    {
        for (int bit = 7; bit >= 0; --bit)
        {
            uint32_t top = (crc >> 31) ^ ((byte >> bit) & 1);
            crc = (crc << 1) ^ (top ? 0x04c11db7 : 0);
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        image[0x1010 + i] = static_cast<uint8_t>(size >> (8 * i));
        image[0x1014 + i] = static_cast<uint8_t>(crc >> (8 * i));
    }

    return image;
}

bool checkImage(const std::vector<uint8_t> &image, size_t chunk, ImageCheck &check)
{
    check.begin();
    for (size_t at = 0; at < image.size(); at += chunk)
    {
        if (!check.feed(image.data() + at, std::min(chunk, image.size() - at)))
            return false;
    }
    return check.valid();
}

void test_image_valid()
{
    ImageCheck check;
    auto image = makeImage(20000);

    // Any upload chunk size
    for (size_t chunk : {size_t(1), size_t(7), size_t(1460), size_t(2048), image.size()})
    {
        TEST_ASSERT_TRUE(checkImage(image, chunk, check));
        TEST_ASSERT_NULL(check.error());
    }
}

void test_image_invalid()
{
    ImageCheck check;
    auto image = makeImage(20000);

    auto damaged = image;
    damaged[15000] ^= 0x01;
    TEST_ASSERT_FALSE(checkImage(damaged, 1460, check));
    TEST_ASSERT_EQUAL_STRING("The firmware is damaged, its CRC doesn't match", check.error());

    auto truncated = image;
    truncated.resize(19999);
    TEST_ASSERT_FALSE(checkImage(truncated, 1460, check));
    TEST_ASSERT_EQUAL_STRING("The firmware is incomplete", check.error());

    auto longer = image;
    longer.push_back(0);
    TEST_ASSERT_FALSE(checkImage(longer, 1460, check));
    TEST_ASSERT_EQUAL_STRING("The firmware is longer than its header says", check.error());

    // Rejected at the first chunk, before anything is flashed
    auto foreign = image;
    foreign[0] = 0x1f;
    check.begin();
    TEST_ASSERT_FALSE(check.feed(foreign.data(), 1460));
    TEST_ASSERT_EQUAL_STRING("Not an ESP8266 firmware", check.error());

    auto noSketch = image;
    noSketch[0x1000] = 0;
    TEST_ASSERT_FALSE(checkImage(noSketch, 1460, check));
    TEST_ASSERT_EQUAL_STRING("No sketch in the firmware", check.error());

    // The magic byte and garbage: what bricked a board before the check
    std::vector<uint8_t> garbage(30000);
    for (size_t i = 0; i < garbage.size(); ++i)
        garbage[i] = static_cast<uint8_t>((i * 2654435761u) >> 13);
    garbage[0] = 0xe9;
    garbage[0x1000] = 0xe9;
    TEST_ASSERT_FALSE(checkImage(garbage, 1460, check));
    TEST_ASSERT_NOT_NULL(check.error());

    check.begin();
    TEST_ASSERT_EQUAL_STRING("The firmware is incomplete", check.error());
}

void test_image_real_firmware()
{
    // The example built by "pio run -e example"
    std::ifstream file(".pio/build/example/firmware.bin", std::ios::binary);
    if (!file)
        TEST_IGNORE_MESSAGE("no .pio/build/example/firmware.bin");

    std::vector<uint8_t> image((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    ImageCheck check;
    TEST_ASSERT_TRUE_MESSAGE(checkImage(image, 1460, check), check.error());
}

void test_message_page()
{
    StringWriter w;
    renderMessage(w, LOCAL, "Restart", "Restarting <now>", 10);

    ASSERT_CONTAINS(w.str(), "<meta http-equiv=\"refresh\" content=\"10;url=/\">");
    ASSERT_CONTAINS(w.str(), "Restarting &lt;now&gt;");
}

void test_assets()
{
    ASSERT_CONTAINS(std::string(STYLE_CSS), "prefers-color-scheme:dark");
    ASSERT_CONTAINS(std::string(APP_JS), "/api/values");
    ASSERT_CONTAINS(std::string(APP_JS), "data-s");
    ASSERT_CONTAINS(std::string(APP_JS), "upload.onprogress");
    // The CSP blocks inline styles: the pages use classes only
    ASSERT_NOT_CONTAINS(std::string(STYLE_CSS), "</");
    // Values are put as text, never as HTML
    ASSERT_NOT_CONTAINS(std::string(APP_JS), "innerHTML");
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_scale_digits);
    RUN_TEST(test_format_scaled);
    RUN_TEST(test_format_scaled_rounds_and_pads);
    RUN_TEST(test_parse_scaled);
    RUN_TEST(test_parse_scaled_large_values_are_exact);
    RUN_TEST(test_parse_scaled_rounds_half_up);
    RUN_TEST(test_parse_scaled_rejects);
    RUN_TEST(test_scaled_round_trip);
    RUN_TEST(test_float);
    RUN_TEST(test_format_usage);
    RUN_TEST(test_valid_text);

    RUN_TEST(test_writer_escapes_html);
    RUN_TEST(test_writer_escapes_json);
    RUN_TEST(test_writer_prints_long_flash_text);

    RUN_TEST(test_field_visibility);
    RUN_TEST(test_form_stores_changed_fields_only);
    RUN_TEST(test_form_unchanged_text_keeps_rounded_value);
    RUN_TEST(test_form_same_value_in_other_text_is_unchanged);
    RUN_TEST(test_form_invalid_field_rejects_all);
    RUN_TEST(test_form_ignores_read_only_and_unknown);

    RUN_TEST(test_csrf);

    RUN_TEST(test_public_routes_are_read_only);
    RUN_TEST(test_local_routes_are_all);
    RUN_TEST(test_networks);
    RUN_TEST(test_wifi_validation);

    RUN_TEST(test_public_dashboard_shows_published_only);
    RUN_TEST(test_local_dashboard);
    RUN_TEST(test_render_escapes);
    RUN_TEST(test_values_json);
    RUN_TEST(test_settings_form);
    RUN_TEST(test_settings_form_keeps_rejected_values);
    RUN_TEST(test_forms_carry_token);
    RUN_TEST(test_sections);
    RUN_TEST(test_dashboard_header);
    RUN_TEST(test_signal_level);
    RUN_TEST(test_icons_have_size);
    RUN_TEST(test_assets_version);
    RUN_TEST(test_wifi_scan);
    RUN_TEST(test_format_age);
    RUN_TEST(test_image_valid);
    RUN_TEST(test_image_invalid);
    RUN_TEST(test_image_real_firmware);
    RUN_TEST(test_message_page);
    RUN_TEST(test_assets);

    return UNITY_END();
}
