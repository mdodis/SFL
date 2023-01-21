/*
===SFL BMP===
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

COMPILE TIME OPTIONS

#define SFL_BMP_IO_IMPLEMENTATION_STDIO 1
    Includes headers + implementation for C's stdlib
    Available functions:
      sfl_bmp_read_context_stdio_init
      sfl_bmp_read_context_stdio_set_file

#define SFML_BMP_CUSTOM_TYPES 0
    Disables include of <stdint.h> for custom type support, instead provided
    by the user. The types are:
    - SflBmpI32  (= int32_t by default)
    - SflBmpU32  (= uint32_t by default)
    - SflBmpI16  (= int16_t by default)
    - SflBmpU16  (= uint16_t by default)
    - SflBmpI8   (= int8_t by default)
    - SflBmpU8   (= uint8_t by default)
    - SflBmpReal (= float by default)

#define SFL_BMP_CEILF(x) (float)(ceil((double)x))
    Defines custom ceilf function

#define SFL_BMP_CUSTOM_PIXEL_FORMATS 0
    Enables the user to set value identifiers for the available pixel formats.
    This can be useful if you were going to translate the pixel format into your
    own.

#define SFL_BMP_ALWAYS_CONVERT 0
    Always converts formats even if they are generally supported by some APIs
    to either B8G8R8A8, B8G8R8X8, or R8G8B8A8

SUPPORT


CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)


TODO
- Correct memory usage (currently, nothing is freed after use)
    - Add sfl_bmp_desc_free to deallocate image data given mem implementation

BYTE ORDER / ENDIANESS
Components are typed in c array order (least significant address comes first).

*/
#ifndef SFL_BMP_H
#define SFL_BMP_H

/**
 *  Options
 */
#ifndef SFL_BMP_IO_IMPLEMENTATION_STDIO
#define SFL_BMP_IO_IMPLEMENTATION_STDIO 1
#endif

#ifndef SFL_BMP_CUSTOM_TYPES
#define SFL_BMP_CUSTOM_TYPES 0
#endif

#ifndef SFL_BMP_CUSTOM_PIXEL_FORMATS
#define SFL_BMP_CUSTOM_PIXEL_FORMATS 0
#endif

#ifndef SFL_BMP_ALWAYS_CONVERT
#define SFL_BMP_ALWAYS_CONVERT 0
#endif


#if !SFL_BMP_CUSTOM_TYPES
#include <stdint.h>
typedef int32_t  SflBmpI32;
typedef uint32_t SflBmpU32;
typedef int16_t  SflBmpI16;
typedef uint16_t SflBmpU16;
typedef int8_t   SflBmpI8;
typedef uint8_t  SflBmpU8;
typedef float    SflBmpReal;
#endif

#if !SFL_BMP_CUSTOM_PIXEL_FORMATS
#define SFL_BMP_PIXEL_FORMAT_B8G8R8A8 1
#define SFL_BMP_PIXEL_FORMAT_B8G8R8   2
#define SFL_BMP_PIXEL_FORMAT_B5G6R5   3
#define SFL_BMP_PIXEL_FORMAT_R8G8B8A8 4
#define SFL_BMP_PIXEL_FORMAT_B8G8R8X8 5
#endif

typedef enum {
    SFL_BMP_IO_SET = 0,
    SFL_BMP_IO_CUR = 1,
    SFL_BMP_IO_END = 2,
} SflBmpIOWhence;

typedef enum {
    /** Image Y axis starts from the bottom */
    SFL_BMP_ATTRIBUTE_FLIPPED = 1 << 0,
} SflBmpAttributes;

typedef struct {
    /** The image data */
    void *data;
    /** The width of the image (> 0) */
    SflBmpI32 width;
    /** The height of the image (> 0) */
    SflBmpI32 height;
    /** The amount of bytes per row */
    SflBmpU32 pitch;
    /** @see: SflBmpAttributes */
    SflBmpU32 attributes;
    /** Size (in bytes) */
    SflBmpU32 size;
    /** Pixel format of image data (always truecolor) */
    SflBmpU32 format;
} SflBmpDesc;

