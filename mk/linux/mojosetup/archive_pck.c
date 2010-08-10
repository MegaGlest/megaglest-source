/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Steffen Pankratz.
 */

#include "fileio.h"
#include "platform.h"

#if !SUPPORT_PCK
MojoArchive *MojoArchive_createPCK(MojoInput *io) { return NULL; }
#else

#define PCK_MAGIC 0x534c4850

typedef struct
{
    uint32 Magic;               // 4 bytes, has to be PCK_MAGIC (0x534c4850)
    uint32 StartOfBinaryData;   // 4 bytes, offset to the data
} PCKheader;

typedef struct
{
    int8 filename[60];          // 60 bytes, null terminated
    uint32 filesize;            //  4 bytes
} PCKentry;

typedef struct
{
    uint64 fileCount;
    uint64 dataStart;
    uint64 nextFileStart;
    int64 nextEnumPos;
    MojoArchiveEntry *archiveEntries;
} PCKinfo;

static boolean MojoInput_pck_ready(MojoInput *io)
{
    return true;  // !!! FIXME?
} // MojoInput_pck_ready

static int64 MojoInput_pck_read(MojoInput *io, void *buf, uint32 bufsize)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    int64 pos = io->tell(io);
    if ((pos + bufsize) > entry->filesize)
        bufsize = (uint32) (entry->filesize - pos);
    return ar->io->read(ar->io, buf, bufsize);
} // MojoInput_pck_read

static boolean MojoInput_pck_seek(MojoInput *io, uint64 pos)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const PCKinfo *info = (PCKinfo *) ar->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    boolean retval = false;
    if (pos < ((uint64) entry->filesize))
    {
        const uint64 newpos = (info->nextFileStart - entry->filesize) + pos;
        retval = ar->io->seek(ar->io, newpos);
    } // if
    return retval;
} // MojoInput_pck_seek

static int64 MojoInput_pck_tell(MojoInput *io)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const PCKinfo *info = (PCKinfo *) ar->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    return ar->io->tell(ar->io) - (info->nextFileStart - entry->filesize);
} // MojoInput_pck_tell

static int64 MojoInput_pck_length(MojoInput *io)
{
    MojoArchive *ar = (MojoArchive *) io->opaque;
    const MojoArchiveEntry *entry = &ar->prevEnum;
    return entry->filesize;
} // MojoInput_pck_length

static MojoInput *MojoInput_pck_duplicate(MojoInput *io)
{
    MojoInput *retval = NULL;
    fatal(_("BUG: Can't duplicate pck inputs"));  // !!! FIXME: why not?
    return retval;
} // MojoInput_pck_duplicate

static void MojoInput_pck_close(MojoInput *io)
{
    free(io);
} // MojoInput_pck_close

// MojoArchive implementation...

static boolean MojoArchive_pck_enumerate(MojoArchive *ar)
{
    MojoArchiveEntry *archiveEntries = NULL;
    PCKinfo *info = (PCKinfo *) ar->opaque;
    const int dataStart = info->dataStart;
    const int fileCount = dataStart / sizeof (PCKentry);
    const size_t len = fileCount * sizeof (MojoArchiveEntry);
    PCKentry fileEntry;
    uint64 i, realFileCount = 0;
    char directory[256] = {'\0'};
    MojoInput *io = ar->io;

    MojoArchive_resetEntry(&ar->prevEnum);

    archiveEntries = (MojoArchiveEntry *) xmalloc(len);

    for (i = 0; i < fileCount; i++)
    {
        int dotdot;
        int64 br;

        br = io->read(io, fileEntry.filename, sizeof (fileEntry.filename));
        if (br != sizeof (fileEntry.filename))
            return false;
        else if (!MojoInput_readui32(io, &fileEntry.filesize))
            return false;

        dotdot = (strcmp(fileEntry.filename, "..") == 0);

        if ((!dotdot) && (fileEntry.filesize == 0x80000000))
        {
            MojoArchiveEntry *entry = &archiveEntries[realFileCount];

            strcat(directory, fileEntry.filename);
            strcat(directory, "/");

            entry->filename = xstrdup(directory);
            entry->type = MOJOARCHIVE_ENTRY_DIR;
            entry->perms = MojoPlatform_defaultDirPerms();
            entry->filesize = 0;
            realFileCount++;
        } // if

        else if ((dotdot) && (fileEntry.filesize == 0x80000000))
        {
            // remove trailing path separator
            char *pathSep;
            const size_t strLength = strlen(directory);
            directory[strLength - 1] = '\0';

            pathSep = strrchr(directory, '/');
            if(pathSep != NULL)
            {
                pathSep++;
                *pathSep = '\0';
            } // if
        } // else if

        else
        {
            MojoArchiveEntry *entry = &archiveEntries[realFileCount];
            if (directory[0] == '\0')
                entry->filename = xstrdup(fileEntry.filename);
            else
            {
                const size_t len = sizeof (char) * strlen(directory) +
                                   strlen(fileEntry.filename) + 1;
                entry->filename = (char *) xmalloc(len);
                strcat(entry->filename, directory);
                strcat(entry->filename, fileEntry.filename);
            } // else

            entry->perms = MojoPlatform_defaultFilePerms();
            entry->type = MOJOARCHIVE_ENTRY_FILE;
            entry->filesize = fileEntry.filesize;

            realFileCount++;
        } // else
    } // for

    info->fileCount = realFileCount;
    info->archiveEntries = archiveEntries;
    info->nextEnumPos = 0;
    info->nextFileStart = dataStart;

    return true;
} // MojoArchive_pck_enumerate


