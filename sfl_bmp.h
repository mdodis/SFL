/* sfl_bmp.h v0.1
A utility library for reading and writing BMP files for all recorded versions of
the format. It's mainly my personal case study on maintaining a relatively old
file format, and the difficulties that come with that.

NOTICE
If you just want to load and display image files for your application, then you
are probably better off with stb_image.h

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
    Includes headers + implementation for C's stdlib for IO
    Available functions:
      sfl_bmp_stdio_init
      sfl_bmp_stdio_set_file
      sfl_bmp_stdio_get_implementation

#define SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB 1
    Includes headers + implementation for C's stdlib for memory allocations
    Available functions:
      sfl_bmp_stdlib_init
      sfl_bmp_stdlib_get_implementation

> If both SFL_BMP_IO_IMPLEMENTATION_STDIO &&
> SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB
> are enabled, then there's another available function:
      sfl_bmp_cstd_init

#define SFL_BMP_IO_IMPLEMENTATION_WINAPI 0
    Includes implementation for Win API
    Available functions:
      sfl_bmp_winapi_io_init
      sfl_bmp_winapi_io_set_file

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

#define SFL_BMP_ALWAYS_CONVERT 1
    Always converts formats even if they are generally supported by some APIs
    to R8G8B8A8

SUPPORT
| Type                  | Header             | Supported |
| --------------------- | ------------------ | --------- |
| Windows 2.0, OS/2 1.x | BITMAPCOREHEADER   | No        |
| OS/2 v2               | OS22XBITMAPHEADER  | No        |
| OS/2 v2 Variant       | OS22XBITMAPHEADER  | No        |
| Windows NT, 3.1x      | BITMAPINFOHEADER   | Partially |
| Undocumented          | BITMAPV2INFOHEADER | No        |
| Adobe                 | BITMAPV3INFOHEADER | No        |
| Windows NT 4, 95      | BITMAPV4HEADER     | No        |
| Windows NT 5, 98      | BITMAPV5HEADER     | Partially |

Encodings
| Type                  | Supported |
| --------------------- | --------- |
| Paletted RLE2         | No        |

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)


TODO
- Correct memory usage (currently, nothing is freed after use)
    - Add sfl_bmp_desc_free to deallocate image data given mem implementation
- sfl_bmp_deinit

BYTE ORDER / ENDIANESS
Components are typed in c array order (least significant address comes first).

REFERENCES
- http://justsolve.archiveteam.org/wiki/BMP
- https://archive.org/details/OS2BBS
- https://www.fileformat.info/format/os2bmp/egff.htm
- https://en.wikipedia.org/wiki/BMP_file_format
- https://entropymine.com/jason/bmpsuite/bmpsuite/html/bmpsuite.html

*/
#ifndef SFL_BMP_H
#define SFL_BMP_H

/**
 *  Options
 */
#ifndef SFL_BMP_IO_IMPLEMENTATION_STDIO
#define SFL_BMP_IO_IMPLEMENTATION_STDIO 1
#endif

#ifndef SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB
#define SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB 1
#endif

#ifndef SFL_BMP_IO_IMPLEMENTATION_WINAPI
#define SFL_BMP_IO_IMPLEMENTATION_WINAPI 0
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
#include <stddef.h>
#include <stdint.h>
typedef size_t   SflBmpUSize;
typedef int32_t  SflBmpI32;
typedef uint32_t SflBmpU32;
typedef int16_t  SflBmpI16;
typedef uint16_t SflBmpU16;
typedef int8_t   SflBmpI8;
typedef uint8_t  SflBmpU8;
typedef float    SflBmpReal;
#endif

#if !SFL_BMP_CUSTOM_PIXEL_FORMATS
#define SFL_BMP_PIXEL_FORMAT_INVALID -1
#define SFL_BMP_PIXEL_FORMAT_UNRECOGNIZED 0
#define SFL_BMP_PIXEL_FORMAT_B8G8R8A8 1
#define SFL_BMP_PIXEL_FORMAT_B8G8R8 2
#define SFL_BMP_PIXEL_FORMAT_B5G6R5 3
#define SFL_BMP_PIXEL_FORMAT_R8G8B8A8 4
#define SFL_BMP_PIXEL_FORMAT_B8G8R8X8 5
#define SFL_BMP_PIXEL_FORMAT_B5G5R5X1 6
#endif

typedef enum
{
    SFL_BMP_IO_SET = 0,
    SFL_BMP_IO_CUR = 1,
    SFL_BMP_IO_END = 2,
} SflBmpIOWhence;

typedef enum
{
    /** Image Y axis starts from the bottom */
    SFL_BMP_ATTRIBUTE_FLIPPED    = 1 << 0,
    /** Uses color table */
    SFL_BMP_ATTRIBUTE_PALETTIZED = 1 << 1,
} SflBmpAttributes;

typedef enum
{
    SFL_BMP_HDR_ID_NA = 0,
    SFL_BMP_HDR_ID_BM = 1,
    SFL_BMP_HDR_ID_BA = 2,
    SFL_BMP_HDR_ID_CI = 3,
    SFL_BMP_HDR_ID_CP = 4,
    SFL_BMP_HDR_ID_IC = 5,
    SFL_BMP_HDR_ID_PT = 6,
} SflBmpHdrID;

typedef enum
{
    SFL_BMP_NFO_ID_NA      = 0, /** Unrecognizble Info header type */
    SFL_BMP_NFO_ID_CORE    = 1, /** ( 12) BITMAPCOREHEADER */
    SFL_BMP_NFO_ID_OS22_V1 = 2, /** ( 64) OS22XBITMAPHEADER */
    SFL_BMP_NFO_ID_OS22_V2 = 3, /** ( 16) OS22XBITMAPHEADER */
    SFL_BMP_NFO_ID_V1      = 4, /** ( 40) BITMAPINFOHEADER */
    SFL_BMP_NFO_ID_V2      = 5, /** ( 52) BITMAPV2INFOHEADER */
    SFL_BMP_NFO_ID_V3      = 6, /** ( 56) BITMAPV3INFOHEADER */
    SFL_BMP_NFO_ID_V4      = 7, /** (108) BITMAPV4HEADER */
    SFL_BMP_NFO_ID_V5      = 8, /** (124) BITMAPV5HEADER */
    SFL_BMP_NFO_ID_COUNT
} SflBmpNfoID;

typedef enum
{
    SFL_BMP_COMPRESSION_NONE      = 0,
    SFL_BMP_COMPRESSION_RLE8      = 1,
    SFL_BMP_COMPRESSION_RLE4      = 2,
    SFL_BMP_COMPRESSION_BITFIELDS = 3,
} SflBmpCompression;

typedef enum
{
    SFL_BMP_COLORSPACE_CALIBRATED_RGB   = 0,
    SFL_BMP_COLORSPACE_SRGB             = 'sRGB',
    SFL_BMP_COLORSPACE_WINDOWS          = 'Win ',
    SFL_BMP_COLORSPACE_PROFILE_LINKED   = 'LINK',
    SFL_BMP_COLORSPACE_PROFILE_EMBEDDED = 'MBED',
} SflBmpColorspace;

typedef enum
{
    SFL_BMP_INTENT_LCS_GM_BUSINESS         = 0x00000001L,
    SFL_BMP_INTENT_LCS_GM_GRAPHICS         = 0x00000002L,
    SFL_BMP_INTENT_LCS_GM_IMAGES           = 0x00000004L,
    SFL_BMP_INTENT_LCS_GM_ABS_COLORIMETRIC = 0x00000008L,
} SflBmpRenderingIntent;

typedef struct {
    /** The image data */
    void*       data;
    /** The palette data */
    void*       palette_data;
    /** The width of the image */
    SflBmpU32   width;
    /** The height of the image */
    SflBmpU32   height;
    /* The width, in pixels per meter*/
    SflBmpU32   physical_width;
    /* The width, in pixels per meter*/
    SflBmpU32   physical_height;
    /** The amount of bytes per row/scan-line */
    SflBmpU32   pitch;
    /** The amount of bytes per pixel */
    SflBmpU32   slice;
    /** @see: SflBmpAttributes */
    int         attributes;
    /** Size (in bytes) */
    SflBmpU32   size;
    /** Pixel format of image data */
    SflBmpU32   format;
    /** File header id, @see SflBmpHdrID */
    SflBmpHdrID file_header_id;
    /** Info header id, @see SflBmpNfoID */
    SflBmpNfoID info_header_id;
    /** Compression method, @see SflBmpCompressionMethod */
    int         compression;
    /** Offset into the pixel data */
    SflBmpU32   offset;
    /** Offset into table data */
    SflBmpU32   table_offset;
    /** Number of color entries into table */
    SflBmpU32   num_table_entries;
    /** Table entry size, in bytes */
    SflBmpU32   table_entry_size;
    /** Color masks (r, g, b, a)*/
    SflBmpU32   mask[4];
} SflBmpDesc;

