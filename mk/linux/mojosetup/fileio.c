/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include "fileio.h"
#include "platform.h"

typedef MojoArchive* (*MojoArchiveCreateEntryPoint)(MojoInput *io);

MojoArchive *MojoArchive_createZIP(MojoInput *io);
MojoArchive *MojoArchive_createTAR(MojoInput *io);
MojoArchive *MojoArchive_createUZ2(MojoInput *io);
MojoArchive *MojoArchive_createPCK(MojoInput *io);

typedef struct
{
    const char *ext;
    MojoArchiveCreateEntryPoint create;
    boolean hasMagic;  // can determine file type from contents?
} MojoArchiveType;

static const MojoArchiveType archives[] =
{
    { "zip", MojoArchive_createZIP, true },
    { "tar", MojoArchive_createTAR, true },
    { "tar.gz", MojoArchive_createTAR, true },
    { "tar.bz2", MojoArchive_createTAR, true },
    { "tgz", MojoArchive_createTAR, true },
    { "tbz2", MojoArchive_createTAR, true },
    { "tb2", MojoArchive_createTAR, true },
    { "tbz", MojoArchive_createTAR, true },
    { "uz2", MojoArchive_createUZ2, false },
    { "pck", MojoArchive_createPCK, true },
};


#if SUPPORT_GZIP

#include "zlib/zlib.h"

#define GZIP_READBUFSIZE (128 * 1024)

static MojoInput *make_gzip_input(MojoInput *origio);

typedef struct GZIPinfo
{
    MojoInput *origio;
    uint64 uncompressed_position;
    uint8 *buffer;
    z_stream stream;
} GZIPinfo;

static voidpf mojoZlibAlloc(voidpf opaque, uInt items, uInt size)
{
    return xmalloc(items * size);
} // mojoZlibAlloc

static void mojoZlibFree(voidpf opaque, voidpf address)
{
    free(address);
} // mojoZlibFree

static void initializeZStream(z_stream *pstr)
{
    memset(pstr, '\0', sizeof (z_stream));
    pstr->zalloc = mojoZlibAlloc;
    pstr->zfree = mojoZlibFree;
} // initializeZStream

static boolean MojoInput_gzip_ready(MojoInput *io)
{
    return true;  // !!! FIXME: ready if there are bytes uncompressed.
} // MojoInput_gzip_ready

static boolean MojoInput_gzip_seek(MojoInput *io, uint64 offset)
{
    // This is all really expensive.
    GZIPinfo *info = (GZIPinfo *) io->opaque;

    /*
     * If seeking backwards, we need to redecode the file
     *  from the start and throw away the compressed bits until we hit
     *  the offset we need. If seeking forward, we still need to
     *  decode, but we don't rewind first.
     */
    if (offset < info->uncompressed_position)
    {
        if (!info->origio->seek(info->origio, 0))
            return false;
        inflateEnd(&info->stream);
        initializeZStream(&info->stream);
        if (inflateInit2(&info->stream, 31) != Z_OK)
            return false;
        info->uncompressed_position = 0;
    } // if

    while (info->uncompressed_position != offset)
    {
        uint8 buf[512];
        uint32 maxread;
        int64 br;

        maxread = (uint32) (offset - info->uncompressed_position);
        if (maxread > sizeof (buf))
            maxread = sizeof (buf);

        br = io->read(io, buf, maxread);
        if (br != maxread)
            return false;
    } /* while */

    return true;
} // MojoInput_gzip_seek

static int64 MojoInput_gzip_tell(MojoInput *io)
{
    return (((GZIPinfo *) io->opaque)->uncompressed_position);
} // MojoInput_gzip_tell

static int64 MojoInput_gzip_length(MojoInput *io)
{
    return -1;
} // MojoInput_gzip_length

static int64 MojoInput_gzip_read(MojoInput *io, void *buf, uint32 bufsize)
{
    GZIPinfo *info = (GZIPinfo *) io->opaque;
    MojoInput *origio = info->origio;
    int64 retval = 0;

    if (bufsize == 0)
        return 0;    // quick rejection.
    else
    {
        info->stream.next_out = buf;
        info->stream.avail_out = bufsize;

        while (retval < ((int64) bufsize))
        {
            const uint32 before = info->stream.total_out;
            int rc;

            if (info->stream.avail_in == 0)
            {
                int64 br;

                br = origio->length(origio) - origio->tell(origio);
                if (br > 0)
                {
                    if (br > GZIP_READBUFSIZE)
                        br = GZIP_READBUFSIZE;

                    br = origio->read(origio, info->buffer, (uint32) br);
                    if (br <= 0)
                        return -1;

                    info->stream.next_in = info->buffer;
                    info->stream.avail_in = (uint32) br;
                } // if
            } // if

            rc = inflate(&info->stream, Z_SYNC_FLUSH);
            retval += (info->stream.total_out - before);

            if (rc != Z_OK)
                return -1;
        } // while
    } // else

    if (retval > 0)
        info->uncompressed_position += (uint32) retval;

    return retval;
} // MojoInput_gzip_read