static const MojoArchiveEntry *MojoArchive_pck_enumNext(MojoArchive *ar)
{
    PCKinfo *info = (PCKinfo *) ar->opaque;
    const MojoArchiveEntry *entry = &info->archiveEntries[info->nextEnumPos];

    if (info->nextEnumPos >= info->fileCount)
        return NULL;

    if (!ar->io->seek(ar->io, info->nextFileStart))
        return NULL;

    info->nextEnumPos++;
    info->nextFileStart += entry->filesize;

    memcpy(&ar->prevEnum, entry, sizeof (ar->prevEnum));

    return &ar->prevEnum;
} // MojoArchive_pck_enumNext


static MojoInput *MojoArchive_pck_openCurrentEntry(MojoArchive *ar)
{
    MojoInput *io = NULL;
    io = (MojoInput *) xmalloc(sizeof (MojoInput));
    io->ready = MojoInput_pck_ready;
    io->read = MojoInput_pck_read;
    io->seek = MojoInput_pck_seek;
    io->tell = MojoInput_pck_tell;
    io->length = MojoInput_pck_length;
    io->duplicate = MojoInput_pck_duplicate;
    io->close = MojoInput_pck_close;
    io->opaque = ar;
    return io;
} // MojoArchive_pck_openCurrentEntry


static void MojoArchive_pck_close(MojoArchive *ar)
{
    int i;
    PCKinfo *info = (PCKinfo *) ar->opaque;
    ar->io->close(ar->io);

    for (i = 0; i < info->fileCount; i++)
    {
        MojoArchiveEntry *entry = &info->archiveEntries[i];
        free(entry->filename);
    } // for

    free(info->archiveEntries);
    free(info);
    free(ar);
} // MojoArchive_pck_close


MojoArchive *MojoArchive_createPCK(MojoInput *io)
{
    MojoArchive *ar = NULL;
    PCKinfo *pckInfo = NULL;
    PCKheader pckHeader;

    if (!MojoInput_readui32(io, &pckHeader.Magic))
        return NULL;
    else if (!MojoInput_readui32(io, &pckHeader.StartOfBinaryData))
        return NULL;

    // Check if this is a *.pck file.
    if (pckHeader.Magic != PCK_MAGIC)
        return NULL;

    pckInfo = (PCKinfo *) xmalloc(sizeof (PCKinfo));
    pckInfo->dataStart = pckHeader.StartOfBinaryData + sizeof (PCKheader);

    ar = (MojoArchive *) xmalloc(sizeof (MojoArchive));
    ar->opaque = pckInfo;
    ar->enumerate = MojoArchive_pck_enumerate;
    ar->enumNext = MojoArchive_pck_enumNext;
    ar->openCurrentEntry = MojoArchive_pck_openCurrentEntry;
    ar->close = MojoArchive_pck_close;
    ar->io = io;

    return ar;
} // MojoArchive_createPCK

#endif // SUPPORT_PCK

// end of archive_pck.c ...

