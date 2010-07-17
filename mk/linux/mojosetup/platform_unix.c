/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if PLATFORM_UNIX

#if PLATFORM_MACOSX
#include <Carbon/Carbon.h>
#undef true
#undef false
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/mount.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

#if MOJOSETUP_HAVE_SYS_UCRED_H
#  ifdef MOJOSETUP_HAVE_MNTENT_H
#    undef MOJOSETUP_HAVE_MNTENT_H /* don't do both... */
#  endif
#  include <sys/ucred.h>
#endif

#if MOJOSETUP_HAVE_MNTENT_H
#  include <mntent.h>
#endif

#if MOJOSETUP_HAVE_SYS_MNTTAB_H
#  include <sys/mnttab.h>
#endif

#if PLATFORM_BEOS
#define DLOPEN_ARGS 0
void *beos_dlopen(const char *fname, int unused);
void *beos_dlsym(void *lib, const char *sym);
void beos_dlclose(void *lib);
void beos_usleep(unsigned long ticks);
#define dlopen beos_dlopen
#define dlsym beos_dlsym
#define dlclose beos_dlclose
#define usleep beos_usleep
#else
#include <dlfcn.h>
#define DLOPEN_ARGS (RTLD_NOW | RTLD_GLOBAL)
#endif

#include "platform.h"
#include "gui.h"
#include "fileio.h"

static struct timeval startup_time;

boolean MojoPlatform_istty(void)
{
    static boolean already_checked = false;  // this never changes in a run.
    static boolean retval = false;
    if (!already_checked)
    {
        retval = isatty(0) && isatty(1) ? true : false;
        already_checked = true;
    } // if

    return retval;
} // MojoPlatform_istty


char *MojoPlatform_currentWorkingDir(void)
{
    char *retval = NULL;
    size_t len;

    // loop a few times in case we don't have a large enough buffer.
    for (len = 128; len <= (16*1024); len *= 2)
    {
        retval = (char *) xrealloc(retval, len);
        if (getcwd(retval, len-1) != NULL)
        {
            size_t slen = strlen(retval);
            if (retval[slen-1] != '/')   // make sure this ends with '/' ...
            {
                retval[slen] = '/';
                retval[slen+1] = '\0';
            } // if
            return retval;
        } // if
    } // for

    free(retval);
    return NULL;
} // MojoPlatform_currentWorkingDir


char *MojoPlatform_readlink(const char *linkname)
{
    size_t alloclen = 16;
    char *retval = NULL;
    char *buf = NULL;
    ssize_t len = -1;

    do
    {
        alloclen *= 2;
        buf = xrealloc(buf, alloclen);
        len = readlink(linkname, buf, alloclen-1);
        if ( (len != -1) && (len < (alloclen-1)) )  // !error && !overflow
        {
            buf[len] = '\0';  // readlink() doesn't null-terminate!
            retval = xrealloc(buf, (size_t) (len+1));  // shrink it down.
        } // if
    } while (len >= (alloclen-1));  // loop if we need a bigger buffer.

    return retval;  // caller must free() this.
} // MojoPlatform_readlink


static void *guaranteeAllocation(void *ptr, size_t len, size_t *_alloclen)
{
    void *retval = NULL;
    size_t alloclen = *_alloclen;
    if (alloclen > len)
        return ptr;

    if (!alloclen)
        alloclen = 1;

    while (alloclen <= len)
        alloclen *= 2;

    retval = xrealloc(ptr, alloclen);
    if (retval != NULL)
        *_alloclen = alloclen;

    return retval;
} // guaranteeAllocation


// This is a mess, but I'm not sure it can be done more cleanly.
static char *realpathInternal(char *path, const char *cwd, int linkloop)
{
    char *linkname = NULL;
    char *retval = NULL;
    size_t len = 0;
    size_t alloclen = 0;

    if (*path == '/')   // absolute path.
    {
        retval = xstrdup("/");
        path++;
        len = 1;
    } // if
    else   // relative path.
    {
        if (cwd != NULL)
            retval = xstrdup(cwd);
        else
        {
            if ((retval = MojoPlatform_currentWorkingDir()) == NULL)
                return NULL;
        } // else
        len = strlen(retval);
    } // else

    while (true)
    {
        struct stat statbuf;
        size_t newlen;

        char *nextpath = strchr(path, '/');
        if (nextpath != NULL)
            *nextpath = '\0';

        newlen = strlen(path);
        retval = guaranteeAllocation(retval, len + newlen + 2, &alloclen);
        strcpy(retval + len, path);

        if (*path == '\0')
            retval[--len] = '\0';  // chop ending "/" bit, it gets readded later.

        else if (strcmp(path, ".") == 0)
        {
            retval[--len] = '\0';  // chop ending "/." bit
            newlen = 0;
        } // else if

        else if (strcmp(path, "..") == 0)
        {
            char *ptr;
            retval[--len] = '\0';  // chop ending "/.." bit
            ptr = strrchr(retval, '/');
            if ((ptr == NULL) || (ptr == retval))
            {
                strcpy(retval, "/");
                len = 0;
            } // if
            else
            {
                *ptr = '\0';
                len -= (size_t) ((retval+len)-ptr);
            } // else
            newlen = 0;
        } // else if

        // it may be a symlink...check it.
        else if (lstat(retval, &statbuf) == -1)
            goto realpathInternal_failed;

        else if (S_ISLNK(statbuf.st_mode))
        {
            char *newresolve = NULL;
            if (linkloop > 255)
                goto realpathInternal_failed;

            linkname = MojoPlatform_readlink(retval);
            if (linkname == NULL)
                goto realpathInternal_failed;

            // chop off symlink name for its cwd.
            retval[len] = '\0';

            // resolve the link...
            newresolve = realpathInternal(linkname, retval, linkloop + 1);
            if (newresolve == NULL)
                goto realpathInternal_failed;

            len = strlen(newresolve);
            retval = guaranteeAllocation(retval, len + 2, &alloclen);
            strcpy(retval, newresolve);
            free(newresolve);
            free(linkname);
            linkname = NULL;
        } // else if

        else
        {
            len += newlen;
        } // else

        if (nextpath == NULL)
            break;  // holy crap we're done!
        else  // append a '/' before the next path element.
        {
            path = nextpath + 1;
            retval[len++] = '/';
            retval[len] = '\0';
        } // else
    } // while

    // Shrink string if we're using more memory than necessary...
    if (alloclen > len+1)
        retval = (char *) xrealloc(retval, len+1);

    return retval;

realpathInternal_failed:
    free(linkname);
    free(retval);
    return NULL;
} // realpathInternal


