/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef _INCL_PLATFORM_H_
#define _INCL_PLATFORM_H_

#include "universal.h"

#ifdef __cplusplus
extern "C" {
#endif

// this is called by your mainline.
int MojoSetup_main(int argc, char **argv);

// Caller must free returned string!
char *MojoPlatform_appBinaryPath(void);

// Caller must free returned string!
char *MojoPlatform_currentWorkingDir(void);

// Caller must free returned string!
char *MojoPlatform_homedir(void);

uint32 MojoPlatform_ticks(void);

// Make current process kill itself immediately, without any sort of internal
//  cleanup, like atexit() handlers or static destructors...the OS will have
//  to sort out the freeing of any resources, and no more code in this
//  process than necessary should run. This function does not return. Try to
//  avoid calling this.
void MojoPlatform_die(void);

// Delete a file from the physical filesystem. This should remove empty
//  directories as well as files. Returns true on success, false on failure.
boolean MojoPlatform_unlink(const char *fname);

// Resolve symlinks, relative paths, etc. Caller free()'s buffer. Returns
//  NULL if path couldn't be resolved.
char *MojoPlatform_realpath(const char *path);

// Create a symlink in the physical filesystem. (src) is the NAME OF THE LINK
//  and (dst) is WHAT IT POINTS TO. This is backwards from the unix symlink()
//  syscall! Returns true if link was created, false otherwise.
boolean MojoPlatform_symlink(const char *src, const char *dst);

// Read the destination from symlink (linkname). Returns a pointer
//  allocated with xmalloc() containing the linkdest on success, and NULL
//  on failure. The caller is responsible for freeing the returned pointer!
char *MojoPlatform_readlink(const char *linkname);

// !!! FIXME: we really can't do this in a 16-bit value...non-Unix platforms
// !!! FIXME:  and Extended Attributes need more.
// Create a directory in the physical filesystem, with (perms) permissions.
//  returns true if directory is created, false otherwise.
boolean MojoPlatform_mkdir(const char *path, uint16 perms);

// Move a file to a new name. This has to be a fast (if not atomic) operation,
//  so if it would require a legitimate copy to another filesystem or device,
//  this should fail, as the standard Unix rename() function does.
// Returns true on successful rename, false otherwise.
boolean MojoPlatform_rename(const char *src, const char *dst);

// Determine if dir/fname exists in the native filesystem. It doesn't matter
//  if it's a directory, file, symlink, etc, we're just looking for the
//  existance of the entry itself. (fname) may be NULL, in which case,
//  (dir) contains the whole path, otherwise, the platform layer needs to
//  build the path: (on Unix: dir/path, on Windows: dir\\path, etc).
//  This is a convenience thing for the caller.
// Returns true if path in question exists, false otherwise.
boolean MojoPlatform_exists(const char *dir, const char *fname);

// Returns true if (fname) in the native filesystem is writable. If (fname)
//  is a directory, this means that the contents of the directory can be
//  added to (create files, delete files, etc). If (fname) is a file, this
//  means that this process has write access to the file.
// Returns false if (fname) isn't writable.
boolean MojoPlatform_writable(const char *fname);

// Returns true if (dir) is a directory in the physical filesystem, false
//  otherwise (including if (dir) doesn't exist). Don't follow symlinks.
boolean MojoPlatform_isdir(const char *dir);

// Returns true if (fname) is a symlink in the physical filesystem, false
//  otherwise (including if (fname) doesn't exist). Don't follow symlinks.
boolean MojoPlatform_issymlink(const char *fname);

// Returns true if stdin and stdout are connected to a tty.
boolean MojoPlatform_istty(void);

// Returns true if (fname) is a regular file in the physical filesystem, false
//  otherwise (including if (fname) doesn't exist). Don't follow symlinks.
boolean MojoPlatform_isfile(const char *fname);

// Returns size, in bytes, of the file (fname) in the physical filesystem.
//  Return -1 if file is missing or not a file. Don't follow symlinks.
int64 MojoPlatform_filesize(const char *fname);

// !!! FIXME: we really can't do this in a 16-bit value...non-Unix platforms
// !!! FIXME:  and Extended Attributes need more.
// !!! FIXME: comment me.
boolean MojoPlatform_perms(const char *fname, uint16 *p);

// !!! FIXME: we really can't do this in a 16-bit value...non-Unix platforms
// !!! FIXME:  and Extended Attributes need more.
// !!! FIXME: comment me.
boolean MojoPlatform_chmod(const char *fname, uint16 p);

// Try to locate a specific piece of media (usually an inserted CD or DVD).
//  (uniquefile) is a path that should exist on the media; its presence
//  should uniquely identify the media.
// Returns the path of the media's mount point in the physical filesystem
//  (something like "E:\\" on Windows or "/mnt/cdrom" on Unix), or NULL if
//  the media isn't found (needed disc isn't inserted, etc).
// Caller must free return value!
char *MojoPlatform_findMedia(const char *uniquefile);

// Flag values for MojoPlatform_fopen().
typedef enum
{
    MOJOFILE_READ=(1<<0),
    MOJOFILE_WRITE=(1<<1),
    MOJOFILE_CREATE=(1<<2),
    MOJOFILE_EXCLUSIVE=(1<<3),
    MOJOFILE_TRUNCATE=(1<<4),
    MOJOFILE_APPEND=(1<<5),
} MojoFileFlags;

typedef enum
{
    MOJOSEEK_SET,
    MOJOSEEK_CURRENT,
    MOJOSEEK_END
} MojoFileSeek;

// !!! FIXME: we really can't do this in a 16-bit value...non-Unix platforms
// !!! FIXME:  and Extended Attributes need more.
// Open file (fname). This wraps a subset of the Unix open() syscall. Use
//  MojoFileFlags for (flags). If creating a file, use (mode) for the new
//  file's permissions.
// Returns an opaque handle on success, NULL on error. Caller must free
//  handle with MojoPlatform_close() when done with it.
void *MojoPlatform_open(const char *fname, uint32 flags, uint16 mode);

// Return a handle that's compatible with MojoPlatform_open()'s return values
//  that represents stdout. May return NULL for platforms that don't support
//  this concept. You need to make sure that stdout itself doesn't really
//  close in MojoPlatform_close(), at least for now.
void *MojoPlatform_stdout(void);

// Read (bytes) bytes from (fd) into (buf). This wraps the Unix read() syscall.
//  Returns number of bytes read, -1 on error.
int64 MojoPlatform_read(void *fd, void *buf, uint32 bytes);

// Write (bytes) bytes from (buf) into (fd). This wraps the Unix write()
//  syscall. Returns number of bytes read, -1 on error.
int64 MojoPlatform_write(void *fd, const void *buf, uint32 bytes);

// Reports byte offset of file pointer in (fd), or -1 on error.
int64 MojoPlatform_tell(void *fd);

// Seek to (offset) byte offset of file pointer in (fd), relative to (whence).
//  This wraps the Unix lseek() syscall. Returns byte offset from the start
//  of the file, -1 on error.
int64 MojoPlatform_seek(void *fd, int64 offset, MojoFileSeek whence);

// Get the size, in bytes, of a file, referenced by its opaque handle.
//  (This pulls the data through an fstat() on Unix.) Retuns -1 on error.
int64 MojoPlatform_flen(void *fd);

// Force any pending data to disk, returns true on success, false if there
//  was an i/o error.
boolean MojoPlatform_flush(void *fd);

// Free any resources associated with (fd), flushing any pending data to disk,
//  and closing the file. (fd) becomes invalid after this call returns
//  successfully. This wraps the Unix close() syscall. Returns true on
//  success, false on i/o error.
boolean MojoPlatform_close(void *fd);

// Enumerate a directory. Returns an opaque pointer that can be used with
//  repeated calls to MojoPlatform_readdir() to enumerate the names of
//  directory entries. Returns NULL on error. Non-NULL values should be passed
//  to MojoPlatform_closedir() for cleanup when you are done with them.
void *MojoPlatform_opendir(const char *dirname);

// Get the next entry in the directory. (dirhandle) is an opaque pointer
//  returned by MojoPlatform_opendir(). Returns NULL if we're at the end of
//  the directory, or a null-terminated UTF-8 string otherwise. The order of
//  results are not guaranteed, and may change between two iterations.
// Caller must free returned string!
char *MojoPlatform_readdir(void *dirhandle);

// Clean up resources used by a directory enumeration. (dirhandle) is an
//  opaque pointer returned by MojoPlatform_opendir(), and becomes invalid
//  after this call.
void MojoPlatform_closedir(void *dirhandle);

// Convert a string into a permissions bitmask. On Unix, this is currently
//  expected to be an octal string like "0755", but may expect other forms
//  in the future, and other platforms may need to interpret permissions
//  differently. (str) may be NULL for defaults, and is considered valid.
// If (str) is not valid, return a reasonable default and set (*valid) to
//  false. Otherwise, set (*valid) to true and return the converted value.
uint16 MojoPlatform_makePermissions(const char *str, boolean *valid);

// Return a default, sane set of permissions for a newly-created file.
uint16 MojoPlatform_defaultFilePerms(void);

// Return a default, sane set of permissions for a newly-created directory.
uint16 MojoPlatform_defaultDirPerms(void);

// Wrappers for Unix dlopen/dlsym/dlclose, sort of. Instead of a filename,
//  these take a memory buffer for the library. If you can't load this
//  directly in RAM, the platform should write it to a temporary file first,
//  and deal with cleanup in MojoPlatform_dlclose(). The memory buffer must be
//  dereferenced in MojoPlatform_dlopen(), as the caller may free() it upon
//  return. Everything else works like the usual Unix calls.
void *MojoPlatform_dlopen(const uint8 *img, size_t len);
void *MojoPlatform_dlsym(void *lib, const char *sym);
void MojoPlatform_dlclose(void *lib);

// Launch the user's preferred browser to view the URL (url).
//  Returns true if the browser launched, false otherwise. We can't know
//  if the URL actually loaded, just if the browser launched. The hope is that
//  the browser will inform the user if there's a problem loading the URL.
boolean MojoPlatform_launchBrowser(const char *url);

// Add a menu item to the Application menu or Start bar or whatever.
//  (data) is 100% platform dependent right now, and this interface will
//  likely change as we come to understand various systems' needs better.
//  On Unix, it expects this to be a path to a FreeDesktop .desktop file.
// Returns (true) on success and (false) on failure.
boolean MojoPlatform_installDesktopMenuItem(const char *data);

// Remove a menu item from the Application menu or Start bar or whatever.
//  (data) is 100% platform dependent right now, and this interface will
//  likely change as we come to understand various systems' needs better.
//  On Unix, it expects this to be a path to a FreeDesktop .desktop file.
// Returns (true) on success and (false) on failure.
boolean MojoPlatform_uninstallDesktopMenuItem(const char *data);

// Run a script from the archive in the OS
int MojoPlatform_runScript(const char *script, boolean devnull, const char **argv);

// Exec a given process name
int MojoPlatform_exec(const char *cmd);

#if !SUPPORT_MULTIARCH
#define MojoPlatform_switchBin(img, len)
#else
void MojoPlatform_switchBin(const uint8 *img, size_t len);
#endif

// Try to spawn a terminal, and possibly relaunch MojoSetup within it.
//  If we can attach to a terminal without relaunching, do so and
//  return true. false for failure to attach/spawn.
//  May not return on success (process replaces itself).
boolean MojoPlatform_spawnTerminal(void);

// Put the calling process to sleep for at least (ticks) milliseconds.
//  This is meant to yield the CPU while spinning in a loop that is polling
//  for input, etc. Pumping the GUI event queue happens elsewhere, not here.
void MojoPlatform_sleep(uint32 ticks);

// Put a line of text to the the system log, whatever that might be on a
//  given platform. (str) is a complete line, but won't end with any newline
//  characters. You should supply if needed.
void MojoPlatform_log(const char *str);

// This tries to decode a graphic file in memory into an RGBA framebuffer.
//  Most platforms return NULL here. No one should call this; use decodeImage()
//  instead, which will try included platform-independent code if this fails.
// This function is just here to allow a platform with the appropriate
//  functionality to work without compiling in stb_image.c, or supply more
//  formats over the built-in code.
// (data) points to the compressed data, (size) is the number of bytes
//  of compressed data. (*w) and (*h) will contain the images dimensions on
//  return.
// Returns NULL on failure (unsupported, etc) and a pointer to the
//  uncompressed data on success. Caller must free() the returned pointer!
uint8 *MojoPlatform_decodeImage(const uint8 *data, uint32 size,
                                uint32 *w, uint32 *h);

// Get the current locale, in the format "xx_YY" where "xx" is the language
//  (en, fr, de...) and "_YY" is the country. (_US, _CA, etc). The country
//  can be omitted. Don't include encoding, it's always UTF-8 at this time.
// Returns locale string, or NULL if it couldn't be determined.
//  Caller must free() the returned pointer!
char *MojoPlatform_locale(void);

// !!! FIXME: document me.
// Caller must free() the returned pointer!
char *MojoPlatform_osType(void);

// !!! FIXME: document me.
// Caller must free() the returned pointer!
char *MojoPlatform_osMachine(void);

// !!! FIXME: document me.
// Caller must free() the returned pointer!
char *MojoPlatform_osVersion(void);

// !!! FIXME: document me.
uint64 MojoPlatform_getuid(void);

// !!! FIXME: document me.
uint64 MojoPlatform_geteuid(void);

// !!! FIXME: document me.
uint64 MojoPlatform_getgid(void);


// Basic platform detection.
#if PLATFORM_WINDOWS
#define PLATFORM_NAME "windows"
#elif PLATFORM_MACOSX
#define PLATFORM_NAME "macosx"
#elif PLATFORM_UNIX
#define PLATFORM_NAME "unix"
#elif PLATFORM_BEOS
#define PLATFORM_NAME "beos"
#else
#error Unknown platform.
#endif

// Basic architecture detection.

#if defined(__powerpc64__)
#define PLATFORM_ARCH "powerpc64"
#elif defined(__ppc__) || defined(__powerpc__) || defined(__POWERPC__)
#define PLATFORM_ARCH "powerpc"
#elif defined(__x86_64__) || defined(_M_X64)
#define PLATFORM_ARCH "x86-64"
#elif defined(__X86__) || defined(__i386__) || defined(i386) || defined (_M_IX86) || defined(__386__)
#define PLATFORM_ARCH "x86"
#else
#error Unknown processor architecture.
#endif

// Other basic truths...
#if PLATFORM_WINDOWS
#define MOJOPLATFORM_ENDLINE "\r\n"
#else
#define MOJOPLATFORM_ENDLINE "\n"
#endif

#ifdef __cplusplus
}
#endif

#endif

// end of platform.h ...

