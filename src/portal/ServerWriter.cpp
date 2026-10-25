/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ServerWriter.h"

namespace portal {

ServerWriter::ServerWriter(ESP8266WebServer &server, int code, PGM_P type)
    : mServer(server)
{
    mChunked = mServer.chunkedResponseModeStart_P(code, type);

    if (!mChunked)
    {
        // HTTP/1.0 has no chunks: the server closes the connection after
        // the body, that ends it
        mServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
        mServer.send_P(code, type, "");
    }
}

ServerWriter::~ServerWriter()
{
    flush();

    if (mChunked)
        mServer.chunkedResponseFinalize();
}

void ServerWriter::write(const char *data, size_t size)
{
    while (size)
    {
        size_t part = std::min(size, sizeof(mBuffer) - mSize);
        memcpy(mBuffer + mSize, data, part);

        mSize += part;
        data += part;
        size -= part;

        if (mSize == sizeof(mBuffer))
            flush();
    }
}

void ServerWriter::flush()
{
    if (mSize)
        mServer.sendContent(mBuffer, mSize);
    mSize = 0;
}

} // namespace portal
