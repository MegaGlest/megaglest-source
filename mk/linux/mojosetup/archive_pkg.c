/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Steffen Pankratz.
 */

#include "fileio.h"
#include "platform.h"

#if !SUPPORT_PKG
MojoArchive *MojoArchive_createPKG(MojoInput *io) { return NULL; }
#else

#define PKG_MAGIC 0x4f504b47

typedef struct
{
    uint32 magic;       // 4 bytes, has to be PKG_MAGIC (0x4f504b47)
    uint32 fileCount;   // 4 bytes, number of files in the archive
} PKGheader;

typedef struct
{
    uint64 nextFileStart;
} PKGinfo;

static boolean MojoInput_pkg_ready(MojoInput *io)
{
    return true;
} // MojoInput_pkg_ready

static int64 MojoInput_pkg_read(MojoInput *io, void *buf, uint32 bufsize)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    int64 pos = io->tell(io);
    if ((pos + bufsize) > entry->filesize)
        bufsize = (uint32) (entry->filesize - pos);
    return ar->io->read(ar->io, buf, bufsize);
} // MojoInput_pkg_read

static boolean MojoInput_pkg_seek(MojoInput *io, uint64 pos)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const PKGinfo *info = (PKGinfo *) ar->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    boolean retval = false;
    if (pos < ((uint64) entry->filesize))
    {
        const uint64 newpos = (info->nextFileStart - entry->filesize) + pos;
        retval = ar->io->seek(ar->io, newpos);
    } // if
    return retval;
} // MojoInput_pkg_seek

static int64 MojoInput_pkg_tell(MojoInput *io)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const PKGinfo *info = (PKGinfo *) ar->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    return ar->io->tell(ar->io) - (info->nextFileStart - entry->filesize);
} // MojoInput_pkg_tell

static int64 MojoInput_pkg_length(MojoInput *io)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    return entry->filesize;
} // MojoInput_pkg_length

static MojoInput *MojoInput_pkg_duplicate(MojoInput *io)
{
    MojoInput *retval = NULL;
    fatal(_("BUG: Can't duplicate pkg inputs"));  // !!! FIXME: why not?
    return retval;
} // MojoInput_pkg_duplicate

static void MojoInput_pkg_close(MojoInput *io)
{
    free(io);
} // MojoInput_pkg_close

// MojoArchive implementation...

static boolean MojoArchive_pkg_enumerate(MojoArchive *ar)
{
    PKGinfo *info = (PKGinfo *) ar->opaque;
    MojoArchive_resetEntry(&ar->prevEnum);

    info->nextFileStart = sizeof (PKGheader);
    return true;
} // MojoArchive_pkg_enumerate


static const MojoArchiveEntry *MojoArchive_pkg_enumNext(MojoArchive *ar)
{
    PKGinfo *info = (PKGinfo *) ar->opaque;
    MojoInput *io = ar->io;
    int64 ret = 0;
    uint32 pathNameLength = 0;
    uint64 pathNameStart = 0;
    uint32 fileNameLength = 0;
    uint64 fileNameStart = 0;
    uint32 fileSize = 0;
    char* backSlash = NULL;

    if (!ar->io->seek(ar->io, info->nextFileStart))
        return NULL;

    // read the path name length
    if (!MojoInput_readui32(io, &pathNameLength))
            return NULL;

    pathNameStart = ar->io->tell(ar->io);

    // skip reading the path name for now
    if (!ar->io->seek(ar->io, pathNameStart + pathNameLength))
        return NULL;

    // read the file name length
    if (!MojoInput_readui32(io, &fileNameLength))
            return NULL;

    fileNameStart = ar->io->tell(ar->io);

    // as both strings are null terminated, we need one byte less
    ar->prevEnum.filename = (char *) xmalloc(pathNameLength + fileNameLength -1);

    // go to the start of the path name
    if (!ar->io->seek(ar->io, pathNameStart))
        return NULL;

    // read the path name
    ret = io->read(io, ar->prevEnum.filename, pathNameLength);
    if (ret != pathNameLength)
        return false;

    // replace backslashes with slashes in the path name
    while((backSlash = strchr(ar->prevEnum.filename, '\\')))
        *backSlash = '/';

    // go the start of the file name
    if (!ar->io->seek(ar->io, fileNameStart))
        return NULL;

    // read the file name
    ret = io->read(io, ar->prevEnum.filename + pathNameLength - 1, fileNameLength);
    if (ret != fileNameLength)
        return false;

    // read the file size
    if (!MojoInput_readui32(io, &fileSize))
        return NULL;

    // skip the next 8 bytes, probably some kind of check sum
    if (!ar->io->seek(ar->io, ar->io->tell(ar->io) + 8))
        return NULL;

    ar->prevEnum.filesize = fileSize;
    ar->prevEnum.perms = MojoPlatform_defaultFilePerms();
    ar->prevEnum.type = MOJOARCHIVE_ENTRY_FILE;

    info->nextFileStart = ar->io->tell(ar->io) + ar->prevEnum.filesize;

    return &ar->prevEnum;
} // MojoArchive_pkg_enumNext


static MojoInput *MojoArchive_pkg_openCurrentEntry(MojoArchive *ar)
{
    MojoInput *io = NULL;
    io = (MojoInput *) xmalloc(sizeof (MojoInput));
    io->ready = MojoInput_pkg_ready;
    io->read = MojoInput_pkg_read;
    io->seek = MojoInput_pkg_seek;
    io->tell = MojoInput_pkg_tell;
    io->length = MojoInput_pkg_length;
    io->duplicate = MojoInput_pkg_duplicate;
    io->close = MojoInput_pkg_close;
    io->opaque = ar;
    return io;
} // MojoArchive_pkg_openCurrentEntry


static void MojoArchive_pkg_close(MojoArchive *ar)
{
    PKGinfo *info = (PKGinfo *) ar->opaque;
    ar->io->close(ar->io);

    free(info);
    free(ar);
} // MojoArchive_pkg_close


MojoArchive *MojoArchive_createPKG(MojoInput *io)
{
    MojoArchive *ar = NULL;
    PKGinfo *pkgInfo = NULL;
    PKGheader pkgHeader;

    if (!MojoInput_readui32(io, &pkgHeader.magic))
        return NULL;
    else if (!MojoInput_readui32(io, &pkgHeader.fileCount))
        return NULL;

    // Check if this is a *.pkg file.
    if (pkgHeader.magic != PKG_MAGIC)
        return NULL;

    pkgInfo = (PKGinfo *) xmalloc(sizeof (PKGinfo));

    ar = (MojoArchive *) xmalloc(sizeof (MojoArchive));
    ar->opaque = pkgInfo;
    ar->enumerate = MojoArchive_pkg_enumerate;
    ar->enumNext = MojoArchive_pkg_enumNext;
    ar->openCurrentEntry = MojoArchive_pkg_openCurrentEntry;
    ar->close = MojoArchive_pkg_close;
    ar->io = io;

    return ar;
} // MojoArchive_createPKG

#endif // SUPPORT_PKG

// end of archive_pkg.c ...