static MojoInput* MojoInput_gzip_duplicate(MojoInput *io)
{
    GZIPinfo *info = (GZIPinfo *) io->opaque;
    MojoInput *retval = NULL;
    MojoInput *newio = info->origio->duplicate(info->origio);
    if (newio != NULL)
    {
        retval = make_gzip_input(newio);
        if (retval != NULL)
            retval->seek(retval, io->tell(io));  // slow, slow, slow...
    } // if
    return retval;
} // MojoInput_gzip_duplicate

static void MojoInput_gzip_close(MojoInput *io)
{
    GZIPinfo *info = (GZIPinfo *) io->opaque;
    if (info->origio != NULL)
        info->origio->close(info->origio);
    inflateEnd(&info->stream);
    free(info->buffer);
    free(info);
    free(io);
} // MojoInput_gzip_close

static MojoInput *make_gzip_input(MojoInput *origio)
{
    MojoInput *io = NULL;
    GZIPinfo *info = (GZIPinfo *) xmalloc(sizeof (GZIPinfo));

    initializeZStream(&info->stream);
    if (inflateInit2(&info->stream, 31) != Z_OK)
    {
        free(info);
        return NULL;
    } // if

    info->origio = origio;
    info->buffer = (uint8 *) xmalloc(GZIP_READBUFSIZE);

    io = (MojoInput *) xmalloc(sizeof (MojoInput));
    io->ready = MojoInput_gzip_ready;
    io->read = MojoInput_gzip_read;
    io->seek = MojoInput_gzip_seek;
    io->tell = MojoInput_gzip_tell;
    io->length = MojoInput_gzip_length;
    io->duplicate = MojoInput_gzip_duplicate;
    io->close = MojoInput_gzip_close;
    io->opaque = info;
    return io;
} // make_gzip_input

#endif  // SUPPORT_GZIP


#if SUPPORT_BZIP2

#include "bzip2/bzlib.h"

#define BZIP2_READBUFSIZE (128 * 1024)

static MojoInput *make_bzip2_input(MojoInput *origio);

typedef struct BZIP2info
{
    MojoInput *origio;
    uint64 uncompressed_position;
    uint8 *buffer;
    bz_stream stream;
} BZIP2info;

static void *mojoBzlib2Alloc(void *opaque, int items, int size)
{
    return xmalloc(items * size);
} // mojoBzlib2Alloc

static void mojoBzlib2Free(void *opaque, void *address)
{
    free(address);
} // mojoBzlib2Free

static void initializeBZ2Stream(bz_stream *pstr)
{
    memset(pstr, '\0', sizeof (bz_stream));
    pstr->bzalloc = mojoBzlib2Alloc;
    pstr->bzfree = mojoBzlib2Free;
} // initializeBZ2Stream

static boolean MojoInput_bzip2_ready(MojoInput *io)
{
    return true;  // !!! FIXME: ready if there are bytes uncompressed.
} // MojoInput_bzip2_ready

static boolean MojoInput_bzip2_seek(MojoInput *io, uint64 offset)
{
    // This is all really expensive.
    BZIP2info *info = (BZIP2info *) io->opaque;

    /*
     * If seeking backwards, we need to redecode the file
     *  from the start and throw away the compressed bits until we hit
     *  the offset we need. If seeking forward, we still need to
     *  decode, but we don't rewind first.
     */
    if (offset < info->uncompressed_position)
    {
#if 0
        /* we do a copy so state is sane if inflateInit2() fails. */
        bz_stream str;
        initializeBZ2Stream(&str);
        if (BZ2_bzDecompressInit(&str, 0, 0) != BZ_OK)
            return false;

        if (!info->origio->seek(info->origio, 0))
            return false;  // !!! FIXME: leaking (str)?

        BZ2_bzDecompressEnd(&info->stream);
        memcpy(&info->stream, &str, sizeof (bz_stream));
#endif

        if (!info->origio->seek(info->origio, 0))
            return false;
        BZ2_bzDecompressEnd(&info->stream);
        initializeBZ2Stream(&info->stream);
        if (BZ2_bzDecompressInit(&info->stream, 0, 0) != BZ_OK)
            return false;
        info->uncompressed_position = 0;
    } // if

    while (info->uncompressed_position != offset)
    {
        uint8 buf[512];
        uint32 maxread;
        int64 br;

        maxread = (uint32) (offset - info->uncompressed_position);
        if (maxread > sizeof (buf))
            maxread = sizeof (buf);

        br = io->read(io, buf, maxread);
        if (br != maxread)
            return false;
    } /* while */

    return true;
} // MojoInput_bzip2_seek

