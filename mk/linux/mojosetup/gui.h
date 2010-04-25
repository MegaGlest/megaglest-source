/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef _INCL_GUI_H_
#define _INCL_GUI_H_

#include "universal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MOJOGUI_PRIORITY_NEVER_TRY = 0,
    MOJOGUI_PRIORITY_USER_REQUESTED,
    MOJOGUI_PRIORITY_TRY_FIRST,
    MOJOGUI_PRIORITY_TRY_NORMAL,
    MOJOGUI_PRIORITY_TRY_LAST,
    MOJOGUI_PRIORITY_TRY_ABSOLUTELY_LAST,
    MOJOGUI_PRIORITY_TOTAL
} MojoGuiPluginPriority;


typedef enum
{
    MOJOGUI_NO,
    MOJOGUI_YES,
    MOJOGUI_ALWAYS,
    MOJOGUI_NEVER
} MojoGuiYNAN;



/*
 * Abstract GUI interfaces.
 */

typedef struct MojoGuiSetupOptions MojoGuiSetupOptions;
struct MojoGuiSetupOptions
{
    const char *description;
    const char *tooltip;
    boolean value;
    boolean is_group_parent;
    uint64 size;
    int opaque;  // GUI drivers shouldn't touch this.
    void *guiopaque;  // For GUI drivers. App won't touch or free this.
    MojoGuiSetupOptions *next_sibling;
    MojoGuiSetupOptions *child;
};


typedef enum
{
    MOJOGUI_SPLASH_NONE,
    MOJOGUI_SPLASH_TOP,
    MOJOGUI_SPLASH_LEFT,
    MOJOGUI_SPLASH_RIGHT,
    MOJOGUI_SPLASH_BOTTOM,
    MOJOGUI_SPLASH_BACKGROUND,
} MojoGuiSplashPos;

typedef struct MojoGuiSplash MojoGuiSplash;
struct MojoGuiSplash
{
    const uint8 *rgba;  // framebuffer.
    uint32 w;  // width in pixels.
    uint32 h;  // height in pixels.
    MojoGuiSplashPos position; // where to put the splash.
};


#define MOJOGUI_ENTRY_POINT MojoSetup_Gui_GetInterface
#define MOJOGUI_ENTRY_POINT_STR DEFINE_TO_STR(MOJOGUI_ENTRY_POINT)

// Increment this value when MojoGui's structure changes.
#define MOJOGUI_INTERFACE_REVISION 6

typedef struct MojoGui MojoGui;
struct MojoGui
{
    uint8 (*priority)(boolean istty);
    const char* (*name)(void);
    boolean (*init)(void);
    void (*deinit)(void);
    void (*msgbox)(const char *title, const char *text);
    boolean (*promptyn)(const char *title, const char *text, boolean def);
    MojoGuiYNAN (*promptynan)(const char *title, const char *text, boolean def);
    boolean (*start)(const char *title, const MojoGuiSplash *splash);
    void (*stop)(void);
    int (*readme)(const char *name, const uint8 *data, size_t len,
                  boolean can_back, boolean can_fwd);
    int (*options)(MojoGuiSetupOptions *opts,
                   boolean can_back, boolean can_fwd);
    char * (*destination)(const char **recommendations, int recnum,
                          int *command, boolean can_back, boolean can_fwd);
    int (*productkey)(const char *desc, const char *fmt, char *buf,
                      const int buflen, boolean can_back, boolean can_fwd);
    boolean (*insertmedia)(const char *medianame);
    void (*progressitem)(void);
    boolean (*progress)(const char *type, const char *component,
                        int percent, const char *item, boolean can_cancel);
    void (*final)(const char *msg);
};

typedef const MojoGui* (*MojoGuiEntryPoint)(int revision,
                                            const MojoSetupEntryPoints *e);

#if !BUILDING_EXTERNAL_PLUGIN
extern const MojoGui *GGui;
const MojoGui *MojoGui_initGuiPlugin(void);
void MojoGui_deinitGuiPlugin(void);
#else

__EXPORT__ const MojoGui *MOJOGUI_ENTRY_POINT(int revision,
                                              const MojoSetupEntryPoints *e);

/*
 * We do this as a macro so we only have to update one place, and it
 *  enforces some details in the plugins. Without effort, plugins don't
 *  support anything but the latest version of the interface.
 */
#define MOJOGUI_PLUGIN(module) \
static const MojoSetupEntryPoints *entry = NULL; \
static uint8 MojoGui_##module##_priority(boolean istty); \
static const char* MojoGui_##module##_name(void) { return #module; } \
static boolean MojoGui_##module##_init(void); \
static void MojoGui_##module##_deinit(void); \
static void MojoGui_##module##_msgbox(const char *title, const char *text); \
static boolean MojoGui_##module##_promptyn(const char *t1, const char *t2, \
                                           boolean d); \
static MojoGuiYNAN MojoGui_##module##_promptynan(const char *t1, \
                                                 const char *t2, boolean d); \
