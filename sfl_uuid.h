/* sfl_uuid.h v0.1
A UUIDv4 generator

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

*/
#ifndef SFL_UUID_H
#define SFL_UUID_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SFL_UUID_BUFFER_SIZE (int)sizeof("xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx")

/**
 * SflUUID Layout
 * @link https://www.rfc-editor.org/rfc/rfc4122#page-7
 *
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          time_low                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |       time_mid                |         time_hi_and_version   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         node (2-5)                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#pragma pack(push, 1)
typedef union {
    uint8_t  bytes[16];
    uint64_t qwords[2];
    struct _dummy_components {
        uint32_t time_low;
        uint16_t time_mid;
        uint16_t time_hi_and_version;
        uint8_t  clock_seq_hi_and_reserved;
        uint8_t  clock_seq_low;
        uint8_t  node[6];
    } comp;
} SflUUID;
#pragma pack(pop)

typedef struct {
    uint64_t seed;
} SflUUIDContext;

extern void sfl_uuid_init(SflUUIDContext* ctx);
extern void sfl_uuid_gen_v4(SflUUIDContext* ctx, SflUUID* out);
extern void sfl_uuid_to_string(const SflUUID* uuid, char* out);
#ifdef __cplusplus
}
#endif
#endif

#ifdef SFL_UUID_IMPLEMENTATION
#include <assert.h>
#if defined(_WIN32)
#include <windows.h>
#endif

/**
 * 64 bit Hash
 * @link http://xoshiro.di.unimi.it/splitmix64.c
 *
 * Author: Sebastiano Vigna [2015] (vigna@acm.org)
 *
 * @param  state seed
 * @return       hash
 */
static inline uint64_t sfl_uuid__splitmix64(uint64_t* state)
{
    uint64_t result = (*state += 0x9E3779B97F4A7C15u);
    result          = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9u;
    result          = (result ^ (result >> 27)) * 0x94D049BB133111EBu;
    return result ^ (result >> 31);
}

/**
 * 32 bit hash
 * @link http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html
 *
 * Author: Melissa O'Neil [2015] (oneill@pcg-random.org)
 *
 * @param  value input
 * @return       hash
 */
static inline uint32_t sfl_uuid__hash(uint32_t value)
{
    static uint32_t multiplier = 0x43b0d7e5u;
    value ^= multiplier;
    multiplier *= 0x931e8875u;
    value *= multiplier;
    value ^= value >> 16;
    return value;
}

static inline uint32_t sfl_uuid__mix(uint32_t x, uint32_t y)
{
    uint32_t result = 0xca01f9ddu * x - 0x4973f715u * y;
    result ^= result >> 16;
    return result;
}

static void sfl_uuid__randomize(SflUUIDContext* ctx, SflUUID* out)
{
    out->qwords[0] = sfl_uuid__splitmix64(&ctx->seed);
    out->qwords[1] = sfl_uuid__splitmix64(&ctx->seed);
}

void sfl_uuid_init(SflUUIDContext* ctx)
{
#if defined(_WIN32)
    LARGE_INTEGER time;
    BOOL          ok = QueryPerformanceCounter(&time);
    assert(ok);

    static uint64_t state0 = 0;

    ctx->seed = state0++ + ((uintptr_t)&time ^ (uint64_t)time.QuadPart);

    uint32_t pid = (uint32_t)GetCurrentProcessId();
    uint32_t tid = (uint32_t)GetCurrentThreadId();

    // clang-format off
    ctx->seed = ctx->seed * 6364136223846793005u + ((uint64_t)(sfl_uuid__mix(sfl_uuid__hash(pid), sfl_uuid__hash(tid))) << 32);
    ctx->seed = ctx->seed * 6364136223846793005u + (uintptr_t)GetCurrentProcessId;
    ctx->seed = ctx->seed * 6364136223846793005u + (uintptr_t)GetCurrentProcessId;
    ctx->seed = ctx->seed * 6364136223846793005u + (uintptr_t)sfl_uuid_gen_v4;
    // clang-format on

#else
#error "Unsupported platform!"
#endif
}

void sfl_uuid_gen_v4(SflUUIDContext* ctx, SflUUID* out)
{
    sfl_uuid__randomize(ctx, out);
    out->bytes[6] = (out->bytes[6] & 0xf) | 0x40;
    out->bytes[8] = (out->bytes[8] & 0x3f) | 0x80;
}

void sfl_uuid_to_string(const SflUUID* uuid, char* out)
{
    static const char hex[] = {
        '0',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        'a',
        'b',
        'c',
        'd',
        'e',
        'f'};
    static const int groups[] = {8, 4, 4, 4, 12};
    int              b        = 0;

    for (int i = 0; i < (int)(sizeof(groups) / sizeof(groups[0])); ++i) {
        for (int j = 0; j < groups[i]; j += 2) {
            uint8_t byte = uuid->bytes[b++];

            *out++ = hex[byte >> 4];
            *out++ = hex[byte & 0xf];
        }
        *out++ = '-';
    }

    *--out = 0;
}

#endif