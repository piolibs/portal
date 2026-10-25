/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <ESP8266WebServer.h>

#include "core/Writer.h"

namespace portal {

// Chunked response through a small buffer: a page is sent in parts of the
// buffer size and never held in the heap
class ServerWriter : public Writer {
public:
    ServerWriter(ESP8266WebServer &server, int code, PGM_P type);
    ~ServerWriter() override;

    void write(const char *data, size_t size) override;

private:
    void flush();

    ESP8266WebServer &mServer;
    bool mChunked;
    size_t mSize = 0;
    char mBuffer[256];
};

} // namespace portal