// Rolling my own realpath, even if the runtime has one, since apparently
//  the spec is a little flakey, and it can overflow PATH_MAX. On BeOS <= 5,
//  we'd have to resort to BPath to do this, too, and I'd rather avoid the C++
//  dependencies and headers.
char *MojoPlatform_realpath(const char *_path)
{
    char *path = xstrdup(_path);
    char *retval = realpathInternal(path, NULL, 0);
    free(path);
    return retval;
} // MojoPlatform_realpath


// (Stolen from physicsfs: http://icculus.org/physfs/ ...)
/*
 * See where program (bin) resides in the $PATH. Returns a copy of the first
 *  element in $PATH that contains it, or NULL if it doesn't exist or there
 *  were other problems.
 *
 * You are expected to free() the return value when you're done with it.
 */
static char *findBinaryInPath(const char *bin)
{
    const char *_envr = getenv("PATH");
    size_t alloc_size = 0;
    char *envr = NULL;
    char *exe = NULL;
    char *start = NULL;
    char *ptr = NULL;

    if ((_envr == NULL) || (bin == NULL))
        return NULL;

    envr = xstrdup(_envr);
    start = envr;

    do
    {
        size_t size;
        ptr = strchr(start, ':');  // find next $PATH separator.
        if (ptr)
            *ptr = '\0';

        size = strlen(start) + strlen(bin) + 2;
        if (size > alloc_size)
        {
            char *x = (char *) xrealloc(exe, size);
            alloc_size = size;
            exe = x;
        } // if

        // build full binary path...
        strcpy(exe, start);
        if ((exe[0] == '\0') || (exe[strlen(exe) - 1] != '/'))
            strcat(exe, "/");
        strcat(exe, bin);

        if (access(exe, X_OK) == 0)  // Exists as executable? We're done.
        {
            strcpy(exe, start);  // i'm lazy. piss off.
            free(envr);
            return(exe);
        } // if

        start = ptr + 1;  // start points to beginning of next element.
    } while (ptr != NULL);

    if (exe != NULL)
        free(exe);

    free(envr);

    return(NULL);  // doesn't exist in path.
} // findBinaryInPath


char *MojoPlatform_appBinaryPath(void)
{
    const char *argv0 = GArgv[0];
    char *retval = NULL;

    if (strchr(argv0, '/') != NULL)  
        retval = MojoPlatform_realpath(argv0); // argv[0] contains a path?
    else  // slow path...have to search the whole $PATH for this one...
    {
        char *found = findBinaryInPath(argv0);
        if (found)
            retval = MojoPlatform_realpath(found);
        free(found);
    } // else

    return retval;
} // MojoPlatform_appBinaryPath


char *MojoPlatform_homedir(void)
{
    const char *envr = getenv("HOME");
    return xstrdup(envr ? envr : "/");
} // MojoPlatform_homedir


// This implementation is a bit naive.
char *MojoPlatform_locale(void)
{
    char *retval = NULL;
    char *ptr = NULL;
    const char *envr = getenv("LANG");
    if (envr != NULL)
    {
        retval = xstrdup(envr);
        ptr = strchr(retval, '.');  // chop off encoding if explicitly listed.
        if (ptr != NULL)
            *ptr = '\0';
        ptr = strchr(retval, '@');  // chop off extra bits if explicitly listed.
        if (ptr != NULL)
            *ptr = '\0';
    } // if

    #if PLATFORM_MACOSX
    else if (CFLocaleCreateCanonicalLocaleIdentifierFromString == NULL)
        retval = NULL; // !!! FIXME: 10.2 compatibility?

    else if (CFLocaleCreateCanonicalLocaleIdentifierFromString != NULL)
    {
        CFPropertyListRef languages = CFPreferencesCopyAppValue(
                                            CFSTR("AppleLanguages"),
                                            kCFPreferencesCurrentApplication);
        if (languages != NULL)
        {
            CFStringRef primary = CFArrayGetValueAtIndex(languages, 0);
            if (primary != NULL)
            {
                CFStringRef locale =
                        CFLocaleCreateCanonicalLocaleIdentifierFromString(
                                                kCFAllocatorDefault, primary);
                if (locale != NULL)
                {
                    const CFIndex len = (CFStringGetLength(locale) + 1) * 6;
                    ptr = (char*) xmalloc(len);
                    CFStringGetCString(locale, ptr, len, kCFStringEncodingUTF8);
                    CFRelease(locale);
                    retval = xrealloc(ptr, strlen(ptr) + 1);
                    // !!! FIXME: this may not be 100% right, but change
                    // !!! FIXME:  xx-YY to xx_YY (lang_country).
                    if (retval[2] == '-')
                        retval[2] = '_';
                    if (retval[3] == '-')
                        retval[3] = '_';
                } // if
            } // if
            CFRelease(languages);
        } // if
    } // else if
    #endif

    return retval;
} // MojoPlatform_locale


