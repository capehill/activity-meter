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

#include "timer.h"
#include "logger.h"

#include <proto/exec.h>
#include <proto/timer.h>
#include <dos/dos.h>

#include <stdio.h>

static ULONG frequency = 0;
static int users = 0;

TimerContext timer;

static void ReadFrequency(void)
{
    struct EClockVal clockVal;
    frequency = ITimer->ReadEClock(&clockVal);

    Log("Timer frequency %lu ticks / second", frequency);
}

BOOL TimerInit(TimerContext * tc)
{
    tc->port = NULL;
    tc->request = NULL;
    tc->device = -1;

    tc->port = IExec->AllocSysObjectTags(ASOT_PORT,
        ASOPORT_Name, "timer_port",
        TAG_DONE);

    if (!tc->port) {
        puts("Couldn't create timer port");
        goto out;
    }

    tc->request = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
        ASOIOR_Size, sizeof(struct TimeRequest),
        ASOIOR_ReplyPort, tc->port,
        TAG_DONE);

    if (!tc->request) {
        puts("Couldn't create timer IO request");
        goto out;
    }

    tc->device = IExec->OpenDevice(TIMERNAME, UNIT_WAITUNTIL,
        (struct IORequest *) tc->request, 0);

    if (tc->device) {
        puts("Couldn't open timer.device");
        goto out;
    }

    if (!ITimer) {
        ITimer = (struct TimerIFace *) IExec->GetInterface(
            (struct Library *) tc->request->Request.io_Device, "main", 1, NULL);

        if (!ITimer) {
            puts("Couldn't get Timer interface");
            goto out;
        }
    }

    if (!frequency) {
        ReadFrequency();
    }

    users++;

    return TRUE;

out:
    TimerQuit(tc);

    return FALSE;
}

void TimerQuit(TimerContext * tc)
{
    if ((--users <= 0) && ITimer) {
        //Log("ITimer user count %d, dropping it", users);
        IExec->DropInterface((struct Interface *) ITimer);
        ITimer = NULL;
    }

    if (tc->device == 0 && tc->request) {
        IExec->CloseDevice((struct IORequest *) tc->request);
        tc->device = -1;
    }

    if (tc->request) {
        IExec->FreeSysObject(ASOT_IOREQUEST, tc->request);
        tc->request = NULL;
    }

    if (tc->port) {
        IExec->FreeSysObject(ASOT_PORT, tc->port);
        tc->port = NULL;
    }
}

uint32 TimerSignal(TimerContext * tc)
{
    if (!tc->port) {
        Log("%s: timer port NULL", __func__);
        return 0;
    }

    return 1L << tc->port->mp_SigBit;
}

void TimerStart(TimerContext * tc, ULONG seconds, ULONG micros)
{
    struct TimeVal tv;
    struct TimeVal increment;

    if (!tc->request) {
        Log("%s: timer request NULL", __func__);
        return;
    }

    if (!ITimer) {
        Log("%s: ITimer NULL", __func__);
        return;
    }

    ITimer->GetSysTime(&tv);

    increment.Seconds = seconds;
    increment.Microseconds = micros;

    ITimer->AddTime(&tv, &increment);

    tc->request->Request.io_Command = TR_ADDREQUEST;
    tc->request->Time.Seconds = tv.Seconds;
    tc->request->Time.Microseconds = tv.Microseconds;

    IExec->SendIO((struct IORequest *) tc->request);
}

void TimerHandleEvents(TimerContext * tc)
{
    struct Message *msg;

    if (!tc->port) {
        Log("%s: timer port NULL", __func__);
        return;
    }

    while ((msg = IExec->GetMsg(tc->port))) {
        const int8 error = ((struct IORequest *)msg)->io_Error;

        if (error) {
            printf("Timer message received with code %d\n", error);
        }
    }
}

void TimerStop(TimerContext * tc)
{
    if (!tc->request) {
        Log("%s: timer request NULL", __func__);
        return;
    }

    if (!IExec->CheckIO((struct IORequest *) tc->request)) {
        Log("%s: aborting timer IO request %p", __func__, tc->request);
        IExec->AbortIO((struct IORequest *) tc->request);
        IExec->WaitIO((struct IORequest *) tc->request);
    }
}

ESignalType TimerWaitForSignal(uint32 timerSig, const char* const name)
{
    const uint32 wait = IExec->Wait(SIGBREAKF_CTRL_C | timerSig);

    if (wait & SIGBREAKF_CTRL_C) {
        puts("*** Control-C detected ***");
        return ESignalType_Break;
    }

    if (wait & timerSig) {
        printf("*** %s timer triggered ***\n", name);
    }

    return ESignalType_Timer;
}

struct TimeVal TimerGetSysTime()
{
    struct TimeVal tv;
    ITimer->GetSysTime(&tv);
    return tv;
}
