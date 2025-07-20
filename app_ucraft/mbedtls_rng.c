#include <stdlib.h>
#include <string.h>

extern int bl_rand(void); //this function was used in the sdk mbedtls implementation to get RNG, may not be high entropy

int mbedtls_hardware_poll(void *data,
                          unsigned char *output, size_t len, size_t *olen)
{
    (void) data; // data is unused, may be NULL

    size_t bytes_written = 0;
    while (bytes_written + sizeof(int) <= len) {
        int rnd = bl_rand();
        memcpy(output + bytes_written, &rnd, sizeof(int));
        bytes_written += sizeof(int);
    }

    if (bytes_written < len) {
        int rnd = bl_rand();
        memcpy(output + bytes_written, &rnd, len - bytes_written);
        bytes_written = len;
    }

    *olen = bytes_written;
    return 0;
}