char *MojoPlatform_osType(void)
{
#if PLATFORM_MACOSX
    return xstrdup("macosx");
#elif PLATFORM_BEOS
    return xstrdup("beos");   // !!! FIXME: zeta? haiku?
#elif defined(linux) || defined(__linux) || defined(__linux__)
    return xstrdup("linux");
#elif defined(__FreeBSD__) || defined(__DragonFly__)
    return xstrdup("freebsd");
#elif defined(__NetBSD__)
    return xstrdup("netbsd");
#elif defined(__OpenBSD__)
    return xstrdup("openbsd");
#elif defined(bsdi) || defined(__bsdi) || defined(__bsdi__)
    return xstrdup("bsdi");
#elif defined(_AIX)
    return xstrdup("aix");
#elif defined(hpux) || defined(__hpux) || defined(__hpux__)
    return xstrdup("hpux");
#elif defined(sgi) || defined(__sgi) || defined(__sgi__) || defined(_SGI_SOURCE)
    return xstrdup("irix");
#elif defined(sun)
    return xstrdup("solaris");
#else
#   error Please define your platform.
    return NULL;
#endif
} // MojoPlatform_ostype


char *MojoPlatform_osVersion()
{
#if PLATFORM_MACOSX
    SInt32 ver, major, minor, patch;
    boolean convert = false;
    char *buf = NULL;
    char dummy = 0;
    int len = 0;

	if (Gestalt(gestaltSystemVersion, &ver) != noErr)
        return NULL;

    if (ver < 0x1030)
        convert = true;  // split (ver) into (major),(minor),(patch).
    else
    {
        // presumably this won't fail. But if it does, we'll just use the
        //  original version value. This might cut the value--10.12.11 will
        //  come out to 10.9.9, for example--but it's better than nothing.
    	if (Gestalt(gestaltSystemVersionMajor, &major) != noErr)
            convert = true;
    	if (Gestalt(gestaltSystemVersionMinor, &minor) != noErr)
            convert = true;
    	if (Gestalt(gestaltSystemVersionBugFix, &patch) != noErr)
            convert = true;
    } /* else */

    if (convert)
    {
        major = ((ver & 0xFF00) >> 8);
        major = (((major / 16) * 10) + (major % 16));
        minor = ((ver & 0xF0) >> 4);
        patch = (ver & 0xF);
    } /* if */

    len = snprintf(&dummy, sizeof (dummy), "%d.%d.%d",
                    (int) major, (int) minor, (int) patch);
    buf = (char *) xmalloc(len+1);
    snprintf(buf, len+1, "%d.%d.%d", (int) major, (int) minor, (int) patch);
    return buf;

#else
    // This information may or may not actually MEAN anything. On BeOS, it's
    //  useful, but on other things, like Linux, it'll give you the kernel
    //  version, which doesn't necessarily help.
    struct utsname un;
    if (uname(&un) == 0)
        return xstrdup(un.release);
#endif

    return NULL;
} // MojoPlatform_osversion


void MojoPlatform_sleep(uint32 ticks)
{
    usleep(ticks * 1000);
} // MojoPlatform_sleep


uint32 MojoPlatform_ticks(void)
{
    uint64 then_ms, now_ms;
    struct timeval now;
    gettimeofday(&now, NULL);
    then_ms = (((uint64) startup_time.tv_sec) * 1000) +
              (((uint64) startup_time.tv_usec) / 1000);
    now_ms = (((uint64) now.tv_sec) * 1000) + (((uint64) now.tv_usec) / 1000);
    return ((uint32) (now_ms - then_ms));
} // MojoPlatform_ticks


void MojoPlatform_die(void)
{
    _exit(86);
} // MojoPlatform_die


boolean MojoPlatform_unlink(const char *fname)
{
    boolean retval = false;
    struct stat statbuf;
    if (lstat(fname, &statbuf) != -1)
    {
        if (S_ISDIR(statbuf.st_mode))
            retval = (rmdir(fname) == 0);
        else
            retval = (unlink(fname) == 0);
    } // if
    return retval;
} // MojoPlatform_unlink


boolean MojoPlatform_symlink(const char *src, const char *dst)
{
    return (symlink(dst, src) == 0);
} // MojoPlatform_symlink


boolean MojoPlatform_mkdir(const char *path, uint16 perms)
{
    // !!! FIXME: error if already exists?
    return (mkdir(path, perms) == 0);
} // MojoPlatform_mkdir


boolean MojoPlatform_rename(const char *src, const char *dst)
{
    return (rename(src, dst) == 0);
} // MojoPlatform_rename


boolean MojoPlatform_exists(const char *dir, const char *fname)
{
    boolean retval = false;
    if (fname == NULL)
        retval = (access(dir, F_OK) != -1);
    else
    {
        const size_t len = strlen(dir) + strlen(fname) + 2;
        char *buf = (char *) xmalloc(len);
        snprintf(buf, len, "%s/%s", dir, fname);
        retval = (access(buf, F_OK) != -1);
        free(buf);
    } // else
    return retval;
} // MojoPlatform_exists