static boolean MojoGui_##module##_start(const char *t, \
                                        const MojoGuiSplash *splash); \
static void MojoGui_##module##_stop(void); \
static int MojoGui_##module##_readme(const char *name, const uint8 *data, \
                                     size_t len, boolean can_back, \
                                     boolean can_fwd); \
static int MojoGui_##module##_options(MojoGuiSetupOptions *opts, \
                              boolean can_back, boolean can_fwd); \
static char *MojoGui_##module##_destination(const char **r, int recnum, \
                            int *command, boolean can_back, boolean can_fwd); \
static int MojoGui_##module##_productkey(const char *desc, const char *fmt, \
                            char *buf, const int buflen, boolean can_back, \
                            boolean can_fwd); \
static boolean MojoGui_##module##_insertmedia(const char *medianame); \
static void MojoGui_##module##_progressitem(void); \
static boolean MojoGui_##module##_progress(const char *typ, const char *comp, \
                                           int percent, const char *item, \
                                           boolean can_cancel); \
static void MojoGui_##module##_final(const char *msg); \
const MojoGui *MojoGuiPlugin_##module(int rev, const MojoSetupEntryPoints *e) \
{ \
    if (rev == MOJOGUI_INTERFACE_REVISION) { \
        static const MojoGui retval = { \
            MojoGui_##module##_priority, \
            MojoGui_##module##_name, \
            MojoGui_##module##_init, \
            MojoGui_##module##_deinit, \
            MojoGui_##module##_msgbox, \
            MojoGui_##module##_promptyn, \
            MojoGui_##module##_promptynan, \
            MojoGui_##module##_start, \
            MojoGui_##module##_stop, \
            MojoGui_##module##_readme, \
            MojoGui_##module##_options, \
            MojoGui_##module##_destination, \
            MojoGui_##module##_productkey, \
            MojoGui_##module##_insertmedia, \
            MojoGui_##module##_progressitem, \
            MojoGui_##module##_progress, \
            MojoGui_##module##_final, \
        }; \
        entry = e; \
        return &retval; \
    } \
    return NULL; \
} \

#define CREATE_MOJOGUI_ENTRY_POINT(module) \
const MojoGui *MOJOGUI_ENTRY_POINT(int rev, const MojoSetupEntryPoints *e) \
{ \
    return MojoGuiPlugin_##module(rev, e); \
} \


// Redefine things that need to go through the plugin entry point interface,
//  so plugins calling into the MojoSetup core can use the same code as the
//  rest of the app.

#ifdef _
#undef _
#endif
#define _(x) entry->translate(x)

#ifdef xmalloc
#undef xmalloc
#endif
#define xmalloc(x) entry->xmalloc(x)

#ifdef xrealloc
#undef xrealloc
#endif
#define xrealloc(x,y) entry->xrealloc(x,y)

#ifdef xstrdup
#undef xstrdup
#endif
#define xstrdup(x) entry->xstrdup(x)

#ifdef xstrncpy
#undef xstrncpy
#endif
#define xstrncpy(x,y,z) entry->xstrcpy(x,y,z)

#ifdef logWarning
#undef logWarning
#endif
#define logWarning entry->logWarning

#ifdef logError
#undef logError
#endif
#define logError entry->logError

#ifdef logInfo
#undef logInfo
#endif
#define logInfo entry->logInfo

#ifdef logDebug
#undef logDebug
#endif
#define logDebug entry->logDebug

#ifdef format
#undef format
#endif
#define format entry->format

#ifdef numstr
#undef numstr
#endif
#define numstr(x) entry->numstr(x)

#ifdef ticks
#undef ticks
#endif
#define ticks() entry->ticks()

#ifdef utf8codepoint
#undef utf8codepoint
#endif
#define utf8codepoint(x) entry->utf8codepoint(x)

#ifdef utf8len
#undef utf8len
#endif
#define utf8len(x) entry->utf8len(x)

#ifdef splitText
#undef splitText
#endif
#define splitText(w,x,y,z) entry->splitText(w,x,y,z)

#ifdef isValidProductKey
#undef isValidProductKey
#endif
#define isValidProductKey(x,y) entry->isValidProductKey(x,y)

#endif


/*
 * make some decisions about which GUI plugins to build...
 *  We list them all here, but some are built, some aren't. Some are DLLs,
 *  some aren't...
 */

const MojoGui *MojoGuiPlugin_stdio(int rev, const MojoSetupEntryPoints *e);
const MojoGui *MojoGuiPlugin_ncurses(int rev, const MojoSetupEntryPoints *e);
const MojoGui *MojoGuiPlugin_gtkplus2(int rev, const MojoSetupEntryPoints *e);
const MojoGui *MojoGuiPlugin_www(int rev, const MojoSetupEntryPoints *e);
const MojoGui *MojoGuiPlugin_cocoa(int rev, const MojoSetupEntryPoints *e);

// !!! FIXME: Qt? KDE? Gnome? Console? wxWidgets?

#ifdef __cplusplus
}
#endif

#endif

// end of gui.h ...