#define PROC_SFL_BMP_IO_READ(name) \
    int name(void* usr, void* ptr, SflBmpUSize size)
#define PROC_SFL_BMP_IO_SEEK(name) \
    int name(void* usr, long offset, SflBmpIOWhence whence)
#define PROC_SFL_BMP_IO_TELL(name) long name(void* usr)
#define PROC_SFL_BMP_IO_WRITE(name) \
    int name(void* usr, void* buf, SflBmpUSize size)

typedef PROC_SFL_BMP_IO_READ(ProcSflBmpIORead);
typedef PROC_SFL_BMP_IO_SEEK(ProcSflBmpIOSeek);
typedef PROC_SFL_BMP_IO_TELL(ProcSflBmpIOTell);
typedef PROC_SFL_BMP_IO_WRITE(ProcSflBmpIOWrite);

typedef struct {
    ProcSflBmpIORead*  read;
    ProcSflBmpIOWrite* write;
    ProcSflBmpIOSeek*  seek;
    ProcSflBmpIOTell*  tell;
    void*              usr;
} SflBmpIOImplementation;

#define PROC_SFL_BMP_MEMORY_ALLOCATE(name) \
    void* name(void* usr, SflBmpUSize size)
#define PROC_SFL_BMP_MEMORY_RELEASE(name) void name(void* usr, void* ptr)
typedef PROC_SFL_BMP_MEMORY_ALLOCATE(ProcSflBmpMemoryAllocate);
typedef PROC_SFL_BMP_MEMORY_RELEASE(ProcSflBmpMemoryRelease);

typedef struct {
    ProcSflBmpMemoryAllocate* allocate;
    ProcSflBmpMemoryRelease*  release;
    void*                     usr;
} SflBmpMemoryImplementation;

typedef struct {
    SflBmpIOImplementation      io;
    SflBmpMemoryImplementation* mem;
} SflBmpContext;

extern void sfl_bmp_init(
    SflBmpContext*              ctx,
    SflBmpIOImplementation*     io,
    SflBmpMemoryImplementation* mem);

extern void sfl_bmp_set_io_usr(SflBmpContext* ctx, void* usr);
extern void sfl_bmp_set_memory_usr(SflBmpContext* ctx, void* usr);

/**
 * Returns description of the file
 * @param ctx  The read context
 * @param desc The descriptor to write to
 */
extern int sfl_bmp_probe(SflBmpContext* ctx, SflBmpDesc* desc);

extern int sfl_bmp_decode(SflBmpContext* ctx, SflBmpDesc* desc);

extern int sfl_bmp_encode(
    SflBmpContext*          ctx,
    SflBmpDesc*             in_desc,
    SflBmpIOImplementation* in_io,
    SflBmpDesc*             out_desc);

extern const char* sfl_bmp_describe_pixel_format(int format);
extern const char* sfl_bmp_describe_hdr_id(SflBmpHdrID id);
extern const char* sfl_bmp_describe_nfo_id(SflBmpNfoID id);
extern const char* sfl_bmp_describe_compression(SflBmpCompression compression);
/**
 * STDIO Implementation
 */
#if SFL_BMP_IO_IMPLEMENTATION_STDIO
#include <stdio.h>
extern void sfl_bmp_stdio_init(
    SflBmpContext* ctx, SflBmpMemoryImplementation* memory);
extern void sfl_bmp_stdio_set_file(SflBmpContext* ctx, FILE* file);
extern SflBmpIOImplementation* sfl_bmp_stdio_get_implementation(void);
#endif

/**
 * STDLIB Implementation
 */
#if SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB
extern void sfl_bmp_stdlib_init(SflBmpContext* ctx, SflBmpIOImplementation* io);
extern SflBmpMemoryImplementation* sfl_bmp_stdlib_get_implementation(void);
#endif

#if SFL_BMP_IO_IMPLEMENTATION_STDIO && SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB
extern void sfl_bmp_cstd_init(SflBmpContext* ctx);
#endif

#if SFL_BMP_IO_IMPLEMENTATION_WINAPI
#include <Windows.h>
extern void sfl_bmp_winapi_io_init(
    SflBmpContext* ctx, SflBmpMemoryImplementation* memory);

extern void sfl_bmp_winapi_io_set_file(SflBmpContext* ctx, HANDLE* file);

extern SflBmpIOImplementation* sfl_bmp_winapi_io_get_implementation(void);
#endif

#endif

#ifdef SFL_BMP_IMPLEMENTATION
#include <string.h>

typedef struct {
    unsigned char* buf;
    SflBmpUSize    curr;
    SflBmpUSize    len;
} SflBmpIOImplementationMemory;

static PROC_SFL_BMP_IO_READ(sfl_bmp_memory_read)
{
    SflBmpIOImplementationMemory* mem = (SflBmpIOImplementationMemory*)usr;
    if ((mem->curr + size) <= mem->len) {
        memcpy(ptr, mem->buf + mem->curr, mem->len);
        mem->curr += size;
        return 1;
    } else {
        return 0;
    }
}

static PROC_SFL_BMP_IO_WRITE(sfl_bmp_memory_write)
{
    SflBmpIOImplementationMemory* mem = (SflBmpIOImplementationMemory*)usr;
    if ((mem->curr + size) <= mem->len) {
        memcpy(mem->buf + mem->curr, buf, size);
        return 1;
    } else {
        return 0;
    }
}

static PROC_SFL_BMP_IO_SEEK(sfl_bmp_memory_seek)
{
    SflBmpIOImplementationMemory* mem = (SflBmpIOImplementationMemory*)usr;
    switch (whence) {
        case SFL_BMP_IO_SET: {
            if (offset < 0) {
                return -1;
            }

            if (((SflBmpUSize)offset) < mem->len) {
                mem->curr = (SflBmpUSize)offset;
                return 0;
            } else {
                return -1;
            }
        } break;

        case SFL_BMP_IO_CUR: {
            if (offset < 0) {
                SflBmpUSize uoffset = (SflBmpUSize)(-offset);

                if (uoffset > mem->curr) {
                    return -1;
                }

                mem->curr -= uoffset;
                return 0;
            } else {
                SflBmpUSize uoffset = (SflBmpUSize)(+offset);

                if ((mem->curr + uoffset) > mem->len) {
                    return -1;
                }

                mem->curr += uoffset;
                return 0;
            }
        } break;

        case SFL_BMP_IO_END: {
            if (offset > 0) {
                return -1;
            }

            SflBmpUSize uoffset = (SflBmpUSize)(-offset);

            if (uoffset > mem->len) {
                return -1;
            }

            mem->curr = mem->len - uoffset;
            return 0;

        } break;
    }

    return -1;
}

static PROC_SFL_BMP_IO_TELL(sfl_bmp_memory_tell)
{
    SflBmpIOImplementationMemory* mem = (SflBmpIOImplementationMemory*)usr;
    return mem->curr;
}

static SflBmpIOImplementation SflBmp_IO_Memory = {
    sfl_bmp_memory_read,
    sfl_bmp_memory_write,
    sfl_bmp_memory_seek,
    sfl_bmp_memory_tell,
};

static void sfl_bmp_memory_init(
    SflBmpIOImplementationMemory* ctx, void* buf, SflBmpUSize len)
{
    ctx->buf  = (unsigned char*)buf;
    ctx->len  = len;
    ctx->curr = 0;
}

static int sfl_bmp__convert(
    SflBmpDesc*             in,
    SflBmpIOImplementation* in_io,
    SflBmpDesc*             out,
    SflBmpIOImplementation* out_io);

static int sfl_bmp__next_block(
    SflBmpDesc*             desc,
    SflBmpIOImplementation* io,
    void*                   buf,
    SflBmpUSize             buf_size,
    SflBmpUSize             offset);

#ifndef SFL_BMP_CEILF
#include <math.h>
#define SFL_BMP_CEILF(x) (float)(ceil((double)x))
#endif

#ifndef SFL_BMP_UNIMPLEMENTED
#define SFL_BMP_UNIMPLEMENTED()
#endif

#pragma pack(push, 1)
typedef struct {
    char      hdr[2];
    SflBmpU32 file_size;
    SflBmpU16 reserved[2];
    /** Offset to the image data */
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
} SflBmpInfoHeader040;

/**
 * OS22XBITMAPHEADER (OS/2 v2 Variant)
 */
typedef struct {
    SflBmpCoreHeader core;
    SflBmpU32        compression;
    SflBmpU32        raw_size;
    SflBmpI32        hres;
    SflBmpI32        vres;
    SflBmpU32        num_colors;
    SflBmpU32        num_important_colors;
    SflBmpU16        units;
    SflBmpU16        reserved;
    SflBmpU16        recording;
    SflBmpU16        rendering;
    SflBmpU16        size1;
    SflBmpU16        size2;
    SflBmpU16        color_encoding;
    SflBmpU16        identifier;
} SflBmpInfoHeader064;

/**
 * BITMAPV5HEADER (Windows NT 5.0, 98 or later)
 */