#define PROC_SFL_BMP_IO_READ(name) int name(void *usr, void *ptr, size_t size)
#define PROC_SFL_BMP_IO_SEEK(name) int name(void *usr, long offset, SflBmpIOWhence whence)
#define PROC_SFL_BMP_IO_TELL(name) long name(void *usr)

typedef PROC_SFL_BMP_IO_READ(ProcSflBmpIORead);
typedef PROC_SFL_BMP_IO_SEEK(ProcSflBmpIOSeek);
typedef PROC_SFL_BMP_IO_TELL(ProcSflBmpIOTell);

typedef struct {
    ProcSflBmpIORead *read;
    ProcSflBmpIOSeek *seek;
    ProcSflBmpIOTell *tell;
    void *usr;
} SflBmpIOImplementation;

#define PROC_SFL_BMP_MEMORY_ALLOCATE(name) void *name(void *usr, size_t size)
#define PROC_SFL_BMP_MEMORY_RELEASE(name)  void  name(void *usr, void *ptr)
typedef PROC_SFL_BMP_MEMORY_ALLOCATE(ProcSflBmpMemoryAllocate);
typedef PROC_SFL_BMP_MEMORY_RELEASE(ProcSflBmpMemoryRelease);

typedef struct {
    ProcSflBmpMemoryAllocate *allocate;
    ProcSflBmpMemoryRelease  *release;
    void *usr;
} SflBmpMemoryImplementation;

typedef struct {
    SflBmpIOImplementation     *io;
    SflBmpMemoryImplementation *mem;
} SflBmpReadContext;

extern void sfl_bmp_read_context_init(
    SflBmpReadContext *ctx, 
    SflBmpIOImplementation *io,
    SflBmpMemoryImplementation *mem);

extern void sfl_bmp_read_context_set_io_usr(
    SflBmpReadContext *ctx, 
    void *usr);
extern void sfl_bmp_read_context_set_memory_usr(
    SflBmpReadContext *ctx, 
    void *usr);

extern int sfl_bmp_read_context_decode(
    SflBmpReadContext *ctx,
    SflBmpDesc *desc);

extern const char *sfl_bmp_describe_pixel_format(int format);

/**
 * STDIO Implementation 
 */
#if SFL_BMP_IO_IMPLEMENTATION_STDIO
#include <stdio.h>
extern void sfl_bmp_read_context_stdio_init(SflBmpReadContext *ctx);
extern void sfl_bmp_read_context_stdio_set_file(
    SflBmpReadContext *ctx, 
    FILE *file);
#endif

#endif

#ifdef SFL_BMP_IMPLEMENTATION

#ifndef SFL_BMP_CEILF
#include <math.h>
#define SFL_BMP_CEILF(x) (float)(ceil((double)x))
#endif

#pragma pack(push, 1)
typedef struct {
    char hdr[2];
    SflBmpU32 file_size;
    SflBmpU16 reserved[2];
    SflBmpU32 offset;
} SflBmpFileHeader;

typedef struct {
    SflBmpU32 size;
    SflBmpU16 width;
    SflBmpU16 height;
    SflBmpU16 planes;
    SflBmpU16 bpp;
} SflBmpCoreHeader;

/**
 * BITMAPINFOHEADER (Windows NT, 3.1x or later)
 */
typedef struct {
    SflBmpU32 info_size;
    SflBmpI32 width;
    SflBmpI32 height;
    SflBmpU16 num_planes;
    SflBmpU16 bpp;
    SflBmpU32 compression;
    SflBmpU32 raw_size;
    SflBmpI32 horizontal_resolution;
    SflBmpI32 vertical_resolution;
    SflBmpU32 num_colors;
    SflBmpU32 num_important_colors;
} SflBmpInfoHeader040;

/**
 * BITMAPV5HEADER (Windows NT 5.0, 98 or later)
 */
typedef struct {
    SflBmpU32 size;
    SflBmpI32 width;
    SflBmpI32 height;
    SflBmpU16 planes;
    SflBmpU16 bpp;
    SflBmpU32 compression;
    SflBmpU32 raw_size;
    SflBmpI32 hres;
    SflBmpI32 vres;
    SflBmpU32 num_colors;
    SflBmpU32 num_important_colors;
    SflBmpU32 red_mask;
    SflBmpU32 green_mask;
    SflBmpU32 blue_mask;
    SflBmpU32 alpha_mask;
    SflBmpU32 color_space;
    SflBmpI32 endpoint_red[3];
    SflBmpI32 endpoint_green[3];
    SflBmpI32 endpoint_blue[3];
    SflBmpU32 gamma_red;
    SflBmpU32 gamma_green;
    SflBmpU32 gamma_blue;
    SflBmpU32 intent;
    SflBmpU32 profile_data_offset;
    SflBmpU32 profile_data_size;
    SflBmpU32 reserved;
} SflBmpInfoHeader124;

