
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

#define __SSE4_1__
#define __SSE4_2__

#include "utf8-encoding/utf8_utils.h"
#include "utf8-encoding/fromutf8-sse.h"
#include "utf8-encoding/utf8_decode_sse41.h"

#include "CPUWarmUp.h"
#include "StopWatch.h"

using namespace utf8;

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
static inline
uint32_t rand_unicode()
{
    uint32_t r = next_random_u32();
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
    uint32_t r = next_random_u32();
    int len = 1 + (r & 0x03);
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

#define IS_SURROGATE(c)  (c)

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
        std::size_t skip = utf8_encode(unicode, p);
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
        std::size_t skip = utf8_encode(unicode, p);
        p += skip;
    }
    while (p < end) {
        *p++ = '\0';
    }
    return p;
}

static
uint64_t buffer_decode_v1(void * buf, size_t size)
{
    uint64_t check_sum = 0;
    char * p = (char *)buf;
    char * end = p + size;
    while (p < end) {
        size_t skip;
        uint32_t unicode = utf8_decode(p, skip);
        check_sum += unicode;
        p += skip;
    }
    return check_sum;
}

static
uint64_t buffer_decode_v2(void * buf, size_t size, void * output)
{
    uint64_t check_sum = 0;
    check_sum += fromUtf8_sse((const char *)buf, size, (uint16_t *)output);
    return check_sum;
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
    static const size_t kTextSize = 64 * 1024 * 1024;
#else
    static const size_t kTextSize = 16 * 1024;
#endif

    size_t textSize = kTextSize;
    size_t utf8_BufSize = textSize * sizeof(char);
    size_t mb4_BufSize  = textSize * sizeof(uint32_t);
    void * utf8_text    = (void *)malloc(utf8_BufSize);
    void * unicode_text = (void *)malloc(mb4_BufSize);
    if (utf8_text != nullptr) {
        printf("buffer init start.\n");
        buffer_fill(utf8_text, utf8_BufSize);
        std::memset(unicode_text, 0, mb4_BufSize);
        printf("buffer init done.\n\n");

        test::StopWatch sw;
        uint64_t check_sum = 0;

        {
            sw.start();
            check_sum += buffer_decode_v1(utf8_text, utf8_BufSize);
            sw.stop();

            double elapsed_time = sw.getElapsedMillisec();
            double throughput = (double)utf8_BufSize / (elapsed_time / 1000.0) / (1024 * 1024);

            printf("check_sum = %" PRIuPTR "\n\n", check_sum);
            printf("elapsed_time: %0.2f ms, throughput: %0.3f MiB/s\n\n",
                   elapsed_time, throughput);
        }

        check_sum = 0;
        std::size_t unicode_len = 0;
        {
            sw.start();
            unicode_len = buffer_decode_v2(utf8_text, utf8_BufSize, unicode_text);
            sw.stop();

            double elapsed_time = sw.getElapsedMillisec();
            double throughput = (double)unicode_len / (elapsed_time / 1000.0) / (1024 * 1024);

            check_sum = unicode_buffer_checksum((uint16_t *)unicode_text, unicode_len);

            printf("check_sum = %" PRIuPTR ", unicode_len = %" PRIuPTR "\n\n", check_sum, unicode_len);
            printf("elapsed_time: %0.2f ms, throughput: %0.3f MiB/s\n\n",
                   elapsed_time, throughput);
        }

        free(unicode_text);
        free(utf8_text);
    }
}

int main(int argc, char * argv[])
{
    printf("Utf8-encoding benchmark.\n\n");

    test::CPU::WarmUp warmUp(1000);

    srand((unsigned)time(NULL));

    benchmark();

#ifdef _DEBUG
    ::system("pause");
#endif
    return 0;
}