static int64 MojoInput_bzip2_tell(MojoInput *io)
{
    return (((BZIP2info *) io->opaque)->uncompressed_position);
} // MojoInput_bzip2_tell

static int64 MojoInput_bzip2_length(MojoInput *io)
{
    return -1;
} // MojoInput_bzip2_length

static int64 MojoInput_bzip2_read(MojoInput *io, void *buf, uint32 bufsize)
{
    BZIP2info *info = (BZIP2info *) io->opaque;
    MojoInput *origio = info->origio;
    int64 retval = 0;

    if (bufsize == 0)
        return 0;    // quick rejection.
    else
    {
        info->stream.next_out = buf;
        info->stream.avail_out = bufsize;

        while (retval < ((int64) bufsize))
        {
            const uint32 before = info->stream.total_out_lo32;
            int rc;

            if (info->stream.avail_in == 0)
            {
                int64 br;

                br = origio->length(origio) - origio->tell(origio);
                if (br > 0)
                {
                    if (br > BZIP2_READBUFSIZE)
                        br = BZIP2_READBUFSIZE;

                    br = origio->read(origio, info->buffer, (uint32) br);
                    if (br <= 0)
                        return -1;

                    info->stream.next_in = (char *) info->buffer;
                    info->stream.avail_in = (uint32) br;
                } // if
            } // if

            rc = BZ2_bzDecompress(&info->stream);
            retval += (info->stream.total_out_lo32 - before);
            if (rc != BZ_OK)
                return -1;
        } // while
    } // else

    if (retval > 0)
        info->uncompressed_position += (uint32) retval;

    return retval;
} // MojoInput_bzip2_read

static MojoInput* MojoInput_bzip2_duplicate(MojoInput *io)
{
    BZIP2info *info = (BZIP2info *) io->opaque;
    MojoInput *retval = NULL;
    MojoInput *newio = info->origio->duplicate(info->origio);
    if (newio != NULL)
    {
        retval = make_bzip2_input(newio);
        if (retval != NULL)
            retval->seek(retval, io->tell(io));  // slow, slow, slow...
    } // if
    return retval;
} // MojoInput_bzip2_duplicate

static void MojoInput_bzip2_close(MojoInput *io)
{
    BZIP2info *info = (BZIP2info *) io->opaque;
    if (info->origio != NULL)
        info->origio->close(info->origio);
    BZ2_bzDecompressEnd(&info->stream);
    free(info->buffer);
    free(info);
    free(io);
} // MojoInput_bzip2_close

static MojoInput *make_bzip2_input(MojoInput *origio)
{
    MojoInput *io = NULL;
    BZIP2info *info = (BZIP2info *) xmalloc(sizeof (BZIP2info));

    initializeBZ2Stream(&info->stream);
    if (BZ2_bzDecompressInit(&info->stream, 0, 0) != BZ_OK)
    {
        free(info);
        return NULL;
    } // if

    info->origio = origio;
    info->buffer = (uint8 *) xmalloc(BZIP2_READBUFSIZE);

    io = (MojoInput *) xmalloc(sizeof (MojoInput));
    io->ready = MojoInput_bzip2_ready;
    io->read = MojoInput_bzip2_read;
    io->seek = MojoInput_bzip2_seek;
    io->tell = MojoInput_bzip2_tell;
    io->length = MojoInput_bzip2_length;
    io->duplicate = MojoInput_bzip2_duplicate;
    io->close = MojoInput_bzip2_close;
    io->opaque = info;
    return io;
} // make_bzip2_input

#endif  // SUPPORT_BZIP2


