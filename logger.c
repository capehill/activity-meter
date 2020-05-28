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

