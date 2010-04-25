/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include "fileio.h"
#include "platform.h"

#if !SUPPORT_UZ2
MojoArchive *MojoArchive_createUZ2(MojoInput *io) { return NULL; }
#else

// UZ2 format is a simple compressed file format used by UnrealEngine2.
//  it's just a stream of blocks like this:
//    uint32 compressed size
//    uint32 uncompressed size
//    uint8 data[compressed size]  <-- unpacks to (uncompressed size) bytes.
// Decompression is handled by zlib's "uncompress" function.

#include "zlib/zlib.h"

#define MAXCOMPSIZE 32768
#define MAXUNCOMPSIZE 33096  // MAXCOMPSIZE + 1%


// MojoInput implementation...

// Decompression is handled in the parent MojoInput, so this just needs to
//  make sure we stay within the bounds of the tarfile entry.

typedef struct UZ2input
{
    MojoInput *io;
    int64 fsize;
    uint64 position;
    uint32 compsize;
    uint8 compbuf[MAXCOMPSIZE];
    uint32 uncompsize;
    uint8 uncompbuf[MAXUNCOMPSIZE];
    uint32 uncompindex;
} UZ2input;

typedef struct UZ2info
{
    char *outname;
    int64 outsize;
    boolean enumerated;
} UZ2info;


