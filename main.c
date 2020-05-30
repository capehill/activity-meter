#include "gui.h"
#include "timer.h"
#include "common.h"
#include "version.h"
#include "logger.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/input.h>

#include <stdio.h>
#include <math.h>

static const char* const stackString __attribute__((used)) = "$STACK:64000";
static const char* const versionString __attribute__((used)) = "$VER:" VERSION_STRING;

typedef struct Counter
{
    size_t left;
    size_t middle;
    size_t right;
    size_t fourth;
    size_t fifth;
    size_t pixels;
    size_t called;
    size_t lastTime;
    size_t keys;
} Counter;

typedef struct Statistics
{
    size_t startTime;
    size_t startTimeCurrent;
    size_t activeMinutes;
    size_t activeSeconds;
    size_t breakMinutes;
    size_t breakSeconds;
    size_t totalBreakDuration;
    size_t currentBreakDuration;
    size_t breaks;
    BOOL breakRegistered;
} Statistics;

static Counter* counter;
static Statistics stats;

static const size_t BREAK_LENGTH = 5 * 60;

char* PixelsString()
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "Pixels travelled: %u", counter->pixels);
    return buf;
}

// All activity
// Current activity

void CalculateStats()
{
    if (stats.startTime == 0) {
        // Initialize
        stats.startTime = counter->lastTime;
        stats.startTimeCurrent = stats.startTime;
    }

    const size_t now = TimerGetSysTime().Seconds;

    stats.activeSeconds = counter->lastTime - stats.startTime;
    stats.activeMinutes = stats.activeSeconds / 60;

    stats.breakSeconds = counter->lastTime ? (now - counter->lastTime) : 0;
    stats.breakMinutes = stats.breakSeconds / 60;

    if (stats.breakSeconds == 0) {
        // Break ended
        stats.startTimeCurrent = counter->lastTime;
    }

    stats.currentBreakDuration = stats.breakSeconds;

    if (!stats.breakRegistered && stats.currentBreakDuration >= BREAK_LENGTH) {
        stats.breaks++;
        stats.breakRegistered = TRUE;
    } else if (stats.breakRegistered && stats.currentBreakDuration < BREAK_LENGTH) {
        stats.breakRegistered = FALSE;
    }

}

char* ActivityString()
{
    static char buf[64];
    snprintf(buf, sizeof(buf), "Activity time: %u min %u secs", stats.activeMinutes, stats.activeSeconds - (stats.activeMinutes * 60));
    return buf;
}

char* BreakString()
{
    static char buf[64];
    snprintf(buf, sizeof(buf), "Break time: %u min %u secs", stats.breakMinutes, stats.breakSeconds - (stats.breakMinutes * 60));
    return buf;
}

char* TotalBreaksString()
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "Total breaks: %u", stats.breaks);
    return buf;
}

char* MouseCounterString()
{
    static char buf[64];
    snprintf(buf, sizeof(buf), "LMB: %u, MMB: %u, RMB: %u, 4th: %u, 5th: %u",
        counter->left, counter->middle, counter->right, counter->fourth, counter->fifth);
    return buf;
}

char* KeyCounterString()
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "Keys pressed: %u", counter->keys);
    return buf;
}

#if 0
static struct InputEvent* InputEventHandler(struct InputEvent* events, APTR data)
{
    if (data) {
        Counter* mc = (Counter *)data;
        mc->called++;
    }

    return events;
}
#else
static struct InputEvent* InputEventHandler(struct InputEvent* events, APTR data)
{
    if (!(events && data)) {
        return events;
    }

    struct InputEvent* e = events;
    Counter* mc = (Counter *)data;

    mc->called++;