typedef struct {
    SflBmpU32 size;
    SflBmpI32 width;
    SflBmpI32 height;
    SflBmpU16 planes;
    SflBmpU16 bpp;
    /** Specifies bitmap compression method. Valid when used with 16, 32 bpp */
    SflBmpU32 compression;
    /** Size of image in bytes. Can be zero for SFL_BMP_COMPRESSION_RGB */
    SflBmpU32 raw_size;
    /** Horizontal resolution in pixels per meter. */
    SflBmpI32 hres;
    /** Vertical resolution in pixels per meter. */
    SflBmpI32 vres;
    /** Count of color entries in table */
    SflBmpU32 num_colors;
    /** Count of required color entries in table. If zero, then all are
     * required. */
    SflBmpU32 num_important_colors;
    /** Red color mask. Valid only if using SFL_BMP_COMPRESSION_BITFIELDS */
    SflBmpU32 red_mask;
    /** Green color mask. Valid only if using SFL_BMP_COMPRESSION_BITFIELDS */
    SflBmpU32 green_mask;
    /** Blue color mask. Valid only if using SFL_BMP_COMPRESSION_BITFIELDS */
    SflBmpU32 blue_mask;
    /** Alpha color mask */
    SflBmpU32 alpha_mask;
    /** The color space of the bitmap */
    SflBmpU32 color_space;
    /** Red endpoint for logical color space */
    SflBmpI32 endpoint_red[3];
    /** Green endpoint for logical color space */
    SflBmpI32 endpoint_green[3];
    /** Blue endpoint for logical color space */
    SflBmpI32 endpoint_blue[3];
    /** Toned response curve for red */
    SflBmpU32 gamma_red;
    /** Toned response curve for green */
    SflBmpU32 gamma_green;
    /** Toned response curve for blue */
    SflBmpU32 gamma_blue;
    /** Rendering intent */
    SflBmpU32 intent;
    /** Offset to start of profile data */
    SflBmpU32 profile_data_offset;
    /** Size in bytes of embedded profile data */
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

    SflBmpU32* color_table;
    SflBmpU32  color_table_count;
    SflBmpU32  color_table_size;
    SflBmpU32  pitch;
    SflBmpU32  r_mask;
    SflBmpU32  g_mask;
    SflBmpU32  b_mask;
    SflBmpU32  a_mask;
} SflBmpDecodeSettings;

typedef struct {
    SflBmpU32 i_pitch;
    SflBmpU32 i_slice;
    SflBmpU32 i_rbits;
    SflBmpU32 i_gbits;
    SflBmpU32 i_bbits;
    SflBmpU32 i_abits;
} SflBmpConvertSettings;

#define SFL_BMP_READ_STRUCT(T, ctx, dst) \
    (ctx->io.read(ctx->io.usr, (void*)dst, sizeof(T)) == 1)

#define SFL_BMP_READ(ctx, dst, size) ctx->io.read(ctx->io.usr, dst, size)

#define SFL_BMP_SEEK(ctx, offset, whence) \
    ctx->io.seek(ctx->io.usr, offset, whence)

#define SFL_BMP_WRITE(ctx, buf, size) ctx->io.write(ctx->io.usr, buf, size)

#define SFL_BMP_ALLOCATE(ctx, size) ctx->mem->allocate(ctx->mem->usr, size)

#define SFL_BMP_RELEASE(ctx, ptr) ctx->mem->release(ctx->mem->usr, ptr)

static int sfl_bmp__read(
    SflBmpIOImplementation* io, void* ptr, SflBmpUSize size)
{
    return io->read(io->usr, ptr, size);
}

static int sfl_bmp__write(
    SflBmpIOImplementation* io, void* ptr, SflBmpUSize size)
{
    return io->write(io->usr, ptr, size);
}

static int sfl_bmp__seek(
    SflBmpIOImplementation* io, long offset, SflBmpIOWhence w)
{
    return io->seek(io->usr, offset, w);
}

static long sfl_bmp__tell(SflBmpIOImplementation* io)
{
    return io->tell(io->usr);
}

static int sfl_bmp_decode_extract(
    SflBmpContext* ctx, SflBmpDesc* in, SflBmpDesc* out);

static int sfl_bmp_decode_extract_palettized(
    SflBmpContext* ctx, SflBmpDesc* in, SflBmpDesc* out);

static int sfl_bmp_extract(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc);

static int sfl_bmp_extract_raw(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc);

static int sfl_bmp_extract_raw_convert(
    SflBmpContext*         ctx,
    SflBmpDecodeSettings*  settings,
    SflBmpConvertSettings* convert_settings,
    SflBmpDesc*            desc);

static int sfl_bmp_extract_paletted(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc);

static int sfl_bmp_extract_paletted_none(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc);

static inline SflBmpI32 sfl_bmp_bit_scan_forward(SflBmpU32 value)
{
    SflBmpI32 result = 0;
    while ((value & 0x1) == 0 && result < 32) {
        value >>= 1;
        result++;
    }

    return result;
}

static inline SflBmpI32 sfl_bmp_bit_count(SflBmpU32 value)
{
    SflBmpI32 result = 0;
    for (int i = 0; i < 32; ++i) {
        result += (value & (0x1 << i)) > 0;
    }
    return result;
}

static SflBmpU32 sfl_bmp_ipow(SflBmpU32 base, SflBmpU32 exponent)
{
    if (exponent == 0) return 1;

    SflBmpU32 result = base;
    for (SflBmpU32 i = 1; i < exponent; ++i) {
        result *= base;
    }

    return result;
}

static SflBmpI32 sfl_bmp_iabs(SflBmpI32 value)
{
    return value < 0 ? -value : value;
}

const char* sfl_bmp_describe_pixel_format(int format)
{
    const char* desc_string = "Invalid format enumeration";
    switch (format) {
        case SFL_BMP_PIXEL_FORMAT_INVALID: {
            desc_string = "Invalid";
        } break;

        case SFL_BMP_PIXEL_FORMAT_UNRECOGNIZED: {
            desc_string = "Unrecognized";
        } break;

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

        case SFL_BMP_PIXEL_FORMAT_B5G5R5X1: {
            desc_string = "B5G5R5X1";
        } break;

        default:
            break;
    }

    return desc_string;
}

const char* sfl_bmp_describe_hdr_id(SflBmpHdrID id)
{
    const char* result = "Invalid";
    switch (id) {
        case SFL_BMP_HDR_ID_NA:
            result = "N/A";
            break;
        case SFL_BMP_HDR_ID_BM:
            result = "BM (Windows 3.1x)";
            break;
        case SFL_BMP_HDR_ID_BA:
            result = "BA (OS/2 struct bitmap array)";
            break;
        case SFL_BMP_HDR_ID_CI:
            result = "CI (OS/2 struct color icon)";
            break;
        case SFL_BMP_HDR_ID_CP:
            result = "CP (OS/2 const color pointer)";
            break;
        case SFL_BMP_HDR_ID_IC:
            result = "IC (OS/2 struct icon)";
            break;
        case SFL_BMP_HDR_ID_PT:
            result = "PT (OS/2 pointer)";
            break;
        default:
            break;
    }
    return result;
}

const char* sfl_bmp_describe_nfo_id(SflBmpNfoID id)
{
    const char* result = "Invalid";
    switch (id) {
        case SFL_BMP_NFO_ID_NA:
            result = "N/A";
            break;
        case SFL_BMP_NFO_ID_CORE:
            result = "BITMAPCOREHEADER (Windows 2.0)";
            break;
        case SFL_BMP_NFO_ID_OS22_V1:
            result = "OS22XBITMAPHEADER (OS/2 1.x)";
            break;
        case SFL_BMP_NFO_ID_OS22_V2:
            result = "BITMAPCOREHEADER2 (OS/2 2.x)";
            break;
        case SFL_BMP_NFO_ID_V1:
            result = "BITMAPINFOHEADER (Windows NT)";
            break;
        case SFL_BMP_NFO_ID_V2:
            result = "BITMAPV2INFOHEADER (ADOBE)";
            break;
        case SFL_BMP_NFO_ID_V3:
            result = "BITMAPV3INFOHEADER (ADOBE)";
            break;
        case SFL_BMP_NFO_ID_V4:
            result = "BITMAPV4HEADER (Windows NT 4, 95)";
            break;
        case SFL_BMP_NFO_ID_V5:
            result = "BITMAPV5HEADER (Windows NT 5, 98)";
            break;
        default:
            break;
    }
    return result;
}

const char* sfl_bmp_describe_compression(SflBmpCompression compression)
{
    const char* result = "Invalid";
    switch (compression) {
        case SFL_BMP_COMPRESSION_NONE:
            result = "None";
            break;
        case SFL_BMP_COMPRESSION_RLE8:
            result = "RLE (8 bits)";
            break;
        case SFL_BMP_COMPRESSION_RLE4:
            result = "RLE (4 bits)";
            break;
        case SFL_BMP_COMPRESSION_BITFIELDS:
            result = "None";
            break;
        default:
            break;
    }
    return result;
}

