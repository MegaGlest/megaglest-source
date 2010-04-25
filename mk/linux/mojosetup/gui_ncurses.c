/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if !SUPPORT_GUI_NCURSES
#error Something is wrong in the build system.
#endif

#define BUILDING_EXTERNAL_PLUGIN 1
#include "gui.h"

MOJOGUI_PLUGIN(ncurses)

#if !GUI_STATIC_LINK_NCURSES
CREATE_MOJOGUI_ENTRY_POINT(ncurses)
#endif

#include <unistd.h>
#include <ctype.h>
// CMake searches for a whole bunch of different possible curses includes
#if defined(HAVE_NCURSESW_NCURSES_H)
#include <ncursesw/ncurses.h>
#elif defined(HAVE_NCURSESW_CURSES_H)
#include <ncursesw/curses.h>
#elif defined(HAVE_NCURSESW_H)
#include <ncursesw.h>
#else
#error ncurses gui enabled, but no known header file found
#endif

#include <locale.h>

// This was built to look roughly like dialog(1), but it's not nearly as
//  robust. Also, I didn't use any of dialog's code, as it is GPL/LGPL,
//  depending on what version you start with. There _is_ a libdialog, but
//  it's never something installed on any systems, and I can't link it
//  statically due to the license.
//
// ncurses is almost always installed as a shared library, though, so we'll
//  just talk to it directly. Fortunately we don't need much of what dialog(1)
//  offers, so rolling our own isn't too painful (well, compared to massive
//  head trauma, I guess).
//
// Pradeep Padala's ncurses HOWTO was very helpful in teaching me curses
//  quickly: http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/index.html

// !!! FIXME: this should all be UTF-8 and Unicode clean with ncursesw, but
// !!! FIXME:  it relies on the terminal accepting UTF-8 output (we don't
// !!! FIXME:  attempt to convert) and assumes all characters fit in one
// !!! FIXME:  column, which they don't necessarily for some Asian languages,
// !!! FIXME:  etc. I'm not sure how to properly figure out column width, if
// !!! FIXME:  it's possible at all, but for that, you should probably
// !!! FIXME:  go to a proper GUI plugin like GTK+ anyhow.

typedef enum
{
    MOJOCOLOR_BACKGROUND=1,
    MOJOCOLOR_BORDERTOP,
    MOJOCOLOR_BORDERBOTTOM,
    MOJOCOLOR_BORDERTITLE,
    MOJOCOLOR_TEXT,
    MOJOCOLOR_TEXTENTRY,
    MOJOCOLOR_BUTTONHOVER,
    MOJOCOLOR_BUTTONNORMAL,
    MOJOCOLOR_BUTTONBORDER,
    MOJOCOLOR_TODO,
    MOJOCOLOR_DONE,
} MojoColor;


typedef struct
{
    WINDOW *mainwin;
    WINDOW *textwin;
    WINDOW **buttons;
    char *title;
    char *text;
    char **textlines;
    char **buttontext;
    int buttoncount;
    int textlinecount;
    int hoverover;
    int textpos;
    boolean hidecursor;
    boolean ndelay;
    int cursval;
} MojoBox;


static char *lastProgressType = NULL;
static char *lastComponent = NULL;
static boolean lastCanCancel = false;
static uint32 percentTicks = 0;
static char *title = NULL;
static MojoBox *progressBox = NULL;