MojoInput *MojoInput_newCompressedStream(MojoInput *origio)
{
#if SUPPORT_GZIP || SUPPORT_BZIP2
    // Look at the first piece of the file to decide if it is compressed
    //  by a general compression algorithm, and if so, wrap the MojoInput
    //  in a decompressor.
    uint8 magic[4];
    const int64 br = origio->read(origio, magic, sizeof (magic));
    if ((origio->seek(origio, 0)) && (br == sizeof (magic)))
    {
        #if SUPPORT_GZIP
        if ((magic[0] == 0x1F) && (magic[1] == 0x8B) && (magic[2] == 0x08))
            return make_gzip_input(origio);
        #endif

        #if SUPPORT_BZIP2
        if ((magic[0] == 0x42) && (magic[1] == 0x5A))
            return make_bzip2_input(origio);
        #endif
    } // if
#endif

    return NULL;
} // MojoInput_newCompressedStream


MojoArchive *MojoArchive_newFromInput(MojoInput *_io, const char *origfname)
{
    int i;
    MojoArchive *retval = NULL;
    const char *ext = NULL;
    MojoInput *io = MojoInput_newCompressedStream(_io);
    if (io == NULL)
        io = _io;

    if (origfname != NULL)
    {
        ext = strrchr(origfname, '/');
        if (ext == NULL)
            ext = strchr(origfname, '.');
        else
            ext = strchr(ext+1, '.');
    } // if

    while (ext != NULL)
    {
        // Try for an exact match by filename extension.
        ext++;  // skip that '.'
        for (i = 0; i < STATICARRAYLEN(archives); i++)
        {
            const MojoArchiveType *arc = &archives[i];
            if (strcasecmp(ext, arc->ext) == 0)
                return arc->create(io);
        } // for
        ext = strchr(ext, '.');
    } // while

    // Try any that could be determined without the file extension...
    for (i = 0; i < STATICARRAYLEN(archives); i++)
    {
        const MojoArchiveType *arc = &archives[i];
        if ((arc->hasMagic) && ((retval = arc->create(io)) != NULL))
            return retval;
    } // for

    io->close(io);
    return NULL;  // nothing can handle this data.
} // MojoArchive_newFromInput


void MojoArchive_resetEntry(MojoArchiveEntry *info)
{
    free(info->filename);
    free(info->linkdest);
    memset(info, '\0', sizeof (MojoArchiveEntry));
} // MojoArchive_resetEntry


// !!! FIXME: I'd rather not use a callback here, but I can't see a cleaner
// !!! FIXME:  way right now...
boolean MojoInput_toPhysicalFile(MojoInput *in, const char *fname, uint16 perms,
                                 MojoChecksums *checksums, int64 maxbytes,
                                 MojoInput_FileCopyCallback cb, void *data)
{
    boolean retval = false;
    uint32 start = MojoPlatform_ticks();
    void *out = NULL;
    boolean iofailure = false;
    int64 flen = 0;
    int64 bw = 0;
    MojoChecksumContext sumctx;

    if (in == NULL)
        return false;

    if (checksums != NULL)
    {
        memset(checksums, '\0', sizeof (MojoChecksums));
        MojoChecksum_init(&sumctx);
    } // if

    // Wait for a ready(), so length() can be meaningful on network streams.
    while ((!in->ready(in)) && (!iofailure))
    {
        MojoPlatform_sleep(100);
        if (cb != NULL)
        {
            if (!cb(MojoPlatform_ticks() - start, 0, 0, -1, data))
                iofailure = true;
        } // if
    } // while

    flen = in->length(in);
    if ((maxbytes >= 0) && (flen > maxbytes))
        flen = maxbytes;

    MojoPlatform_unlink(fname);
    if (!iofailure)
    {
        const uint32 flags = MOJOFILE_WRITE|MOJOFILE_CREATE|MOJOFILE_TRUNCATE;
        const uint16 mode = MojoPlatform_defaultFilePerms();
        out = MojoPlatform_open(fname, flags, mode);
    } // if

    if (out != NULL)
    {
        while (!iofailure)
        {
            int64 br = 0;
            int64 maxread = sizeof (scratchbuf_128k);

            // see if we need to clamp to eof or maxbytes...
            if (flen >= 0)
            {
                const int64 avail = flen - bw;
                if (avail < maxread)
                {
                    maxread = avail;
                    if (maxread == 0)
                        break;  // nothing left to do, break out.
                } // if
            } // if

            // If there's a callback, then poll. Otherwise, just block on
            //  the reads from the MojoInput.
            if ((cb != NULL) && (!in->ready(in)))
                MojoPlatform_sleep(100);
            else
            {
                br = in->read(in, scratchbuf_128k, (uint32) maxread);
                if (br == 0)  // we're done!
                    break;
                else if (br < 0)
                    iofailure = true;
                else
                {
                    if (MojoPlatform_write(out, scratchbuf_128k, (uint32) br) != br)
                        iofailure = true;
                    else
                    {
                        if (checksums != NULL)
                            MojoChecksum_append(&sumctx, scratchbuf_128k, (uint32) br);
                        bw += br;
                    } // else
                } // else
            } // else

            if (cb != NULL)
            {
                if (!cb(MojoPlatform_ticks() - start, br, bw, flen, data))
                    iofailure = true;
            } // if
        } // while

        if (MojoPlatform_close(out) != 0)
            iofailure = true;
        else if (bw != flen)
            iofailure = true;

        if (iofailure)
            MojoPlatform_unlink(fname);
        else
        {
            MojoPlatform_chmod(fname, perms);
            if (checksums != NULL)
                MojoChecksum_finish(&sumctx, checksums);
            retval = true;
        } // else
    } // if

    in->close(in);
    return retval;
} // MojoInput_toPhysicalFile