boolean MojoPlatform_writable(const char *fname)
{
    return (access(fname, W_OK) == 0);
} // MojoPlatform_writable


boolean MojoPlatform_isdir(const char *dir)
{
    boolean retval = false;
    struct stat statbuf;
    if (lstat(dir, &statbuf) != -1)
    {
        if (S_ISDIR(statbuf.st_mode))
            retval = true;
    } // if
    return retval;
} // MojoPlatform_isdir


boolean MojoPlatform_issymlink(const char *dir)
{
    boolean retval = false;
    struct stat statbuf;
    if (lstat(dir, &statbuf) != -1)
    {
        if (S_ISLNK(statbuf.st_mode))
            retval = true;
    } // if
    return retval;
} // MojoPlatform_issymlink


boolean MojoPlatform_isfile(const char *dir)
{
    boolean retval = false;
    struct stat statbuf;
    if (lstat(dir, &statbuf) != -1)
    {
        if (S_ISREG(statbuf.st_mode))
            retval = true;
    } // if
    return retval;
} // MojoPlatform_isfile


void *MojoPlatform_stdout(void)
{
    int *retval = (int *) xmalloc(sizeof (int));
    *retval = 1;  // stdout.
    return retval;
} // MojoPlatform_stdout


void *MojoPlatform_open(const char *fname, uint32 flags, uint16 mode)
{
    void *retval = NULL;
    int fd = -1;
    int unixflags = 0;

    if ((flags & MOJOFILE_READ) && (flags & MOJOFILE_WRITE))
        unixflags |= O_RDWR;
    else if (flags & MOJOFILE_READ)
        unixflags |= O_RDONLY;
    else if (flags & MOJOFILE_WRITE)
        unixflags |= O_WRONLY;
    else
        return NULL;  // have to specify SOMETHING.

    if (flags & MOJOFILE_APPEND)
        unixflags |= O_APPEND;
    if (flags & MOJOFILE_TRUNCATE)
        unixflags |= O_TRUNC;
    if (flags & MOJOFILE_CREATE)
        unixflags |= O_CREAT;
    if (flags & MOJOFILE_EXCLUSIVE)
        unixflags |= O_EXCL;

    fd = open(fname, unixflags, (mode_t) mode);
    if (fd != -1)
    {
        int *intptr = (int *) xmalloc(sizeof (int));
        *intptr = fd;
        retval = intptr;
    } // if

    return retval;
} // MojoPlatform_open


int64 MojoPlatform_read(void *fd, void *buf, uint32 bytes)
{
    return (int64) read(*((int *) fd), buf, bytes);
} // MojoPlatform_read


int64 MojoPlatform_write(void *fd, const void *buf, uint32 bytes)
{
    return (int64) write(*((int *) fd), buf, bytes);
} // MojoPlatform_write


int64 MojoPlatform_tell(void *fd)
{
    return (int64) lseek(*((int *) fd), 0, SEEK_CUR);
} // MojoPlatform_tell


int64 MojoPlatform_seek(void *fd, int64 offset, MojoFileSeek whence)
{
    int unixwhence;
    switch (whence)
    {
        case MOJOSEEK_SET: unixwhence = SEEK_SET; break;
        case MOJOSEEK_CURRENT: unixwhence = SEEK_CUR; break;
        case MOJOSEEK_END: unixwhence = SEEK_END; break;
        default: return -1;  // !!! FIXME: maybe just abort?
    } // switch

    return (int64) lseek(*((int *) fd), offset, unixwhence);
} // MojoPlatform_seek


int64 MojoPlatform_flen(void *fd)
{
    struct stat statbuf;
    if (fstat(*((int *) fd), &statbuf) == -1)
        return -1;
    return((int64) statbuf.st_size);
} // MojoPlatform_flen


boolean MojoPlatform_flush(void *fd)
{
    return (fsync(*((int *) fd)) == 0);
} // MojoPlatform_flush


boolean MojoPlatform_close(void *fd)
{
    boolean retval = false;
    int handle = *((int *) fd);

    // don't close stdin, stdout, or stderr.
    if ((handle == 0) || (handle == 1) || (handle == 2))
    {
        free(fd);
        return true;
    } // if

    if (close(handle) == 0)
        free(fd);
    return retval;
} // MojoPlatform_close


void *MojoPlatform_opendir(const char *dirname)
{
    return opendir(dirname);
} // MojoPlatform_opendir


char *MojoPlatform_readdir(void *_dirhandle)
{
    DIR *dirhandle = (DIR *) _dirhandle;
    struct dirent *dent = NULL;

    while ((dent = readdir(dirhandle)) != NULL)
    {
        if (strcmp(dent->d_name, ".") == 0)
            continue;  // skip these.

        else if (strcmp(dent->d_name, "..") == 0)
            continue;  // skip these, too.

        else
            break;  // found a valid entry, go on.
    } // while

    return ((dent) ? xstrdup(dent->d_name) : NULL);
} // MojoPlatform_readdir


void MojoPlatform_closedir(void *dirhandle)
{
    closedir((DIR *) dirhandle);
} // MojoPlatform_closedir


