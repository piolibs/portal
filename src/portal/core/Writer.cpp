/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <algorithm>
#include <cstdio>

#include "Writer.h"

namespace portal {

void Writer::print(const char *text)
{
    write(text, strlen(text));
}

void Writer::printP(PGM_P text)
{
    // Flash is read by words on the ESP8266, copy it through a RAM buffer
    char buffer[64];
    size_t size = strlen_P(text);

    for (size_t offset = 0; offset < size; offset += sizeof(buffer))
    {
        size_t part = std::min(sizeof(buffer), size - offset);
        memcpy_P(buffer, text + offset, part);
        write(buffer, part);
    }
}

void Writer::html(const char *text)
{
    const char *start = text;

    for (const char *p = text; *p; ++p)
    {
        const char *entity = nullptr;

        switch (*p)
        {
        case '&': entity = "&amp;"; break;
        case '<': entity = "&lt;"; break;
        case '>': entity = "&gt;"; break;
        case '"': entity = "&quot;"; break;
        case '\'': entity = "&#39;"; break;
        default: continue;
        }

        write(start, p - start);
        print(entity);
        start = p + 1;
    }

    print(start);
}

void Writer::json(const char *text)
{
    const char *start = text;

    for (const char *p = text; *p; ++p)
    {
        unsigned char c = static_cast<unsigned char>(*p);

        // '<' is escaped too, so a value can't close a <script> element
        if (c >= 0x20 && c != '"' && c != '\\' && c != '<')
            continue;

        write(start, p - start);

        char escape[8];
        if (c == '"' || c == '\\')
            snprintf(escape, sizeof(escape), "\\%c", c);
        else
            snprintf(escape, sizeof(escape), "\\u%04x", c);
        print(escape);

        start = p + 1;
    }

    print(start);
}

} // namespace portal
