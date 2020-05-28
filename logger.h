#pragma once

void Log(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));
void LogDebug(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));


