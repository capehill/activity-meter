/*

MIT License

Copyright (c) 2020 Juha Niemimaki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "logger.h"

#include <proto/exec.h>

#include <stdio.h>
#include <stdarg.h>

static BOOL verbose = FALSE;

static void LogImpl(const char * fmt, va_list ap)
{
    char buffer[16 * 1024];
    const int len = vsnprintf(buffer, sizeof(buffer), fmt, ap);

    char* ptr = buffer;

    while (TRUE) {
        char serialBuffer[4 * 1024]; // Sashimi has 4k buffer
        const size_t wantedToWrite = snprintf(serialBuffer, sizeof(serialBuffer), "%s\n", ptr);

        IExec->DebugPrintF("%s", serialBuffer);

        if (wantedToWrite < sizeof(serialBuffer)) {
            break;
        }

        ptr += sizeof(serialBuffer) - 1;
    }

    if (len >= (int)sizeof(buffer)) {
        IExec->DebugPrintF("*** Line truncated: %d bytes buffer needed ***\n", len);
    }
}

void Log(const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    LogImpl(fmt, ap);

    va_end(ap);
}

void LogDebug(const char * fmt, ...)
{
    if (verbose) {
        va_list ap;
        va_start(ap, fmt);

        LogImpl(fmt, ap);

        va_end(ap);
    }
}

