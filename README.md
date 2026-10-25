# portal

Lightweight web portal for ESP8266: WiFi setup, a dashboard of values, a
settings page, system info and firmware update. Pages are made for phones (one
column, the menu at the bottom) and follow the system light/dark theme; they
use the system font and inline icons, so they work without internet on the
setup access point.

Two ports, no passwords:

- **local** (80) — for the LAN, everything is available;
- **public** (e.g. 8080) — to publish to the internet through a port forward:
  only the dashboard and its values. The other pages are not registered on
  that port (404) and nothing there accepts a POST.

Fields are bound to [config](https://github.com/piolibs/config) parameters:
the pages read the current values from config on every request, a saved form
stores only the changed values and writes the journal once.

## Usage

```cpp
#include <flash_hal.h>
#include <config.h>
#include <portal.h>

constexpr unsigned char WATER = config::Config::ID::CUSTOM_START + 1;
constexpr unsigned char MEASUREMENT = config::Config::ID::CUSTOM_START + 2;

config::FlashEsp flash(FS_PHYS_ADDR, FS_PHYS_SIZE);
config::Journal journal(flash);

portal::Portal web;

void setup()
{
    config::Config::getInstance()
        .add<config::PersistCounter>(WATER, 0)
        .add<std::string>(MEASUREMENT, "test")
        .read(journal);

    web.name("flow-meter")       // page title and setup access point name
        .version(APP_VERSION)
        .ports(80, 8080)         // local and public ports, 0 disables public
        .storage(journal);       // written after the settings are saved

    // Liters shown as m3, on the public dashboard too, editable locally
    web.counter(WATER, "water", "Water", "m³", 1'000, 3)
        .section("Meters")
        .publish()
        .editable([](const uint32_t &liters) { counter.setValue(liters); });

    // A setting: not on the dashboard, only on the settings page
    web.text(MEASUREMENT, "measurement", "Measurement", 16).hide().editable();

    // Dashboard line computed on each render
    web.item("Device time", [] { return clockText(); }, portal::Access::Public);

    // Extra route, local port only
    web.on("/request", [](ESP8266WebServer &server) {
        server.send(200, "text/plain", "Measurements are requested");
    });

    if (!web.connect())          // stored WiFi or the setup access point
        ESP.restart();

    web.begin();
}

void loop()
{
    web.process();
}
```

Fields:

| Method | Config type | Shown as |
|---|---|---|
| `counter(id, key, label, unit, scale, decimals)` | `PersistCounter` | value / scale, e.g. liters as m3 |
| `number(id, key, label, unit, decimals)` | `float` | fixed decimals |
| `text(id, key, label, maxLength)` | `std::string` | text |

A field is on the local dashboard by default; `publish()` shows it on the
public one too, `hide()` removes it from the dashboards. `section(title)` starts
a group with a heading on the dashboard and the settings page. `editable(onChange)`
puts it on the settings page; `onChange` is called after the value is stored.
A form with an invalid value saves nothing.

## Pages

| Path | Local | Public |
|---|---|---|
| `/` dashboard, values refresh every 30 s | yes | published fields only, no menu |
| `/api/values` JSON of the dashboard values | yes | yes |
| `/edit` settings | yes | — |
| `/wifi` credentials; networks by the Scan button, a tap fills the form | yes | — |
| `/info` system info, restart | yes | — |
| `/update` firmware upload with progress | yes | — |
| `on(path, ...)` extra routes | yes | — |

## WiFi

`connect()` connects with the credentials stored by the SDK (WiFiManager
stores them the same way, so a device keeps its network after the switch).
Without them, or if the network doesn't answer in 30 s, it starts the setup
access point with a captive portal: any page opens the WiFi form. The station
keeps connecting meanwhile. `connect()` returns false after `setupTimeout()`
seconds (180 by default) without requests.

With `-D PORTAL_SETUP_ON_BOOT=60` (or `setupOnBoot(60)`) the setup access point
starts on every boot for 60 seconds, extended by each request of its clients,
also when the saved network is available: to test the setup or move the device to another
network. A network set by the form is used at once, otherwise the saved one
when the time is over.

## Security

- The public port registers read-only routes only; its dashboard has no menu
  and shows only published fields.
- The local port has no password, but every change (settings, WiFi, scan,
  restart, update) is a POST with a random per-boot token, so another site
  opened in a LAN browser can't post the forms. Routes added by `on()` are GET
  without a token: keep them free of changes that matter.
- A firmware upload checks the token before the first chunk is flashed and
  again when it ends. The update is committed only when the file is a whole
  ESP8266 Arduino firmware: its size and CRC written by `elf2bin.py` must
  match, so a wrong or damaged file never replaces the running firmware (the
  core's updater alone checks the first byte only). Gzipped `.bin.gz` files
  are not accepted.
- Form values are validated before anything is stored; all output is escaped.
- Responses have `Content-Security-Policy`, `X-Frame-Options: DENY`,
  `X-Content-Type-Options: nosniff` and `Cache-Control: no-store`. The
  stylesheet and script are cached, their links carry a version of their
  content, so a browser takes the new ones after a firmware update.
- There is no TLS on the ESP8266: publish the public port only, never the
  local one.

## Tests

```bash
pio test -e native                  # host tests of src/portal/core
pio test -e board-esp8266           # device tests on a NodeMCU board
pio run -e example -t upload        # examples/basic on a test board, then:
test/smoke.sh <board ip> 8080       # HTTP checks; --write also saves a form and scans
```

Pages are streamed in 256 byte chunks and never built in the heap.
