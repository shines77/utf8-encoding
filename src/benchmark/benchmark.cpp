
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <string>
#include <cstring>
#include <memory>

#ifndef __SSE4_1__
#define __SSE4_1__
#endif

#ifndef __SSE4_2__
#define __SSE4_2__
#endif

#include "utf8-encoding/utf8_utils.h"
#include "utf8-encoding/fromutf8-sse.h"
#include "utf8-encoding/utf8_decode_sse41.h"

#include "CPUWarmUp.h"
#include "StopWatch.h"

static const size_t KiB = 1024;
static const size_t MiB = 1024 * KiB;
static const size_t GiB = 1024 * MiB;

#define IS_SURROGATE(ucs4)  (((ucs4) - 0xD800u) < 2048u)

static inline
uint32_t next_random_u32()
{
#if (RAND_MAX == 0x7FFF)
    uint32_t rnd32 = (((uint32_t)rand() & 0x03) << 30) |
                      ((uint32_t)rand() << 15) |
                       (uint32_t)rand();
#else
    uint32_t rnd32 = ((uint32_t)rand() << 16) | (uint32_t)rand();
#endif
    return rnd32;
}

static inline
uint64_t next_random_u64()
{
#if (RAND_MAX == 0x7FFF)
    uint64_t rnd64 = (((uint64_t)rand() & 0x0F) << 60) |
                      ((uint64_t)rand() << 45) |
                      ((uint64_t)rand() << 30) |
                      ((uint64_t)rand() << 15) |
                       (uint64_t)rand();
#else
    uint64_t rnd64 = ((uint64_t)rand() << 32) | (uint64_t)rand();
#endif
    return rnd64;
}

/* Generate a random codepoint whose UTF-8 length is uniformly selected. */
//
// See: https://blog.csdn.net/sdibt513/article/details/89641187
// See: https://blog.csdn.net/hyongilfmmm/article/details/112037596
//
static inline
uint32_t rand_unicode()
{
Retry:
    uint32_t r = next_random_u32();
    // U+4E00 ~ U+9FA5: (CJK Unified Ideographs)
    // 0x4E00 - 0x9FBF: (CJK Unified Ideographs)
    // 0xA000 - 0xA48F: (Yi Syllables)
    if (r > 0x9FA5u && r < 0xA000u)
        goto Retry;
    if (r >= 0xD800u && r <= 0xDFFFu)
        goto Retry;
    if (r >= 0xE000u && r <= 0xF8FFu)
        goto Retry;
    if (r >= 0xFFF0u && r <= 0xFFFFu)
        goto Retry;
    int len = 1 + (r % 3);
    r >>= 2;
    switch (len) {
        case 1:
            return (r % 128);
        case 2:
            return (128 + (r % (2048 - 128)));
        case 3:
            return (2048 + (r % (65536 - 2048)));
    }
    abort();
}

/* Generate a random codepoint whose UTF-8 length is uniformly selected. */
static inline
uint32_t rand_unicode_mb4()
{
Retry:
    uint32_t r = next_random_u32();
    // U+4E00 ~ U+9FA5: (CJK Unified Ideographs)
    // 0x4E00 - 0x9FBF: (CJK Unified Ideographs)
    // 0xA000 - 0xA48F: (Yi Syllables)
    if (r > 0x9FA5u && r < 0xA000u)
        goto Retry;
    if (r >= 0xD800u && r <= 0xDFFFu)
        goto Retry;
    if (r >= 0xE000u && r <= 0xF8FFu)
        goto Retry;
    if (r >= 0xFFF0u && r <= 0xFFFFu)
        goto Retry;
    int len = 1 + (r & 0x03u);
    r >>= 2;
    switch (len) {
        case 1:
            return (r % 128);
        case 2:
            return (128 + (r % (2048 - 128)));
        case 3:
            return (2048 + (r % (65536 - 2048)));
        case 4:
            return (65536 + (r % (131072 - 65536)));
    }
    abort();
}

/*
 * Fill buffer with random characters, with evenly-distributed encoded lengths.
 */
static
void * buffer_fill(void * buf, size_t size)
{
    char * p = (char *)buf;
    char * end = p + size;
    while ((p + 4) < end) {
        uint32_t unicode = rand_unicode();
        std::size_t skip = utf8::utf8_encode(unicode, p);
        p += skip;
    }
    while (p < end) {
        *p++ = '\0';
    }
    return p;
}

