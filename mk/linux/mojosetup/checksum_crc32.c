#include "universal.h"

#if SUPPORT_CRC32

void MojoCrc32_init(MojoCrc32 *context)
{
    *context = (MojoCrc32) 0xFFFFFFFF;
} // MojoCrc32_init


void MojoCrc32_append(MojoCrc32 *_crc, const uint8 *buf, uint32 len)
{
    uint32 crc = (uint32) *_crc;

    uint32 n;
    for (n = 0; n < len; n++)
    {
        uint32 xorval = (uint32) ((crc ^ buf[n]) & 0xFF);
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        xorval = ((xorval & 1) ? (0xEDB88320 ^ (xorval >> 1)) : (xorval >> 1));
        crc = xorval ^ (crc >> 8);
    } // for

    *_crc = (MojoCrc32) crc;
} // MojoCrc32_append


void MojoCrc32_finish(MojoCrc32 *context, uint32 *digest)
{
    *digest = (*context ^ 0xFFFFFFFF);
} // MojoCrc32_finish


#endif  // SUPPORT_CRC32

#if TEST_CRC32
int main(int argc, char **argv)
{
    int i = 0;
    for (i = 1; i < argc; i++)
    {
        FILE *in = NULL;
        MojoCrc32 ctx;
        MojoCrc32_init(&ctx);
        in = fopen(argv[i], "rb");
        if (!in)
            perror("fopen");
        else
        {
            uint32 digest = 0;
            int err = 0;
            while ( (!err) && (!feof(in)) )
            {
                uint8 buf[1024];
                size_t rc = fread(buf, 1, sizeof (buf), in);
                if (rc > 0)
                    MojoCrc32_append(&ctx, buf, rc);
                err = ferror(in);
            } // while

            if (err)
                perror("fread");
            fclose(in);
            MojoCrc32_finish(&ctx, &digest);
            if (!err)
                printf("%s: %X\n", argv[i], (unsigned int) digest);
        } // else
    } // for

    return 0;
} // main
#endif

// end of checksum_crc32.c ...