int64 MojoPlatform_filesize(const char *fname)
{
    int retval = -1;
    struct stat statbuf;
    if ( (lstat(fname, &statbuf) != -1) && (S_ISREG(statbuf.st_mode)) )
        retval = (int64) statbuf.st_size;
    return retval;
} // MojoPlatform_filesize


boolean MojoPlatform_perms(const char *fname, uint16 *p)
{
    boolean retval = false;
    struct stat statbuf;
    if (stat(fname, &statbuf) != -1)
    {
        *p = statbuf.st_mode;
        retval = true;
    } // if
    return retval;
} // MojoPlatform_perms


uint16 MojoPlatform_defaultFilePerms(void)
{
    return 0644;
} // MojoPlatform_defaultFilePerms


uint16 MojoPlatform_defaultDirPerms(void)
{
    return 0755;
} // MojoPlatform_defaultDirPerms


uint16 MojoPlatform_makePermissions(const char *str, boolean *_valid)
{
    uint16 retval = 0644;
    boolean valid = true;
    if (str != NULL)
    {
        char *endptr = NULL;
        long strval = strtol(str, &endptr, 8);
        // complete string was a valid number?
        valid = ((*endptr == '\0') && (strval >= 0) && (strval <= 0xFFFF));
        if (valid)
            retval = (uint16) strval;
    } // if

    *_valid = valid;
    return retval;
} // MojoPlatform_makePermissions


boolean MojoPlatform_chmod(const char *fname, uint16 p)
{
    return (chmod(fname, p) != -1);
} // MojoPlatform_chmod


char *MojoPlatform_findMedia(const char *uniquefile)
{
#if MOJOSETUP_HAVE_SYS_UCRED_H
    int i = 0;
    struct statfs *mntbufp = NULL;
    int mounts = getmntinfo(&mntbufp, MNT_WAIT);
    for (i = 0; i < mounts; i++)
    {
        const char *mnt = mntbufp[i].f_mntonname;
        if (MojoPlatform_exists(mnt, uniquefile))
            return xstrdup(mnt);
    } // for

#elif MOJOSETUP_HAVE_MNTENT_H
    FILE *mounts = setmntent("/etc/mtab", "r");
    if (mounts != NULL)
    {
        struct mntent *ent = NULL;
        while ((ent = getmntent(mounts)) != NULL)
        {
            const char *mnt = ent->mnt_dir;
            if (MojoPlatform_exists(mnt, uniquefile))
            {
                endmntent(mounts);
                return xstrdup(mnt);
            } // if
        } // while
        endmntent(mounts);
    } // if

#elif MOJOSETUP_HAVE_SYS_MNTTAB_H
    FILE *mounts = fopen(MNTTAB, "r");
    if (mounts != NULL)
    {
        struct mnttab ent;
        while (getmntent(mounts, &ent) == 0)
        {
            const char *mnt = ent.mnt_mountp;
            if (MojoPlatform_exists(mnt, uniquefile))
            {
                fclose(mounts);
                return xstrdup(mnt);
            } // if
        } // while
        fclose(mounts);
    } // if

#else
#   warning No mountpoint detection on this platform...
#endif

    return NULL;
} // MojoPlatform_findMedia


void MojoPlatform_log(const char *str)
{
    syslog(LOG_USER | LOG_INFO, "%s", str);

#if PLATFORM_MACOSX
    // put to stdout too, if this isn't the stdio UI.
    // This will let the info show up in /Applications/Utilities/Console.app
    if ((GGui != NULL) && (strcmp(GGui->name(), "stdio") != 0))
        printf("%s\n", str);
#endif
} // MojoPlatform_log


static boolean testTmpDir(const char *dname, char *buf,
                          size_t len, const char *tmpl)
{
    boolean retval = false;
    if ( (dname != NULL) && (access(dname, R_OK | W_OK | X_OK) == 0) )
    {
        struct stat statbuf;
        if ( (stat(dname, &statbuf) == 0) && (S_ISDIR(statbuf.st_mode)) )
        {
            const size_t rc = snprintf(buf, len, "%s/%s", dname, tmpl);
            if (rc < len)
                retval = true;
        } // if
    } // if

    return retval;
} // testTmpDir


void *MojoPlatform_dlopen(const uint8 *img, size_t len)
{
    // Write the image to a temporary file, dlopen() it, and delete it
    //  immediately. The inode will be kept around by the Unix kernel until
    //  we either dlclose() it or the process terminates, but we don't have
    //  to worry about polluting the /tmp directory or cleaning this up later.
    // We'll try every reasonable temp directory location until we find one
    //  that works, in case (say) one lets us write a file, but there
    //  isn't enough space for the data.

    // /dev/shm may be able to avoid writing to physical media...try it first.
    const char *dirs[] = { "/dev/shm", getenv("TMPDIR"), P_tmpdir, "/tmp" };
    const char *tmpl = "mojosetup-plugin-XXXXXX";
    char fname[PATH_MAX];
    void *retval = NULL;
    int i = 0;

    if (dlopen == NULL)   // weak symbol on older Mac OS X
        return NULL;

    #ifndef P_tmpdir  // glibc defines this, maybe others.
    #define P_tmpdir NULL
    #endif

    for (i = 0; (i < STATICARRAYLEN(dirs)) && (retval == NULL); i++)
    {
        if (testTmpDir(dirs[i], fname, sizeof (fname), tmpl))
        {
            const int fd = mkstemp(fname);
            if (fd != -1)
            {
                const size_t bw = write(fd, img, len);
                const int rc = close(fd);
                if ((bw == len) && (rc != -1))
                    retval = dlopen(fname, DLOPEN_ARGS);
                unlink(fname);
            } // if
        } // if
    } // for

    return retval;
} // MojoPlatform_dlopen