static int sfl_bmp__is_compressed(SflBmpDesc* desc)
{
    switch (desc->compression) {
        case SFL_BMP_COMPRESSION_RLE4:
        case SFL_BMP_COMPRESSION_RLE8:
            return 1;

        case SFL_BMP_COMPRESSION_NONE:
        case SFL_BMP_COMPRESSION_BITFIELDS:
        default:
            return 0;
    }
}

static int sfl_bmp__nfo_size(SflBmpNfoID id)
{
    switch (id) {
        default:
        case SFL_BMP_NFO_ID_NA:
        case SFL_BMP_NFO_ID_CORE:
        case SFL_BMP_NFO_ID_OS22_V1:
        case SFL_BMP_NFO_ID_OS22_V2:
        case SFL_BMP_NFO_ID_V1:
        case SFL_BMP_NFO_ID_V2:
        case SFL_BMP_NFO_ID_V3:
        case SFL_BMP_NFO_ID_V4:
            return -1;

        case SFL_BMP_NFO_ID_V5:
            return sizeof(SflBmpInfoHeader124);
    }
}

static int sfl_bmp__fill_desc(SflBmpDesc* desc);

void sfl_bmp_init(
    SflBmpContext*              ctx,
    SflBmpIOImplementation*     io,
    SflBmpMemoryImplementation* mem)
{
    ctx->io       = *io;
    ctx->mem      = mem;
    ctx->io.usr   = 0;
    ctx->mem->usr = 0;
}

void sfl_bmp_set_io_usr(SflBmpContext* ctx, void* usr) { ctx->io.usr = usr; }

void sfl_bmp_set_memory_usr(SflBmpContext* ctx, void* usr)
{
    ctx->mem->usr = usr;
}

static SflBmpHdrID sfl_bmp_get_hdr_id(char* header)
{
    if (header[0] == 'B' && header[1] == 'M') {
        return SFL_BMP_HDR_ID_BM;
    }

    if (header[0] == 'B' && header[1] == 'A') {
        return SFL_BMP_HDR_ID_BA;
    }

    if (header[0] == 'C' && header[1] == 'I') {
        return SFL_BMP_HDR_ID_CI;
    }

    if (header[0] == 'C' && header[1] == 'P') {
        return SFL_BMP_HDR_ID_CP;
    }

    if (header[0] == 'I' && header[1] == 'C') {
        return SFL_BMP_HDR_ID_IC;
    }

    if (header[0] == 'P' && header[1] == 'T') {
        return SFL_BMP_HDR_ID_PT;
    }

    return SFL_BMP_HDR_ID_NA;
}

static const char* sfl_bmp__hdr_id_to_str(SflBmpHdrID id)
{
    switch (id) {
        case SFL_BMP_HDR_ID_NA:
            return "NA";
        case SFL_BMP_HDR_ID_BM:
            return "BM";
        case SFL_BMP_HDR_ID_BA:
            return "BA";
        case SFL_BMP_HDR_ID_CI:
            return "CI";
        case SFL_BMP_HDR_ID_CP:
            return "CP";
        case SFL_BMP_HDR_ID_IC:
            return "IC";
        case SFL_BMP_HDR_ID_PT:
            return "PT";
        default:
            return 0;
    }
}

static SflBmpNfoID sfl_bmp_get_nfo_id(SflBmpU32 info_size)
{
    switch (info_size) {
        case 12:
            return SFL_BMP_NFO_ID_CORE;
            break;
        case 64:
            return SFL_BMP_NFO_ID_OS22_V1;
            break;
        case 16:
            return SFL_BMP_NFO_ID_OS22_V2;
            break;
        case 40:
            return SFL_BMP_NFO_ID_V1;
            break;
        case 52:
            return SFL_BMP_NFO_ID_V2;
            break;
        case 56:
            return SFL_BMP_NFO_ID_V3;
            break;
        case 108:
            return SFL_BMP_NFO_ID_V4;
            break;
        case 124:
            return SFL_BMP_NFO_ID_V5;
            break;
        default:
            return SFL_BMP_NFO_ID_NA;
            break;
    }
}

static int sfl_bmp__bpp_from_pixel_format(int format)
{
    switch (format) {
        case SFL_BMP_PIXEL_FORMAT_UNRECOGNIZED:
        case SFL_BMP_PIXEL_FORMAT_B8G8R8A8:
        case SFL_BMP_PIXEL_FORMAT_R8G8B8A8:
        case SFL_BMP_PIXEL_FORMAT_B8G8R8X8:
            return 32;

        case SFL_BMP_PIXEL_FORMAT_B8G8R8:
            return 24;

        case SFL_BMP_PIXEL_FORMAT_B5G6R5:
        case SFL_BMP_PIXEL_FORMAT_B5G5R5X1:
            return 16;

        case SFL_BMP_PIXEL_FORMAT_INVALID:
        default:
            return -1;
    }
}

static int sfl_bmp__bitmasks_from_pixel_format(int format, SflBmpU32* masks)
{
    int ok = 1;
    switch (format) {
        case SFL_BMP_PIXEL_FORMAT_B8G8R8A8: {
            masks[0] = 0x00ff0000;
            masks[1] = 0x0000ff00;
            masks[2] = 0x000000ff;
            masks[3] = 0xff000000;
        } break;

        case SFL_BMP_PIXEL_FORMAT_B8G8R8: {
            masks[0] = 0x00ff0000;
            masks[1] = 0x0000ff00;
            masks[2] = 0x000000ff;
            masks[3] = 0;
        } break;

        case SFL_BMP_PIXEL_FORMAT_B5G6R5: {
            masks[0] = 0b1111100000000000;
            masks[1] = 0b0000011111100000;
            masks[2] = 0b0000000000011111;
            masks[3] = 0;
        } break;

        case SFL_BMP_PIXEL_FORMAT_B5G5R5X1: {
            masks[0] = 0b0111110000000000;
            masks[1] = 0b0000001111100000;
            masks[2] = 0b0000000000011111;
            masks[3] = 0;
        } break;

        case SFL_BMP_PIXEL_FORMAT_R8G8B8A8: {
            masks[0] = 0xff000000;
            masks[1] = 0x00ff0000;
            masks[2] = 0x0000ff00;
            masks[3] = 0x000000ff;
        } break;

        case SFL_BMP_PIXEL_FORMAT_B8G8R8X8: {
            masks[0] = 0x00ff0000;
            masks[1] = 0x0000ff00;
            masks[2] = 0x000000ff;
            masks[3] = 0x00000000;
        } break;

        case SFL_BMP_PIXEL_FORMAT_UNRECOGNIZED: {
        } break;

        default: {
            ok = 0;
        } break;
    }

    return ok;
}

static int sfl_bmp_decide_pixel_format_from_bitmasks(SflBmpU32* masks)
{
    SflBmpU32 test_mask[4];
#define SFL_BMP__TEST_MASKS(m1, m2)                              \
    ((m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]) && \
     (m1[3] == m2[3]))

#define SFL_BMP__TEST_FORMAT(format, test_mask, mask)                     \
    do {                                                                  \
        int __r = sfl_bmp__bitmasks_from_pixel_format(format, test_mask); \
        if (!__r) return 0;                                               \
        if (SFL_BMP__TEST_MASKS(test_mask, mask)) return 1;               \
    } while (0)

    SFL_BMP__TEST_FORMAT(SFL_BMP_PIXEL_FORMAT_B8G8R8A8, test_mask, masks);
    SFL_BMP__TEST_FORMAT(SFL_BMP_PIXEL_FORMAT_B8G8R8, test_mask, masks);
    SFL_BMP__TEST_FORMAT(SFL_BMP_PIXEL_FORMAT_B5G6R5, test_mask, masks);
    SFL_BMP__TEST_FORMAT(SFL_BMP_PIXEL_FORMAT_R8G8B8A8, test_mask, masks);
    SFL_BMP__TEST_FORMAT(SFL_BMP_PIXEL_FORMAT_B8G8R8X8, test_mask, masks);
    SFL_BMP__TEST_FORMAT(SFL_BMP_PIXEL_FORMAT_B5G5R5X1, test_mask, masks);

#undef SFL_BMP__TEST
#undef SFL_BMP__TEST_MASKS
    return SFL_BMP_PIXEL_FORMAT_UNRECOGNIZED;
}