#pragma pack(pop)

typedef struct {
    /** 0 or count of different colors in palette */
    SflBmpU32 num_colors;
    SflBmpU32 compression;
    SflBmpU32 bpp;
    SflBmpI32 width;
    SflBmpI32 height;
    /** Offset into image data */
    SflBmpU32 offset;
    /** Offset into palette data */
    SflBmpU32 table_offset;

    SflBmpU32 *color_table;
    SflBmpU32 color_table_count;
    SflBmpU32 color_table_size;
    SflBmpU32 pitch;
    SflBmpU32 r_mask;
    SflBmpU32 g_mask;
    SflBmpU32 b_mask;
    SflBmpU32 a_mask;
} SflBmpDecodeSettings;

typedef struct {
    SflBmpU32 i_pitch;
    SflBmpU32 i_slice;
    SflBmpU32 i_rbits;
    SflBmpU32 i_gbits;
    SflBmpU32 i_bbits;
    SflBmpU32 i_abits;
} SflBmpConvertSettings;

enum {
    SFL_BMP_COMPRESSION_NONE      = 0,
    SFL_BMP_COMPRESSION_RLE8      = 1,
    SFL_BMP_COMPRESSION_RLE4      = 2,
    SFL_BMP_COMPRESSION_BITFIELDS = 3,
};

#define SFL_BMP_READ_STRUCT(T, ctx, dst) \
    (ctx->io->read(ctx->io->usr, (void*)dst, sizeof(T)) == 1)

#define SFL_BMP_READ(ctx, dst, size) \
    ctx->io->read(ctx->io->usr, dst, size)

#define SFL_BMP_SEEK(ctx, offset, whence) \
    ctx->io->seek(ctx->io->usr, offset, whence)

#define SFL_BMP_ALLOCATE(ctx, size) \
    ctx->mem->allocate(ctx->mem->usr, size)

#define SFL_BMP_RELEASE(ctx, ptr) \
    ctx->mem->release(ctx->mem->usr, ptr)

static int sfl_bmp_read_context_decode040(
    SflBmpReadContext *ctx,
    SflBmpFileHeader *file_header,
    SflBmpDesc *desc);

static int sfl_bmp_read_context_decode124(
    SflBmpReadContext *ctx,
    SflBmpFileHeader *file_header,
    SflBmpDesc *desc);

static int sfl_bmp_read_context_extract(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc);

static int sfl_bmp_read_context_extract_raw(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc);

static int sfl_bmp_read_context_extract_raw_convert(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpConvertSettings *convert_settings,
    SflBmpDesc *desc);

static int sfl_bmp_read_context_extract_paletted(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc);

static int sfl_bmp_read_context_extract_paletted_none(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc);

static inline SflBmpI32 sfl_bmp_bit_scan_forward(SflBmpU32 value) {
    SflBmpI32 result = 0;
    while ((value & 0x1) == 0 && result < 32) {
        value >>= 1;
        result++;
    }

    return result;
}

static inline SflBmpI32 sfl_bmp_bit_count(SflBmpU32 value) {
    SflBmpI32 result = 0;
    for (int i = 0; i < 32; ++i) {
        result += (value & (0x1 << i)) > 0;
    }
    return result;
}

static SflBmpU32 sfl_bmp_ipow(SflBmpU32 base, SflBmpU32 exponent) {
    if (exponent == 0)
        return 1;

    SflBmpU32 result = base;
    for (SflBmpU32 i = 1; i < exponent; ++i) {
        result *= base;
    }

    return result;
}

