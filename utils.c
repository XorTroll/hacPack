#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "utils.h"
#include "filepath.h"
#include "sha.h"

/* Taken mostly from ctrtool. */
void memdump(FILE *f, const char *prefix, const void *data, size_t size) {
    uint8_t *p = (uint8_t *)data;

    unsigned int prefix_len = strlen(prefix);
    size_t offset = 0;
    int first = 1;

    while (size) {
        unsigned int max = 32;

        if (max > size) {
            max = size;
        }

        if (first) {
            fprintf(f, "%s", prefix);
            first = 0;
        } else {
            fprintf(f, "%*s", prefix_len, "");
        }

        for (unsigned int i = 0; i < max; i++) {
            fprintf(f, "%02X", p[offset++]);
        }

        fprintf(f, "\n");

        size -= max;
    }
}

// Code by NullModel https://github.com/ENCODE-DCC/kentUtils/commits?author=NullModel
char hexTab[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
'8', '9', 'a', 'b', 'c', 'd', 'e', 'f', };
void hexBinaryString(unsigned char *in, int inSize, char *out, int outSize)
/* Convert possibly long binary string to hex string.
 * Out size needs to be at least 2x inSize+1 */
{
assert(inSize * 2 +1 <= outSize);
while (--inSize >= 0)
    {
    unsigned char c = *in++;
    *out++ = hexTab[c>>4];
    *out++ = hexTab[c&0xf];
    }
*out = 0;
}