int sfl_bmp_probe(SflBmpContext* ctx, SflBmpDesc* desc)
{
    SflBmpHdrID hdr_id = SFL_BMP_HDR_ID_NA;
    SflBmpNfoID nfo_id = SFL_BMP_NFO_ID_NA;
    SflBmpU16   bpp    = 0;
    int         rc     = 0;

    /* Read in file header and determine file type */
    SflBmpFileHeader file_header;
    if (!SFL_BMP_READ_STRUCT(SflBmpFileHeader, ctx, &file_header)) {
        goto EXIT_PROC;
    }

    hdr_id = sfl_bmp_get_hdr_id(file_header.hdr);
    if (hdr_id == SFL_BMP_HDR_ID_NA) {
        goto EXIT_PROC;
    }

    /* Read in  header size and get info header kind */
    SflBmpU32 info_header_size;
    if (!SFL_BMP_READ(ctx, &info_header_size, sizeof(info_header_size))) {
        goto EXIT_PROC;
    }

    if (SFL_BMP_SEEK(ctx, -((long)sizeof(info_header_size)), SFL_BMP_IO_CUR)) {
        goto EXIT_PROC;
    }

    nfo_id = sfl_bmp_get_nfo_id(info_header_size);

    /* Set initial values for descriptor */
    desc->data              = 0;
    desc->attributes        = 0;
    desc->offset            = file_header.offset;
    desc->file_header_id    = hdr_id;
    desc->info_header_id    = nfo_id;
    desc->num_table_entries = 0;
    desc->table_offset      = 0;
    desc->physical_width    = 0;
    desc->physical_height   = 0;

    switch (nfo_id) {
        case SFL_BMP_NFO_ID_CORE: {
            /*
            @note: Core headers for Windows 2.0 and OS/2 1.x differ in dimension
            signage
            */

        } break;

        case SFL_BMP_NFO_ID_OS22_V1: {
        } break;

        case SFL_BMP_NFO_ID_OS22_V2: {
        } break;

        case SFL_BMP_NFO_ID_V1: {
            SflBmpInfoHeader040 info_header;
            if (!SFL_BMP_READ_STRUCT(SflBmpInfoHeader040, ctx, &info_header)) {
                goto EXIT_PROC;
            }
            desc->width           = info_header.width;
            desc->height          = info_header.height;
            desc->physical_width  = info_header.hres;
            desc->physical_height = info_header.vres;

            if (info_header.height > 0) {
                desc->attributes |= SFL_BMP_ATTRIBUTE_FLIPPED;
            }

            /* v1 doesn't support alpha */
            desc->mask[3] = 0;

            /*
            msdocs on BITMAPINFOHEADER say that these are the only
            possible values
            */
            if (info_header.compression == SFL_BMP_COMPRESSION_BITFIELDS) {
                /*
                > Palette field contains three 4 byte color masks that specify
                > the red, green, and blue components
                From msdocs on BITMAPINFOHEADER
                */

                SflBmpU32 masks[3];
                if (!SFL_BMP_READ(ctx, masks, sizeof(SflBmpU32) * 3)) {
                    goto EXIT_PROC;
                }

                desc->mask[0] = masks[0];
                desc->mask[1] = masks[1];
                desc->mask[2] = masks[2];
            } else if (info_header.compression != SFL_BMP_COMPRESSION_NONE) {
                goto EXIT_PROC;
            }
            desc->num_table_entries = info_header.num_colors;
            desc->compression       = info_header.compression;
            bpp                     = info_header.bpp;
        } break;

        case SFL_BMP_NFO_ID_V2: {
        } break;

        case SFL_BMP_NFO_ID_V3: {
        } break;

        case SFL_BMP_NFO_ID_V4: {
        } break;

        case SFL_BMP_NFO_ID_V5: {
            SflBmpInfoHeader124 info_header;
            if (!SFL_BMP_READ_STRUCT(SflBmpInfoHeader124, ctx, &info_header)) {
                goto EXIT_PROC;
            }
            desc->width           = info_header.width;
            desc->height          = sfl_bmp_iabs(info_header.height);
            desc->physical_width  = info_header.hres;
            desc->physical_height = info_header.vres;

            if (info_header.height > 0) {
                desc->attributes |= SFL_BMP_ATTRIBUTE_FLIPPED;
            }

            desc->mask[0]           = info_header.red_mask;
            desc->mask[1]           = info_header.green_mask;
            desc->mask[2]           = info_header.blue_mask;
            desc->mask[3]           = info_header.alpha_mask;
            /*
            @todo: technically, this is incorrect. Due to overlap with other
            headers, # of palette entries are usually 4 with this header, which
            includes the masks inside the struct decl. So, handle this, and
            report the correct amount of entries in the palette
            */
            desc->num_table_entries = info_header.num_colors;
            desc->compression = (SflBmpCompression)info_header.compression;
            bpp               = info_header.bpp;
        } break;

        case SFL_BMP_NFO_ID_NA:
        default:
            break;
    }

    /* @todo: or ALPHA_BITFIELDS */
    if (desc->compression == SFL_BMP_COMPRESSION_BITFIELDS) {
        desc->table_offset =
            sizeof(SflBmpFileHeader) + info_header_size + sizeof(SflBmpU32) * 3;
    } else {
        desc->table_offset = sizeof(SflBmpFileHeader) + info_header_size;
    }

    if (desc->info_header_id == SFL_BMP_NFO_ID_OS22_V1) {
        desc->table_entry_size = 3;
    } else {
        desc->table_entry_size = 4;
    }

    desc->pitch = (SflBmpU32)SFL_BMP_CEILF((bpp * desc->width) / 32.f) * 4;

    desc->size = desc->pitch * desc->height;

    /* If bits per pixel is <= 8, then the bitmap is always palettized */
    switch (bpp) {
        case 1:
        case 4:
        case 8: {
            desc->attributes |= SFL_BMP_ATTRIBUTE_PALETTIZED;

            if (desc->compression == SFL_BMP_COMPRESSION_RLE4) {
                if (bpp != 4) {
                    goto EXIT_PROC;
                }
            } else if (desc->compression == SFL_BMP_COMPRESSION_RLE8) {
                if (bpp != 8) {
                    goto EXIT_PROC;
                }
            }

            desc->format =
                sfl_bmp_decide_pixel_format_from_bitmasks(desc->mask);
        } break;

        case 16: {
            desc->slice = 2;
            if (desc->compression == SFL_BMP_COMPRESSION_NONE) {
                /*
                @note: Documentation mentions that the most significant bit
                isn't used in 16 bpp
                */
                desc->format = SFL_BMP_PIXEL_FORMAT_B5G5R5X1;
                sfl_bmp__bitmasks_from_pixel_format(desc->format, desc->mask);
            } else if (desc->compression == SFL_BMP_COMPRESSION_BITFIELDS) {
                desc->format =
                    sfl_bmp_decide_pixel_format_from_bitmasks(desc->mask);
            } else {
                goto EXIT_PROC;
            }
        } break;

        case 24: {
            desc->slice  = 3;
            desc->format = SFL_BMP_PIXEL_FORMAT_B8G8R8;
            if (desc->compression != SFL_BMP_COMPRESSION_NONE) {
                goto EXIT_PROC;
            }
        } break;

        case 32: {
            desc->slice = 4;
            if (desc->compression == SFL_BMP_COMPRESSION_BITFIELDS) {
                desc->format =
                    sfl_bmp_decide_pixel_format_from_bitmasks(desc->mask);
            } else if (desc->compression == SFL_BMP_COMPRESSION_NONE) {
                desc->format = SFL_BMP_PIXEL_FORMAT_B8G8R8X8;
            } else {
                goto EXIT_PROC;
            }
        } break;

        default: {
            goto EXIT_PROC;
        } break;
    }

    rc = 1;
EXIT_PROC:
    SFL_BMP_SEEK(ctx, 0, SFL_BMP_IO_SET);
    return rc;
}

int sfl_bmp_decode(SflBmpContext* ctx, SflBmpDesc* desc)
{
    SflBmpDesc intermediate_desc;
    if (!sfl_bmp_probe(ctx, &intermediate_desc)) {
        return 0;
    }

    desc->width          = intermediate_desc.width;
    desc->height         = intermediate_desc.height;
    desc->file_header_id = intermediate_desc.file_header_id;
    desc->info_header_id = intermediate_desc.info_header_id;

    if (!sfl_bmp__fill_desc(desc)) {
        return 0;
    }

    if (desc->attributes & SFL_BMP_ATTRIBUTE_PALETTIZED) {
        SFL_BMP_UNIMPLEMENTED();
    } else {
        desc->data = SFL_BMP_ALLOCATE(ctx, desc->size);
    }

    SflBmpIOImplementationMemory desc_io_memory;
    sfl_bmp_memory_init(&desc_io_memory, desc->data, desc->size);

    sfl_bmp__convert(&intermediate_desc, &ctx->io, desc, &SflBmp_IO_Memory);

    return 1;
}

static int sfl_bmp__next_block(
    SflBmpDesc*             desc,
    SflBmpIOImplementation* io,
    void*                   buf,
    SflBmpUSize             buf_size,
    SflBmpUSize             offset)
{
    if (desc->slice > buf_size) {
        return -1;
    }

    int col = (int)(offset % desc->pitch);

    int skip_bytes = 0;
    if ((col + desc->slice) > desc->pitch) {
        skip_bytes = desc->pitch - col;
    }

    if (skip_bytes != 0) {
        if (sfl_bmp__seek(io, skip_bytes, SFL_BMP_IO_CUR)) {
            return -1;
        }
    }

    if (!sfl_bmp__read(io, buf, desc->slice)) {
        return -1;
    }

    return desc->slice + skip_bytes;
}

