#pragma once

void logLine(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));
void logAlways(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));
void logDebug(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));