const char *sfl_bmp_describe_pixel_format(int format) {
    const char *desc_string = "Invalid Format";
    switch (format) {
        case SFL_BMP_PIXEL_FORMAT_B8G8R8A8: {
            desc_string = "B8G8R8A8";
        } break;

        case SFL_BMP_PIXEL_FORMAT_B8G8R8: {
            desc_string = "B8G8R8";
        } break;

        case SFL_BMP_PIXEL_FORMAT_B5G6R5: {
            desc_string = "B5G6R6";
        } break;

        case SFL_BMP_PIXEL_FORMAT_R8G8B8A8: {
            desc_string = "R8G8B8A8";
        } break;

        case SFL_BMP_PIXEL_FORMAT_B8G8R8X8: {
            desc_string = "B8G8R8X8";
        } break;
        default: break;
    }

    return desc_string;
}

void sfl_bmp_read_context_init(
    SflBmpReadContext *ctx, 
    SflBmpIOImplementation *io,
    SflBmpMemoryImplementation *mem)
{
    ctx->io = io;
    ctx->mem = mem;
    ctx->io->usr = 0;
    ctx->mem->usr = 0;
}


void sfl_bmp_read_context_set_io_usr(SflBmpReadContext *ctx, void *usr) {
    ctx->io->usr = usr;
}

void sfl_bmp_read_context_set_memory_usr(SflBmpReadContext *ctx, void *usr) {
    ctx->mem->usr = usr;
}

int sfl_bmp_read_context_decode(SflBmpReadContext *ctx, SflBmpDesc *desc) {
    /** Read in file header and determine file type */
    SflBmpFileHeader file_header;
    if (!SFL_BMP_READ_STRUCT(SflBmpFileHeader, ctx, &file_header)) {
        goto EXIT_ERROR;
    }

    if (file_header.hdr[0] != 'B' || file_header.hdr[1] != 'M')  {
        goto EXIT_ERROR;
    }

    /** Read/Determine info header version */
    SflBmpU32 info_header_size;

    if (!SFL_BMP_READ(ctx, &info_header_size, sizeof(info_header_size))) {
        goto EXIT_ERROR;
    }

    if (SFL_BMP_SEEK(ctx, -((long)sizeof(info_header_size)), SFL_BMP_IO_CUR)) {
        goto EXIT_ERROR;
    }

    switch (info_header_size) {

        case sizeof(SflBmpInfoHeader040): {
            return sfl_bmp_read_context_decode040(ctx, &file_header, desc);
        } break;

        case sizeof(SflBmpInfoHeader124): {
            return sfl_bmp_read_context_decode124(ctx, &file_header, desc);
        } break;

        default: {
            goto EXIT_ERROR;
        } break;
    }

    return 1;
EXIT_ERROR:
    return 0;
}


static int sfl_bmp_read_context_decode040(
    SflBmpReadContext *ctx,
    SflBmpFileHeader *file_header,
    SflBmpDesc *desc)
{
    SflBmpInfoHeader040 info_header;
    if (!SFL_BMP_READ_STRUCT(SflBmpInfoHeader040, ctx, &info_header)) {
        goto EXIT_ERROR;
    }
    SflBmpDecodeSettings settings;
    settings.num_colors = info_header.num_colors;
    settings.compression = info_header.compression;
    settings.bpp = info_header.bpp;
    settings.width = info_header.width;
    settings.height = info_header.height;
    settings.offset = file_header->offset;
    settings.table_offset = sizeof(SflBmpFileHeader) + sizeof(info_header);
    
    return sfl_bmp_read_context_extract(
        ctx,
        &settings,
        desc);
    
EXIT_ERROR:
    return 0;
}


static int sfl_bmp_read_context_decode124(
    SflBmpReadContext *ctx,
    SflBmpFileHeader *file_header,
    SflBmpDesc *desc)
{
    SflBmpInfoHeader124 info_header;
    if (!SFL_BMP_READ_STRUCT(SflBmpInfoHeader124, ctx, &info_header)) {
        goto EXIT_ERROR;
    }

    SflBmpDecodeSettings settings;
    settings.num_colors = info_header.num_colors;
    settings.compression = info_header.compression;
    settings.bpp = info_header.bpp;
    settings.width = info_header.width;
    settings.height = info_header.height;
    settings.offset = file_header->offset;
    settings.table_offset = sizeof(SflBmpFileHeader) + sizeof(info_header);
    
    settings.r_mask = info_header.red_mask;
    settings.g_mask = info_header.green_mask;
    settings.b_mask = info_header.blue_mask;
    settings.a_mask = info_header.alpha_mask;
    return sfl_bmp_read_context_extract(
        ctx,
        &settings,
        desc);
    
EXIT_ERROR:
    return 0;
}