static int sfl_bmp__write_block(
    SflBmpDesc* desc, SflBmpIOImplementation* io, void* buf, SflBmpUSize size)
{
    if (size < desc->slice) {
        return 0;
    }

    return io->write(io->usr, buf, desc->slice);
    return 1;
}

static int sfl_bmp__convert(
    SflBmpDesc*             in,
    SflBmpIOImplementation* in_io,
    SflBmpDesc*             out,
    SflBmpIOImplementation* out_io)
{
    SflBmpU32 in_buf          = 0;
    int       x               = 0;
    int       y               = 0;
    int       bytes_processed = 0;
    if (sfl_bmp__seek(in_io, in->offset, SFL_BMP_IO_SET)) {
        return 0;
    }

    /* Amounts to shift each pixel value */
    const SflBmpI32 irs = sfl_bmp_bit_scan_forward(in->mask[0]);
    const SflBmpI32 igs = sfl_bmp_bit_scan_forward(in->mask[1]);
    const SflBmpI32 ibs = sfl_bmp_bit_scan_forward(in->mask[2]);
    const SflBmpI32 ias = sfl_bmp_bit_scan_forward(in->mask[3]);

    const SflBmpI32 ors = sfl_bmp_bit_scan_forward(out->mask[0]);
    const SflBmpI32 ogs = sfl_bmp_bit_scan_forward(out->mask[1]);
    const SflBmpI32 obs = sfl_bmp_bit_scan_forward(out->mask[2]);
    const SflBmpI32 oas = sfl_bmp_bit_scan_forward(out->mask[3]);

    /* Amount of bits representing component */
    const SflBmpI32 irc = sfl_bmp_bit_count(in->mask[0]);
    const SflBmpI32 igc = sfl_bmp_bit_count(in->mask[1]);
    const SflBmpI32 ibc = sfl_bmp_bit_count(in->mask[2]);
    const SflBmpI32 iac = sfl_bmp_bit_count(in->mask[3]);

    const SflBmpI32 orc = sfl_bmp_bit_count(in->mask[0]);
    const SflBmpI32 ogc = sfl_bmp_bit_count(in->mask[1]);
    const SflBmpI32 obc = sfl_bmp_bit_count(in->mask[2]);
    const SflBmpI32 oac = sfl_bmp_bit_count(in->mask[3]);

    /* Max value of each component */
    const SflBmpReal imr = (SflBmpReal)(sfl_bmp_ipow(2, irc) - 1);
    const SflBmpReal img = (SflBmpReal)(sfl_bmp_ipow(2, igc) - 1);
    const SflBmpReal imb = (SflBmpReal)(sfl_bmp_ipow(2, ibc) - 1);
    const SflBmpReal ima = (SflBmpReal)(sfl_bmp_ipow(2, iac) - 1);

    const SflBmpReal omr = (SflBmpReal)(sfl_bmp_ipow(2, orc) - 1);
    const SflBmpReal omg = (SflBmpReal)(sfl_bmp_ipow(2, ogc) - 1);
    const SflBmpReal omb = (SflBmpReal)(sfl_bmp_ipow(2, obc) - 1);
    const SflBmpReal oma = (SflBmpReal)(sfl_bmp_ipow(2, oac) - 1);

    for (;;) {
        int num_bytes = sfl_bmp__next_block(
            in,
            in_io,
            &in_buf,
            sizeof(in_buf),
            bytes_processed);

        if (num_bytes <= 0) {
            return 0;
        }

        /* Let's pretend there's no compression for now */

        /* Convert to normalized colors */
        SflBmpReal ir = (SflBmpReal)((in_buf & in->mask[0]) >> irs);
        SflBmpReal ig = (SflBmpReal)((in_buf & in->mask[1]) >> igs);
        SflBmpReal ib = (SflBmpReal)((in_buf & in->mask[2]) >> ibs);
        SflBmpReal ia = (SflBmpReal)((in_buf & in->mask[3]) >> ias);

        ir = imr != 0.0f ? ir / imr : 0.0f;
        ig = img != 0.0f ? ig / img : 0.0f;
        ib = imb != 0.0f ? ib / imb : 0.0f;
        ia = ima != 0.0f ? ia / ima : 0.0f;

        /* Convert to target pixel */
        SflBmpI32 tr = (((SflBmpU32)SFL_BMP_CEILF(ir * omr)) << ors);
        SflBmpI32 tg = (((SflBmpU32)SFL_BMP_CEILF(ig * omg)) << ogs);
        SflBmpI32 tb = (((SflBmpU32)SFL_BMP_CEILF(ib * omb)) << obs);
        SflBmpI32 ta = (((SflBmpU32)SFL_BMP_CEILF(ia * oma)) << oas);

        SflBmpU32 pixel = tr | tg | tb | ta;

        sfl_bmp__write_block(out, out_io, &pixel, sizeof(pixel));
        bytes_processed += num_bytes;

        if (bytes_processed == in->size) {
            break;
        }
    }
    return 1;
}

static int sfl_bmp__fill_desc(SflBmpDesc* desc)
{
    sfl_bmp__bitmasks_from_pixel_format(desc->format, desc->mask);

    if (desc->attributes & SFL_BMP_ATTRIBUTE_PALETTIZED) {
        SFL_BMP_UNIMPLEMENTED();
        return 0;
    } else {
        int bpp = sfl_bmp__bpp_from_pixel_format(desc->format);

        desc->slice = bpp / 8;
        desc->pitch = (SflBmpU32)SFL_BMP_CEILF((bpp * desc->width) / 32.f) * 4;
        desc->size  = desc->pitch * desc->height;
    }
    return 1;
}

