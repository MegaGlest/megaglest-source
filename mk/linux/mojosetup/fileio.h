/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef _INCL_FILEIO_H_
#define _INCL_FILEIO_H_

#include "universal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File i/o may go through multiple layers: the archive attached to the binary,
 *  then an archive in there that's being read entirely out of memory that's
 *  being uncompressed to on the fly, or it might be a straight read from a
 *  regular uncompressed file on physical media. It might be a single file
 *  compressed with bzip2. As such, we have to have an abstraction over the
 *  usual channels...basically what we need here is archives-within-archives,
 *  done transparently and with arbitrary depth, although usually not more
 *  than one deep. This also works as a general transport layer, so the
 *  abstraction could be extended to network connections and such, too.
 */

// Abstract input interface. Files, memory, archive entries, etc.
typedef struct MojoInput MojoInput;
struct MojoInput
{
    // public
    boolean (*ready)(MojoInput *io);
    int64 (*read)(MojoInput *io, void *buf, uint32 bufsize);
    boolean (*seek)(MojoInput *io, uint64 pos);
    int64 (*tell)(MojoInput *io);
    int64 (*length)(MojoInput *io);
    MojoInput* (*duplicate)(MojoInput *io);
    void (*close)(MojoInput *io);

    // private
    void *opaque;
};

// This copies the memory, so you may free (ptr) after this function returns.
MojoInput *MojoInput_newFromMemory(const uint8 *ptr, uint32 len, int constant);
MojoInput *MojoInput_newFromFile(const char *fname);


typedef enum
{
    MOJOARCHIVE_ENTRY_UNKNOWN = 0,
    MOJOARCHIVE_ENTRY_FILE,
    MOJOARCHIVE_ENTRY_DIR,
    MOJOARCHIVE_ENTRY_SYMLINK,
} MojoArchiveEntryType;

// Abstract archive interface. Archives, directories, etc.
typedef struct MojoArchiveEntry
{
    char *filename;
    char *linkdest;
    MojoArchiveEntryType type;
    int64 filesize;
    uint16 perms;
} MojoArchiveEntry;

void MojoArchive_resetEntry(MojoArchiveEntry *info);


typedef struct MojoArchive MojoArchive;
struct MojoArchive 
{
    // public
    boolean (*enumerate)(MojoArchive *ar);
    const MojoArchiveEntry* (*enumNext)(MojoArchive *ar);
    MojoInput* (*openCurrentEntry)(MojoArchive *ar);
    void (*close)(MojoArchive *ar);

    // private
    MojoInput *io;
    MojoArchiveEntry prevEnum;
    int64 offsetOfStart;  // byte offset in MojoInput where archive starts.
    void *opaque;
};

MojoArchive *MojoArchive_newFromDirectory(const char *dirname);
MojoArchive *MojoArchive_newFromInput(MojoInput *io, const char *origfname);

// This will reset enumeration in the archive, don't use it while iterating!
// Also, this can be very slow depending on the archive in question, so
//  try to limit your random access filename lookups to known-fast quantities
//  (like directories on the physical filesystem or a zipfile...tarballs and
//  zipfiles-in-zipfiles will bog down here, for example).
MojoInput *MojoInput_newFromArchivePath(MojoArchive *ar, const char *fname);

// Wrap (origio) in a new MojoInput that decompresses a compressed stream
//  on the fly. Returns NULL on error or if (origio) isn't a supported
//  compressed format. The returned MojoInput wraps the original input;
//  closing the returned MojoInput will close (origio), too, and you should
//  consider origio lost. If this function returns non-NULL, you should not,
//  under any circumstances, interact directly with origio again, as the
//  new MojoInput now owns it.
MojoInput *MojoInput_newCompressedStream(MojoInput *origio);

// !!! FIXME: fill in missing documentation here.
extern MojoArchive *GBaseArchive;
extern const char *GBaseArchivePath;
MojoArchive *MojoArchive_initBaseArchive(void);
void MojoArchive_deinitBaseArchive(void);

typedef boolean (*MojoInput_FileCopyCallback)(uint32 ticks, int64 justwrote,
                                            int64 bw, int64 total, void *data);
boolean MojoInput_toPhysicalFile(MojoInput *in, const char *fname, uint16 perms,
                                 MojoChecksums *checksums, int64 maxbytes,
                                 MojoInput_FileCopyCallback cb, void *data);

MojoInput *MojoInput_newFromURL(const char *url);

// Read a littleendian, unsigned 16-bit integer from (io), swapping it to
//  the correct byteorder for the platform, and moving the file pointer
//  ahead 2 bytes. Returns true on successful read and fills the swapped
//  value into (*ui16), false on i/o error or EOF.
boolean MojoInput_readui16(MojoInput *io, uint16 *ui16);

// Read a littleendian, unsigned 32-bit integer from (io), swapping it to
//  the correct byteorder for the platform, and moving the file pointer
//  ahead 4 bytes. Returns true on successful read and fills the swapped
//  value into (*ui32), false on i/o error or EOF.
boolean MojoInput_readui32(MojoInput *io, uint32 *ui32);

// (Please note that there are not bigendian versions of MojoInput_readuiXX()
//  at the moment, as we don't need them for our current feature set. However,
//  they could be added easily enough.)

#ifdef __cplusplus
}
#endif

#endif

// end of fileio.h ...