void *MojoPlatform_dlsym(void *lib, const char *sym)
{
    #if PLATFORM_MACOSX
    if (dlsym == NULL) return NULL;  // weak symbol on older Mac OS X
    #endif

    return dlsym(lib, sym);
} // MojoPlatform_dlsym


void MojoPlatform_dlclose(void *lib)
{
    #if PLATFORM_MACOSX
    if (dlclose == NULL) return;  // weak symbol on older Mac OS X
    #endif

    dlclose(lib);
} // MojoPlatform_dlclose


static int runScriptString(const char *str, boolean devnull, const char **_argv)
{
    int retval = 127;
    pid_t pid = 0;
    int pipes[2];

    if (pipe(pipes) == -1)
        return retval;

    pid = fork();
    if (pid == -1)  // -1 == fork() failed.
    {
        close(pipes[0]);
        close(pipes[1]);
        return retval;
    } // if

    else if (pid == 0)  // we're the child process.
    {
        int argc = 0;
        const char **argv = NULL;

        close(pipes[1]);  // close the writing end.
        dup2(pipes[0], 0);  // replace stdin.
        if (devnull)
        {
            dup2(open("/dev/null", O_WRONLY), 1);  // replace stdout
            dup2(open("/dev/null", O_WRONLY), 2);  // replace stderr
        } // if

        while (_argv[argc++] != NULL) { /* no-op */ }
        argv = (const char **) xmalloc(sizeof (char *) * argc+3);
        argv[0] = "/bin/sh";
        argv[1] = "-s";
        for (argc = 0; _argv[argc] != NULL; argc++)
            argv[argc+2] = _argv[argc];
        argv[argc+2] = NULL;

        execv(argv[0], (char **) argv);
        _exit(retval);  // uhoh, failed.
    } // else if

    else  // we're the parent (pid == child process id).
    {
        int status = 0;
        size_t len = strlen(str);
        boolean failed = false;
        close(pipes[0]);  // close the reading end.
        failed |= (write(pipes[1], str, len) != len);
        failed |= (close(pipes[1]) == -1);

        // !!! FIXME: we need a GGui->pump() or something here if we'll block.
        if (waitpid(pid, &status, 0) != -1)
        {
            if (WIFEXITED(status))
                retval = WEXITSTATUS(status);
        } // if
    } // else

    return retval;
} // runScriptString


int MojoPlatform_runScript(const char *script, boolean devnull, const char **argv)
{
    int retval = 127;
    char *str = NULL;
    MojoInput *in = MojoInput_newFromArchivePath(GBaseArchive, script);
    if (in != NULL)
    {
        int64 len = in->length(in);
        if (len > 0)
        {
            str = (char *) xmalloc(len + 1);
            if (in->read(in, str, len) == len)
                str[len] = '\0';
            else
            {
                free(str);
                str = NULL;
            } // else
        } // if

        in->close(in);
    } // if

    if (str != NULL)
        retval = runScriptString(str, devnull, argv);

    free(str);

    return retval;
} // runScript


#if !PLATFORM_MACOSX && !PLATFORM_BEOS
static char *shellEscape(const char *str)
{
    size_t len = 0;
    char *retval = NULL;
    const char *ptr = NULL;
    char *dst = NULL;

    for (ptr = str; *ptr; ptr++)
        len += (*ptr == '\'') ? 4 : 1;

    retval = (char *) xmalloc(len + 3);  // +2 single quotes and a null char.
    dst = retval;
    *(dst++) = '\'';

    for (ptr = str; *ptr; ptr++)
    {
        const char ch = *ptr;
        if (ch != '\'')
            *(dst++) = ch;
        else
        {
            *(dst++) = '\'';
            *(dst++) = '\\';
            *(dst++) = '\'';
            *(dst++) = '\'';
        } // else
    } // for

    *(dst++) = '\'';
    *(dst++) = '\0';
    return retval;
} // shellEscape


static boolean unix_launchXdgUtil(const char *util, const char **argv)
{
    char *path = findBinaryInPath(util);
    int rc = 0;

    // !!! FIXME: do I really need to be using system() and
    // !!! FIXME:  suffering with shell escaping?

    if (path != NULL)  // it's installed on the system; use that.
    {
        char *cmd = shellEscape(util);
        char *tmp = NULL;
        int i;

        // just in case there's a space in the $PATH entry...
        tmp = shellEscape(path);
        free(path);
        path = tmp;

        for (i = 0; argv[i]; i++)
        {
            char *escaped = shellEscape(argv[i]);
            tmp = format("%0 %1", cmd, escaped);
            free(escaped);
            free(cmd);
            cmd = tmp;
        } // for

        tmp = format("%0/%1 >/dev/null 2>&1", path, cmd);
        free(cmd);
        cmd = tmp;
        rc = system(cmd);
        logDebug("system( %0 ) returned %1", cmd, numstr(rc));
        free(cmd);
        free(path);
    } // if

    else  // try our fallback copy of xdg-utils in GBaseArchive?
    {
        char *script = format("meta/xdg-utils/%0", util);
        rc = MojoPlatform_runScript(script, true, argv);
        logDebug("internal script '%0' returned %1", script, numstr(rc));
        free(script);
    } // if

    return (rc == 0);
} // unix_launchXdgUtil


