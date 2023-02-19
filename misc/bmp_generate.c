#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

#define ADD_STRING(f, s) fwrite(s, strlen(s), 1, f)
#define ADD_BYTES(f, ...) do {           \
    unsigned char _ar[] = {__VA_ARGS__}; \
    fwrite(_ar, 1, ARRAY_COUNT(_ar), f); \
} while(0)

#define ADD_U32(f, x) do {         \
    uint32_t _x = (x);             \
    fwrite(&_x, sizeof(_x), 1, f); \
} while(0)

#define ADD_I32(f, x) do {         \
    int32_t _x = (x);              \
    fwrite(&_x, sizeof(_x), 1, f); \
} while(0)

#define ADD_U16(f, x) do {         \
    uint16_t _x = (x);             \
    fwrite(&_x, sizeof(_x), 1, f); \
} while(0)

#define ADD_I16(f, x) do {         \
    int16_t _x = (x);              \
    fwrite(&_x, sizeof(_x), 1, f); \
} while(0)

#define ADD_U8(f, x) do {          \
    uint8_t _x = (x);              \
    fwrite(&_x, sizeof(_x), 1, f); \
} while(0)


#define PROC_GENERATE(name) void name(FILE *f)
typedef PROC_GENERATE(ProcGenerate);

typedef struct {
    ProcGenerate *generate_proc;
    const char *tag;
    const char *description;
} Generator;

static PROC_GENERATE(gen_v5_1bpp);
static PROC_GENERATE(gen_v1_16bpp_rgb);

static const Generator The_Generators[] = {
    {
        gen_v5_1bpp,
        "v5-1bpp",
        "Generates v5 bitmap with 1 bit per pixel"
    },
    {
        gen_v1_16bpp_rgb,
        "v1-16bpp-rgb",
        "Generates v1 bitmap with 16 bits per pixel and RGB compression"
    }
};

static void print_help() {
    puts("Invocation: bmp_generate <tag> <path>");
    for (int i = 0; i < ARRAY_COUNT(The_Generators); ++i) {
        Generator *generator = &The_Generators[i];
        printf("%-20s %s\n", generator->tag, generator->description);
    }
}

static Generator *find_generator(const char *tag) {
    for (int i = 0; i < ARRAY_COUNT(The_Generators); ++i) {
        Generator *generator = &The_Generators[i];
        if (strcmp(tag, generator->tag) == 0) {
            return generator;
        }
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    
    if (argc != 3) {
        puts("Invalid number of arguments");
        print_help();
        return -1;
    }

    Generator *generator = find_generator(argv[1]);
    if (generator == 0) {
        puts("Generator not found");
        print_help();
        return -1;
    }

    FILE *f = fopen(argv[2], "wb");
    if (!f) {
        puts("Could not open file");
        return -1;
    }

    generator->generate_proc(f);
    printf("Output file size: %u\n", ftell(f));
    fclose(f);

    return 0;
}

static PROC_GENERATE(gen_v5_1bpp) {
    // Add file header: size, 0, offset 
    ADD_STRING(f, "BM");
    ADD_U32(f, 154);     // file size 
    ADD_U32(f, 0);       // reserved[2]
    ADD_U32(f, 146);     // offset to pixel data

    // Add info header SflBmpInfoHeader124 (14 bytes) @todo: check offsets again
    ADD_U32(f, 124);     // header size
    ADD_I32(f, 2);       // width
    ADD_I32(f, 2);       // height
    ADD_U16(f, 1);       // planes
    ADD_U16(f, 1);       // bpp
    ADD_U32(f, 0);       // compression (BI_RGB)
    ADD_U32(f, 8);       // raw size
    ADD_I32(f, 2);       // hres
    ADD_I32(f, 2);       // vres
    ADD_U32(f, 2);       // num colors
    ADD_U32(f, 0);       // num important colors
    ADD_U32(f, 0x00ff0000); // red mask
    ADD_U32(f, 0x0000ff00); // green mask
    ADD_U32(f, 0x000000ff); // blue mask
    ADD_U32(f, 0);          // alpha mask
    ADD_U32(f, 'sRGB');     // color space

    // endpoints
    ADD_I32(f, 0); ADD_I32(f, 0); ADD_I32(f, 0);
    ADD_I32(f, 0); ADD_I32(f, 0); ADD_I32(f, 0);
    ADD_I32(f, 0); ADD_I32(f, 0); ADD_I32(f, 0);

    ADD_U32(f, 0);   // gamma red
    ADD_U32(f, 0);   // gamma green
    ADD_U32(f, 0);   // gamma blue
    ADD_U32(f, 0);   // intent
    ADD_U32(f, 0);   // profile data offset
    ADD_U32(f, 0);   // profile data size
    ADD_U32(f, 0);   // reserved

    // Color table (136 bytes)
    ADD_U32(f, 0x00ff0000);
    ADD_U32(f, 0x0000ff00);

    // Image data (padded) (144 bytes)
    ADD_BYTES(f, 
        0b10000000, 0x00, 0x00, 0x00,
        0b01000000, 0x00, 0x00, 0x00);
}

static PROC_GENERATE(gen_v1_16bpp_rgb) {
    ADD_STRING(f, "BM");
    ADD_U32(f, 70);      // file size 
    ADD_U32(f, 0);       // reserved[2]
    ADD_U32(f, 54);      // @todo offset to pixel data

    // Add info header BITMAPINFOHEADER (14 bytes)
    ADD_U32(f, 40);      // header size
    ADD_I32(f, 2);       // width
    ADD_I32(f, 2);       // height
    ADD_U16(f, 1);       // planes
    ADD_U16(f, 16);      // bpp
    ADD_U32(f, 0);       // compression: none
    ADD_U32(f, 8);       // @todo raw size
    ADD_I32(f, 0);       // hres
    ADD_I32(f, 0);       // vres
    ADD_U32(f, 0);       // @todo num colors
    ADD_U32(f, 0);       // @todo num important colors

    // Image data (54 bytes)
    ADD_BYTES(f,
        0xe0, 0x03,  0xff, 0x7f,
        0x00, 0x7c,  0x1f, 0x00);
}
