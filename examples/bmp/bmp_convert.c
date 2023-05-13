/* SFL bmp_probe v0.1

This example uses sfl_bmp.h to read a BMP image file, and convert it to another
format of a BMP file

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
bmp_convert <path_to_read> <path_to_write>

- path_to_read:   Relative or absolute path to file or directory to read
- path_to_write:  Relative or absolute path to file or directory to write

Example: bmp_convert ./input.bmp ./output.bmp

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)
*/
#include <stdio.h>

#include "sfl_bmp.h"

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    SflBmpContext read_ctx, write_ctx;
    FILE *        inf, *outf;

    sfl_bmp_cstd_init(&read_ctx);
    sfl_bmp_cstd_init(&write_ctx);

    inf = fopen(argv[1], "rb");
    fseek(inf, 0, SEEK_END);
    printf("File size %ld\n", ftell(inf));
    fseek(inf, 0, SEEK_SET);
    sfl_bmp_stdio_set_file(&read_ctx, inf);

    SflBmpDesc in_desc;
    if (!sfl_bmp_probe(&read_ctx, &in_desc)) {
        return -1;
    }

    SflBmpDesc out_desc     = {0};
    out_desc.format         = SFL_BMP_PIXEL_FORMAT_B8G8R8X8;
    out_desc.compression    = SFL_BMP_COMPRESSION_NONE;
    out_desc.file_header_id = SFL_BMP_HDR_ID_BM;
    out_desc.info_header_id = SFL_BMP_NFO_ID_V5;

    outf = fopen(argv[2], "wb");
    sfl_bmp_stdio_set_file(&write_ctx, outf);
    sfl_bmp_encode(&write_ctx, &in_desc, &read_ctx.io, &out_desc);

    fclose(outf);
    fclose(inf);
    return 0;
}

#define SFL_BMP_IMPLEMENTATION
#include "sfl_bmp.h"