static boolean unix_launchBrowser(const char *url)
{
    const char *argv[] = { url, NULL };
    return unix_launchXdgUtil("xdg-open", argv);
} // unix_launchBrowser


boolean xdgDesktopMenuItem(const char *action, const char *data)
{
    // xdg-utils, being shell scripts, don't do well with paths containing
    //  spaces. We attempt to mitigate this by chdir()'ing to the directory
    //  with the file to install.
    const char *ptr = strrchr(data, '/');
    boolean retval = false;
    if (ptr == NULL)
    {
        const char *argv[] = { action, data, NULL };
        retval = unix_launchXdgUtil("xdg-desktop-menu", argv);
    }
    else
    {
        char *working_dir = MojoPlatform_currentWorkingDir();
        if (working_dir != NULL)
        {
            char *cpy = xstrdup(data);
            char *fname = cpy + ((size_t)(ptr-data));
            const char *argv[] = { action, fname+1, NULL };
            *(fname++) = '\0';
            if (chdir(cpy) == 0)
            {
                retval = unix_launchXdgUtil("xdg-desktop-menu", argv);
                if (chdir(working_dir) == -1)  // deep trouble!
                    fatal("Failed to chdir to '%0'", working_dir);
            } // if
            free(cpy);
            free(working_dir);
        } // if
    } // else
    return retval;
} // xdgDesktopMenuItem
#endif


boolean MojoPlatform_launchBrowser(const char *url)
{
#if PLATFORM_MACOSX
    CFURLRef cfurl = CFURLCreateWithBytes(NULL, (const UInt8 *) url,
                                    strlen(url), kCFStringEncodingUTF8, NULL);
    const OSStatus err = LSOpenCFURLRef(cfurl, NULL);
    CFRelease(cfurl);
    return (err == noErr);
#elif PLATFORM_BEOS
    extern int beos_launchBrowser(const char *url);
    return beos_launchBrowser(url) ? true : false;
#else
    return unix_launchBrowser(url);
#endif
} // MojoPlatform_launchBrowser


boolean MojoPlatform_installDesktopMenuItem(const char *data)
{
#if PLATFORM_MACOSX || PLATFORM_BEOS
    // !!! FIXME: write me.
    STUBBED("desktop menu support");
    return false;
#else
    return xdgDesktopMenuItem("install", data);
#endif
} // MojoPlatform_installDesktopMenuItem


boolean MojoPlatform_uninstallDesktopMenuItem(const char *data)
{
#if PLATFORM_MACOSX || PLATFORM_BEOS
    // !!! FIXME: write me.
    STUBBED("desktop menu support");
    return false;
#else
    return xdgDesktopMenuItem("uninstall", data);
#endif
} // MojoPlatform_uninstallDesktopMenuItem


int MojoPlatform_exec(const char *cmd)
{
    execl(cmd, cmd, NULL);
    return errno;
} // MojoPlatform_exec


#if SUPPORT_MULTIARCH
void MojoPlatform_switchBin(const uint8 *img, size_t len)
{
    const char *dirs[] = { "/dev/shm", getenv("TMPDIR"), P_tmpdir, "/tmp" };
    const char *tmpl = "mojosetup-switch-bin-XXXXXX";
    char fname[PATH_MAX];
    int i = 0;

    for (i = 0; i < STATICARRAYLEN(dirs); i++)
    {
        if (testTmpDir(dirs[i], fname, len, tmpl))
        {
            const int fd = mkstemp(fname);
            if (fd != -1)
            {
                const size_t bw = write(fd, img, len);
                const int rc = close(fd);
                if ((bw == len) && (rc != -1))
                {
                    const char *tmpstr = GArgv[0];
                    chmod(fname, 0700);
                    GArgv[0] = fname;
                    execv(fname, (char * const *) GArgv);
                    // only hits this line if process wasn't replaced.
                    GArgv[0] = tmpstr;
                } // if
                unlink(fname);
            } // if
        } // if
    } // for

    // couldn't replace current process.
} // MojoPlatform_switchBin
#endif


