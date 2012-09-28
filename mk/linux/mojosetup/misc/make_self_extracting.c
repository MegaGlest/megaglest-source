/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _MSC_VER
#define off_t __int64
#define ftello _ftelli64
typedef unsigned __int64 uint64;
#else
#include <stdint.h>
typedef uint64_t uint64;
#endif

static int usage(const char *argv0)
{
    fprintf(stderr, "\nUSAGE: %s <exe> <archive_to_append>\n\n", argv0);
    return 1;
} // usage

static int is_self_extractable(const char *fname)
{
    unsigned char magic[4];

    FILE *io = fopen(fname, "rb");
    if (io == NULL)
    {
        perror("fopen");
        fprintf(stderr, "Failed to open '%s' for reading.\n", fname);
        return -1;
    } // if

    if (fread(magic, 4, 1, io) != 1)
    {
        perror("fread");
        fclose(io);
        fprintf(stderr, "Failed to read '%s'.\n", fname);
        return -1;
    } // if

    fclose(io);

    if ( (magic[0] == 0x50) && (magic[1] == 0x4B) &&
         (magic[2] == 0x03) && (magic[3] == 0x04) )
        return 1;  // it's a .zip file, they handle self-extracting themselves.

    return 0;
} // is_self_extractable

static off_t get_file_size(const char *fname)
{
    off_t retval = -1;

    #define FAIL(x) do { \
        perror(x); \
        fprintf(stderr, "couldn't determine size of %s\n", fname); \
        if (io) \
            fclose(io); \
        return -1; \
    } while (0)

    FILE *io = fopen(fname, "rb");
    if (io == NULL)
        FAIL("fopen");
    else if (fseek(io, 0, SEEK_END) != 0)
        FAIL("fseek");
    else if ((retval = ftello(io)) == -1)
        FAIL("ftello");
    else if (fclose(io) == EOF)
        FAIL("fclose");
    #undef FAIL

    return retval;
} // get_file_size

int main(int argc, char **argv)
{
    static unsigned char buf[1024 * 1024];
    FILE *in = NULL;
    FILE *out = NULL;
    off_t exesize = -1;
    off_t arcsize = -1;
    off_t bytes = 0;
    int is_self = 0;

    assert(sizeof (off_t) == 8);

    if (argc != 3)
        return usage(argv[0]);

    if ((exesize = get_file_size(argv[1])) < 0)
        return 1;
    else if ((arcsize = get_file_size(argv[2])) < 0)
        return 1;
    else if ((is_self = is_self_extractable(argv[2])) < 0)
        return 1;

    if ((out = fopen(argv[1], "ab")) == NULL)
    {
        perror("fopen");
        fprintf(stderr, "Failed to open '%s' for appending.\n", argv[1]);
        return 1;
    } // if

    if ((in = fopen(argv[2], "rb")) == NULL)
    {
        perror("fopen");
        fclose(out);
        fprintf(stderr, "Failed to open '%s' for reading.\n", argv[2]);
        return 1;
    } // if

    bytes = arcsize;
    while (bytes > 0)
    {
        size_t rc = (size_t) (bytes > sizeof (buf) ? sizeof (buf) : bytes);
        rc = fread(buf, 1, rc, in);
        if (ferror(in))
        {
            perror("fread");
            break;
        } // if

        assert(rc != 0);

        if (fwrite(buf, rc, 1, out) != 1)
        {
            perror("fwrite");
            break;
        } // if

        bytes -= (off_t) rc;
    } // while

    if (bytes > 0)
    {
        fclose(in);
        fclose(out);
        fprintf(stderr, "Failed to write '%s'. File may be corrupted.\n", argv[1]);
        return 1;
    } // if

    fclose(in);

    if (!is_self)
    {
        if (fwrite("MOJOBASE", 8, 1, out) != 1)
        {
            perror("fwrite");
            fclose(out);
            fprintf(stderr, "Failed to write '%s'. File may be corrupted.\n", argv[1]);
            return 1;
        } // if

        buf[0] = (unsigned char) ((((uint64) arcsize) >>  0) & 0xFF);
        buf[1] = (unsigned char) ((((uint64) arcsize) >>  8) & 0xFF);
        buf[2] = (unsigned char) ((((uint64) arcsize) >> 16) & 0xFF);
        buf[3] = (unsigned char) ((((uint64) arcsize) >> 24) & 0xFF);
        buf[4] = (unsigned char) ((((uint64) arcsize) >> 32) & 0xFF);
        buf[5] = (unsigned char) ((((uint64) arcsize) >> 40) & 0xFF);
        buf[6] = (unsigned char) ((((uint64) arcsize) >> 48) & 0xFF);
        buf[7] = (unsigned char) ((((uint64) arcsize) >> 56) & 0xFF);

        if (fwrite(buf, 8, 1, out) != 1)
        {
            perror("fwrite");
            fclose(out);
            fprintf(stderr, "Failed to write '%s'. File may be corrupted.\n", argv[1]);
            return 1;
        } // if
    } // if

    if (fclose(out) == EOF)
    {
        perror("fclose");
        fprintf(stderr, "Failed to close '%s'. File may be corrupted.\n", argv[1]);
        return 1;
    } // if

    return 0;
} // main

// end of make_self_extracting.c ...

