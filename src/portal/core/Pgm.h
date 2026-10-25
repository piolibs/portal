/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

// Flash string helpers: the core is built for the ESP8266 and for host tests

#include <cstring>

#if defined(ARDUINO)
#include <pgmspace.h>
#else
#define PROGMEM
#define PGM_P const char *
#define PSTR(s) (s)
#define strlen_P strlen
#define memcpy_P memcpy
#define strcmp_P strcmp
#endif
