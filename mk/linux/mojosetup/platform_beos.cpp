/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// You also need to compile unix.c, but this lets us add some things that
//  cause conflicts between the Be headers and MojoSetup, and use C++. These
//  are largely POSIX things that are missing from BeOS 5...keeping them here
//  even on Zeta lets us be binary compatible across everything, I think.

#if PLATFORM_BEOS

#include <stdio.h>
#include <string.h>
#include <be/app/Roster.h>
#include <be/kernel/image.h>
#include <be/kernel/OS.h>

extern "C" { int beos_launchBrowser(const char *url); }
int beos_launchBrowser(const char *url)
{
    static struct { const char *prot, const char *mime } protmimemap[] =
    {
        { "http://", "text/html" },
        { "https://", "text/html" },
        { "file://", "text/html" },  // let the web browser handle this.
        { "ftp://", "application/x-vnd.Be.URL.ftp" },
        { "mailto:", "text/x-email" },
        { NULL, NULL }
    };

    const char *mime = NULL;

    for (int i = 0; protmimemap[i].prot != NULL; i++)
    {
        const char *prot = protmimemap[i].prot;
        if (strncmp(url, prot, strlen(prot)) == 0)
        {
            mime = protmimemap[i].mime;
            break;
        } // if
    } // for

    if (mime == NULL)
        return false;  // no handler for this protocol.

    // be_roster is a global object supplied by the system runtime.
    return (be_roster->Launch(mime, 1, &url) == B_OK);
} // beos_launchBrowser


extern "C" { void *beos_dlopen(const char *fname, int unused); }
void *beos_dlopen(const char *fname, int unused)
{
    const image_id lib = load_add_on(fname);
    (void) unused;
    if (lib < 0)
        return NULL;
    return (void *) lib;
} // beos_dlopen


extern "C" { void *beos_dlsym(void *lib, const char *sym); }
void *beos_dlsym(void *lib, const char *sym)
{
    void *addr = NULL;
    if (get_image_symbol((image_id) lib, sym, B_SYMBOL_TYPE_TEXT, &addr))
        return NULL;
    return addr;
} // beos_dlsym


extern "C" { void beos_dlclose(void *lib); }
void beos_dlclose(void *lib)
{
    unload_add_on((image_id) lib);
} // beos_dlclose


extern "C" { void beos_usleep(unsigned long microseconds); }
void beos_usleep(unsigned long microseconds)
{
    snooze(microseconds);
} // beos_usleep

#endif  // PLATFORM_BEOS

// end of beos.c ...

