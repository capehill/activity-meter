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

#pragma once

#include <exec/types.h>

typedef struct TimerContext
{
    struct MsgPort* port;
    struct TimeRequest* request;
    BYTE device;
} TimerContext;

typedef enum ESignalType {
    ESignalType_Break,
    ESignalType_Timer
} ESignalType;

extern TimerContext timer;

BOOL TimerInit(TimerContext * tc);
void TimerQuit(TimerContext * tc);

uint32 TimerSignal(TimerContext * tc);

void TimerStart(TimerContext * tc, ULONG seconds, ULONG micros);
void TimerStop(TimerContext * tc);
void TimerHandleEvents(TimerContext * tc);

ESignalType TimerWaitForSignal(uint32 timerSig, const char* const name);

struct TimeVal TimerGetSysTime();

