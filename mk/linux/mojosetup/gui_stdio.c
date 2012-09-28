/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if !SUPPORT_GUI_STDIO
#error Something is wrong in the build system.
#endif

#define BUILDING_EXTERNAL_PLUGIN 1
#include "gui.h"

MOJOGUI_PLUGIN(stdio)

#if !GUI_STATIC_LINK_STDIO
CREATE_MOJOGUI_ENTRY_POINT(stdio)
#endif

#include <ctype.h>

static char *lastProgressType = NULL;
static char *lastComponent = NULL;
static uint32 percentTicks = 0;

static int read_stdin(char *buf, int len)
{
    if (fgets(buf, len, stdin) == NULL)
        return -1;

    len = strlen(buf) - 1;
    while ( (len >= 0) && ((buf[len] == '\n') || (buf[len] == '\r')) )
        buf[len--] = '\0';

    return len+1;
} // read_stdin


static int readstr(const char *prompt, char *buf, int len,
                   boolean back, boolean fwd)
{
    // !!! FIXME: if read_stdin() returns -1, we return 0, which makes it
    // !!! FIXME:  indistinguishable from "user hit enter" ... maybe we should
    // !!! FIXME:  abort in read_stdin() if i/o fails?

    int retval = 0;
    char *backstr = (back) ? xstrdup(_("back")) : NULL;

    if (prompt != NULL)
        printf("%s\n", prompt);

    if (back)
    {
        char *fmt = xstrdup(_("Type '%0' to go back."));
        char *msg = format(fmt, backstr);
        printf("%s\n", msg);
        free(msg);
        free(fmt);
    } // if

    if (fwd)
    {
        printf("%s", _("Press enter to continue."));
        printf("\n");
    } // if

    printf("%s",_("> "));
    fflush(stdout);

    if ((retval = read_stdin(buf, len)) >= 0)
    {
        if ((back) && (strcmp(buf, backstr) == 0))  // !!! FIXME: utf8casecmp?
            retval = -1;
    } // if

    free(backstr);
    return retval;
} // readstr


static uint8 MojoGui_stdio_priority(boolean istty)
{
    // if not a tty and no other GUI plugins worked out, let the base
    //  application try to spawn a terminal and try again. If it can't do so,
    //  it will panic() and thus end the process, so we don't end up blocking
    //  on some prompt the user can't see.

    if (!istty)
        return MOJOGUI_PRIORITY_NEVER_TRY;

    return MOJOGUI_PRIORITY_TRY_ABSOLUTELY_LAST;  // always a last resort.
} // MojoGui_stdio_priority


static boolean MojoGui_stdio_init(void)
{
    percentTicks = 0;
    return true;  // always succeeds.
} // MojoGui_stdio_init


static void MojoGui_stdio_deinit(void)
{
    free(lastProgressType);
    free(lastComponent);
    lastProgressType = NULL;
    lastComponent = NULL;
} // MojoGui_stdio_deinit


static void MojoGui_stdio_msgbox(const char *title, const char *text)
{
    char buf[128];
    char *fmt = xstrdup(_("NOTICE: %0\n[hit enter]"));
    char *msg = format(fmt, text);
    printf("%s\n", msg);
    free(msg);
    free(fmt);
    fflush(stdout);
    read_stdin(buf, sizeof (buf));
} // MojoGui_stdio_msgbox


static boolean MojoGui_stdio_promptyn(const char *title, const char *text,
                                      boolean defval)
{
    boolean retval = false;
    if (!feof(stdin))
    {
        const char *_fmt = ((defval) ? _("%0 [Y/n]: ") : _("%0 [y/N]: "));
        char *fmt = xstrdup(_fmt);
        char *msg = format(fmt, text);
        char *localized_no = xstrdup(_("N"));
        char *localized_yes = xstrdup(_("Y"));
        boolean getout = false;
        char buf[128];

        while (!getout)
        {
            int rc = 0;

            getout = true;  // we may reset this later.
            printf("%s", msg);
            fflush(stdout);
            rc = read_stdin(buf, sizeof (buf));

            if (rc < 0)
                retval = false;
            else if (rc == 0)
                retval = defval;
            else if (strcasecmp(buf, localized_no) == 0)
                retval = false;
            else if (strcasecmp(buf, localized_yes) == 0)
                retval = true;
            else
                getout = false;  // try again.
        } // while

        free(localized_yes);
        free(localized_no);
        free(msg);
        free(fmt);
    } // if

    return retval;
} // MojoGui_stdio_promptyn


