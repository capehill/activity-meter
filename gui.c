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

#include "gui.h"
#include "logger.h"
#include "timer.h"
#include "version.h"
#include "common.h"

#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>

#include <classes/requester.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <libraries/gadtools.h>

#include <stdio.h>

enum EObject {
    OID_Window,
    OID_AboutWindow,
    OID_Pixels,
    OID_AllActivity,
    OID_CurrentActivity,
    OID_BreakDuration,
    OID_Breaks,
    OID_MouseCounter,
    OID_KeyCounter,
    OID_Count // KEEP LAST
};

typedef enum EMenu {
    MID_Iconify = 1,
    MID_About,
    MID_Quit
} EMenu;

static struct NewMenu menus[] = {
    { NM_TITLE, "Activity meter", NULL, 0, 0, NULL },
    { NM_ITEM, "Iconify", "I", 0, 0, (APTR)MID_Iconify },
    { NM_ITEM, "About...", "?", 0, 0, (APTR)MID_About },
    { NM_ITEM, "Quit", "Q", 0, 0, (APTR)MID_Quit },
    { NM_END, NULL, NULL, 0, 0, NULL }
};

static Object* objects[OID_Count];
static struct Window* window;
static struct MsgPort* port;

static const ULONG seconds = 1;
static const ULONG micros = 0;

static char* GetApplicationName()
{
    #define maxPathLen 255

    static char pathBuffer[maxPathLen];

    if (IDOS->GetCliProgramName(pathBuffer, maxPathLen - 1)) {
        Log("GetCliProgramName: '%s'", pathBuffer);
    } else {
        Log("Failed to get CLI program name, checking task node");

        struct Task* me = IExec->FindTask(NULL);
        snprintf(pathBuffer, maxPathLen, "%s", ((struct Node *)me)->ln_Name);
    }

    Log("Application name: '%s'", pathBuffer);

    return pathBuffer;
}

static struct DiskObject* MyGetDiskObject()
{
    struct DiskObject *diskObject = NULL;

    BPTR oldDir = IDOS->SetCurrentDir(IDOS->GetProgramDir());
    diskObject = IIcon->GetDiskObject(GetApplicationName());
    IDOS->SetCurrentDir(oldDir);

    return diskObject;
}

static void ShowAboutWindow()
{
    objects[OID_AboutWindow] = IIntuition->NewObject(NULL, "requester.class",
        REQ_TitleText, "About Activity meter",
        REQ_BodyText, VERSION_STRING DATE_STRING,
        REQ_GadgetText, "_Ok",
        REQ_Image, REQIMAGE_INFO,
        TAG_DONE);

    if (objects[OID_AboutWindow]) {
        IIntuition->SetAttrs(objects[OID_Window], WA_BusyPointer, TRUE, TAG_DONE);
        IIntuition->IDoMethod(objects[OID_AboutWindow], RM_OPENREQ, NULL, window, NULL, TAG_DONE);
        IIntuition->SetAttrs(objects[OID_Window], WA_BusyPointer, FALSE, TAG_DONE);
        IIntuition->DisposeObject(objects[OID_AboutWindow]);
        objects[OID_AboutWindow] = NULL;
    }
}