static int sfl_bmp_read_context_extract(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc)
{
    desc->width = settings->width;
    desc->height = settings->height;
    desc->attributes = 0;
    
    SflBmpU32 pitch = (SflBmpU32)SFL_BMP_CEILF(
        (settings->bpp * settings->width) / 32.f) * 4;

    SflBmpU32 size = pitch * desc->height;
    desc->size = size;
    desc->pitch = pitch;

    /** In BMP files, positive height means the image is flipped. */
    if (settings->height > 0) {
        desc->attributes |= SFL_BMP_ATTRIBUTE_FLIPPED;
    } else {
        desc->height = -desc->height;
    }

    switch (settings->bpp) {
        case 32: {
            if (settings->a_mask == 0) {
                desc->format = SFL_BMP_PIXEL_FORMAT_B8G8R8X8;
            } else {
                desc->format = SFL_BMP_PIXEL_FORMAT_B8G8R8A8;
            }
            return sfl_bmp_read_context_extract_raw(ctx, settings, desc);
        } break;

        case 24: {
            desc->format = SFL_BMP_PIXEL_FORMAT_B8G8R8;
            SflBmpConvertSettings convert_settings;
            convert_settings.i_rbits = settings->r_mask;
            convert_settings.i_gbits = settings->g_mask;
            convert_settings.i_bbits = settings->b_mask;
            convert_settings.i_abits = settings->a_mask;
            convert_settings.i_pitch = desc->pitch;
            convert_settings.i_slice = 3;
            return sfl_bmp_read_context_extract_raw_convert(
                ctx, 
                settings,
                &convert_settings,
                desc);
        } break;

        case 16: {
            desc->format = SFL_BMP_PIXEL_FORMAT_B5G6R5;
#if SFL_BMP_ALWAYS_CONVERT
            SflBmpConvertSettings convert_settings;
            convert_settings.i_rbits = settings->r_mask;
            convert_settings.i_gbits = settings->g_mask;
            convert_settings.i_bbits = settings->b_mask;
            convert_settings.i_abits = settings->a_mask;
            convert_settings.i_pitch = desc->pitch;
            convert_settings.i_slice = 2;
            return sfl_bmp_read_context_extract_raw_convert(
                ctx, 
                settings,
                &convert_settings,
                desc);
#else
            return sfl_bmp_read_context_extract_raw(ctx, settings, desc);
#endif
        } break;

        case 4: {
            return sfl_bmp_read_context_extract_paletted(ctx, settings, desc);
        } break;

        default: {
            return 0;
        } break;
    }
}

static int sfl_bmp_read_context_extract_raw(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc)
{
    SFL_BMP_SEEK(ctx, settings->offset, SFL_BMP_IO_SET);
    desc->data = SFL_BMP_ALLOCATE(ctx, desc->size);
    if (!SFL_BMP_READ(ctx, desc->data, desc->size)) {
        return 0;
    }
    return 1;
}

