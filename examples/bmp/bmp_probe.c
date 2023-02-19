/* SFL bmp_probe v0.1

This example uses sfl_bmp.h to probe a BMP image file, and output its relevant
attributes.

MIT License

Copyright (c) 2023 Michael Dodis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

USAGE
bmp_probe <path>

- path:  Relative or absolute path to file or directory

Example: bmp_probe ./myfile.bmp

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)
*/
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

    SflBmpContext ctx;
    sfl_bmp_cstd_init(&ctx);

    sfl_bmp_stdio_set_file(&ctx, f);

    SflBmpDesc desc;
    if (!sfl_bmp_probe(&ctx, &desc)) {
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
    printf("%-20s %u\n", "# Palette Entries", desc.num_table_entries);
    return 0;
}

#define SFL_BMP_IMPLEMENTATION
#include "sfl_bmp.h"