static
void * mb4_buffer_fill(void * buf, size_t size)
{
    char * p = (char *)buf;
    char * end = p + size;
    while ((p + 4) < end) {
        uint32_t unicode = rand_unicode_mb4();
        std::size_t skip = utf8::utf8_encode(unicode, p);
        p += skip;
    }
    while (p < end) {
        *p++ = '\0';
    }
    return p;
}

static
uint64_t buffer_decode_v1_checksum(void * buf, size_t size)
{
    uint64_t check_sum = 0;
    char * p = (char *)buf;
    char * end = p + size;
    while (p < end) {
        size_t skip;
        uint32_t unicode = utf8::utf8_decode(p, skip);
        check_sum += unicode;
        p += skip;
    }
    return check_sum;
}

static
size_t buffer_decode_v1(void * buf, size_t size, void * output)
{
    char * p = (char *)buf;
    char * end = p + size;
    uint16_t * unicode = (uint16_t *)output;
    uint16_t * unicode_first = (uint16_t *)output;
    while (p < end) {
        size_t skip;
        uint32_t code_point = utf8::utf8_decode(p, skip);
        *unicode++ = code_point;
        p += skip;
    }
    return (size_t)((unicode - unicode_first) * sizeof(uint16_t));
}

static
size_t buffer_decode_v2(void * buf, size_t size, void * output)
{
    size_t unicode_len = fromUtf8_sse((const char *)buf, size, (uint16_t *)output);
    return unicode_len;
}

uint64_t unicode_buffer_checksum(uint16_t * unicode_text, size_t unicode_len)
{
    uint64_t check_sum = 0;
    uint16_t * p   = unicode_text;
    uint16_t * end = unicode_text + unicode_len;
    while (p < end) {
        uint32_t unicode = *p++;
        check_sum += unicode;
    }

    return check_sum;
}

void benchmark()
{
#ifndef _DEBUG
    static const size_t kTextSize = 64 * MiB;
#else
    static const size_t kTextSize = 16 * KiB;
#endif

    size_t textSize = kTextSize;
    size_t utf8_BufSize     = textSize * sizeof(char);
    size_t mb4_BufSize      = textSize * sizeof(uint32_t);
    void * utf8_text        = (void *)malloc(utf8_BufSize);
    void * unicode_text_0   = (void *)malloc(mb4_BufSize);
    void * unicode_text_1   = (void *)malloc(mb4_BufSize);
    if (utf8_text != nullptr) {
        printf("buffer init begin.\n");
        buffer_fill(utf8_text, utf8_BufSize);
        if (unicode_text_0 != nullptr)
            std::memset(unicode_text_0, 0, mb4_BufSize);
        if (unicode_text_1 != nullptr)
            std::memset(unicode_text_1, 0, mb4_BufSize);
        printf("buffer init done.\n\n");

        test::StopWatch sw;

        if (unicode_text_0 != nullptr) {
            sw.start();
            std::size_t unicode_len = buffer_decode_v1(utf8_text, utf8_BufSize, unicode_text_0);
            sw.stop();

            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;

            uint64_t check_sum = unicode_buffer_checksum((uint16_t *)unicode_text_0, unicode_len);

            printf("check_sum = %" PRIuPTR ", utf8_BufSize = %0.2f MiB\n\n",
                   check_sum, (double)utf8_BufSize / MiB);
            printf("elapsed_time: %0.2f ms, throughput: %0.3f MiB/s\n\n",
                   elapsed_time / 1000.0, throughput);
        }
        
        if (unicode_text_1 != nullptr) {
            sw.start();
            std::size_t unicode_len = buffer_decode_v2(utf8_text, utf8_BufSize, unicode_text_1);
            sw.stop();

            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)unicode_len * 2 / elapsed_time / MiB;

            uint64_t check_sum = unicode_buffer_checksum((uint16_t *)unicode_text_1, unicode_len);

            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB\n\n",
                   check_sum, (double)unicode_len * 2 / MiB);
            printf("elapsed_time: %0.2f ms, throughput: %0.3f MiB/s\n\n",
                   elapsed_time / 1000.0, throughput);
        }

        if (unicode_text_0 != nullptr)
            free(unicode_text_0);
        if (unicode_text_1 != nullptr)
            free(unicode_text_1);
        free(utf8_text);
    }
}

int main(int argc, char * argv[])
{
    printf("\n");
    printf("Utf8-encoding benchmark v1.0.0.\n");
    printf("\n");

    test::CPU::WarmUp warmUp(1000);

    srand((unsigned)time(NULL));

    benchmark();

#ifdef _DEBUG
    ::system("pause");
#endif
    return 0;
}
