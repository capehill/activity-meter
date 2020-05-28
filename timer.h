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
void TimerDelay(ULONG seconds);

//double TimerTicksToSeconds(const uint64 ticks);
//double timer_ticks_to_ms(const uint64 ticks);
//double timer_ticks_to_us(const uint64 ticks);

struct TimeVal TimerGetSysTime();

