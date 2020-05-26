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
    OID_Activity,
    OID_BreakDuration,
    OID_Breaks,
    OID_MouseCounter,
    OID_KeyCounter,
    OID_Count // KEEP LAST
};

enum EGadget {
    GID_Trace,
    GID_Pause,
    GID_StartProfiling,
    GID_FinishProfiling
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

static char* getApplicationName()
{
    #define maxPathLen 255

    static char pathBuffer[maxPathLen];

    if (IDOS->GetCliProgramName(pathBuffer, maxPathLen - 1)) {
        logLine("GetCliProgramName: '%s'", pathBuffer);
    } else {
        logLine("Failed to get CLI program name, checking task node");

        struct Task* me = IExec->FindTask(NULL);
        snprintf(pathBuffer, maxPathLen, "%s", ((struct Node *)me)->ln_Name);
    }

    logLine("Application name: '%s'", pathBuffer);

    return pathBuffer;
}

static struct DiskObject* getDiskObject()
{
    struct DiskObject *diskObject = NULL;

    BPTR oldDir = IDOS->SetCurrentDir(IDOS->GetProgramDir());
    diskObject = IIcon->GetDiskObject(getApplicationName());
    IDOS->SetCurrentDir(oldDir);

    return diskObject;
}

static void show_about_window()
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

static Object* create_gui()
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
        WINDOW_Icon, getDiskObject(),
        WINDOW_AppPort, port, // Iconification needs it
        WINDOW_GadgetHelp, TRUE,
        WINDOW_NewMenu, menus,
        WINDOW_Layout, IIntuition->NewObject(NULL, "layout.gadget",
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_AddChild, IIntuition->NewObject(NULL, "layout.gadget",
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                LAYOUT_Label, "Information",
                LAYOUT_BevelStyle, BVS_GROUP,
                LAYOUT_AddChild, objects[OID_Activity] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, activity_string(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_BreakDuration] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, break_string(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_Breaks] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, total_breaks_string(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_MouseCounter] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, mouse_counter_string(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_Pixels] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, pixels_string(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                LAYOUT_AddChild, objects[OID_KeyCounter] = IIntuition->NewObject(NULL, "button.gadget",
                    GA_ReadOnly, TRUE,
                    GA_Text, key_counter_string(),
                    BUTTON_BevelStyle, BVS_NONE,
                    BUTTON_Transparent, TRUE,
                    TAG_DONE),
                TAG_DONE), // vertical layout.gadget

            TAG_DONE), // vertical layout.gadget
        TAG_DONE); // window.class
}

static void refresh_object(Object * object)
{
    IIntuition->RefreshGList((struct Gadget *)object, window, NULL, -1);
}

static void refresh()
{
    IIntuition->SetAttrs(objects[OID_MouseCounter], GA_Text, mouse_counter_string(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_KeyCounter], GA_Text, key_counter_string(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_Pixels], GA_Text, pixels_string(), TAG_DONE);

    IIntuition->SetAttrs(objects[OID_Activity], GA_Text, activity_string(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_BreakDuration], GA_Text, break_string(), TAG_DONE);
    IIntuition->SetAttrs(objects[OID_Breaks], GA_Text, total_breaks_string(), TAG_DONE);

    refresh_object(objects[OID_MouseCounter]);
    refresh_object(objects[OID_KeyCounter]);
    refresh_object(objects[OID_Pixels]);

    refresh_object(objects[OID_Activity]);
    refresh_object(objects[OID_BreakDuration]);
    refresh_object(objects[OID_Breaks]);
}

static void handle_gadgets(int id)
{
    //printf("Gadget %d\n", id);

    switch (id) {
        case GID_Trace:
            break;
    }
}

static void handle_iconify(void)
{
    window = NULL;
    IIntuition->IDoMethod(objects[OID_Window], WM_ICONIFY);
}

static void handle_uniconify(void)
{
    window = (struct Window *)IIntuition->IDoMethod(objects[OID_Window], WM_OPEN);
}

static BOOL handle_menupick(uint16 menuNumber)
{
    struct MenuItem* item = IIntuition->ItemAddress(window->MenuStrip, menuNumber);

    if (item) {
        const EMenu id = (EMenu)GTMENUITEM_USERDATA(item);
        //printf("menu %x, menu num %d, item num %d, userdata %d\n", menuNumber, MENUNUM(menuNumber), ITEMNUM(menuNumber), (EMenu)GTMENUITEM_USERDATA(item));
        switch (id) {
            case MID_Iconify: handle_iconify(); break;
            case MID_About: show_about_window(); break;
            case MID_Quit: return FALSE;
        }
    }

    return TRUE;
}

static void handle_events(void)
{
    uint32 signal = 0;
    IIntuition->GetAttr(WINDOW_SigMask, objects[OID_Window], &signal);

    const uint32 timerSignal = timer_signal(&timer);

    BOOL running = TRUE;

    while (running) {
        uint32 wait = IExec->Wait(signal | SIGBREAKF_CTRL_C | timerSignal);

        if (wait & SIGBREAKF_CTRL_C) {
            puts("*** Break ***");
            running = FALSE;
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
                    case WMHI_GADGETUP:
                        handle_gadgets(result & WMHI_GADGETMASK);
                        break;
                    case WMHI_ICONIFY:
                        handle_iconify();
                        break;
                    case WMHI_UNICONIFY:
                        handle_uniconify();
                        break;
                    case WMHI_MENUPICK:
                        running = handle_menupick(result & WMHI_MENUMASK);
                        break;
                }
            }
        }

        if (wait & timerSignal) {
            timer_handle_events(&timer);
            if (window) {
                refresh();
            }
            timer_start(&timer, seconds, micros);
        }
    }
}

// When profiling, Pause/Resume buttons are disabled
void run_gui()
{
	port = IExec->AllocSysObjectTags(ASOT_PORT,
		ASOPORT_Name, "app_port",
		TAG_DONE);

    objects[OID_Window] = create_gui();

    if (objects[OID_Window]) {
        if ((window = (struct Window *)IIntuition->IDoMethod(objects[OID_Window], WM_OPEN))) {
            timer_start(&timer, seconds, micros);
            handle_events();
        } else {
            puts("Failed to open window");
        }

        timer_stop(&timer);

        IIntuition->DisposeObject(objects[OID_Window]);
    } else {
        puts("Failed to create window");
    }

    if (port) {
        IExec->FreeSysObject(ASOT_PORT, port);
    }
}