static MojoGuiYNAN MojoGui_stdio_promptynan(const char *title, const char *txt,
                                            boolean defval)
{
    MojoGuiYNAN retval = MOJOGUI_NO;
    if (!feof(stdin))
    {
        char *fmt = xstrdup(_("%0\n[y/n/Always/Never]: "));
        char *msg = format(fmt, txt);
        char *localized_no = xstrdup(_("N"));
        char *localized_yes = xstrdup(_("Y"));
        char *localized_always = xstrdup(_("Always"));
        char *localized_never = xstrdup(_("Never"));
        boolean getout = false;
        char buf[128];

        while (!getout)
        {
            int rc = 0;

            getout = true;  // we may reset this later.
            printf("%s\n", msg);
            fflush(stdout);
            rc = read_stdin(buf, sizeof (buf));

            if (rc < 0)
                retval = MOJOGUI_NO;
            else if (rc == 0)
                retval = (defval) ? MOJOGUI_YES : MOJOGUI_NO;
            else if (strcasecmp(buf, localized_no) == 0)
                retval = MOJOGUI_NO;
            else if (strcasecmp(buf, localized_yes) == 0)
                retval = MOJOGUI_YES;
            else if (strcasecmp(buf, localized_always) == 0)
                retval = MOJOGUI_ALWAYS;
            else if (strcasecmp(buf, localized_never) == 0)
                retval = MOJOGUI_NEVER;
            else
                getout = false;  // try again.
        } // while

        free(localized_never);
        free(localized_always);
        free(localized_yes);
        free(localized_no);
        free(msg);
        free(fmt);
    } // if

    return retval;
} // MojoGui_stdio_promptynan


static boolean MojoGui_stdio_start(const char *title,
                                   const MojoGuiSplash *splash)
{
    printf("%s\n", title);
    return true;
} // MojoGui_stdio_start


static void MojoGui_stdio_stop(void)
{
    // no-op.
} // MojoGui_stdio_stop


static void dumb_pager(const char *name, const char *data, size_t datalen)
{
    const int MAX_PAGE_LINES = 21;
    char *fmt = xstrdup(_("(%0-%1 of %2 lines, see more?)"));
    int i = 0;
    int w = 0;
    int linecount = 0;
    boolean getout = false;
    char **lines = splitText(data, 80, &linecount, &w);

    assert(linecount >= 0);

    printf("%s\n", name);

    if (lines == NULL)  // failed to parse?!
        printf("%s\n", data);  // just dump it all. Oh well.
    else
    {
        int printed = 0;
        do
        {
            for (i = 0; (i < MAX_PAGE_LINES) && (printed < linecount); i++)
                printf("%s", lines[printed++]);

            if (printed >= linecount)
                getout = true;
            else
            {
                char *msg = NULL;
                printf("\n");
                msg = format(fmt, numstr((printed-i)+1),
                             numstr(printed), numstr(linecount));
                getout = !MojoGui_stdio_promptyn("", msg, true);
                free(msg);
                printf("\n");
            } // else
        } while (!getout);

        for (i = 0; i < linecount; i++)
            free(lines[i]);
        free(lines);
    } // while

    free(fmt);
} // dumb_pager


static int MojoGui_stdio_readme(const char *name, const uint8 *_data,
                                size_t datalen, boolean can_back,
                                boolean can_fwd)
{
    const char *data = (const char *) _data;
    char buf[256];
    int retval = -1;
    boolean failed = true;

    // !!! FIXME: popen() isn't reliable.
    #if 0  //PLATFORM_UNIX
    const size_t namelen = strlen(name);
    const char *programs[] = { getenv("PAGER"), "more", "less -M", "less" };
    int i = 0;

    // flush streams, so output doesn't mingle with the popen()'d process.
    fflush(stdout);
    fflush(stderr);

    for (i = 0; i < STATICARRAYLEN(programs); i++)
    {
        const char *cmd = programs[i];
        if (cmd != NULL)
        {
            FILE *io = popen(cmd, "w");
            if (io != NULL)
            {
                failed = false;
                if (!failed) failed = (fwrite("\n", 1, 1, io) != 1);
                if (!failed) failed = (fwrite(name, namelen, 1, io) != 1);
                if (!failed) failed = (fwrite("\n", 1, 1, io) != 1);
                if (!failed) failed = (fwrite(data, datalen, 1, io) != 1);
                if (!failed) failed = (fwrite("\n", 1, 1, io) != 1);
                failed |= (pclose(io) != 0);  // call whether we failed or not.
                if (!failed)
                    break;  // it worked, we're done!
            } // if
        } // if
    } // for
    #endif // PLATFORM_UNIX

    if (failed)  // We're not Unix, or none of the pagers worked?
        dumb_pager(name, data, datalen);

    // Put up the "hit enter to continue (or 'back' to go back)" prompt,
    //  but only if there's an choice to be made here.
    if ((!can_back) || (readstr(NULL, buf, sizeof (buf), can_back, true) >= 0))
        retval = 1;

    return retval;
} // MojoGui_stdio_readme