static void drawButton(MojoBox *mojobox, int button)
{
    const boolean hover = (mojobox->hoverover == button);
    int borderattr = 0;
    WINDOW *win = mojobox->buttons[button];
    const char *str = mojobox->buttontext[button];
    int w, h;
    getmaxyx(win, h, w);

    if (!hover)
        wbkgdset(win, COLOR_PAIR(MOJOCOLOR_BUTTONNORMAL));
    else
    {
        borderattr = COLOR_PAIR(MOJOCOLOR_BUTTONBORDER) | A_BOLD;
        wbkgdset(win, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER));
    } // else

    werase(win);
    wmove(win, 0, 0);
    waddch(win, borderattr | '<');
    wmove(win, 0, w-1);
    waddch(win, borderattr | '>');
    wmove(win, 0, 2);

    if (!hover)
        waddstr(win, str);
    else
    {
        wattron(win, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
        waddstr(win, str);
        wattroff(win, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
    } // else
} // drawButton


static void drawText(MojoBox *mojobox)
{
    int i;
    const int tcount = mojobox->textlinecount;
    int pos = mojobox->textpos;
    int w, h;
    WINDOW *win = mojobox->textwin;
    getmaxyx(win, h, w);

    werase(mojobox->textwin);
    for (i = 0; (pos < tcount) && (i < h); i++, pos++)
        mvwaddstr(win, i, 0, mojobox->textlines[pos]);

    if (tcount > h)
    {
        const int pct = (int) ((((double) pos) / ((double) tcount)) * 100.0);
        win = mojobox->mainwin;
        wattron(win, COLOR_PAIR(MOJOCOLOR_BORDERTITLE) | A_BOLD);
        mvwprintw(win, h+1, w-5, "(%3d%%)", pct);
        wattroff(win, COLOR_PAIR(MOJOCOLOR_BORDERTITLE) | A_BOLD);
    } // if
} // drawText


static void drawBackground(WINDOW *win)
{
    wclear(win);
    if (title != NULL)
    {
        int w, h;
        getmaxyx(win, h, w);
        wattron(win, COLOR_PAIR(MOJOCOLOR_BACKGROUND) | A_BOLD);
        mvwaddstr(win, 0, 0, title);
        mvwhline(win, 1, 1, ACS_HLINE, w-2);
        wattroff(win, COLOR_PAIR(MOJOCOLOR_BACKGROUND) | A_BOLD);
    } // if
} // drawBackground


static void confirmTerminalSize(void)
{
    int scrw = 0;
    int scrh = 0;
    char *msg = NULL;
    int len = 0;
    int x = 0;
    int y = 0;

    while (1)   // loop until the window meets a minimum dimension requirement.
    {
        getmaxyx(stdscr, scrh, scrw);
        scrh--; // -1 to save the title at the top of the screen...

        if (scrw < 30)  // too thin
            msg = xstrdup(_("[Make the window wider!]"));
        else if (scrh < 10)  // too short
            msg = xstrdup(_("[Make the window taller!]"));
        else
            break;  // we're good, get out.

        len = utf8len(msg);
        y = scrh / 2;
        x = ((scrw - len) / 2);

        if (y < 0) y = 0;
        if (x < 0) x = 0;

        wclear(stdscr);
        wmove(stdscr, y, x);
        wrefresh(stdscr);
        wmove(stdscr, y, x);
        wattron(stdscr, COLOR_PAIR(MOJOCOLOR_BACKGROUND) | A_BOLD);
        waddstr(stdscr, msg);
        wattroff(stdscr, COLOR_PAIR(MOJOCOLOR_BACKGROUND) | A_BOLD);
        nodelay(stdscr, 0);
        wrefresh(stdscr);
        free(msg);

        while (wgetch(stdscr) != KEY_RESIZE) { /* no-op. */ }
    } // while
} // confirmTerminalSize


static MojoBox *makeBox(const char *title, const char *text,
                        char **buttons, int bcount,
                        boolean ndelay, boolean hidecursor)
{
    MojoBox *retval = NULL;
    WINDOW *win = NULL;
    int scrw, scrh;
    int len = 0;
    int buttonsw = 0;
    int x = 0;
    int y = 0;
    int h = 0;
    int w = 0;
    int texth = 0;
    int i;

    confirmTerminalSize();  // blocks until window is large enough to continue.

    getmaxyx(stdscr, scrh, scrw);
    scrh--; // -1 to save the title at the top of the screen...

    retval = (MojoBox *) xmalloc(sizeof (MojoBox));
    retval->hidecursor = hidecursor;
    retval->ndelay = ndelay;
    retval->cursval = ((hidecursor) ? curs_set(0) : ERR);
    retval->title = xstrdup(title);
    retval->text = xstrdup(text);
    retval->buttoncount = bcount;
    retval->buttons = (WINDOW **) xmalloc(sizeof (WINDOW*) * bcount);
    retval->buttontext = (char **) xmalloc(sizeof (char*) * bcount);

    for (i = 0; i < bcount; i++)
        retval->buttontext[i] = xstrdup(buttons[i]);

    retval->textlines = splitText(retval->text, scrw-4,
                                  &retval->textlinecount, &w);

    len = utf8len(title);
    if (len > scrw-4)
    {
        len = scrw-4;
        strcpy(&retval->title[len-3], "...");  // !!! FIXME: not Unicode safe!
    } // if

    if (len > w)
        w = len;

    if (bcount > 0)
    {
        for (i = 0; i < bcount; i++)
            buttonsw += utf8len(buttons[i]) + 5;  // '<', ' ', ' ', '>', ' '
        if (buttonsw > w)
            w = buttonsw;
        // !!! FIXME: what if these overflow the screen?
    } // if

    w += 4;
    h = retval->textlinecount + 2;
    if (bcount > 0)
        h += 2;

    if (h > scrh-2)
        h = scrh-2;

    x = (scrw - w) / 2;
    y = ((scrh - h) / 2) + 1;

    // can't have negative coordinates, so in case we survived the call to
    //  confirmTerminalSize() but still need more, just draw as much as
    //  possible from the top/left to fill the window.
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    win = retval->mainwin = newwin(h, w, y, x);
	keypad(win, TRUE);
    nodelay(win, ndelay);
    wbkgdset(win, COLOR_PAIR(MOJOCOLOR_TEXT));
    wclear(win);
    waddch(win, ACS_ULCORNER | A_BOLD | COLOR_PAIR(MOJOCOLOR_BORDERTOP));
    whline(win, ACS_HLINE | A_BOLD | COLOR_PAIR(MOJOCOLOR_BORDERTOP), w-2);
    wmove(win, 0, w-1);
    waddch(win, ACS_URCORNER | COLOR_PAIR(MOJOCOLOR_BORDERBOTTOM));
    wmove(win, 1, 0);
    wvline(win, ACS_VLINE | A_BOLD | COLOR_PAIR(MOJOCOLOR_BORDERTOP), h-2);
    wmove(win, 1, w-1);
    wvline(win, ACS_VLINE | COLOR_PAIR(MOJOCOLOR_BORDERBOTTOM), h-2);
    wmove(win, h-1, 0);
    waddch(win, ACS_LLCORNER | A_BOLD | COLOR_PAIR(MOJOCOLOR_BORDERTOP));
    whline(win, ACS_HLINE | COLOR_PAIR(MOJOCOLOR_BORDERBOTTOM), w-2);
    wmove(win, h-1, w-1);
    waddch(win, ACS_LRCORNER | COLOR_PAIR(MOJOCOLOR_BORDERBOTTOM));

    len = utf8len(retval->title);
    wmove(win, 0, ((w-len)/2)-1);
    wattron(win, COLOR_PAIR(MOJOCOLOR_BORDERTITLE) | A_BOLD);
    waddch(win, ' ');
    waddstr(win, retval->title);
    wmove(win, 0, ((w-len)/2)+len);
    waddch(win, ' ');
    wattroff(win, COLOR_PAIR(MOJOCOLOR_BORDERTITLE) | A_BOLD);

    if (bcount > 0)
    {
        const int buttony = (y + h) - 2;
        int buttonx = (x + w) - ((w - buttonsw) / 2);
        wmove(win, h-3, 1);
        whline(win, ACS_HLINE | A_BOLD | COLOR_PAIR(MOJOCOLOR_BORDERTOP), w-2);
        for (i = 0; i < bcount; i++)
        {
            len = utf8len(buttons[i]) + 4;
            buttonx -= len+1;
            win = retval->buttons[i] = newwin(1, len, buttony, buttonx);
            keypad(win, TRUE);
            nodelay(win, ndelay);
        } // for
    } // if

    texth = h-2;
    if (bcount > 0)
        texth -= 2;
    win = retval->textwin = newwin(texth, w-4, y+1, x+2);
	keypad(win, TRUE);
    nodelay(win, ndelay);
    wbkgdset(win, COLOR_PAIR(MOJOCOLOR_TEXT));
    drawText(retval);

    drawBackground(stdscr);
    wnoutrefresh(stdscr);
    wnoutrefresh(retval->mainwin);
    wnoutrefresh(retval->textwin);
    for (i = 0; i < bcount; i++)
    {
        drawButton(retval, i);
        wnoutrefresh(retval->buttons[i]);
    } // for

    doupdate();  // push it all to the screen.

    return retval;
} // makeBox


static void freeBox(MojoBox *mojobox, boolean clearscreen)
{
    if (mojobox != NULL)
    {
        int i;
        const int bcount = mojobox->buttoncount;
        const int tcount = mojobox->textlinecount;

        if (mojobox->cursval != ERR)
            curs_set(mojobox->cursval);

        for (i = 0; i < bcount; i++)
        {
            free(mojobox->buttontext[i]);
            delwin(mojobox->buttons[i]);
        } // for

        free(mojobox->buttontext);
        free(mojobox->buttons);

        delwin(mojobox->textwin);
        delwin(mojobox->mainwin);

        free(mojobox->title);
        free(mojobox->text);

        for (i = 0; i < tcount; i++)
            free(mojobox->textlines[i]);

        free(mojobox->textlines);
        free(mojobox);

        if (clearscreen)
        {
            wclear(stdscr);
            wrefresh(stdscr);
        } // if
    } // if
} // freeBox


static int upkeepBox(MojoBox **_mojobox, int ch)
{
    static boolean justResized = false;
    MEVENT mevt;
    int i;
    int w, h;
    MojoBox *mojobox = *_mojobox;
    if (mojobox == NULL)
        return -2;

    if (justResized)   // !!! FIXME: this is a kludge.
    {
        justResized = false;
        if (ch == ERR)
            return -1;
    } // if

    switch (ch)
    {
        case ERR:
            return -2;

        case '\r':
        case '\n':
        case KEY_ENTER:
        case ' ':
            return (mojobox->buttoncount <= 0) ? -1 : mojobox->hoverover;

        case '\e':
            return mojobox->buttoncount-1;

        case KEY_UP:
            if (mojobox->textpos > 0)
            {
                mojobox->textpos--;
                drawText(mojobox);
                wrefresh(mojobox->textwin);
            } // if
            return -1;

        case KEY_DOWN:
            getmaxyx(mojobox->textwin, h, w);
            if (mojobox->textpos < (mojobox->textlinecount-h))
            {
                mojobox->textpos++;
                drawText(mojobox);
                wrefresh(mojobox->textwin);
            } // if
            return -1;

        case KEY_PPAGE:
            if (mojobox->textpos > 0)
            {
                getmaxyx(mojobox->textwin, h, w);
                mojobox->textpos -= h;
                if (mojobox->textpos < 0)
                    mojobox->textpos = 0;
                drawText(mojobox);
                wrefresh(mojobox->textwin);
            } // if
            return -1;

        case KEY_NPAGE:
            getmaxyx(mojobox->textwin, h, w);
            if (mojobox->textpos < (mojobox->textlinecount-h))
            {
                mojobox->textpos += h;
                if (mojobox->textpos > (mojobox->textlinecount-h))
                    mojobox->textpos = (mojobox->textlinecount-h);
                drawText(mojobox);
                wrefresh(mojobox->textwin);
            } // if
            return -1;

        case KEY_LEFT:
            if (mojobox->buttoncount > 1)
            {
                if (mojobox->hoverover < (mojobox->buttoncount-1))
                {
                    mojobox->hoverover++;
                    drawButton(mojobox, mojobox->hoverover-1);
                    drawButton(mojobox, mojobox->hoverover);
                    wrefresh(mojobox->buttons[mojobox->hoverover-1]);
                    wrefresh(mojobox->buttons[mojobox->hoverover]);
                } // if
            } // if
            return -1;

        case KEY_RIGHT:
            if (mojobox->buttoncount > 1)
            {
                if (mojobox->hoverover > 0)
                {
                    mojobox->hoverover--;
                    drawButton(mojobox, mojobox->hoverover+1);
                    drawButton(mojobox, mojobox->hoverover);
                    wrefresh(mojobox->buttons[mojobox->hoverover+1]);
                    wrefresh(mojobox->buttons[mojobox->hoverover]);
                } // if
            } // if
            return -1;

        case 12:  // ctrl-L...redraw everything on the screen.
            redrawwin(stdscr);
            wnoutrefresh(stdscr);
            redrawwin(mojobox->mainwin);
            wnoutrefresh(mojobox->mainwin);
            redrawwin(mojobox->textwin);
            wnoutrefresh(mojobox->textwin);
            for (i = 0; i < mojobox->buttoncount; i++)
            {
                redrawwin(mojobox->buttons[i]);
                wnoutrefresh(mojobox->buttons[i]);
            } // for
            doupdate();  // push it all to the screen.
            return -1;

        case KEY_RESIZE:
            mojobox = makeBox(mojobox->title, mojobox->text,
                              mojobox->buttontext, mojobox->buttoncount,
                              mojobox->ndelay, mojobox->hidecursor);
            mojobox->cursval = (*_mojobox)->cursval;  // keep this sane.
            mojobox->hoverover = (*_mojobox)->hoverover;
            freeBox(*_mojobox, false);
            if (mojobox->hidecursor)
                curs_set(0); // make sure this stays sane.
            *_mojobox = mojobox;
            justResized = true;  // !!! FIXME: kludge.
            return -1;

        case KEY_MOUSE:
            if ((getmouse(&mevt) == OK) && (mevt.bstate & BUTTON1_CLICKED))
            {
                int i;
                for (i = 0; i < mojobox->buttoncount; i++)
                {
                    int x1, y1, x2, y2;
                    getbegyx(mojobox->buttons[i], y1, x1);
                    getmaxyx(mojobox->buttons[i], y2, x2);
                    x2 += x1;
                    y2 += y1;
                    if ( (mevt.x >= x1) && (mevt.x < x2) &&
                         (mevt.y >= y1) && (mevt.y < y2) )
                        return i;
                } // for
            } // if
            return -1;
    } // switch

    return -1;
} // upkeepBox


static uint8 MojoGui_ncurses_priority(boolean istty)
{
    if (!istty)
        return MOJOGUI_PRIORITY_NEVER_TRY;  // need a terminal for this!
    else if (getenv("DISPLAY") != NULL)
        return MOJOGUI_PRIORITY_TRY_LAST;  // let graphical stuff go first.
    return MOJOGUI_PRIORITY_TRY_NORMAL;
} // MojoGui_ncurses_priority


static boolean MojoGui_ncurses_init(void)
{
    setlocale(LC_CTYPE, ""); // !!! FIXME: we assume you have a UTF-8 terminal.
    if (initscr() == NULL)
    {
        logInfo("ncurses: initscr() failed, use another UI.");
        return false;
    } // if

	cbreak();
	keypad(stdscr, TRUE);
	noecho();
    start_color();
    mousemask(BUTTON1_CLICKED, NULL);
    init_pair(MOJOCOLOR_BACKGROUND, COLOR_CYAN, COLOR_BLUE);
    init_pair(MOJOCOLOR_BORDERTOP, COLOR_WHITE, COLOR_WHITE);
    init_pair(MOJOCOLOR_BORDERBOTTOM, COLOR_BLACK, COLOR_WHITE);
    init_pair(MOJOCOLOR_BORDERTITLE, COLOR_YELLOW, COLOR_WHITE);
    init_pair(MOJOCOLOR_TEXT, COLOR_BLACK, COLOR_WHITE);
    init_pair(MOJOCOLOR_TEXTENTRY, COLOR_WHITE, COLOR_BLUE);
    init_pair(MOJOCOLOR_BUTTONHOVER, COLOR_YELLOW, COLOR_BLUE);
    init_pair(MOJOCOLOR_BUTTONNORMAL, COLOR_BLACK, COLOR_WHITE);
    init_pair(MOJOCOLOR_BUTTONBORDER, COLOR_WHITE, COLOR_BLUE);
    init_pair(MOJOCOLOR_DONE, COLOR_YELLOW, COLOR_RED);
    init_pair(MOJOCOLOR_TODO, COLOR_CYAN, COLOR_BLUE);

    wbkgdset(stdscr, COLOR_PAIR(MOJOCOLOR_BACKGROUND));
    wclear(stdscr);
    wrefresh(stdscr);

    percentTicks = 0;
    return true;   // always succeeds.
} // MojoGui_ncurses_init


static void MojoGui_ncurses_deinit(void)
{
    freeBox(progressBox, false);
    progressBox = NULL;
    endwin();
    delwin(stdscr);  // not sure if this is safe, but valgrind said it leaks.
    stdscr = NULL;
    free(title);
    title = NULL;
    free(lastComponent);
    lastComponent = NULL;
    free(lastProgressType);
    lastProgressType = NULL;
} // MojoGui_ncurses_deinit


static void MojoGui_ncurses_msgbox(const char *title, const char *text)
{
    char *localized_ok = xstrdup(_("OK"));
    MojoBox *mojobox = makeBox(title, text, &localized_ok, 1, false, true);
    while (upkeepBox(&mojobox, wgetch(mojobox->mainwin)) == -1) {}
    freeBox(mojobox, true);
    free(localized_ok);
} // MojoGui_ncurses_msgbox


static boolean MojoGui_ncurses_promptyn(const char *title, const char *text,
                                        boolean defval)
{
    char *localized_yes = xstrdup(_("Yes"));
    char *localized_no = xstrdup(_("No"));
    char *buttons[] = { localized_yes, localized_no };
    MojoBox *mojobox = makeBox(title, text, buttons, 2, false, true);
    int rc = 0;

    // set the default to "no" instead of "yes"?
    if (defval == false)
    {
        mojobox->hoverover = 1;
        drawButton(mojobox, 0);
        drawButton(mojobox, 1);
        wrefresh(mojobox->buttons[0]);
        wrefresh(mojobox->buttons[1]);
    } // if

    while ((rc = upkeepBox(&mojobox, wgetch(mojobox->mainwin))) == -1) {}
    freeBox(mojobox, true);
    free(localized_yes);
    free(localized_no);
    return (rc == 0);
} // MojoGui_ncurses_promptyn


static MojoGuiYNAN MojoGui_ncurses_promptynan(const char *title,
                                              const char *text,
                                              boolean defval)
{
    char *loc_yes = xstrdup(_("Yes"));
    char *loc_no = xstrdup(_("No"));
    char *loc_always = xstrdup(_("Always"));
    char *loc_never = xstrdup(_("Never"));
    char *buttons[] = { loc_yes, loc_always, loc_never, loc_no };
    MojoBox *mojobox = makeBox(title, text, buttons, 4, false, true);
    int rc = 0;

    // set the default to "no" instead of "yes"?
    if (defval == false)
    {
        mojobox->hoverover = 3;
        drawButton(mojobox, 0);
        drawButton(mojobox, 3);
        wrefresh(mojobox->buttons[0]);
        wrefresh(mojobox->buttons[3]);
    } // if

    while ((rc = upkeepBox(&mojobox, wgetch(mojobox->mainwin))) == -1) {}
    freeBox(mojobox, true);
    free(loc_yes);
    free(loc_no);
    free(loc_always);
    free(loc_never);

    switch (rc)
    {
        case 0: return MOJOGUI_YES;
        case 1: return MOJOGUI_ALWAYS;
        case 2: return MOJOGUI_NEVER;
        case 3: return MOJOGUI_NO;
    } // switch

    assert(false && "BUG: unhandled case in switch statement!");
    return MOJOGUI_NO;
} // MojoGui_ncurses_promptynan


static boolean MojoGui_ncurses_start(const char *_title,
                                     const MojoGuiSplash *splash)
{
    free(title);
    title = xstrdup(_title);
    drawBackground(stdscr);
    wrefresh(stdscr);
    return true;
} // MojoGui_ncurses_start


static void MojoGui_ncurses_stop(void)
{
    free(title);
    title = NULL;
    drawBackground(stdscr);
    wrefresh(stdscr);
} // MojoGui_ncurses_stop


static int MojoGui_ncurses_readme(const char *name, const uint8 *data,
                                    size_t datalen, boolean can_back,
                                    boolean can_fwd)
{
    MojoBox *mojobox = NULL;
    char *buttons[3] = { NULL, NULL, NULL };
    int bcount = 0;
    int backbutton = -99;
    int fwdbutton = -99;
    int rc = 0;
    int i = 0;

    if (can_fwd)
    {
        fwdbutton = bcount++;
        buttons[fwdbutton] = xstrdup(_("Next"));
    } // if

    if (can_back)
    {
        backbutton = bcount++;
        buttons[backbutton] = xstrdup(_("Back"));
    } // if

    buttons[bcount++] = xstrdup(_("Cancel"));

    mojobox = makeBox(name, (char *) data, buttons, bcount, false, true);
    while ((rc = upkeepBox(&mojobox, wgetch(mojobox->mainwin))) == -1) {}
    freeBox(mojobox, true);

    for (i = 0; i < bcount; i++)
        free(buttons[i]);

    if (rc == backbutton)
        return -1;
    else if (rc == fwdbutton)
        return 1;

    return 0;  // error? Cancel?
} // MojoGui_ncurses_readme


static int toggle_option(MojoGuiSetupOptions *parent,
                         MojoGuiSetupOptions *opts, int *line, int target)
{
    int rc = -1;
    if ((opts != NULL) && (target > *line))
    {
        const char *desc = opts->description;
        boolean blanked = false;
        blanked = ( (opts->is_group_parent) && ((!desc) || (!(*desc))) );

        if ((!blanked) && (++(*line) == target))
        {
            const boolean toggled = ((opts->value) ? false : true);

            if (opts->is_group_parent)
                return 0;

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

            return 1;  // we found it, bail.
        } // if

        if (opts->value) // if option is toggled on, descend to children.
            rc = toggle_option(opts, opts->child, line, target);
        if (rc == -1)
            rc = toggle_option(parent, opts->next_sibling, line, target);
    } // if

    return rc;
} // toggle_option


// This code is pretty scary.
static void build_options(MojoGuiSetupOptions *opts, int *line, int level,
                          int maxw, char **lines)
{
    if (opts != NULL)
    {
        const char *desc = opts->description;
        char *spacebuf = (char *) xmalloc(maxw + 1);
        char *buf = (char *) xmalloc(maxw + 1);
        int len = 0;
        int spacing = level * 2;

        if ((desc != NULL) && (*desc == '\0'))
            desc = NULL;

        if (spacing > (maxw-5))
            spacing = 0;  // oh well.

        if (spacing > 0)
            memset(spacebuf, ' ', spacing);  // null-term'd by xmalloc().

        if (opts->is_group_parent)
        {
            if (desc != NULL)
                len = snprintf(buf, maxw-2, "%s%s", spacebuf, desc);
        } // if
        else
        {
            (*line)++;
            len = snprintf(buf, maxw-2, "%s[%c] %s", spacebuf,
                            opts->value ? 'X' : ' ', desc);
        } // else

        free(spacebuf);

        if (len >= maxw-1)
            strcpy(buf+(maxw-4), "...");  // !!! FIXME: Unicode issues!

        if (len > 0)
        {
            const size_t newlen = strlen(*lines) + strlen(buf) + 2;
            *lines = (char*) xrealloc(*lines, newlen);
            strcat(*lines, buf);
            strcat(*lines, "\n");  // I'm sorry, Joel Spolsky!
        } // if

        if ((opts->value) || (opts->is_group_parent))
        {
            int newlev = level + 1;
            if ((opts->is_group_parent) && (desc == NULL))
                newlev--;
            build_options(opts->child, line, newlev, maxw, lines);
        } // if

        build_options(opts->next_sibling, line, level, maxw, lines);
    } // if
} // build_options


static int optionBox(const char *title, MojoGuiSetupOptions *opts,
                     boolean can_back, boolean can_fwd)
{
    MojoBox *mojobox = NULL;
    char *buttons[4] = { NULL, NULL, NULL, NULL };
    boolean ignoreerr = false;
    int lasthoverover = 0;
    int lasttextpos = 0;
    int bcount = 0;
    int backbutton = -99;
    int fwdbutton = -99;
    int togglebutton = -99;
    int cancelbutton = -99;
    int selected = 0;
    int ch = 0;
    int rc = -1;
    int i = 0;

    if (can_fwd)
    {
        fwdbutton = bcount++;
        buttons[fwdbutton] = xstrdup(_("Next"));
    } // if

    if (can_back)
    {
        backbutton = bcount++;
        buttons[backbutton] = xstrdup(_("Back"));
    } // if

    lasthoverover = togglebutton = bcount++;
    buttons[togglebutton] = xstrdup(_("Toggle"));
    cancelbutton = bcount++;
    buttons[cancelbutton] = xstrdup(_("Cancel"));

    do
    {
        if (mojobox == NULL)
        {
            int y = 0;
            int line = 0;
            int maxw, maxh;
            getmaxyx(stdscr, maxh, maxw);
            char *text = xstrdup("");
            build_options(opts, &line, 0, maxw-6, &text);
            mojobox = makeBox(title, text, buttons, bcount, false, true);
            free(text);

            getmaxyx(mojobox->textwin, maxh, maxw);

            if (lasthoverover != mojobox->hoverover)
            {
                const int orighover = mojobox->hoverover;
                mojobox->hoverover = lasthoverover;
                drawButton(mojobox, orighover);
                drawButton(mojobox, lasthoverover);
                wrefresh(mojobox->buttons[orighover]);
                wrefresh(mojobox->buttons[lasthoverover]);
            } // if

            if (lasttextpos != mojobox->textpos)
            {
                mojobox->textpos = lasttextpos;
                drawText(mojobox);
            } // if

            if (selected >= (mojobox->textlinecount - 1))
                selected = mojobox->textlinecount - 1;
            if (selected >= mojobox->textpos+maxh)
                selected = (mojobox->textpos+maxh) - 1;
            y = selected - lasttextpos;

            wattron(mojobox->textwin, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
            mvwhline(mojobox->textwin, y, 0, ' ', maxw);
            mvwaddstr(mojobox->textwin, y, 0, mojobox->textlines[selected]);
            wattroff(mojobox->textwin, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
            wrefresh(mojobox->textwin);
        } // if

        lasttextpos = mojobox->textpos;
        lasthoverover = mojobox->hoverover;

        ch = wgetch(mojobox->mainwin);

        if (ignoreerr)  // kludge.
        {
            ignoreerr = false;
            if (ch == ERR)
                continue;
        } // if

        if (ch == KEY_RESIZE)
        {
            freeBox(mojobox, false);  // catch and rebuild without upkeepBox,
            mojobox = NULL;           //  so we can rebuild the text ourself.
            ignoreerr = true;  // kludge.
        } // if

        else if (ch == KEY_UP)
        {
            if (selected > 0)
            {
                WINDOW *win = mojobox->textwin;
                int maxw, maxh;
                int y = --selected - mojobox->textpos;
                getmaxyx(win, maxh, maxw);
                if (selected < mojobox->textpos)
                {
                    upkeepBox(&mojobox, ch);  // upkeepBox does scrolling
                    y++;
                } // if
                else
                {
                    wattron(win, COLOR_PAIR(MOJOCOLOR_TEXT));
                    mvwhline(win, y+1, 0, ' ', maxw);
                    mvwaddstr(win, y+1, 0, mojobox->textlines[selected+1]);
                    wattroff(win, COLOR_PAIR(MOJOCOLOR_TEXT));
                } // else
                wattron(win, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
                mvwhline(win, y, 0, ' ', maxw);
                mvwaddstr(win, y, 0, mojobox->textlines[selected]);
                wattroff(win, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
                wrefresh(win);
            } // if
        } // else if

        else if (ch == KEY_DOWN)
        {
            if (selected < (mojobox->textlinecount-1))
            {
                WINDOW *win = mojobox->textwin;
                int maxw, maxh;
                int y = ++selected - mojobox->textpos;
                getmaxyx(win, maxh, maxw);
                if (selected >= mojobox->textpos+maxh)
                {
                    upkeepBox(&mojobox, ch);  // upkeepBox does scrolling
                    y--;
                } // if
                else
                {
                    wattron(win, COLOR_PAIR(MOJOCOLOR_TEXT));
                    mvwhline(win, y-1, 0, ' ', maxw);
                    mvwaddstr(win, y-1, 0, mojobox->textlines[selected-1]);
                    wattroff(win, COLOR_PAIR(MOJOCOLOR_TEXT));
                } // else
                wattron(win, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
                mvwhline(win, y, 0, ' ', maxw);
                mvwaddstr(win, y, 0, mojobox->textlines[selected]);
                wattroff(win, COLOR_PAIR(MOJOCOLOR_BUTTONHOVER) | A_BOLD);
                wrefresh(win);
            } // if
        } // else if

        else if ((ch == KEY_NPAGE) || (ch == KEY_NPAGE))
        {
            // !!! FIXME: maybe handle this when I'm not so lazy.
            // !!! FIXME:  For now, this if statement is to block
            // !!! FIXME:  upkeepBox() from scrolling and screwing up state.
        } // else if

        else  // let upkeepBox handle other input (button selection, etc).
        {
            rc = upkeepBox(&mojobox, ch);
            if (rc == togglebutton)
            {
                int line = 0;
                rc = -1;  // reset so we don't stop processing input.
                if (toggle_option(NULL, opts, &line, selected+1) == 1)
                {
                    freeBox(mojobox, false);  // rebuild to reflect new options...
                    mojobox = NULL;
                } // if
            } // if
        } // else
    } while (rc == -1);

    freeBox(mojobox, true);

    for (i = 0; i < bcount; i++)
        free(buttons[i]);

    if (rc == backbutton)
        return -1;
    else if (rc == fwdbutton)
        return 1;

    return 0;  // error? Cancel?
} // optionBox


static int MojoGui_ncurses_options(MojoGuiSetupOptions *opts,
                                   boolean can_back, boolean can_fwd)
{
    char *title = xstrdup(_("Options"));
    int rc = optionBox(title, opts, can_back, can_fwd);
    free(title);
    return rc;
} // MojoGui_ncurses_options


static char *inputBox(const char *prompt, int *command, boolean can_back,
                      const char *defval)
{
    char *text = NULL;
    int w, h;
    int i;
    int ch;
    int rc = -1;
    MojoBox *mojobox = NULL;
    size_t retvalalloc = 64;
    size_t retvallen = 0;
    char *retval = NULL;
    char *buttons[3] = { NULL, NULL, NULL };
    int drawpos = 0;
    int drawlen = 0;
    int bcount = 0;
    int backbutton = -1;
    int cancelbutton = -1;

    if (defval == NULL)
        retval = (char *) xmalloc(retvalalloc);
    else
    {
        const size_t defvallen = strlen(defval);
        if ((defvallen * 2) > retvalalloc)
            retvalalloc = defvallen * 2;
        retval = (char *) xmalloc(retvalalloc);
        retvallen = defvallen;
        strcpy(retval, defval);
    } // else

    buttons[bcount++] = xstrdup(_("OK"));

    if (can_back)
    {
        backbutton = bcount++;
        buttons[backbutton] = xstrdup(_("Back"));
    } // if

    cancelbutton = bcount++;
    buttons[cancelbutton] = xstrdup(_("Cancel"));

    getmaxyx(stdscr, h, w);
    w -= 10;
    text = (char *) xmalloc(w+4);
    text[0] = '\n';
    memset(text+1, ' ', w);
    text[w+1] = '\n';
    text[w+2] = ' ';
    text[w+3] = '\0';
    mojobox = makeBox(prompt, text, buttons, bcount, false, false);
    free(text);
    text = NULL;

    do
    {
        getmaxyx(mojobox->textwin, h, w);
        w -= 2;

        if (drawpos >= retvallen)
            drawpos = 0;
        while ((drawlen = (retvallen - drawpos)) >= w)
            drawpos += 5;

        wattron(mojobox->textwin, COLOR_PAIR(MOJOCOLOR_TEXTENTRY) | A_BOLD);
        mvwhline(mojobox->textwin, 1, 1, ' ', w);  // blank line...
        mvwaddstr(mojobox->textwin, 1, 1, retval + drawpos);
        wattroff(mojobox->textwin, COLOR_PAIR(MOJOCOLOR_TEXTENTRY) | A_BOLD);
        wrefresh(mojobox->textwin);

        ch = wgetch(mojobox->mainwin);
        if ( (ch > 0) && (ch < KEY_MIN) && (isprint(ch)) )  // regular key.
        {
            if (retvalalloc <= retvallen)
            {
                retvalalloc *= 2;
                retval = xrealloc(retval, retvalalloc);
            } // if
            retval[retvallen++] = (char) ch;
            retval[retvallen] = '\0';
        } // if

        else if (ch == KEY_BACKSPACE)
        {
            if (retvallen > 0)
                retval[--retvallen] = '\0';
        } // else if

        else if (ch == KEY_RESIZE)
        {
            wrefresh(stdscr);
            getmaxyx(stdscr, h, w);
            w -= 10;
            text = (char *) xrealloc(mojobox->text, w+4);
            text[0] = '\n';
            memset(text+1, ' ', w);
            text[w+1] = '\n';
            text[w+2] = ' ';
            text[w+3] = '\0';
            mojobox->text = text;
            text = NULL;
            upkeepBox(&mojobox, KEY_RESIZE);  // let real resize happen...
        } // else if

        else
        {
            rc = upkeepBox(&mojobox, ch);
        } // else
    } while (rc == -1);

    freeBox(mojobox, true);

    for (i = 0; i < bcount; i++)
        free(buttons[i]);

    if (rc == backbutton)
        *command = -1;
    else if (rc == cancelbutton)
        *command = 0;
    else
        *command = 1;

    if (*command <= 0)
    {
        free(retval);
        retval = NULL;
    } // if

    return retval;
} // inputBox


static char *MojoGui_ncurses_destination(const char **recommends, int recnum,
                                         int *command, boolean can_back,
                                         boolean can_fwd)
{
    char *retval = NULL;
    while (true)
    {
        const char *localized = NULL;
        char *title = NULL;
        char *choosetxt = NULL;
        int rc = 0;

        if (recnum > 0)  // recommendations available.
        {
            int chosen = -1;
            MojoGuiSetupOptions opts;
            MojoGuiSetupOptions *prev = &opts;
            MojoGuiSetupOptions *next = NULL;
            MojoGuiSetupOptions *opt = NULL;
            memset(&opts, '\0', sizeof (MojoGuiSetupOptions));
            int i;
            for (i = 0; i < recnum; i++)
            {
                opt = (MojoGuiSetupOptions *) xmalloc(sizeof (*opt));
                opt->description = recommends[i];
                opt->size = -1;
                prev->next_sibling = opt;
                prev = opt;
            } // for

            choosetxt = xstrdup(_("(I want to specify a path.)"));
            opt = (MojoGuiSetupOptions *) xmalloc(sizeof (*opt));
            opt->description = choosetxt;
            opt->size = -1;
            prev->next_sibling = opt;
            prev = opt;

            opts.child = opts.next_sibling;  // fix this field.
            opts.next_sibling = NULL;
            opts.value = opts.child->value = true;  // make first default.
            opts.is_group_parent = true;
            opts.size = -1;

            title = xstrdup(_("Destination"));
            rc = optionBox(title, &opts, can_back, can_fwd);
            free(title);

            for (i = 0, next = opts.child; next != NULL; i++)
            {
                if (next->value)
                    chosen = i;
                prev = next;
                next = prev->next_sibling;
                free(prev);
            } // for

            free(choosetxt);

            *command = rc;
            if (rc <= 0)  // back or cancel.
                return NULL;

            else if ((chosen >= 0) && (chosen < recnum))  // a specific entry
                return xstrdup(recommends[chosen]);
        } // if

        // either no recommendations or user wants to enter own path...

        localized = _("Enter path where files will be installed.");
        title = xstrdup(localized);
        retval = inputBox(title, &rc, (can_back) || (recnum > 0), NULL);
        free(title);

        // user cancelled or entered text, or hit back and we aren't falling
        //  back to the option list...return.
        if ( (rc >= 0) || ((rc == -1) && (recnum == 0)) )
        {
            *command = rc;
            return retval;
        } // if

        // falling back to the option list again...loop.
    } // while

    // Shouldn't ever hit this, but just in case...
    *command = 0;
    return NULL;
} // MojoGui_ncurses_destination


static int MojoGui_ncurses_productkey(const char *desc, const char *fmt,
                                      char *buf, const int buflen,
                                      boolean can_back, boolean can_fwd)
{
    // !!! FIXME: need text option for (desc).
    // !!! FIXME: need max text entry of (buflen)
    // !!! FIXME: need to disable "next" button if code is invalid.
    char *prompt = xstrdup(_("Please enter your product key"));
    boolean getout = false;
    int retval = 0;

    while (!getout)
    {
        char *text = inputBox(prompt, &retval, can_back, buf);

        if (retval != 1)
            getout = true;
        else
        {
            snprintf(buf, buflen, "%s", text);
            if (isValidProductKey(fmt, text))
                getout = true;
            else
            {
                // !!! FIXME: just improve inputBox.
                // We can't check the input character-by-character, so reuse
                //  the failed-verification localized string.
                char *failtitle = xstrdup(_("Invalid product key"));
                char *failstr = xstrdup(_("That key appears to be invalid. Please try again."));
                MojoGui_ncurses_msgbox(failtitle, failstr);
                free(failstr);
                free(failtitle);
            } // else
        } // else
        free(text);
    } // while

    free(prompt);

    return retval;
} // MojoGui_ncurses_productkey


static boolean MojoGui_ncurses_insertmedia(const char *medianame)
{
    char *fmt = xstrdup(_("Please insert '%0'"));
    char *text = format(fmt, medianame);
    char *localized_ok = xstrdup(_("OK"));
    char *localized_cancel = xstrdup(_("Cancel"));
    char *buttons[] = { localized_ok, localized_cancel };
    MojoBox *mojobox = NULL;
    int rc = 0;

    mojobox = makeBox(_("Media change"), text, buttons, 2, false, true);
    while ((rc = upkeepBox(&mojobox, wgetch(mojobox->mainwin))) == -1) {}

    freeBox(mojobox, true);
    free(localized_cancel);
    free(localized_ok);
    free(text);
    free(fmt);
    return (rc == 0);
} // MojoGui_ncurses_insertmedia


static void MojoGui_ncurses_progressitem(void)
{
    // no-op in this UI target.
} // MojoGui_ncurses_progressitem


static boolean MojoGui_ncurses_progress(const char *type, const char *component,
                                        int percent, const char *item,
                                        boolean can_cancel)
{
    const uint32 now = ticks();
    boolean rebuild = (progressBox == NULL);
    int ch = 0;
    int rc = -1;

    if ( (lastComponent == NULL) ||
         (strcmp(lastComponent, component) != 0) ||
         (lastProgressType == NULL) ||
         (strcmp(lastProgressType, type) != 0) ||
         (lastCanCancel != can_cancel) )
    {
        free(lastProgressType);
        free(lastComponent);
        lastProgressType = xstrdup(type);
        lastComponent = xstrdup(component);
        lastCanCancel = can_cancel;
        rebuild = true;
    } // if

    if (rebuild)
    {
        int w, h;
        char *text = NULL;
        char *localized_cancel = (can_cancel) ? xstrdup(_("Cancel")) : NULL;
        char *buttons[] = { localized_cancel };
        const int buttoncount = (can_cancel) ? 1 : 0;
        char *spacebuf = NULL;
        getmaxyx(stdscr, h, w);
        w -= 10;
        text = (char *) xmalloc((w * 3) + 16);
        if (snprintf(text, w, "%s", component) > (w-4))
            strcpy((text+w)-4, "...");  // !!! FIXME: Unicode problem.
        strcat(text, "\n\n");
        spacebuf = (char *) xmalloc(w+1);
        memset(spacebuf, ' ', w);  // xmalloc provides null termination.
        strcat(text, spacebuf);
        free(spacebuf);
        strcat(text, "\n\n ");

        freeBox(progressBox, false);
        progressBox = makeBox(type, text, buttons, buttoncount, true, true);
        free(text);
        free(localized_cancel);
    } // if

    // limit update spam... will only write every one second, tops.
    if ((rebuild) || (percentTicks <= now))
    {
        static boolean unknownToggle = false;
        char *buf = NULL;
        WINDOW *win = progressBox->textwin;
        int w, h;
        getmaxyx(win, h, w);
        w -= 2;
        buf = (char *) xmalloc(w+1);

        if (percent < 0)
        {
            const boolean origToggle = unknownToggle;
            int i;
            wmove(win, h-3, 1);
            for (i = 0; i < w; i++)
            {
                if (unknownToggle)
                    waddch(win, ' ' | COLOR_PAIR(MOJOCOLOR_TODO));
                else
                    waddch(win, ' ' | COLOR_PAIR(MOJOCOLOR_DONE));
                unknownToggle = !unknownToggle;
            } // for
            unknownToggle = !origToggle;  // animate by reversing next time.
        } // if
        else
        {
            int cells = (int) ( ((double) w) * (((double) percent) / 100.0) );
            snprintf(buf, w+1, "%d%%", percent);
            mvwaddstr(win, h-3, ((w+2) - utf8len(buf)) / 2, buf);
            mvwchgat(win, h-3, 1, cells, A_BOLD, MOJOCOLOR_DONE, NULL);
            mvwchgat(win, h-3, 1+cells, w-cells, A_BOLD, MOJOCOLOR_TODO, NULL);
        } // else

        wtouchln(win, h-3, 1, 1);  // force reattributed cells to update.

        if (snprintf(buf, w+1, "%s", item) > (w-4))
            strcpy((buf+w)-4, "...");  // !!! FIXME: Unicode problem.
        mvwhline(win, h-2, 1, ' ', w);
        mvwaddstr(win, h-2, ((w+2) - utf8len(buf)) / 2, buf);

        free(buf);
        wrefresh(win);

        percentTicks = now + 1000;
    } // if

    // !!! FIXME: check for input here for cancel button, resize, etc...
    ch = wgetch(progressBox->mainwin);
    if (ch == KEY_RESIZE)
    {
        freeBox(progressBox, false);
        progressBox = NULL;
    } // if
    else if (ch == ERR)  // can't distinguish between an error and a timeout!
    {
        // do nothing...
    } // else if
    else
    {
        rc = upkeepBox(&progressBox, ch);
    } // else

    return (rc == -1);
} // MojoGui_ncurses_progress


static void MojoGui_ncurses_final(const char *msg)
{
    char *title = xstrdup(_("Finish"));
    MojoGui_ncurses_msgbox(title, msg);
    free(title);
} // MojoGui_ncurses_final

// end of gui_ncurses.c ...