boolean MojoPlatform_spawnTerminal(void)
{
#if PLATFORM_BEOS
    #error write me.
    // "/boot/apps/Terminal"

#elif PLATFORM_MACOSX
    // this is nasty...it'd be nice if Terminal.app just took command lines.
    boolean failed = false;
    FILE *io = NULL;
    char *cmd = NULL;
    char *ptr = NULL;
    char *binpath = MojoPlatform_appBinaryPath();
    size_t len = (strlen(binpath) * 5) + 3;
    int i = 0;
    for (i = 1; i < GArgc; i++)
        len += (strlen(GArgv[i]) * 5) + 3;

    ptr = cmd = (char *) xmalloc(len+1);
    for (i = 0; i < GArgc; i++)
    {
        const char *str = (i == 0) ? binpath : GArgv[i];
        if (i != 0)
            *(ptr++) = ' ';
        *(ptr++) = '\'';
        while (*str)
        {
            const char ch = *(str++);
            if (ch == '\'')
            {
                // have to escape for both AppleScript and /bin/sh.   :/
                *(ptr++) = '\'';
                *(ptr++) = '\\';
                *(ptr++) = '\\';
                *(ptr++) = '\'';
                *(ptr++) = '\'';
            } // if
            else
            {
                *(ptr++) = ch;
            } // else
        } // while
        *(ptr++) = '\'';
    } // for

    free(binpath);

    *ptr = '\0';
    ptr = format(
        "ignoring application responses\n"
        "tell application \"Terminal\" to do script \"clear ; echo %0 -notermspawn=1 ; exit\"\n"
        "tell application \"Terminal\" to tell its front window to set its custom title to \"MojoSetup\"\n"
        "tell application \"Terminal\" to tell its front window to set its title displays device name to false\n"
        "tell application \"Terminal\" to tell its front window to set its title displays shell path to false\n"
        "tell application \"Terminal\" to tell its front window to set its title displays window size to false\n"
        "tell application \"Terminal\" to tell its front window to set its title displays file name to false\n"
        "tell application \"Terminal\" to tell its front window to set its title displays custom title to true\n"
        "tell application \"Terminal\" to activate\n"
        "end ignoring\n", cmd);

    free(cmd);

    io = popen("osascript -", "w");
    if (io == NULL)
        failed = true;
    else
    {
        failed |= (fwrite(ptr, strlen(ptr), 1, io) != 1);
        failed |= (pclose(io) != 0);
    } // else

    free(ptr);

    if (!failed)
        exit(0);

    // we'll return false at the end to note we failed.

#else

    // urgh
    static const char *terms[] = {
        "gnome-terminal", "konsole", "kvt", "xterm", "rxvt",
        "dtterm", "eterm", "Eterm", "aterm"
    };

    char *binpath = MojoPlatform_appBinaryPath();
    const char *tryfirst = NULL;
    const int max_added_args = 5;
    const unsigned int argc = GArgc + max_added_args;
    const char **argv = NULL;
    int i = 0;
    int startarg = 0;

    if (getenv("DISPLAY") == NULL)
        return false;  // don't bother if we don't have X.

    else if (getenv("GNOME_DESKTOP_SESSION_ID") != NULL)  // this is gnome?
        tryfirst = "gnome-terminal";

    else if (getenv("KDE_FULL_SESSION") != NULL)  // this KDE >= 3.2?
        tryfirst = "konsole";

    argv = xmalloc((argc + 1) * sizeof(char *));

    for (i = -1; i < ((int) STATICARRAYLEN(terms)); i++)
    {
        int is_gnome_term = false;
        int argi = 0;
        const char *trythis = (i == -1) ? tryfirst : terms[i];
        if (trythis == NULL)
            continue;

        // !!! FIXME: hack. I'm sure other terminal emulators have needs, too.
        is_gnome_term = (strcmp(trythis, "gnome-terminal") == 0);

        argv[argi++] = trythis;
        argv[argi++] = is_gnome_term ? "--title" : "-title";
        argv[argi++] = "MojoSetup";
        argv[argi++] = is_gnome_term ? "-x" : "-e";
        argv[argi++] = binpath;
        argv[argi++] = "-notermspawn=1";
        assert(argi-1 <= max_added_args);

        for (startarg = argi-1; argi <= argc; argi++)  // include ending NULL.
        {
            argv[argi] = GArgv[argi - startarg];
            if (argv[argi] == NULL)
                break;
        } // for

        execvp(trythis, (char * const *) argv);
    } // for

    // Still here? We failed. Mankind is wiped out in the Robot Wars.

    free(argv);
    free(binpath);
#endif

    return false;
} // MojoPlatform_spawnTerminal


uint8 *MojoPlatform_decodeImage(const uint8 *data, uint32 size,
                                uint32 *w, uint32 *h)
{
    // !!! FIXME: try Quartz APIs on Mac OS X?
    return NULL; // no platform-specific APIs. Just use the built-in ones.
} // MojoPlatform_decodeImage


uint64 MojoPlatform_getuid(void)
{
    return (uint64) getuid();
} // MojoPlatform_getuid


uint64 MojoPlatform_geteuid(void)
{
    return (uint64) geteuid();
} // MojoPlatform_geteuid


uint64 MojoPlatform_getgid(void)
{
    return (uint64) getgid();
} // MojoPlatform_getgid


static void signal_catcher(int sig)
{
    static boolean first_shot = true;
    if (first_shot)
    {
        first_shot = false;
        logError("Caught signal #%0", numstr(sig));
    } // if
} // signal_catcher

static void crash_catcher(int sig)
{
    signal_catcher(sig);
    MojoSetup_crashed();
} // crash_catcher

static void termination_catcher(int sig)
{
    signal_catcher(sig);
    MojoSetup_terminated();
} // termination_catcher


static void install_signals(void)
{
    static int crash_sigs[] = { SIGSEGV,SIGILL,SIGBUS,SIGFPE,SIGTRAP,SIGABRT };
    static int term_sigs[] = { SIGQUIT, SIGINT, SIGTERM, SIGHUP };
    static int ignore_sigs[] = { SIGPIPE };
    int i;

    for (i = 0; i < STATICARRAYLEN(crash_sigs); i++)
        signal(crash_sigs[i], crash_catcher);
    for (i = 0; i < STATICARRAYLEN(term_sigs); i++)
        signal(term_sigs[i], termination_catcher);
    for (i = 0; i < STATICARRAYLEN(ignore_sigs); i++)
        signal(ignore_sigs[i], SIG_IGN);
} // install_signals


int main(int argc, char **argv)
{
    gettimeofday(&startup_time, NULL);
    openlog("mojosetup", LOG_PID, LOG_USER);
    atexit(closelog);
    install_signals();
    return MojoSetup_main(argc, argv);
} // main

#endif  // PLATFORM_UNIX

// end of unix.c ...