static void toggle_option(MojoGuiSetupOptions *parent,
                          MojoGuiSetupOptions *opts, int *line, int target)
{
    if ((opts != NULL) && (target > *line))
    {
        if (!opts->is_group_parent)
        {
            if (++(*line) == target)
            {
                const boolean toggled = ((opts->value) ? false : true);

                // "radio buttons" in a group?
                if ((parent) && (parent->is_group_parent))
                {
                    if (toggled)  // drop unless we weren't the current toggle.
                    {
                        // set all siblings to false...
                        MojoGuiSetupOptions *i = parent->child;
                        while (i != NULL)
                        {
                            i->value = false;
                            i = i->next_sibling;
                        } // while
                        opts->value = true;  // reset us to be true.
                    } // if
                } // if

                else  // individual "check box" was chosen.
                {
                    opts->value = toggled;
                } // else

                return;  // we found it, bail.
            } // if
        } // if

        if (opts->value) // if option is toggled on, descend to children.
            toggle_option(opts, opts->child, line, target);
        toggle_option(parent, opts->next_sibling, line, target);
    } // if
} // toggle_option


static void print_options(MojoGuiSetupOptions *opts, int *line, int level)
{
    if (opts != NULL)
    {
        int i;
        int spacing = 1;
        if (opts->is_group_parent)
            spacing += 6;
        else
        {
            (*line)++;
            printf("%2d  [%c]", *line, opts->value ? 'X' : ' ');
        } // else

        for (i = 0; i < (level + spacing); i++)
            putchar(' ');

        printf("%s%s\n", opts->description, opts->is_group_parent ? ":" : "");

        if ((opts->value) || (opts->is_group_parent))
            print_options(opts->child, line, level+1);
        print_options(opts->next_sibling, line, level);
    } // if
} // print_options


static int MojoGui_stdio_options(MojoGuiSetupOptions *opts,
                                 boolean can_back, boolean can_fwd)
{
    const char *inst_opts_str = xstrdup(_("Options"));
    const char *prompt = xstrdup(_("Choose number to change."));
    int retval = -1;
    boolean getout = false;
    char buf[128];
    int len = 0;

    while (!getout)
    {
        int line = 0;

        printf("\n\n");
        printf("%s", inst_opts_str);
        printf("\n");
        print_options(opts, &line, 1);
        printf("\n");

        if ((len = readstr(prompt, buf, sizeof (buf), can_back, true)) < 0)
            getout = true;
        else if (len == 0)
        {
            getout = true;
            retval = 1;
        } // else if
        else
        {
            char *endptr = NULL;
            int target = (int) strtol(buf, &endptr, 10);
            if (*endptr == '\0')  // complete string was a valid number?
            {
                line = 0;
                toggle_option(NULL, opts, &line, target);
            } // if
        } // else
    } // while

    free((void *) inst_opts_str);
    free((void *) prompt);

    return retval;
} // MojoGui_stdio_options