static int sfl_bmp_read_context_extract_raw_convert(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpConvertSettings *convert_settings,
    SflBmpDesc *desc)
{
    desc->format = SFL_BMP_PIXEL_FORMAT_R8G8B8A8;
    desc->pitch = desc->width * sizeof(SflBmpU32);
    desc->size = desc->pitch * desc->height;
    desc->data = SFL_BMP_ALLOCATE(ctx, desc->size);
    SflBmpU32 *data = (SflBmpU32*)desc->data;

    /* Amounts to shift each pixel value */
    const SflBmpI32 rs = sfl_bmp_bit_scan_forward(convert_settings->i_rbits);
    const SflBmpI32 gs = sfl_bmp_bit_scan_forward(convert_settings->i_gbits);
    const SflBmpI32 bs = sfl_bmp_bit_scan_forward(convert_settings->i_bbits);
    const SflBmpI32 as = sfl_bmp_bit_scan_forward(convert_settings->i_abits);

    /* Amount of bits representing component */
    const SflBmpI32 rc = sfl_bmp_bit_count(convert_settings->i_rbits);
    const SflBmpI32 gc = sfl_bmp_bit_count(convert_settings->i_gbits);
    const SflBmpI32 bc = sfl_bmp_bit_count(convert_settings->i_bbits);
    const SflBmpI32 ac = sfl_bmp_bit_count(convert_settings->i_abits);

    /* Max value of each component */
    const SflBmpReal mr = (SflBmpReal)(sfl_bmp_ipow(2, rc) - 1);
    const SflBmpReal mg = (SflBmpReal)(sfl_bmp_ipow(2, gc) - 1);
    const SflBmpReal mb = (SflBmpReal)(sfl_bmp_ipow(2, bc) - 1);
    const SflBmpReal ma = (SflBmpReal)(sfl_bmp_ipow(2, ac) - 1);

    const SflBmpU32 skip = convert_settings->i_pitch - 
        (desc->width * convert_settings->i_slice);

    const int is_flipped = settings->height > 0 ? 1 : 0;
    desc->attributes &= ~SFL_BMP_ATTRIBUTE_FLIPPED;
    SFL_BMP_SEEK(ctx, settings->offset, SFL_BMP_IO_SET);
 
    for (int y = 0; y < desc->height; ++y) {

        for (int x = 0; x < desc->width; ++x) {
            SflBmpU32 pixel;
            if (!SFL_BMP_READ(ctx, &pixel, convert_settings->i_slice)) {
                goto EXIT_ERROR;
            }
            
            SflBmpReal r = (SflBmpReal)((pixel & convert_settings->i_rbits) >> rs);
            SflBmpReal g = (SflBmpReal)((pixel & convert_settings->i_gbits) >> gs);
            SflBmpReal b = (SflBmpReal)((pixel & convert_settings->i_bbits) >> bs);
            SflBmpReal a = (SflBmpReal)((pixel & convert_settings->i_abits) >> as);

            /* Ternary op handles case where any of these components is zero */
            SflBmpU32 cr = (rc != 0) ? (SflBmpU32)SFL_BMP_CEILF((r / mr) * 255) : 0;
            SflBmpU32 cg = (gc != 0) ? (SflBmpU32)SFL_BMP_CEILF((g / mg) * 255) : 0;
            SflBmpU32 cb = (bc != 0) ? (SflBmpU32)SFL_BMP_CEILF((b / mb) * 255) : 0;
            SflBmpU32 ca = (ac != 0) ? (SflBmpU32)SFL_BMP_CEILF((a / ma) * 255) : 0;

            if (convert_settings->i_abits == 0) {
                ca = 0xFF;
            }

            SflBmpU32 pixel32 = 
                (cr <<  0) |
                (cg <<  8) |
                (cb << 16) |
                (ca << 24);

            if (is_flipped) {
                data[(desc->height - y - 1) * desc->width + x] = pixel32;
            } else {
                data[y * desc->width + x] = pixel32;
            }
        }

        if (SFL_BMP_SEEK(ctx, (long)skip, SFL_BMP_IO_CUR)) {
            goto EXIT_ERROR;
        }
    }

    return 1;
EXIT_ERROR:
    return 0;
}

static int sfl_bmp_read_context_extract_paletted(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc)
{
    SflBmpU32 color_table_count;
    if (settings->num_colors == 0) {
        color_table_count = sfl_bmp_ipow(2, settings->bpp);
    } else {
        color_table_count = settings->num_colors;
    }
    const SflBmpU32 color_table_size = color_table_count * sizeof(SflBmpU32);

    const SflBmpU32 converted_size = 
        desc->height * 
        desc->width * 
        sizeof(SflBmpU32);

    settings->pitch = desc->pitch;

    desc->format = SFL_BMP_PIXEL_FORMAT_B8G8R8A8;
    desc->pitch = desc->width * sizeof(SflBmpU32);
    desc->attributes = 0;

    SflBmpU32 *color_table = (SflBmpU32*)SFL_BMP_ALLOCATE(ctx, color_table_size);
    if (!color_table) {
        return 0;
    }

    desc->data = SFL_BMP_ALLOCATE(ctx, converted_size);
    if (!desc->data) {
        return 0;
    }

    SFL_BMP_SEEK(ctx, settings->table_offset, SFL_BMP_IO_SET);

    if (!SFL_BMP_READ(ctx, color_table, color_table_size)) {
        return 0;
    }
    settings->color_table = color_table;
    settings->color_table_count = color_table_count;
    settings->color_table_size = color_table_size;

    switch (settings->compression) {
        case SFL_BMP_COMPRESSION_NONE: {
            return sfl_bmp_read_context_extract_paletted_none(
                ctx, 
                settings, 
                desc);
        } break;

        default: {
            return 0;
        } break;
    }
}

