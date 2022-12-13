#define SFL_BMP_IMPLEMENTATION
#include "sfl_bmp.h"
#include <stdio.h>

/**
 * Invocation: <executable> <image_to_test>
 */
int main(int argc, char const *argv[]) {
    if (argc < 2) {
        puts("Invalid number of arguments");
        return -1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == 0) {
        puts("File doesn't exist");
        return -1;
    }

    SflBmpReadContext read_context;
    sfl_bmp_read_context_init_stdio(&read_context, (void*)f);

    SflBmpDesc desc;
    if (sfl_bmp_read_context_decode(&read_context, &desc) == 0) {
        perror("Failed.");
        return -1;
    }

    return 0;
}