static char *MojoGui_stdio_destination(const char **recommends, int recnum,
                                       int *command, boolean can_back,
                                       boolean can_fwd)
{
    const char *instdeststr = xstrdup(_("Destination"));
    const char *prompt = NULL;
    char *retval = NULL;
    boolean getout = false;
    char buf[128];
    int len = 0;
    int i = 0;

    *command = -1;

    if (recnum > 0)
        prompt = xstrdup(_("Choose install destination by number (hit enter for #1), or enter your own."));
    else
        prompt = xstrdup(_("Enter path where files will be installed."));

    while (!getout)
    {
        printf("\n\n%s\n", instdeststr);
        for (i = 0; i < recnum; i++)
            printf("  %2d  %s\n", i+1, recommends[i]);
        printf("\n");

        if ((len = readstr(prompt, buf, sizeof (buf), can_back, false)) < 0)
            getout = true;

        else if ((len == 0) && (recnum > 0))   // default to first in list.
        {
            retval = xstrdup(recommends[0]);
            *command = 1;
            getout = true;
        } // else if

        else if (len > 0)
        {
            char *endptr = NULL;
            int target = (int) strtol(buf, &endptr, 10);
            // complete string was a valid number?
            if ((*endptr == '\0') && (target > 0) && (target <= recnum))
                retval = xstrdup(recommends[target-1]);
            else
                retval = xstrdup(buf);

            *command = 1;
            getout = true;
        } // else
    } // while

    free((void *) prompt);
    free((void *) instdeststr);

    return retval;
} // MojoGui_stdio_destination


static int MojoGui_stdio_productkey(const char *desc, const char *fmt,
                                    char *buf, const int buflen,
                                    boolean can_back, boolean can_fwd)
{
    const char *prompt = xstrdup(_("Please enter your product key"));
    char *defval = ((*buf) ? xstrdup(buf) : NULL);
    boolean getout = false;
    int retval = -1;
    char *msg = NULL;

    if (defval != NULL)
    {
        char *locfmt = xstrdup(_("(just press enter to use '%0')"));
        msg = format(locfmt, defval);
        free(locfmt);
    } // if

    while (!getout)
    {
        int len;
        printf("\n\n%s\n", desc);
        if (msg != NULL)
            printf("%s\n", msg);
        if ((len = readstr(prompt, buf, buflen, can_back, false)) < 0)
            getout = true;
        else
        {
            if ((len == 0) && (defval != NULL))
                strcpy(buf, defval);

            if (isValidProductKey(fmt, buf))
            {
                retval = 1;
                getout = true;
            } // else if
            else
            {
                // We can't check the input character-by-character, so reuse
                //  the failed-verification localized string.
                printf("\n%s\n\n",
                        _("That key appears to be invalid. Please try again."));
            } // else
        } // else
    } // while

    free(msg);
    free(defval);
    free((void *) prompt);

    return retval;
} // MojoGui_stdio_productkey


static boolean MojoGui_stdio_insertmedia(const char *medianame)
{
    char buf[32];
    char *fmt = xstrdup(_("Please insert '%0'"));
    char *msg = format(fmt, medianame);
    printf("%s\n", _("Media change"));
    printf("%s\n", msg);
    free(msg);
    free(fmt);
    return (readstr(NULL, buf, sizeof (buf), false, true) >= 0);
} // MojoGui_stdio_insertmedia


static void MojoGui_stdio_progressitem(void)
{
    // force new line of output on next call to MojoGui_stdio_progress()
    percentTicks = 0;
} // MojoGui_stdio_progressitem


static boolean MojoGui_stdio_progress(const char *type, const char *component,
                                      int percent, const char *item,
                                      boolean can_cancel)
{
    const uint32 now = ticks();

    if ( (lastComponent == NULL) ||
         (strcmp(lastComponent, component) != 0) ||
         (lastProgressType == NULL) ||
         (strcmp(lastProgressType, type) != 0) )
    {
        free(lastProgressType);
        free(lastComponent);
        lastProgressType = xstrdup(type);
        lastComponent = xstrdup(component);
        printf("%s\n%s\n", type, component);
    } // if

    // limit update spam... will only write every one second, tops,
    //  on any given filename, but it writes each filename at least once
    //  so it doesn't look like we only installed a few things.
    if (percentTicks <= now)
    {
        char *fmt = NULL;
        char *msg = NULL;
        percentTicks = now + 1000;
        if (percent < 0)
            printf("%s\n", item);
        else
        {
            fmt = xstrdup(_("%0 (total progress: %1%%)"));
            msg = format(fmt, item, numstr(percent));
            printf("%s\n", msg);
            free(msg);
            free(fmt);
        } // else
    } // if

    return true;
} // MojoGui_stdio_progress


static void MojoGui_stdio_final(const char *msg)
{
    printf("%s\n\n", msg);
    fflush(stdout);
} // MojoGui_stdio_final

// end of gui_stdio.c ...