static int sfl_bmp_read_context_extract_paletted_none(
    SflBmpReadContext *ctx,
    SflBmpDecodeSettings *settings,
    SflBmpDesc *desc)
{
    const int is_flipped = settings->height > 0 ? 1 : 0;
    SflBmpU32 curr_offset = settings->offset;

    SFL_BMP_SEEK(ctx, settings->offset, SFL_BMP_IO_SET);
    
    SflBmpU32 *out_pixels = (SflBmpU32*)desc->data;

    for (SflBmpI32 c = 0; c < desc->height; ++c) {
        SflBmpI32 y = is_flipped
            ? desc->height - 1 - c
            : c;
        SflBmpU32 curr_row = curr_offset;
        SflBmpU32 row_end = curr_offset + settings->pitch;
        SflBmpI32 x = 0;

        while (1) {
            if (x == desc->width) {
                curr_row = row_end;
                break;
            }

            SflBmpU8 code;
            if (!SFL_BMP_READ(ctx, &code, 1)) {
                return 0;
            }

            const SflBmpU8 left_pixel  = code >> 4;
            const SflBmpU8 right_pixel = code & 0x0f;

            SflBmpU32 image_offset = desc->width * y + x;
            SflBmpU32 *data = out_pixels + image_offset;
            data[0] = settings->color_table[left_pixel];
            
            if (x < desc->width) {
                data[1] = settings->color_table[right_pixel];
                x++;
            }
        }
    }

    return 1;
}

#if SFL_BMP_IO_IMPLEMENTATION_STDIO
#include <stdio.h>
#include <stdlib.h>

static PROC_SFL_BMP_IO_READ(sfl_bmp_stdlib_read) {
    FILE *f = (FILE*)usr;
    return fread(ptr, size, 1, f) == 1;
}

static PROC_SFL_BMP_IO_SEEK(sfl_bmp_stdlib_seek) {
    int whence_stdio = -1;

    switch (whence) {
        case SFL_BMP_IO_SET: whence_stdio = SEEK_SET; break;
        case SFL_BMP_IO_CUR: whence_stdio = SEEK_CUR; break;
        case SFL_BMP_IO_END: whence_stdio = SEEK_END; break;
        default: break;
    }


    FILE *f = (FILE*)usr;
    return fseek(f, offset, whence_stdio);
}

static PROC_SFL_BMP_IO_TELL(sfl_bmp_stdlib_tell) {
    FILE *f = (FILE*)usr;
    return ftell(f);
}

static PROC_SFL_BMP_MEMORY_ALLOCATE(sfl_bmp_stdlib_allocate) {
    return malloc(size);
}

static PROC_SFL_BMP_MEMORY_RELEASE(sfl_bmp_stdlib_release) {
    free(ptr);
}

static SflBmpIOImplementation SflBmp_IO_STDLIB = {
    sfl_bmp_stdlib_read,
    sfl_bmp_stdlib_seek,
    sfl_bmp_stdlib_tell,
};

static SflBmpMemoryImplementation SflBmp_Memory_STDLIB = {
    sfl_bmp_stdlib_allocate,
    sfl_bmp_stdlib_release,
};

void sfl_bmp_read_context_stdio_init(SflBmpReadContext *ctx) {
    sfl_bmp_read_context_init(ctx, &SflBmp_IO_STDLIB, &SflBmp_Memory_STDLIB);
}

void sfl_bmp_read_context_stdio_set_file(
    SflBmpReadContext *ctx, 
    FILE *file)
{
    sfl_bmp_read_context_set_io_usr(ctx, (void*)file);
}

#endif

#undef SFL_BMP_READ_STRUCT
#undef SFL_BMP_READ
#undef SFL_BMP_SEEK
#endif