static int sfl_bmp_decode_extract(
    SflBmpContext* ctx, SflBmpDesc* in, SflBmpDesc* out)
{
    out->width             = in->width;
    out->height            = in->height;
    out->attributes        = 0;
    out->table_offset      = 0;
    out->table_entry_size  = 0;
    out->num_table_entries = 0;
    out->palette_data      = 0;
    out->compression       = SFL_BMP_COMPRESSION_NONE;
    out->pitch             = out->width * sizeof(SflBmpU32);
    out->size              = out->pitch * out->height;
    out->format            = SFL_BMP_PIXEL_FORMAT_R8G8B8A8;
    out->data              = SFL_BMP_ALLOCATE(ctx, out->size);
    sfl_bmp__bitmasks_from_pixel_format(out->format, out->mask);
    SflBmpU32* data = (SflBmpU32*)out->data;

    /* Amounts to shift each pixel value */
    const SflBmpI32 rs = sfl_bmp_bit_scan_forward(in->mask[0]);
    const SflBmpI32 gs = sfl_bmp_bit_scan_forward(in->mask[1]);
    const SflBmpI32 bs = sfl_bmp_bit_scan_forward(in->mask[2]);
    const SflBmpI32 as = sfl_bmp_bit_scan_forward(in->mask[3]);

    /* Amount of bits representing component */
    const SflBmpI32 rc = sfl_bmp_bit_count(in->mask[0]);
    const SflBmpI32 gc = sfl_bmp_bit_count(in->mask[1]);
    const SflBmpI32 bc = sfl_bmp_bit_count(in->mask[2]);
    const SflBmpI32 ac = sfl_bmp_bit_count(in->mask[3]);

    /* Max value of each component */
    const SflBmpReal mr = (SflBmpReal)(sfl_bmp_ipow(2, rc) - 1);
    const SflBmpReal mg = (SflBmpReal)(sfl_bmp_ipow(2, gc) - 1);
    const SflBmpReal mb = (SflBmpReal)(sfl_bmp_ipow(2, bc) - 1);
    const SflBmpReal ma = (SflBmpReal)(sfl_bmp_ipow(2, ac) - 1);

    const SflBmpU32 skip = in->pitch - (in->width - in->slice);

    const int is_flipped = in->attributes & SFL_BMP_ATTRIBUTE_FLIPPED;
    SFL_BMP_SEEK(ctx, in->offset, SFL_BMP_IO_SET);

    for (int y = 0; y < in->height; ++y) {
        for (int x = 0; x < in->height; ++x) {
            SflBmpU32 pixel;
            if (!SFL_BMP_READ(ctx, &pixel, in->slice)) {
                goto EXIT_ERROR;
            }

            SflBmpReal r = (SflBmpReal)((pixel & in->mask[0]) >> rs);
            SflBmpReal g = (SflBmpReal)((pixel & in->mask[1]) >> gs);
            SflBmpReal b = (SflBmpReal)((pixel & in->mask[2]) >> bs);
            SflBmpReal a = (SflBmpReal)((pixel & in->mask[3]) >> as);

            /* Ternary op handles case where any of these components is zero */
            SflBmpU32 cr =
                (rc != 0) ? (SflBmpU32)SFL_BMP_CEILF((r / mr) * 255) : 0;
            SflBmpU32 cg =
                (gc != 0) ? (SflBmpU32)SFL_BMP_CEILF((g / mg) * 255) : 0;
            SflBmpU32 cb =
                (bc != 0) ? (SflBmpU32)SFL_BMP_CEILF((b / mb) * 255) : 0;
            SflBmpU32 ca =
                (ac != 0) ? (SflBmpU32)SFL_BMP_CEILF((a / ma) * 255) : 0;

            if (in->mask[3] == 0) {
                ca = 0xff;
            }

            SflBmpU32 pixel32 = (cr << 0) | (cg << 8) | (cb << 16) | (ca << 24);

            if (is_flipped) {
                data[(in->height - y - 1) * in->width + x] = pixel32;
            } else {
                data[y * in->width + x] = pixel32;
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

static int sfl_bmp_decode_extract_palettized(
    SflBmpContext* ctx, SflBmpDesc* in, SflBmpDesc* out)
{
    return 0;
}

static int sfl_bmp__check_nfo_compat(SflBmpDesc* desc);

int sfl_bmp_encode(
    SflBmpContext*          ctx,
    SflBmpDesc*             in_desc,
    SflBmpIOImplementation* in_io,
    SflBmpDesc*             out_desc)
{
    out_desc->width           = in_desc->width;
    out_desc->height          = in_desc->height;
    out_desc->physical_width  = in_desc->physical_width;
    out_desc->physical_height = in_desc->physical_height;

    /* @todo: Add actual checks for file header, for now it's only 'BM' */
    if (!sfl_bmp__fill_desc(out_desc)) {
        return 0;
    }

    /* Info Header compatibility */
    sfl_bmp__check_nfo_compat(out_desc);

    /* Write file header */
    SflBmpFileHeader hdr;
    int              nfo_size = sfl_bmp__nfo_size(out_desc->info_header_id);
    if (nfo_size == -1) {
        return 0;
    }
    const char* hdr_str = sfl_bmp__hdr_id_to_str(out_desc->file_header_id);
    hdr.hdr[0]          = hdr_str[0];
    hdr.hdr[1]          = hdr_str[1];
    hdr.reserved[0]     = 0;
    hdr.reserved[1]     = 0;
    hdr.file_size       = sizeof(SflBmpFileHeader) + nfo_size + out_desc->size;
    hdr.offset          = sizeof(SflBmpFileHeader) + nfo_size;

    if (!SFL_BMP_WRITE(ctx, &hdr, sizeof(hdr))) {
        return 0;
    }

    /* Write info header */
    int pfbpp = sfl_bmp__bpp_from_pixel_format(out_desc->format);

    switch (out_desc->info_header_id) {
        default:
        case SFL_BMP_NFO_ID_NA:
        case SFL_BMP_NFO_ID_CORE:
        case SFL_BMP_NFO_ID_OS22_V1:
        case SFL_BMP_NFO_ID_OS22_V2:
        case SFL_BMP_NFO_ID_V1:
        case SFL_BMP_NFO_ID_V2:
        case SFL_BMP_NFO_ID_V3:
        case SFL_BMP_NFO_ID_V4:
            return 0;
        case SFL_BMP_NFO_ID_V5: {
            SflBmpInfoHeader124 info;
            info.size                 = sizeof(info);
            info.width                = out_desc->width;
            info.height               = out_desc->height;
            info.planes               = 1;
            info.bpp                  = pfbpp;
            info.compression          = out_desc->compression;
            info.raw_size             = out_desc->size;
            info.hres                 = out_desc->physical_width;
            info.vres                 = out_desc->physical_height;
            info.num_colors           = 0;
            info.num_important_colors = 0;
            info.red_mask             = out_desc->mask[0];
            info.green_mask           = out_desc->mask[1];
            info.blue_mask            = out_desc->mask[2];
            info.alpha_mask           = out_desc->mask[3];
            /* @todo */
            info.color_space          = SFL_BMP_COLORSPACE_SRGB;
            info.endpoint_red[0]      = 0;
            info.endpoint_red[1]      = 0;
            info.endpoint_red[2]      = 0;
            info.endpoint_green[0]    = 0;
            info.endpoint_green[1]    = 0;
            info.endpoint_green[2]    = 0;
            info.endpoint_blue[0]     = 0;
            info.endpoint_blue[1]     = 0;
            info.endpoint_blue[2]     = 0;
            info.gamma_red            = 0;
            info.gamma_green          = 0;
            info.gamma_blue           = 0;
            info.intent               = SFL_BMP_INTENT_LCS_GM_IMAGES;
            info.profile_data_offset  = 0;
            info.profile_data_size    = 0;

            info.reserved = 0;

            if (!SFL_BMP_WRITE(ctx, &info, sizeof(info))) {
                return 0;
            }
        } break;
    }

    /* @todo: palette table */
    return sfl_bmp__convert(in_desc, in_io, out_desc, &ctx->io);
}

static int sfl_bmp__check_nfo_compat(SflBmpDesc* desc)
{
#define SFL_BMP_MUST(x)     \
    do {                    \
        if (!(x)) return 0; \
    } while (0)

    switch (desc->info_header_id) {
        case SFL_BMP_NFO_ID_NA:
        case SFL_BMP_NFO_ID_CORE:
        case SFL_BMP_NFO_ID_OS22_V1:
        case SFL_BMP_NFO_ID_OS22_V2:
        case SFL_BMP_NFO_ID_V1:
        case SFL_BMP_NFO_ID_V2:
        case SFL_BMP_NFO_ID_V3:
        case SFL_BMP_NFO_ID_V4:
        case SFL_BMP_NFO_ID_V5:
            if (!sfl_bmp__is_compressed(desc)) {
                if ((desc->format == SFL_BMP_PIXEL_FORMAT_B8G8R8X8) ||
                    (desc->format == SFL_BMP_PIXEL_FORMAT_B5G5R5X1))
                {
                    SFL_BMP_MUST(desc->compression == SFL_BMP_COMPRESSION_NONE);
                } else {
                    SFL_BMP_MUST(
                        desc->compression == SFL_BMP_COMPRESSION_BITFIELDS);
                }
            } else {
            }
            break;
    }

    return 1;
#undef SFL_BMP_MUST
}

static int sfl_bmp_extract(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc)
{
    desc->width      = settings->width;
    desc->height     = settings->height;
    desc->attributes = 0;

    SflBmpU32 pitch =
        (SflBmpU32)SFL_BMP_CEILF((settings->bpp * settings->width) / 32.f) * 4;

    SflBmpU32 size = pitch * desc->height;
    desc->size     = size;
    desc->pitch    = pitch;

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
            return sfl_bmp_extract_raw(ctx, settings, desc);
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
            return sfl_bmp_extract_raw_convert(
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
            return sfl_bmp_extract_raw_convert(
                ctx,
                settings,
                &convert_settings,
                desc);
#else
            return sfl_bmp_extract_raw(ctx, settings, desc);
#endif
        } break;

        case 4: {
            return sfl_bmp_extract_paletted(ctx, settings, desc);
        } break;

        default: {
            return 0;
        } break;
    }
}

static int sfl_bmp_extract_raw(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc)
{
    SFL_BMP_SEEK(ctx, settings->offset, SFL_BMP_IO_SET);
    desc->data = SFL_BMP_ALLOCATE(ctx, desc->size);
    if (!SFL_BMP_READ(ctx, desc->data, desc->size)) {
        return 0;
    }
    return 1;
}

static int sfl_bmp_extract_raw_convert(
    SflBmpContext*         ctx,
    SflBmpDecodeSettings*  settings,
    SflBmpConvertSettings* convert_settings,
    SflBmpDesc*            desc)
{
    desc->format    = SFL_BMP_PIXEL_FORMAT_R8G8B8A8;
    desc->pitch     = desc->width * sizeof(SflBmpU32);
    desc->size      = desc->pitch * desc->height;
    desc->data      = SFL_BMP_ALLOCATE(ctx, desc->size);
    SflBmpU32* data = (SflBmpU32*)desc->data;

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

    const SflBmpU32 skip =
        convert_settings->i_pitch - (desc->width * convert_settings->i_slice);

    const int is_flipped = settings->height > 0 ? 1 : 0;
    desc->attributes &= ~SFL_BMP_ATTRIBUTE_FLIPPED;
    SFL_BMP_SEEK(ctx, settings->offset, SFL_BMP_IO_SET);

    for (int y = 0; y < desc->height; ++y) {
        for (int x = 0; x < desc->width; ++x) {
            SflBmpU32 pixel;
            if (!SFL_BMP_READ(ctx, &pixel, convert_settings->i_slice)) {
                goto EXIT_ERROR;
            }

            SflBmpReal r =
                (SflBmpReal)((pixel & convert_settings->i_rbits) >> rs);
            SflBmpReal g =
                (SflBmpReal)((pixel & convert_settings->i_gbits) >> gs);
            SflBmpReal b =
                (SflBmpReal)((pixel & convert_settings->i_bbits) >> bs);
            SflBmpReal a =
                (SflBmpReal)((pixel & convert_settings->i_abits) >> as);

            /* Ternary op handles case where any of these components is zero */
            SflBmpU32 cr =
                (rc != 0) ? (SflBmpU32)SFL_BMP_CEILF((r / mr) * 255) : 0;
            SflBmpU32 cg =
                (gc != 0) ? (SflBmpU32)SFL_BMP_CEILF((g / mg) * 255) : 0;
            SflBmpU32 cb =
                (bc != 0) ? (SflBmpU32)SFL_BMP_CEILF((b / mb) * 255) : 0;
            SflBmpU32 ca =
                (ac != 0) ? (SflBmpU32)SFL_BMP_CEILF((a / ma) * 255) : 0;

            if (convert_settings->i_abits == 0) {
                ca = 0xFF;
            }

            SflBmpU32 pixel32 = (cr << 0) | (cg << 8) | (cb << 16) | (ca << 24);

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

static int sfl_bmp_extract_paletted(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc)
{
    SflBmpU32 color_table_count;
    if (settings->num_colors == 0) {
        color_table_count = sfl_bmp_ipow(2, settings->bpp);
    } else {
        color_table_count = settings->num_colors;
    }
    const SflBmpU32 color_table_size = color_table_count * sizeof(SflBmpU32);

    const SflBmpU32 converted_size =
        desc->height * desc->width * sizeof(SflBmpU32);

    settings->pitch = desc->pitch;

    desc->format     = SFL_BMP_PIXEL_FORMAT_B8G8R8A8;
    desc->pitch      = desc->width * sizeof(SflBmpU32);
    desc->attributes = 0;

    SflBmpU32* color_table =
        (SflBmpU32*)SFL_BMP_ALLOCATE(ctx, color_table_size);
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
    settings->color_table       = color_table;
    settings->color_table_count = color_table_count;
    settings->color_table_size  = color_table_size;

    switch (settings->compression) {
        case SFL_BMP_COMPRESSION_NONE: {
            return sfl_bmp_extract_paletted_none(ctx, settings, desc);
        } break;

        default: {
            return 0;
        } break;
    }
}

static int sfl_bmp_extract_paletted_none(
    SflBmpContext* ctx, SflBmpDecodeSettings* settings, SflBmpDesc* desc)
{
    const int is_flipped  = settings->height > 0 ? 1 : 0;
    SflBmpU32 curr_offset = settings->offset;

    SFL_BMP_SEEK(ctx, settings->offset, SFL_BMP_IO_SET);

    SflBmpU32* out_pixels = (SflBmpU32*)desc->data;

    for (SflBmpI32 c = 0; c < desc->height; ++c) {
        SflBmpI32 y        = is_flipped ? desc->height - 1 - c : c;
        SflBmpU32 curr_row = curr_offset;
        SflBmpU32 row_end  = curr_offset + settings->pitch;
        SflBmpI32 x        = 0;

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

            SflBmpU32  image_offset = desc->width * y + x;
            SflBmpU32* data         = out_pixels + image_offset;
            data[0]                 = settings->color_table[left_pixel];

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

static PROC_SFL_BMP_IO_READ(sfl_bmp_stdlib_read)
{
    FILE* f   = (FILE*)usr;
    int   ret = fread(ptr, size, 1, f);
    return ret == 1;
}

static PROC_SFL_BMP_IO_SEEK(sfl_bmp_stdlib_seek)
{
    int whence_stdio = -1;

    switch (whence) {
        case SFL_BMP_IO_SET:
            whence_stdio = SEEK_SET;
            break;
        case SFL_BMP_IO_CUR:
            whence_stdio = SEEK_CUR;
            break;
        case SFL_BMP_IO_END:
            whence_stdio = SEEK_END;
            break;
        default:
            break;
    }

    FILE* f = (FILE*)usr;
    return fseek(f, offset, whence_stdio);
}

static PROC_SFL_BMP_IO_TELL(sfl_bmp_stdlib_tell)
{
    FILE* f = (FILE*)usr;
    return ftell(f);
}

static PROC_SFL_BMP_IO_WRITE(sfl_bmp_stdlib_write)
{
    FILE* f = (FILE*)usr;
    return fwrite(buf, size, 1, f) == 1;
}

static SflBmpIOImplementation SflBmp_IO_STDLIB = {
    sfl_bmp_stdlib_read,
    sfl_bmp_stdlib_write,
    sfl_bmp_stdlib_seek,
    sfl_bmp_stdlib_tell,
};

void sfl_bmp_stdio_init(SflBmpContext* ctx, SflBmpMemoryImplementation* memory)
{
    sfl_bmp_init(ctx, &SflBmp_IO_STDLIB, memory);
}

void sfl_bmp_stdio_set_file(SflBmpContext* ctx, FILE* file)
{
    sfl_bmp_set_io_usr(ctx, (void*)file);
}

SflBmpIOImplementation* sfl_bmp_stdio_get_implementation(void)
{
    return &SflBmp_IO_STDLIB;
}

#endif

#if SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB
#include <stdlib.h>

static PROC_SFL_BMP_MEMORY_ALLOCATE(sfl_bmp_stdlib_allocate)
{
    return malloc(size);
}

static PROC_SFL_BMP_MEMORY_RELEASE(sfl_bmp_stdlib_release) { free(ptr); }

static SflBmpMemoryImplementation SflBmp_Memory_STDLIB = {
    sfl_bmp_stdlib_allocate,
    sfl_bmp_stdlib_release,
};

void sfl_bmp_stdlib_init(SflBmpContext* ctx, SflBmpIOImplementation* io)
{
    sfl_bmp_init(ctx, io, &SflBmp_Memory_STDLIB);
}

SflBmpMemoryImplementation* sfl_bmp_stdlib_get_implementation(void)
{
    return &SflBmp_Memory_STDLIB;
}

/* SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB */
#endif

#if SFL_BMP_IO_IMPLEMENTATION_STDIO && SFL_BMP_MEMORY_IMPLEMENTATION_STDLIB
void sfl_bmp_cstd_init(SflBmpContext* ctx)
{
    sfl_bmp_init(ctx, &SflBmp_IO_STDLIB, &SflBmp_Memory_STDLIB);
}

#endif

#if SFL_BMP_IO_IMPLEMENTATION_WINAPI

static PROC_SFL_BMP_IO_READ(sfl_bmp_winapi_read)
{
    HANDLE* f = (HANDLE*)usr;
    DWORD   bytes_read;
    BOOL    success = ReadFile(*f, ptr, size, &bytes_read, NULL);
    return success && (bytes_read == size);
}

static PROC_SFL_BMP_IO_SEEK(sfl_bmp_winapi_seek)
{
    DWORD move_method = 3;

    switch (whence) {
        case SFL_BMP_IO_SET:
            move_method = FILE_BEGIN;
            break;
        case SFL_BMP_IO_CUR:
            move_method = FILE_CURRENT;
            break;
        case SFL_BMP_IO_END:
            move_method = FILE_END;
            break;
        default: {
            return -1;
        } break;
    }

    HANDLE* f      = (HANDLE*)usr;
    DWORD   result = SetFilePointer(*f, offset, 0, move_method);
    if (result == INVALID_SET_FILE_POINTER) {
        /*
        INVALID_SET_FILE_POINTER is a valid low order dword return value, so we
        have to check with GetLastError() if this failed
        */
        if (GetLastError() == ERROR_SUCCESS) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return 0;
    }
}

static PROC_SFL_BMP_IO_TELL(sfl_bmp_winapi_tell)
{
    HANDLE* f      = (HANDLE*)usr;
    DWORD   result = SetFilePointer(*f, 0, 0, 0);
    if (result == INVALID_SET_FILE_POINTER) {
        /*
        INVALID_SET_FILE_POINTER is a valid low order dword return value, so we
        have to check with GetLastError() if this failed
        */
        if (GetLastError() == ERROR_SUCCESS) {
            return (long)result;
        } else {
            return -1;
        }
    } else {
        return (long)result;
    }
}

static SflBmpIOImplementation SflBmp_IO_WINAPI = {
    sfl_bmp_winapi_read,
    0,
    sfl_bmp_winapi_seek,
    sfl_bmp_winapi_tell,
};

void sfl_bmp_winapi_io_init(
    SflBmpContext* ctx, SflBmpMemoryImplementation* memory)
{
    sfl_bmp_init(ctx, &SflBmp_IO_WINAPI, memory);
}

void sfl_bmp_winapi_io_set_file(SflBmpContext* ctx, HANDLE* file)
{
    ctx->io.usr = (void*)file;
}

SflBmpIOImplementation* sfl_bmp_winapi_io_get_implementation(void)
{
    return &SflBmp_IO_WINAPI;
}

/* SFL_BMP_IO_IMPLEMENTATION_WINAPI */
#endif

#undef SFL_BMP_READ_STRUCT
#undef SFL_BMP_READ
#undef SFL_BMP_SEEK
#endif