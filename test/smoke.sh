#!/usr/bin/env bash
#
# Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# HTTP checks of a running portal, e.g. examples/basic on a test board:
#
#   test/smoke.sh <host> [public port] [--write]
#
# Without --write nothing is changed on the device: only GET requests and
# POST requests that must be refused. --write also saves the settings form
# (a text setting is changed and restored), use it on a test board only.

set -u

HOST="${1:?usage: $0 <host> [public port] [--write]}"
PUBLIC_PORT="${2:-8080}"
WRITE="${3:-}"

LOCAL="http://${HOST}"
PUBLIC="http://${HOST}:${PUBLIC_PORT}"

FAILED=0
PASSED=0

pass() { PASSED=$((PASSED + 1)); echo "ok    $1"; }
fail() { FAILED=$((FAILED + 1)); echo "FAIL  $1"; }

# status <expected code> <description> <curl args...>
status() {
    local expected="$1" name="$2"
    shift 2
    local code
    code=$(curl -s -o /dev/null -m 15 -w '%{http_code}' "$@")
    if [ "$code" = "$expected" ]; then pass "$name"; else fail "$name: $code, expected $expected"; fi
}

# body <pattern> <description> <curl args...>, the body must match
body() {
    local pattern="$1" name="$2"
    shift 2
    if curl -s -m 30 "$@" | grep -q -- "$pattern"; then pass "$name"; else fail "$name"; fi
}

# no_body <pattern> <description> <url>, the body must not match
no_body() {
    if curl -s -m 15 "$3" | grep -q -- "$1"; then fail "$2"; else pass "$2"; fi
}

# header <pattern> <description> <url>
header() {
    if curl -s -m 15 -D - -o /dev/null "$3" | grep -qi -- "$1"; then pass "$2"; else fail "$2"; fi
}

echo "== local ${LOCAL}"

status 200 "dashboard" "${LOCAL}/"
body "<nav" "dashboard has the menu" "${LOCAL}/"
status 200 "values" "${LOCAL}/api/values"
header "content-type: application/json" "values are json" "${LOCAL}/api/values"
status 200 "settings" "${LOCAL}/edit"
body 'name="t" value="[0-9a-f]\{16\}"' "settings form has a token" "${LOCAL}/edit"
status 200 "info" "${LOCAL}/info"
status 200 "update page" "${LOCAL}/update"
status 200 "style" "${LOCAL}/style.css"
header "cache-control: max-age" "style is cached" "${LOCAL}/style.css"
status 200 "script" "${LOCAL}/app.js"
status 404 "unknown page" "${LOCAL}/unknown"

header "x-frame-options: deny" "no framing" "${LOCAL}/"
header "content-security-policy: default-src 'self'" "content security policy" "${LOCAL}/"
header "x-content-type-options: nosniff" "no sniffing" "${LOCAL}/"
header "cache-control: no-store" "pages are not cached" "${LOCAL}/edit"

status 403 "settings without token" -X POST -d "measurement=x" "${LOCAL}/edit"
status 403 "settings with a wrong token" -X POST -d "t=0000000000000000&measurement=x" "${LOCAL}/edit"
status 403 "wifi without token" -X POST -d "s=x&p=12345678" "${LOCAL}/wifi"
status 403 "scan without token" -X POST "${LOCAL}/wifi/scan"
status 403 "restart without token" -X POST "${LOCAL}/restart"
# A sized file: curl uploads /dev/null in chunks, the ESP8266 server takes none
FIRMWARE=$(mktemp)
trap 'rm -f "$FIRMWARE"' EXIT
head -c 1024 /dev/zero > "$FIRMWARE"
status 403 "update without token" -X POST -F "firmware=@${FIRMWARE}" "${LOCAL}/update"
status 403 "update with a wrong token" -X POST -F "firmware=@${FIRMWARE}" "${LOCAL}/update?t=0000000000000000"

echo "== public ${PUBLIC}"

status 200 "dashboard" "${PUBLIC}/"
no_body "<nav" "dashboard has no menu" "${PUBLIC}/"
no_body "/edit" "dashboard has no settings link" "${PUBLIC}/"
no_body "<form" "dashboard has no forms" "${PUBLIC}/"
status 200 "values" "${PUBLIC}/api/values"
status 200 "style" "${PUBLIC}/style.css"
status 200 "script" "${PUBLIC}/app.js"
header "x-frame-options: deny" "no framing" "${PUBLIC}/"

for page in /edit /wifi /info /update /restart /request /unknown; do
    status 404 "no GET ${page}" "${PUBLIC}${page}"
done

for page in / /edit /wifi /wifi/scan /restart /update /api/values; do
    status 404 "no POST ${page}" -X POST -d "t=x" "${PUBLIC}${page}"
done

if [ "$WRITE" = "--write" ]; then
    echo "== settings form (writes)"

    token=$(curl -s -m 15 "${LOCAL}/edit" | grep -o 'name="t" value="[0-9a-f]*"' | grep -o '[0-9a-f]\{16\}')
    old=$(curl -s -m 15 "${LOCAL}/edit" | grep -o 'name="measurement" value="[^"]*"' | sed 's/.*value="//; s/"$//')

    status 400 "invalid value is rejected" -X POST -d "t=${token}&measurement=%01" "${LOCAL}/edit"
    status 303 "settings are saved" -X POST -d "t=${token}&measurement=smoke" "${LOCAL}/edit"
    body 'name="measurement" value="smoke"' "saved value is shown" "${LOCAL}/edit"
    status 303 "settings are restored" -X POST --data-urlencode "t=${token}" --data-urlencode "measurement=${old}" "${LOCAL}/edit"

    echo "== firmware update checks (nothing is flashed)"

    uptime() { curl -s -m 10 "${LOCAL}/info" | grep -o '<span>Uptime</span><span>[^<]*' | sed 's/.*<span>//'; }
    upload_token=$(curl -s -m 15 "${LOCAL}/update" | grep -o 'update?t=[0-9a-f]*' | cut -d= -f2)
    before=$(uptime)

    status 400 "update without a file" -X POST -d "x=1" "${LOCAL}/update?t=${upload_token}"
    head -c 20000 /dev/urandom > "$FIRMWARE"
    body "Not an ESP8266 firmware" "random data is rejected" -F "firmware=@${FIRMWARE}" "${LOCAL}/update?t=${upload_token}"
    # The magic bytes and garbage: the core's updater alone flashes it
    { printf '\xe9'; head -c 4095 /dev/urandom; printf '\xe9'; head -c 30000 /dev/urandom; } > "$FIRMWARE"
    status 500 "garbage with the magic bytes is rejected" -F "firmware=@${FIRMWARE}" "${LOCAL}/update?t=${upload_token}"
    sleep 3
    after=$(uptime)
    if [ -n "$after" ] && [[ "$after" > "$before" ]]; then pass "no restart after rejected updates"; else fail "no restart after rejected updates: $before -> $after"; fi

    echo "== wifi scan (the radio scans for a few seconds)"

    status 303 "scan starts" -X POST -d "t=${token}" "${LOCAL}/wifi/scan"
    body "Scanning…" "page shows the scan" "${LOCAL}/wifi"
    body 'http-equiv="refresh" content="4;url=/wifi"' "page reloads while scanning" "${LOCAL}/wifi"
    sleep 8
    body 'data-s="' "networks are listed" "${LOCAL}/wifi"
    body '<span>· just now</span>' "list has its age" "${LOCAL}/wifi"
fi

echo "== ${PASSED} passed, ${FAILED} failed"
[ "$FAILED" -eq 0 ]
