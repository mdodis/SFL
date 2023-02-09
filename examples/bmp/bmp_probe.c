#include <stdio.h>
#include "sfl_bmp.h"

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "%s: Not a file.\n", argv[1]);
        return -1;
    }

    SflBmpReadContext ctx;
    sfl_bmp_read_context_cstd_init(&ctx);

    sfl_bmp_read_context_stdio_set_file(&ctx, f);

    SflBmpDesc desc;
    if (!sfl_bmp_read_context_probe(&ctx, &desc)) {
        fprintf(stderr, "%s: Invalid file format.\n", argv[1]);
        return -1;
    }

    printf("%-20s %d\n", "Width",  desc.width);
    printf("%-20s %d\n", "Height", desc.height);
    printf("%-20s %s\n", "Pixel Format", sfl_bmp_describe_pixel_format(desc.format));
    printf("%-20s %s\n", "Hdr", sfl_bmp_describe_hdr_id(desc.file_header_id));
    printf("%-20s %s\n", "Nfo", sfl_bmp_describe_nfo_id(desc.info_header_id));
    printf("%-20s %s\n", "Compression", sfl_bmp_describe_compression(desc.compression));
    printf("%-20s %u\n", "Data Size", desc.size);
    printf("%-20s %u\n", "Pitch", desc.pitch);
    printf("%-20s %s\n", "Flipped", (desc.attributes & SFL_BMP_ATTRIBUTE_FLIPPED) ? "Yes" : "No");
    printf("%-20s %s\n", "Palettized", (desc.attributes & SFL_BMP_ATTRIBUTE_PALETTIZED) ? "Yes" : "No");
    printf("%-20s 0x%08x\n", "R mask", desc.mask[0]);
    printf("%-20s 0x%08x\n", "G mask", desc.mask[1]);
    printf("%-20s 0x%08x\n", "B mask", desc.mask[2]);
    printf("%-20s 0x%08x\n", "A mask", desc.mask[3]);
    return 0;
}

#define SFL_BMP_IMPLEMENTATION
#include "sfl_bmp.h"