    while (e) {
        if (e->ie_Class == IECLASS_RAWMOUSE) {
            const int x = e->ie_X;
            const int y = e->ie_Y;

            mc->lastTime = e->ie_TimeStamp.Seconds;
            mc->pixels += sqrt(x * x + y * y);

            switch (e->ie_Code) {
                case IECODE_LBUTTON:
                    mc->left++;
                    break;
                case IECODE_MBUTTON:
                    mc->middle++;
                    break;
                case IECODE_RBUTTON:
                    mc->right++;
                    break;
                case IECODE_4TH_BUTTON:
                    mc->fourth++;
                    break;
                case IECODE_5TH_BUTTON:
                    mc->fifth++;
                    break;
            }
        } else if (e->ie_Class == IECLASS_RAWKEY && !(e->ie_Code & IECODE_UP_PREFIX)) {
            mc->lastTime = e->ie_TimeStamp.Seconds;
            mc->keys++;
        }

        e = e->ie_NextEvent;
    }

    return events;
}
#endif

static void SendCommand(struct IOStdReq * req, struct Interrupt * is, const int command)
{
    Log("Send command %d", command);

    req->io_Command = command;
    req->io_Data = is;

    const int err = IExec->DoIO((struct IORequest *)req);

    Log("err %d, %d", err, req->io_Error);
}

static void SetupHandler(struct IOStdReq * req)
{
    counter = IExec->AllocVecTags(sizeof(Counter),
        AVT_Type, MEMF_SHARED,
        AVT_ClearWithValue, 0,
        TAG_DONE);

    if (!counter) {
        puts("Failed to allocate event handler data");
        return;
    }

    struct Interrupt* is = (struct Interrupt *)IExec->AllocSysObjectTags(ASOT_INTERRUPT,
        ASOINTR_Code, InputEventHandler,
        ASOINTR_Data, counter,
        TAG_DONE);

    if (is) {
        is->is_Node.ln_Name = "Activity meter";
        is->is_Node.ln_Pri = 101;

        SendCommand(req, is, IND_ADDHANDLER);

        RunGui();

        SendCommand(req, is, IND_REMHANDLER);

        IExec->FreeSysObject(ASOT_INTERRUPT, is);

        Log("Stats: left %zu, middle %zu, right %zu. Distance %zu pixels, called %zu times, keys %zu",
            counter->left,
            counter->middle,
            counter->right,
            counter->pixels,
            counter->called,
            counter->keys);
    } else {
        puts("Failed to allocate interrupt");
    }

    IExec->FreeVec(counter);
}

#if 1
static void CheckStack()
{
    struct Task* task = IExec->FindTask(NULL);

    uint32* upper = (uint32 *)task->tc_SPUpper;
    uint32* lower = (uint32 *)task->tc_SPLower;

    for (uint32* ptr = lower; ptr <= upper; ptr++) {
        if (*ptr != 0 && *ptr != 0xbad1bad3) {
            Log("....%u bytes left on stack, used %u", (ptr - lower) * 4, (upper - ptr) * 4);
            return;
        }
    }
}
#endif

int main(void)
{
    if (!TimerInit(&timer)) {
        return -1;
    }

    struct MsgPort* port = IExec->AllocSysObjectTags(ASOT_PORT,
        ASOPORT_Name, "id_port",
        TAG_DONE);

    if (port) {
        struct IOStdReq* req = (struct IOStdReq *)IExec->AllocSysObjectTags(ASOT_IOREQUEST,
            ASOIOR_Size, sizeof(struct IOStdReq),
            ASOIOR_ReplyPort, port,
            TAG_DONE);

        if (req) {
            BYTE device = IExec->OpenDevice(INPUTNAME, 0, (struct IORequest *)req, 0);

            if (device == 0) {
                SetupHandler(req);
                IExec->CloseDevice((struct IORequest *)req);
            } else {
                puts("Failed to open input.device");
            }

            IExec->FreeSysObject(ASOT_IOREQUEST, req);
        } else {
            puts("Failed to allocate IO request");
        }

        IExec->FreeSysObject(ASOT_PORT, port);
    } else {
        puts("Failed to allocate message port");
    }

    TimerQuit(&timer);

    CheckStack();

    return 0;
}