static Object* CreateGui()
{
    return IIntuition->NewObject(NULL, "window.class",
        WA_ScreenTitle, VERSION_STRING DATE_STRING,
        WA_Title, NAME_STRING,
        WA_Activate, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_Width, 200,
        WA_Height, 50,
        WINDOW_Position, WPOS_CENTERMOUSE,
        WINDOW_IconifyGadget, TRUE,
        WINDOW_Icon, MyGetDiskObject(),
        WINDOW_AppPort, port, // Iconification needs it
        WINDOW_GadgetHelp, TRUE,
        WINDOW_NewMenu, menus,
        WINDOW_Layout, IIntuition->NewObject(NULL, "layout.gadget",
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_AddChild, IIntuition->NewObject(NULL, "layout.gadget",
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                LAYOUT_Label, "Information",
                LAYOUT_BevelStyle, BVS_GROUP,
                LAYOUT_AddChild, objects[OID_AllActivity] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, AllActivityString(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_CurrentActivity] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, CurrentActivityString(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_BreakDuration] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, BreakString(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_Breaks] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, TotalBreaksString(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_MouseCounter] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, MouseCounterString(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_Pixels] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, PixelsString(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_KeyCounter] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, KeyCounterString(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                TAG_DONE), // vertical layout.gadget

            TAG_DONE), // vertical layout.gadget
        TAG_DONE); // window.class
}

static void RefreshObject(Object * object)
{
    IIntuition->RefreshGList((struct Gadget *)object, window, NULL, -1);
}

static void Refresh()
{
    CalculateStats();

    IIntuition->SetAttrs(objects[OID_MouseCounter], GA_Text, MouseCounterString(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_KeyCounter], GA_Text, KeyCounterString(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_Pixels], GA_Text, PixelsString(), TAG_DONE);

    IIntuition->SetAttrs(objects[OID_AllActivity], GA_Text, AllActivityString(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_CurrentActivity], GA_Text, CurrentActivityString(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_BreakDuration], GA_Text, BreakString(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_Breaks], GA_Text, TotalBreaksString(), TAG_DONE);

    RefreshObject(objects[OID_MouseCounter]);
    RefreshObject(objects[OID_KeyCounter]);
    RefreshObject(objects[OID_Pixels]);

    RefreshObject(objects[OID_AllActivity]);
    RefreshObject(objects[OID_CurrentActivity]);
    RefreshObject(objects[OID_BreakDuration]);
    RefreshObject(objects[OID_Breaks]);
}

static void HandleIconify(void)
{
    window = NULL;
    IIntuition->IDoMethod(objects[OID_Window], WM_ICONIFY);
}

static void HandleUniconify(void)
{
    window = (struct Window *)IIntuition->IDoMethod(objects[OID_Window], WM_OPEN);
}

static BOOL HandleMenuPick(uint16 menuNumber)
{
    struct MenuItem* item = IIntuition->ItemAddress(window->MenuStrip, menuNumber);

    if (item) {
        const EMenu id = (EMenu)GTMENUITEM_USERDATA(item);
        //printf("menu %x, menu num %d, item num %d, userdata %d\n", menuNumber, MENUNUM(menuNumber), ITEMNUM(menuNumber), (EMenu)GTMENUITEM_USERDATA(item));
        switch (id) {
            case MID_Iconify: HandleIconify(); break;
            case MID_About: ShowAboutWindow(); break;
            case MID_Quit: return FALSE;
        }
    }

    return TRUE;
}

static void HandleEvents(void)
{
    uint32 signal = 0;
    IIntuition->GetAttr(WINDOW_SigMask, objects[OID_Window], &signal);

    const uint32 timerSignal = TimerSignal(&timer);

    BOOL running = TRUE;

    while (running) {
        uint32 wait = IExec->Wait(signal | SIGBREAKF_CTRL_C | timerSignal);

        if (wait & SIGBREAKF_CTRL_C) {
            puts("*** Break ***");
            break;
        }

        if (wait & signal) {
            uint32 result;
            int16 code = 0;

            while ((result = IIntuition->IDoMethod(objects[OID_Window], WM_HANDLEINPUT, &code)) != WMHI_LASTMSG) {
                switch (result & WMHI_CLASSMASK) {
                    case WMHI_CLOSEWINDOW:
                        running = FALSE;
                        break;
                    case WMHI_ICONIFY:
                        HandleIconify();
                        break;
                    case WMHI_UNICONIFY:
                        HandleUniconify();
                        break;
                    case WMHI_MENUPICK:
                        running = HandleMenuPick(result & WMHI_MENUMASK);
                        break;
                    default:
                        //Log("Unknown event %lu", result);
                        break;
                }
            }
        }

        if (wait & timerSignal) {
            TimerHandleEvents(&timer);
            if (window) {
                Refresh();
            }
            TimerStart(&timer, seconds, micros);
        }
    }
}

void RunGui()
{
	port = IExec->AllocSysObjectTags(ASOT_PORT,
		ASOPORT_Name, "app_port",
		TAG_DONE);

    objects[OID_Window] = CreateGui();

    if (objects[OID_Window]) {
        if ((window = (struct Window *)IIntuition->IDoMethod(objects[OID_Window], WM_OPEN))) {
            TimerStart(&timer, seconds, micros);
            HandleEvents();
        } else {
            puts("Failed to open window");
        }

        TimerStop(&timer);

        IIntuition->DisposeObject(objects[OID_Window]);
    } else {
        puts("Failed to create window");
    }

    if (port) {
        IExec->FreeSysObject(ASOT_PORT, port);
    }
}
