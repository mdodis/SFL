#define SFL_BMP_ALWAYS_CONVERT 1
#define SFL_BMP_IMPLEMENTATION
#include "sfl_bmp.h"
#include <stdio.h>

typedef struct {
    const char *filepath;
    int expected_format;
} TestCase;

static TestCase Test_Cases[] = {
    {
        "data/BMP_Raw_24Bit.bmp",
        SFL_BMP_PIXEL_FORMAT_R8G8B8A8
    },
};

static int test(SflBmpReadContext *ctx, TestCase *test_case) {
}


/**
 * Test a given pixel of an image description
 * @param desc  The image description
 * @param x     The right axis offset
 * @param y     The up axis offset (screen coordinate system)
 * @param color The expected color, in R8G8B8A8 format 
 */
static inline void test_pixel(SflBmpDesc *desc, int x, int y, uint32_t color) {

    uint32_t *data = (uint32_t*)desc->data;
    
    int ry = y;
    /* flip y if image is flipped */
    if (desc->attributes & SFL_BMP_ATTRIBUTE_FLIPPED) {
        ry = desc->height - y - 1;
    }
    
    uint32_t pixel = data[ry * desc->width + x];
    /** Always converted, so format is one of the following 3*/
    switch (desc->format) {
        case SFL_BMP_PIXEL_FORMAT_B8G8R8A8:
        case SFL_BMP_PIXEL_FORMAT_B8G8R8X8: {
            // pixel = reverse_byte_order(pixel);
            uint8_t *components = (uint8_t*)(&pixel);
            uint32_t new_pixel = 
                (components[2] <<  0) |
                (components[1] <<  8) |
                (components[0] << 16) |
                (components[3] << 24);
            pixel = new_pixel;
            if (desc->format == SFL_BMP_PIXEL_FORMAT_B8G8R8X8) {
                ((uint8_t*)(&pixel))[3] = 0xff;
            }
        } break;
        default: break;
    }

    uint8_t *expected = (uint8_t*)&color;
    uint8_t *test = (uint8_t*)&pixel;
    
    for (int i = 0; i < 4; ++i) {
        if (expected[i] != test[i]) {
            printf("%d %d pixel test color mismatch: %08x != (expected) %08x\n",
                x, y,
                pixel,
                color);
            return;
        }
    }
}

/**
 * Invocation: <executable>
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
    sfl_bmp_read_context_stdio_init(&read_context);
    sfl_bmp_read_context_stdio_set_file(&read_context, f);

    SflBmpDesc desc;
    if (sfl_bmp_read_context_decode(&read_context, &desc) == 0) {
        perror("Failed.");
        return -1;
    }

    printf("Image format: %s\n",
        sfl_bmp_describe_pixel_format(desc.format));

    printf("Image is flipped: %d\n", 
        desc.attributes & SFL_BMP_ATTRIBUTE_FLIPPED);

    test_pixel(&desc, 0, 0, 0xff0000ff);
    test_pixel(&desc, 1, 0, 0xffff0000);
    test_pixel(&desc, 0, 1, 0xff00ff00);
    test_pixel(&desc, 1, 1, 0xffffffff);

    return 0;
}