MojoInput *MojoInput_newFromArchivePath(MojoArchive *ar, const char *fname)
{
    MojoInput *retval = NULL;
    if (ar->enumerate(ar))
    {
        const MojoArchiveEntry *entinfo;
        while ((entinfo = ar->enumNext(ar)) != NULL)
        {
            if (strcmp(entinfo->filename, fname) == 0)
            {
                if (entinfo->type == MOJOARCHIVE_ENTRY_FILE)
                    retval = ar->openCurrentEntry(ar);
                break;
            } // if
        } // while
    } // if

    return retval;
} // MojoInput_newFromArchivePath



// MojoInputs from files on the OS filesystem.

typedef struct
{
    void *handle;
    char *path;
} MojoInputFileInstance;

static boolean MojoInput_file_ready(MojoInput *io)
{
    // !!! FIXME: select()? Does that help with network filesystems?
    return true;
} // MojoInput_file_ready

static int64 MojoInput_file_read(MojoInput *io, void *buf, uint32 bufsize)
{
    MojoInputFileInstance *inst = (MojoInputFileInstance *) io->opaque;
    return MojoPlatform_read(inst->handle, buf, bufsize);
} // MojoInput_file_read

static boolean MojoInput_file_seek(MojoInput *io, uint64 pos)
{
    MojoInputFileInstance *inst = (MojoInputFileInstance *) io->opaque;
    return (MojoPlatform_seek(inst->handle, pos, MOJOSEEK_SET) == pos);
} // MojoInput_file_seek

static int64 MojoInput_file_tell(MojoInput *io)
{
    MojoInputFileInstance *inst = (MojoInputFileInstance *) io->opaque;
    return MojoPlatform_tell(inst->handle);
} // MojoInput_file_tell

static int64 MojoInput_file_length(MojoInput *io)
{
    MojoInputFileInstance *inst = (MojoInputFileInstance *) io->opaque;
    return MojoPlatform_flen(inst->handle);
} // MojoInput_file_length

static MojoInput *MojoInput_file_duplicate(MojoInput *io)
{
    MojoInputFileInstance *inst = (MojoInputFileInstance *) io->opaque;
    return MojoInput_newFromFile(inst->path);
} // MojoInput_file_duplicate

static void MojoInput_file_close(MojoInput *io)
{
    MojoInputFileInstance *inst = (MojoInputFileInstance *) io->opaque;
    MojoPlatform_close(inst->handle);
    free(inst->path);
    free(inst);
    free(io);
} // MojoInput_file_close

MojoInput *MojoInput_newFromFile(const char *path)
{
    MojoInput *io = NULL;
    void *f = NULL;

    f = MojoPlatform_open(path, MOJOFILE_READ, 0);
    if (f != NULL)
    {
        MojoInputFileInstance *inst;
        inst = (MojoInputFileInstance *) xmalloc(sizeof (MojoInputFileInstance));
        inst->path = xstrdup(path);
        inst->handle = f;

        io = (MojoInput *) xmalloc(sizeof (MojoInput));
        io->ready = MojoInput_file_ready;
        io->read = MojoInput_file_read;
        io->seek = MojoInput_file_seek;
        io->tell = MojoInput_file_tell;
        io->length = MojoInput_file_length;
        io->duplicate = MojoInput_file_duplicate;
        io->close = MojoInput_file_close;
        io->opaque = inst;
    } // if

    return io;
} // MojoInput_newFromFile



// MojoInputs from blocks of memory.

