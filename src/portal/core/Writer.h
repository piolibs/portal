/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <cstddef>
#include <string>

#include "Pgm.h"

namespace portal {

// Output of a page, written in parts so a page is never built in the heap
class Writer {
public:
    virtual ~Writer() = default;

    virtual void write(const char *data, size_t size) = 0;

    void print(const char *text);
    void print(const std::string &text) { write(text.data(), text.size()); }

    // Text stored in flash (PSTR / PROGMEM)
    void printP(PGM_P text);

    // Text escaped for HTML content and attribute values
    void html(const char *text);
    void html(const std::string &text) { html(text.c_str()); }

    // Text escaped for a JSON string, without the quotes
    void json(const char *text);
    void json(const std::string &text) { json(text.c_str()); }
};

// Writer to a string, for tests and small responses
class StringWriter : public Writer {
public:
    void write(const char *data, size_t size) override { mData.append(data, size); }

    const std::string &str() const { return mData; }

private:
    std::string mData;
};

} // namespace portal