static boolean readui32(MojoInput *io, uint32 *ui32)
{
    uint8 buf[sizeof (uint32)];
    if (io->read(io, buf, sizeof (buf)) != sizeof (buf))
        return false;

    *ui32 = (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
    return true;
} // readui32

static boolean unpack(UZ2input *inp)
{
    MojoInput *io = inp->io;
    uLongf ul = (uLongf) inp->uncompsize;

    // we checked these formally elsewhere.
    assert(inp->compsize > 0);
    assert(inp->uncompsize > 0);
    assert(inp->compsize <= MAXCOMPSIZE);
    assert(inp->uncompsize <= MAXUNCOMPSIZE);

    if (io->read(io, inp->compbuf, inp->compsize) != inp->compsize)
        return false;
    if (uncompress(inp->uncompbuf, &ul, inp->compbuf, inp->compsize) != Z_OK)
        return false;
    if (ul != ((uLongf) inp->uncompsize))  // corrupt data.
        return false;

    inp->uncompindex = 0;
    return true;
} // unpack

static boolean MojoInput_uz2_ready(MojoInput *io)
{
    UZ2input *input = (UZ2input *) io->opaque;
    if (input->uncompsize > 0)
        return true;
    return true;  // !!! FIXME: need to know we have a full compressed block.
} // MojoInput_uz2_ready

static int64 MojoInput_uz2_read(MojoInput *io, void *_buf, uint32 bufsize)
{
    uint8 *buf = (uint8 *) _buf;
    UZ2input *input = (UZ2input *) io->opaque;
    int64 retval = 0;
    while (bufsize > 0)
    {
        const uint32 available = input->uncompsize - input->uncompindex;
        const uint32 cpy = (available < bufsize) ? available : bufsize;
        if (available == 0)
        {
            if (input->position == input->fsize)
                return 0;
            else if (!readui32(input->io, &input->compsize))
                return (retval == 0) ? -1 : retval;
            else if (!readui32(input->io, &input->uncompsize))
                return (retval == 0) ? -1 : retval;
            else if (!unpack(input))
                return (retval == 0) ? -1 : retval;
            continue;  // try again.
        } // if

        memcpy(buf, input->uncompbuf + input->uncompindex, cpy);
        buf += cpy;
        bufsize -= cpy;
        retval += cpy;
        input->uncompindex += cpy;
        input->position += cpy;
    } // while

    return retval;
} // MojoInput_uz2_read

static boolean MojoInput_uz2_seek(MojoInput *io, uint64 pos)
{
    UZ2input *input = (UZ2input *) io->opaque;
    int64 seekpos = 0;

    // in a perfect world, this wouldn't seek from the start if moving
    //  forward. But oh well.
    input->position = 0;
    while (input->position < pos)
    {
        if (!input->io->seek(input->io, seekpos))
            return false;
        else if (!readui32(io, &input->compsize))
            return false;
        else if (!readui32(io, &input->uncompsize))
            return false;

        // we checked these formally elsewhere.
        assert(input->compsize > 0);
        assert(input->uncompsize > 0);
        assert(input->compsize <= MAXCOMPSIZE);
        assert(input->uncompsize <= MAXUNCOMPSIZE);

        input->position += input->uncompsize;
        seekpos += (sizeof (uint32) * 2) + input->compsize;
    } // while

    // we are positioned on the compressed block that contains the seek target.
    if (!unpack(input))
        return false;

    input->position -= input->uncompsize;
    input->uncompindex = (uint32) (pos - input->position);
    input->position += input->uncompindex;

    return true;
} // MojoInput_uz2_seek

static int64 MojoInput_uz2_tell(MojoInput *io)
{
    return (int64) (((UZ2input *) io->opaque)->position);
} // MojoInput_uz2_tell

static int64 MojoInput_uz2_length(MojoInput *io)
{
    return ((UZ2input *) io->opaque)->fsize;
} // MojoInput_uz2_length

static MojoInput *MojoInput_uz2_duplicate(MojoInput *io)
{
    MojoInput *retval = NULL;
    UZ2input *input = (UZ2input *) io->opaque;
    MojoInput *newio = input->io->duplicate(input->io);

    if (newio != NULL)
    {
        UZ2input *newopaque = (UZ2input *) xmalloc(sizeof (UZ2input));
        newopaque->io = newio;
        newopaque->fsize = input->fsize;
        // everything else is properly zero'd by xmalloc().
        retval = (MojoInput *) xmalloc(sizeof (MojoInput));
        memcpy(retval, io, sizeof (MojoInput));
        retval->opaque = newopaque;
    } // if

    return retval;
} // MojoInput_uz2_duplicate

static void MojoInput_uz2_close(MojoInput *io)
{
    UZ2input *input = (UZ2input *) io->opaque;
    input->io->close(input->io);
    free(input);
    free(io);
} // MojoInput_uz2_close


// MojoArchive implementation...

static boolean MojoArchive_uz2_enumerate(MojoArchive *ar)
{
    UZ2info *info = (UZ2info *) ar->opaque;
    MojoArchive_resetEntry(&ar->prevEnum);
    info->enumerated = false;
    return true;
} // MojoArchive_uz2_enumerate


static const MojoArchiveEntry *MojoArchive_uz2_enumNext(MojoArchive *ar)
{
    UZ2info *info = (UZ2info *) ar->opaque;

    MojoArchive_resetEntry(&ar->prevEnum);
    if (info->enumerated)
        return NULL;  // only one file in this "archive".

    ar->prevEnum.perms = MojoPlatform_defaultFilePerms();
    ar->prevEnum.filesize = info->outsize;
    ar->prevEnum.filename = xstrdup(info->outname);
    ar->prevEnum.type = MOJOARCHIVE_ENTRY_FILE;

    info->enumerated = true;
    return &ar->prevEnum;
} // MojoArchive_uz2_enumNext


static MojoInput *MojoArchive_uz2_openCurrentEntry(MojoArchive *ar)
{
    UZ2info *info = (UZ2info *) ar->opaque;
    MojoInput *io = NULL;
    UZ2input *opaque = NULL;
    MojoInput *dupio = NULL;

    if (!info->enumerated)
        return NULL;

    dupio = ar->io->duplicate(ar->io);
    if (dupio == NULL)
        return NULL;

    opaque = (UZ2input *) xmalloc(sizeof (UZ2input));
    opaque->io = dupio;
    opaque->fsize = info->outsize;
    // rest is zero'd by xmalloc().

    io = (MojoInput *) xmalloc(sizeof (MojoInput));
    io->ready = MojoInput_uz2_ready;
    io->read = MojoInput_uz2_read;
    io->seek = MojoInput_uz2_seek;
    io->tell = MojoInput_uz2_tell;
    io->length = MojoInput_uz2_length;
    io->duplicate = MojoInput_uz2_duplicate;
    io->close = MojoInput_uz2_close;
    io->opaque = opaque;

    return io;
} // MojoArchive_uz2_openCurrentEntry


static void MojoArchive_uz2_close(MojoArchive *ar)
{
    UZ2info *info = (UZ2info *) ar->opaque;
    MojoArchive_resetEntry(&ar->prevEnum);
    ar->io->close(ar->io);
    free(info->outname);
    free(info);
    free(ar);
} // MojoArchive_uz2_close


// Unfortunately, we have to walk the whole file, but we don't have to actually
//  do any decompression work here. Just seek, read 8 bytes, repeat until EOF.
static int64 calculate_uz2_outsize(MojoInput *io)
{
    int64 retval = 0;
    uint32 compsize = 0;
    uint32 uncompsize = 0;
    int64 pos = 0;

    if (!io->seek(io, 0))
        return -1;

    while (readui32(io, &compsize))
    {
        if (!readui32(io, &uncompsize))
            return -1;
        else if ((compsize > MAXCOMPSIZE) || (uncompsize > MAXUNCOMPSIZE))
            return -1;
        else if ((compsize == 0) || (uncompsize == 0))
            return -1;
        retval += uncompsize;
        pos += (sizeof (uint32) * 2) + compsize;
        if (!io->seek(io, pos))
            return -1;
    } // while

    if (!io->seek(io, 0))  // make sure we're back to the start.
        return -1;

    return retval;
} // calculate_uz2_outsize


MojoArchive *MojoArchive_createUZ2(MojoInput *io, const char *origfname)
{
    MojoArchive *ar = NULL;
    const char *fname = NULL;
    char *outname = NULL;
    size_t len = 0;
    int64 outsize = 0;
    UZ2info *uz2info = NULL;

    // There's no magic in a UZ2 that allows us to identify the format.
    //  The higher-level won't call this unless the file extension in
    //  (origfname) is ".uz2"

    // Figure out the output name ("x.uz2" would produce "x").
    if (origfname == NULL)
        return NULL;  // just in case.
    fname = strrchr(origfname, '/');
    if (fname == NULL)
        fname = origfname;
    else
        fname++;

    len = strlen(fname) - 4;  // -4 == ".uz2"
    if (strcasecmp(fname + len, ".uz2") != 0)
        return NULL;  // just in case.

    outsize = calculate_uz2_outsize(io);
    if (outsize < 0)
        return NULL;  // wasn't really a uz2? Corrupt/truncated file?
    outname = (char *) xmalloc(len+1);
    memcpy(outname, fname, len);
    outname[len] = '\0';

    uz2info = (UZ2info *) xmalloc(sizeof (UZ2info));
    uz2info->enumerated = false;
    uz2info->outname = outname;
    uz2info->outsize = outsize;

    ar = (MojoArchive *) xmalloc(sizeof (MojoArchive));
    ar->opaque = uz2info;
    ar->enumerate = MojoArchive_uz2_enumerate;
    ar->enumNext = MojoArchive_uz2_enumNext;
    ar->openCurrentEntry = MojoArchive_uz2_openCurrentEntry;
    ar->close = MojoArchive_uz2_close;
    ar->io = io;
    return ar;
} // MojoArchive_createUZ2

#endif // SUPPORT_UZ2

// end of archive_uz2.c ...