typedef struct
{
    void *ptr;  // original pointer from xmalloc()
    uint32 *refcount;  // address in xmalloc()'d block for reference count.
    const uint8 *data; // base of actual "file" data in xmalloc()'d block.
    uint32 len;  // size, in bytes, of "file" data.
    uint32 pos;  // current read position.
} MojoInputMemInstance;

static boolean MojoInput_memory_ready(MojoInput *io)
{
    return true;  // always ready!
} // MojoInput_memory_ready

static int64 MojoInput_memory_read(MojoInput *io, void *buf, uint32 bufsize)
{
    MojoInputMemInstance *inst = (MojoInputMemInstance *) io->opaque;
    const uint32 avail = inst->len - inst->pos;
    if (bufsize > avail)
        bufsize = avail;
    memcpy(buf, inst->data + inst->pos, bufsize);
    inst->pos += bufsize;
    return bufsize;
} // MojoInput_memory_read

static boolean MojoInput_memory_seek(MojoInput *io, uint64 pos)
{
    MojoInputMemInstance *inst = (MojoInputMemInstance *) io->opaque;
    if (pos > (uint64) inst->len)
        return false;
    inst->pos = (uint32) pos;
    return true;
} // MojoInput_memory_seek

static int64 MojoInput_memory_tell(MojoInput *io)
{
    MojoInputMemInstance *inst = (MojoInputMemInstance *) io->opaque;
    return (int64) inst->pos;
} // MojoInput_memory_tell

static int64 MojoInput_memory_length(MojoInput *io)
{
    MojoInputMemInstance *inst = (MojoInputMemInstance *) io->opaque;
    return (int64) inst->len;
} // MojoInput_memory_length

static MojoInput *MojoInput_memory_duplicate(MojoInput *io)
{
    MojoInputMemInstance *srcinst = (MojoInputMemInstance *) io->opaque;
    MojoInput *retval = NULL;
    MojoInputMemInstance *inst = NULL;

    if (srcinst->refcount != NULL)
    {
        // we don't copy the data for each duplicate; we just bump a reference
        //  counter. We free the data when all referencers are closed.
        (*srcinst->refcount)++;  // !!! FIXME: not thread safe!
    } // if

    inst = (MojoInputMemInstance*) xmalloc(sizeof (MojoInputMemInstance));
    memcpy(inst, srcinst, sizeof (MojoInputMemInstance));
    inst->pos = 0;

    retval = (MojoInput *) xmalloc(sizeof (MojoInput));
    memcpy(retval, io, sizeof (MojoInput));
    retval->opaque = inst;

    return retval;
} // MojoInput_memory_duplicate

static void MojoInput_memory_close(MojoInput *io)
{
    MojoInputMemInstance *inst = (MojoInputMemInstance *) io->opaque;

    if (inst->refcount != NULL)  // memory we have to free?
    {
        assert(*inst->refcount > 0);
        if (--(*inst->refcount) == 0)  // !!! FIXME: not thread safe!
            free(inst->ptr);
    } // if

    free(inst);
    free(io);
} // MojoInput_memory_close

MojoInput *MojoInput_newFromMemory(const uint8 *ptr, uint32 len, int constant)
{
    MojoInput *io = (MojoInput *) xmalloc(sizeof (MojoInput));
    MojoInputMemInstance *inst = (MojoInputMemInstance*)
                                    xmalloc(sizeof (MojoInputMemInstance));

    if (constant)
        inst->data = ptr;
    else
    {
        inst->ptr = xmalloc(len + sizeof (uint32));
        inst->refcount = (uint32 *) inst->ptr;
        inst->data = ((const uint8 *) inst->ptr) + sizeof (uint32);
        *inst->refcount = 1;
        memcpy((void *) inst->data, ptr, len);
    } // else

    inst->len = len;

    io->ready = MojoInput_memory_ready;
    io->read = MojoInput_memory_read;
    io->seek = MojoInput_memory_seek;
    io->tell = MojoInput_memory_tell;
    io->length = MojoInput_memory_length;
    io->duplicate = MojoInput_memory_duplicate;
    io->close = MojoInput_memory_close;
    io->opaque = inst;

    return io;
} // MojoInput_newFromMemory




// MojoArchives from directories on the OS filesystem.

typedef struct DirStack
{
    void *dir;
    char *basepath;
    struct DirStack *next;
} DirStack;

static void pushDirStack(DirStack **_stack, const char *basepath, void *dir)
{
    DirStack *stack = (DirStack *) xmalloc(sizeof (DirStack));
    stack->dir = dir;
    stack->basepath = xstrdup(basepath);
    stack->next = *_stack;
    *_stack = stack;
} // pushDirStack

static void popDirStack(DirStack **_stack)
{
    DirStack *stack = *_stack;
    if (stack != NULL)
    {
        DirStack *next = stack->next;
        if (stack->dir)
            MojoPlatform_closedir(stack->dir);
        free(stack->basepath);
        free(stack);
        *_stack = next;
    } // if
} // popDirStack

static void freeDirStack(DirStack **_stack)
{
    while (*_stack)
        popDirStack(_stack);
} // freeDirStack


typedef struct
{
    DirStack *dirs;
    char *base;
} MojoArchiveDirInstance;

static boolean MojoArchive_dir_enumerate(MojoArchive *ar)
{
    MojoArchiveDirInstance *inst = (MojoArchiveDirInstance *) ar->opaque;
    void *dir = NULL;

    freeDirStack(&inst->dirs);
    MojoArchive_resetEntry(&ar->prevEnum);

    dir = MojoPlatform_opendir(inst->base);
    if (dir != NULL)
        pushDirStack(&inst->dirs, inst->base, dir);

    return (dir != NULL);
} // MojoArchive_dir_enumerate


static const MojoArchiveEntry *MojoArchive_dir_enumNext(MojoArchive *ar)
{
    uint16 perms = 0644; //(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    char *fullpath = NULL;
    char *dent = NULL;  // "dent" == "directory entry"
    MojoArchiveDirInstance *inst = (MojoArchiveDirInstance *) ar->opaque;
    const char *basepath;

    MojoArchive_resetEntry(&ar->prevEnum);

    if (inst->dirs == NULL)
        return NULL;

    basepath = inst->dirs->basepath;

    // if readdir fails, it's end of dir (!!! FIXME: what about i/o failures?)
    dent = MojoPlatform_readdir(inst->dirs->dir);
    if (dent == NULL)  // end of dir?
    {
        popDirStack(&inst->dirs);
        return MojoArchive_dir_enumNext(ar);  // try higher level in tree.
    } // if

    // MojoPlatform layer shouldn't return "." or ".." paths.
    assert((strcmp(dent, ".") != 0) && (strcmp(dent, "..") != 0));

    fullpath = (char *) xmalloc(strlen(basepath) + strlen(dent) + 2);
    sprintf(fullpath, "%s/%s", basepath, dent);
    free(dent);

    ar->prevEnum.filename = xstrdup(fullpath + strlen(inst->base) + 1);
    ar->prevEnum.filesize = 0;
    ar->prevEnum.type = MOJOARCHIVE_ENTRY_UNKNOWN;

    // We currently force the perms from physical files, since CDs on
    //  Linux tend to mark every files as executable and read-only. If you
    //  want to install something with specific permissions, wrap it in a
    //  tarball, or use Setup.File.permissions, or return a permissions
    //  string from Setup.File.filter.
    //MojoPlatform_perms(fullpath, &perms);
    ar->prevEnum.perms = perms;

    if (MojoPlatform_isfile(fullpath))
    {
        ar->prevEnum.type = MOJOARCHIVE_ENTRY_FILE;
        ar->prevEnum.filesize = MojoPlatform_filesize(fullpath);
    } // if

    else if (MojoPlatform_issymlink(fullpath))
    {
        ar->prevEnum.type = MOJOARCHIVE_ENTRY_SYMLINK;
        ar->prevEnum.linkdest = MojoPlatform_readlink(fullpath);
        if (ar->prevEnum.linkdest == NULL)
        {
            free(fullpath);
            return MojoArchive_dir_enumNext(ar);
        } // if
    } // else if

    else if (MojoPlatform_isdir(fullpath))
    {
        void *dir = MojoPlatform_opendir(fullpath);
        ar->prevEnum.type = MOJOARCHIVE_ENTRY_DIR;
        if (dir == NULL)
        {
            free(fullpath);
            return MojoArchive_dir_enumNext(ar);
        } // if

        // push this dir on the stack. Next enum will start there.
        pushDirStack(&inst->dirs, fullpath, dir);
    } // else if

    else
    {
        assert(false && "possible file i/o error?");
    } // else

    free(fullpath);
    return &ar->prevEnum;
} // MojoArchive_dir_enumNext


static MojoInput *MojoArchive_dir_openCurrentEntry(MojoArchive *ar)
{
    MojoInput *retval = NULL;
    MojoArchiveDirInstance *inst = (MojoArchiveDirInstance *) ar->opaque;

    if ((inst->dirs != NULL) && (ar->prevEnum.type == MOJOARCHIVE_ENTRY_FILE))
    {
        char *fullpath = (char *) xmalloc(strlen(inst->base) +
                                          strlen(ar->prevEnum.filename) + 2);
        sprintf(fullpath, "%s/%s", inst->base, ar->prevEnum.filename);
        retval = MojoInput_newFromFile(fullpath);
        free(fullpath);
    } // if

    return retval;
} // MojoArchive_dir_openCurrentEntry


static void MojoArchive_dir_close(MojoArchive *ar)
{
    MojoArchiveDirInstance *inst = (MojoArchiveDirInstance *) ar->opaque;
    freeDirStack(&inst->dirs);
    free(inst->base);
    free(inst);
    MojoArchive_resetEntry(&ar->prevEnum);
    free(ar);
} // MojoArchive_dir_close


MojoArchive *MojoArchive_newFromDirectory(const char *dirname)
{
    MojoArchive *ar = NULL;
    MojoArchiveDirInstance *inst;
    char *real = MojoPlatform_realpath(dirname);

    if (real == NULL)
        return NULL;

    if (!MojoPlatform_exists(real, NULL))
        return NULL;

    if (!MojoPlatform_isdir(real))
        return NULL;

    inst = (MojoArchiveDirInstance *) xmalloc(sizeof (MojoArchiveDirInstance));
    inst->base = real;
    ar = (MojoArchive *) xmalloc(sizeof (MojoArchive));
    ar->enumerate = MojoArchive_dir_enumerate;
    ar->enumNext = MojoArchive_dir_enumNext;
    ar->openCurrentEntry = MojoArchive_dir_openCurrentEntry;
    ar->close = MojoArchive_dir_close;
    ar->offsetOfStart = -1;  // doesn't mean anything here.
    ar->opaque = inst;
    return ar;
} // MojoArchive_newFromDirectory




MojoArchive *GBaseArchive = NULL;
const char *GBaseArchivePath = NULL;

MojoArchive *MojoArchive_initBaseArchive(void)
{
    char *basepath = NULL;
    const char *cmd = NULL;
    MojoInput *io = NULL;

    if (GBaseArchive != NULL)
        return GBaseArchive;  // already initialized.

    if ((cmd = cmdlinestr("base", "MOJOSETUP_BASE", NULL)) != NULL)
    {
        char *real = MojoPlatform_realpath(cmd);
        if (real != NULL)
        {
            if (MojoPlatform_isdir(real))
                GBaseArchive = MojoArchive_newFromDirectory(real);
            else
            {
                io = MojoInput_newFromFile(real);
                if (io != NULL)
                    GBaseArchive = MojoArchive_newFromInput(io, real);
            } // else

            if (GBaseArchive != NULL)
                basepath = real;
            else
                free(real);
        } // if
    } // else if

    else
    {
        basepath = MojoPlatform_appBinaryPath();
        io = MojoInput_newFromFile(basepath);

        if (io != NULL)
            GBaseArchive = MojoArchive_newFromInput(io, basepath);

        if (GBaseArchive == NULL)
        {
            // Just use the same directory as the binary instead.
            char *ptr = strrchr(basepath, '/');
            if (ptr != NULL)
                *ptr = '\0';
            else
            {
                free(basepath);  // oh well, try cwd.
                basepath = MojoPlatform_currentWorkingDir();
            } // else
            GBaseArchive = MojoArchive_newFromDirectory(basepath);

            // !!! FIXME: failing this, maybe default.mojosetup?
        } // if
    } // else

    if (GBaseArchive == NULL)
    {
        free(basepath);
        basepath = NULL;
    } // if
    GBaseArchivePath = basepath;

    return GBaseArchive;
} // MojoArchive_initBaseArchive


void MojoArchive_deinitBaseArchive(void)
{
    if (GBaseArchive != NULL)
    {
        GBaseArchive->close(GBaseArchive);
        GBaseArchive = NULL;
    } // if

    free((void *) GBaseArchivePath);
    GBaseArchivePath = NULL;
} // MojoArchive_deinitBaseArchive


// This stub is here if we didn't compile in libfetch...
#if !SUPPORT_URL_HTTP && !SUPPORT_URL_FTP
MojoInput *MojoInput_newFromURL(const char *url)
{
    logError("No networking support in this build.");
    return NULL;
} // MojoInput_newFromURL
#endif

// end of fileio.c ...

