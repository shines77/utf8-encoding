
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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

#include "utf8-encoding/fromutf8-sse.h"
#include "utf8-encoding/utf8_utils.h"
#include "utf8-encoding/utf8_decode_sse41.h"

#include "CPUWarmUp.h"
#include "StopWatch.h"

static const size_t KiB = 1024;
static const size_t MiB = 1024 * KiB;
static const size_t GiB = 1024 * MiB;

static const double kSecond     = 1.0;
static const double kMillisecs  = 1000.0;
static const double kMicrosecs  = 1000.0 * kMillisecs;
static const double kNanosecs   = 1000.0 * kMicrosecs;

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

// Returns a number in the range [min, max) - uint32
template <uint32_t min_val, uint32_t max_val>
static inline
uint32_t get_range_u32(uint32_t num)
{
    if (min_val < max_val) {
        return (min_val + (num % (uint32_t)(max_val - min_val)));
    } else if (min_val > max_val) {
        return (max_val + (num % (uint32_t)(min_val - max_val)));
    } else {
        return num;
    }
}

// Returns a number in the range [min, max) - uint64
template <uint64_t min_val, uint64_t max_val>
static inline
uint64_t get_range_u32(uint64_t num)
{
    if (min_val < max_val) {
        return (min_val + (num % (uint64_t)(max_val - min_val)));
    } else if (min_val > max_val) {
        return (max_val + (num % (uint64_t)(min_val - max_val)));
    } else {
        return num;
    }
}

/* Generate a random codepoint whose UTF-8 length is uniformly selected. */
//
// See: https://blog.csdn.net/sdibt513/article/details/89641187
// See: https://blog.csdn.net/hyongilfmmm/article/details/112037596
//
static inline
uint32_t rand_unicode()
{
    uint32_t code_point;
Retry:
    uint32_t rnd = next_random_u32();
    int len = (rnd % 3);
    rnd >>= 2;
    switch (len) {
        case 0:
            code_point = get_range_u32<32, 128>(rnd);
            break;

        case 1:
            code_point = get_range_u32<128, 2048>(rnd);
            break;

        case 2:
            code_point = get_range_u32<2048, 65536>(rnd);
            // U+4E00 ~ U+9FA5: (CJK Unified Ideographs)
            // 0x4E00 - 0x9FBF: (CJK Unified Ideographs)
            // 0xA000 - 0xA48F: (Yi Syllables)
            if (code_point > 0x9FA5u && code_point < 0xA000u)
                goto Retry;
            if (code_point >= 0xD800u && code_point <= 0xDFFFu)
                goto Retry;
            if (code_point >= 0xE000u && code_point <= 0xF8FFu)
                goto Retry;
            if (code_point >= 0xFFF0u && code_point <= 0xFFFFu)
                goto Retry;
            break;

        default:
            code_point = ' ';
            assert(false);
            break;
    }
    return code_point;
}

/* Generate a random codepoint whose UTF-8 length is uniformly selected. */
static inline
uint32_t rand_unicode_mb4()
{
    uint32_t code_point;
Retry:
    uint32_t rnd = next_random_u32();
    int len = (rnd % 4);
    rnd >>= 2;
    switch (len) {
        case 0:
            code_point = get_range_u32<32, 128>(rnd);
            break;

        case 1:
            code_point = get_range_u32<128, 2048>(rnd);
            break;

        case 2:
            code_point = get_range_u32<2048, 65536>(rnd);
            // U+4E00 ~ U+9FA5: (CJK Unified Ideographs)
            // 0x4E00 - 0x9FBF: (CJK Unified Ideographs)
            // 0xA000 - 0xA48F: (Yi Syllables)
            if (code_point > 0x9FA5u && code_point < 0xA000u)
                goto Retry;
            if (code_point >= 0xD800u && code_point <= 0xDFFFu)
                goto Retry;
            if (code_point >= 0xE000u && code_point <= 0xF8FFu)
                goto Retry;
            if (code_point >= 0xFFF0u && code_point <= 0xFFFFu)
                goto Retry;
            break;

        case 3:
            code_point = get_range_u32<65536, 131072>(rnd);
            break;

        default:
            code_point = ' ';
            assert(false);
            break;
    }
    return code_point;
}

/*
 * Fill buffer with random characters, with evenly-distributed encoded lengths.
 */
static
void * mb3_buffer_fill(void * buf, size_t size)
{
    char * p = (char *)buf;
    char * end = p + size;
    while ((p + 3) <= end) {
        uint32_t code_point = rand_unicode();
        std::size_t skip = utf8::utf8_encode(code_point, p);
        p += skip;
    }
    while (p < end) {
        *p++ = (uint8_t)((rand() % 127) + 1);
    }
    return p;
}

static
void * mb4_buffer_fill(void * buf, size_t size)
{
    char * p = (char *)buf;
    char * end = p + size;
    while ((p + 4) <= end) {
        uint32_t code_point = rand_unicode_mb4();
        std::size_t skip = utf8::utf8_encode(code_point, p);
        p += skip;
    }
    while (p < end) {
        *p++ = (uint8_t)((rand() % 127) + 1);
    }
    return p;
}

static
uint64_t mb3_buffer_decode_checksum(void * buf, size_t size)
{
    uint64_t check_sum = 0;
    char * p = (char *)buf;
    char * end = p + size;
    while ((p + 3) <= end) {
        size_t skip;
        uint32_t code_point = utf8::utf8_decode(p, skip);
        check_sum += code_point;
        p += skip;
    }

    {
        size_t skip;
        uint32_t code_point = utf8::utf8_decode(p, skip);
        p += skip;
        if (p <= end) {
            check_sum += code_point;
        }
    }
    return check_sum;
}

static
size_t mb3_buffer_decode(void * buf, size_t size, void * output)
{
    char * p = (char *)buf;
    char * end = p + size;
    uint16_t * unicode = (uint16_t *)output;
    uint16_t * unicode_first = (uint16_t *)output;
    while ((p + 3) <= end) {
        size_t skip;
        uint32_t code_point = utf8::utf8_decode(p, skip);
        assert(code_point <= 0xFFFFu);
        *unicode++ = code_point;
        p += skip;
    }

    {
        size_t skip;
        uint32_t code_point = utf8::utf8_decode(p, skip);
        p += skip;
        if (p <= end) {
            assert(code_point <= 0xFFFFu);
            *unicode++ = code_point;
        }
    }
    return (size_t)(unicode - unicode_first);
}

static inline
size_t mb3_buffer_decode_sse(void * buf, size_t size, void * output)
{
    size_t unicode_len = fromUtf8_sse((const char *)buf, size, (uint16_t *)output);
    return unicode_len;
}

static inline
size_t mb3_buffer_decode_sse2(void * buf, size_t size, void * output)
{
    size_t unicode_len = utf8::utf8_decode_sse41((const char *)buf, size, (uint16_t *)output);
    return unicode_len;
}

uint64_t unicode16_buffer_checksum(uint16_t * unicode_text, size_t unicode_len)
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

bool file_is_exists(const char * filename)
{
    bool is_exists = false;
    if (filename != nullptr) {
        FILE * fp = fopen(filename, "r");
        if (fp != nullptr) {
            is_exists = true;
            fclose(fp);
        }
    }

    return is_exists;
}

size_t read_text_file(const char * filename, void ** text)
{
    size_t read_bytes = 0;
    void * buffer = nullptr;

    if (filename != nullptr) {
        FILE * fp = fopen(filename, "rb");
        if (fp != nullptr) {
            fseek(fp, 0, SEEK_END);
            size_t file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            if (file_size != 0) {
                buffer = malloc(file_size);
                if (buffer != nullptr) {
                    read_bytes = fread(buffer, sizeof(char), file_size, fp);
                    assert(read_bytes == file_size);
                }
            }

            if (text) {
                *text = buffer;
            }
            
            fclose(fp);
        }
    } else {
        if (text) {
            *text = nullptr;
        }
    }
    return read_bytes;
}

void mb_buffer_save(const char * filename, const char * buffer, size_t size)
{
    FILE * fp = fopen(filename, "wb");
    if (fp != NULL) {
        fwrite((void *)buffer, sizeof(char), size, fp);
        fclose(fp);
    }
}

void unicode16_buffer_save(const char * filename, const uint16_t * buffer, size_t size)
{
    FILE * fp = fopen(filename, "wb");
    if (fp != NULL) {
        fwrite((void *)buffer, sizeof(uint16_t), size, fp);
        fclose(fp);
    }
}

void rand_mb3_benchmark(size_t text_capacity, bool save_to_file)
{
    size_t unicode_len_0, unicode_len_1, unicode_len_2;

    printf("----------------------------------------------------------------------\n\n");
    printf("rand_mb3_benchmark(): text_capacity = %0.2f MiB (%" PRIuPTR " bytes)\n\n",
           (double)text_capacity / MiB, text_capacity);

    size_t textSize         = text_capacity;
    size_t utf8_BufSize     = textSize * sizeof(char);
    size_t utf16_BufSize    = textSize * sizeof(uint16_t);
    void * utf8_text        = (void *)malloc(utf8_BufSize);
    void * unicode_text_0   = (void *)malloc(utf16_BufSize);
    void * unicode_text_1   = (void *)malloc(utf16_BufSize);
    void * unicode_text_2   = (void *)malloc(utf16_BufSize);
    if (utf8_text != nullptr) {
        printf("buffer init begin.\n");
        // Gerenate random unicode chars (Multi-bytes <= 3)
        mb3_buffer_fill(utf8_text, utf8_BufSize);
        if (unicode_text_0 != nullptr)
            std::memset(unicode_text_0, 0, utf16_BufSize);
        if (unicode_text_1 != nullptr)
            std::memset(unicode_text_1, 0, utf16_BufSize);
        if (unicode_text_2 != nullptr)
            std::memset(unicode_text_2, 0, utf16_BufSize);
        printf("buffer init done.\n\n");

        test::StopWatch sw;

        if (unicode_text_0 != nullptr) {
            sw.start();
            std::size_t unicode_len = mb3_buffer_decode(utf8_text, utf8_BufSize, unicode_text_0);
            sw.stop();

            unicode_len_0 = unicode_len;
            std::size_t unicode_bytes = unicode_len * sizeof(uint16_t);
            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;
            double tick = elapsed_time * kNanosecs / utf8_BufSize;

            uint64_t check_sum = unicode16_buffer_checksum((uint16_t *)unicode_text_0, unicode_len);

            printf("utf8::utf8_encode():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed_time: %0.2f ms, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMillisecs, throughput, tick);
        }
        
        if (unicode_text_1 != nullptr) {
            sw.start();
            std::size_t unicode_len = mb3_buffer_decode_sse(utf8_text, utf8_BufSize, unicode_text_1);
            sw.stop();

            unicode_len_1 = unicode_len;
            std::size_t unicode_bytes = unicode_len * sizeof(uint16_t);
            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;
            double tick = elapsed_time * kNanosecs / utf8_BufSize;

            uint64_t check_sum = unicode16_buffer_checksum((uint16_t *)unicode_text_1, unicode_len);

            printf("fromUtf8_sse41():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed_time: %0.2f ms, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMillisecs, throughput, tick);
        }

        if (unicode_text_2 != nullptr) {
            sw.start();
            std::size_t unicode_len = mb3_buffer_decode_sse2(utf8_text, utf8_BufSize, unicode_text_2);
            sw.stop();

            unicode_len_2 = unicode_len;
            std::size_t unicode_bytes = unicode_len * sizeof(uint16_t);
            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;
            double tick = elapsed_time * kNanosecs / utf8_BufSize;

            uint64_t check_sum = unicode16_buffer_checksum((uint16_t *)unicode_text_2, unicode_len);

            printf("utf8::utf8_decode_sse41():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed_time: %0.2f ms, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMillisecs, throughput, tick);
        }

        if (unicode_text_0 != nullptr) {
            if (save_to_file)
                unicode16_buffer_save("rand_unicode_text_0.txt", (const uint16_t *)unicode_text_0, unicode_len_0);
            free(unicode_text_0);
        }
        if (unicode_text_1 != nullptr) {
            if (save_to_file)
                unicode16_buffer_save("rand_unicode_text_1.txt", (const uint16_t *)unicode_text_1, unicode_len_1);
            free(unicode_text_1);
        }
        if (unicode_text_2 != nullptr) {
            if (save_to_file)
                unicode16_buffer_save("rand_unicode_text_2.txt", (const uint16_t *)unicode_text_2, unicode_len_2);
            free(unicode_text_2);
        }

        if (save_to_file) {
            mb_buffer_save("rand_utf8_text.txt", (const char *)utf8_text, utf8_BufSize);
        }
        free(utf8_text);
    }

    printf("----------------------------------------------------------------------\n\n");
}

void text_mb3_benchmark(const char * text_file, bool save_to_file)
{
    void * utf8_text = nullptr;
    size_t text_capacity = read_text_file(text_file, &utf8_text);
    if (text_capacity == 0 || utf8_text == nullptr) {
        printf("ERROR: text_file: %s, text_capacity: %" PRIuPTR " bytes\n\n", text_file, text_capacity);
        return;
    }

    printf("----------------------------------------------------------------------\n\n");
    printf("text_mb3_benchmark(): text_capacity = %0.2f MiB (%" PRIuPTR " bytes)\n",
           (double)text_capacity / MiB, text_capacity);
    printf("text_file: %s\n\n", text_file);

    size_t textSize         = text_capacity;
    size_t utf8_BufSize     = textSize * sizeof(char);
    size_t utf16_BufSize    = textSize * sizeof(uint16_t);
    
    size_t unicode_len_0, unicode_len_1, unicode_len_2;
    void * unicode_text_0   = (void *)malloc(utf16_BufSize);
    void * unicode_text_1   = (void *)malloc(utf16_BufSize);
    void * unicode_text_2   = (void *)malloc(utf16_BufSize);
    if (utf8_text != nullptr) {
        printf("buffer init begin.\n");
        if (unicode_text_0 != nullptr)
            std::memset(unicode_text_0, 0, utf16_BufSize);
        if (unicode_text_1 != nullptr)
            std::memset(unicode_text_1, 0, utf16_BufSize);
        if (unicode_text_2 != nullptr)
            std::memset(unicode_text_2, 0, utf16_BufSize);
        printf("buffer init done.\n\n");

        test::StopWatch sw;

        if (unicode_text_0 != nullptr) {
            sw.start();
            std::size_t unicode_len = mb3_buffer_decode(utf8_text, utf8_BufSize, unicode_text_0);
            sw.stop();

            unicode_len_0 = unicode_len;
            std::size_t unicode_bytes = unicode_len * sizeof(uint16_t);
            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;
            double tick = elapsed_time * kNanosecs / utf8_BufSize;

            uint64_t check_sum = unicode16_buffer_checksum((uint16_t *)unicode_text_0, unicode_len);

            printf("utf8::utf8_encode():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed_time: %0.2f us, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMicrosecs, throughput, tick);
        }
        
        if (unicode_text_1 != nullptr) {
            sw.start();
            std::size_t unicode_len = mb3_buffer_decode_sse(utf8_text, utf8_BufSize, unicode_text_1);
            sw.stop();

            unicode_len_1 = unicode_len;
            std::size_t unicode_bytes = unicode_len * sizeof(uint16_t);
            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;
            double tick = elapsed_time * kNanosecs / utf8_BufSize;

            uint64_t check_sum = unicode16_buffer_checksum((uint16_t *)unicode_text_1, unicode_len);

            printf("fromUtf8_sse41():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed_time: %0.2f us, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMicrosecs, throughput, tick);
        }

        if (unicode_text_2 != nullptr) {
            sw.start();
            std::size_t unicode_len = mb3_buffer_decode_sse2(utf8_text, utf8_BufSize, unicode_text_2);
            sw.stop();

            unicode_len_2 = unicode_len;
            std::size_t unicode_bytes = unicode_len * sizeof(uint16_t);
            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;
            double tick = elapsed_time * kNanosecs / utf8_BufSize;

            uint64_t check_sum = unicode16_buffer_checksum((uint16_t *)unicode_text_2, unicode_len);

            printf("utf8::utf8_decode_sse41():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed_time: %0.2f us, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMicrosecs, throughput, tick);
        }

        if (unicode_text_0 != nullptr) {
            if (save_to_file)
                unicode16_buffer_save("unicode_text_0.txt", (const uint16_t *)unicode_text_0, unicode_len_0);
            free(unicode_text_0);
        }
        if (unicode_text_1 != nullptr) {
            if (save_to_file)
                unicode16_buffer_save("unicode_text_1.txt", (const uint16_t *)unicode_text_1, unicode_len_1);
            free(unicode_text_1);
        }
        if (unicode_text_2 != nullptr) {
            if (save_to_file)
                unicode16_buffer_save("unicode_text_2.txt", (const uint16_t *)unicode_text_2, unicode_len_2);
            free(unicode_text_2);
        }

        free(utf8_text);
    }

    printf("----------------------------------------------------------------------\n\n");
}

void benchmark(const char * text_file)
{
#ifndef _DEBUG
    static const size_t kTextSize      = 64 * MiB;
    static const size_t kTextSize_save = 2  * MiB;
#else
    static const size_t kTextSize      = 64 * KiB;
    static const size_t kTextSize_save = 16 * KiB;
#endif

    //rand_mb3_benchmark(kTextSize_save, true);
    rand_mb3_benchmark(kTextSize,      false);

    text_mb3_benchmark(text_file, true);
}

const char * get_default_text_file()
{
#if defined(_MSC_VER)
    const char * default_text_file_0    = "..\\..\\..\\texts\\long_chinese.txt";
    const char * default_text_file_root = ".\\texts\\long_chinese.txt";
#else
    const char * default_text_file_0    = "../texts/long_chinese.txt";
    const char * default_text_file_root = "./texts/long_chinese.txt";
#endif

    const char * default_text_file;

    bool is_exists = file_is_exists(default_text_file_0);
    if (is_exists) {
        default_text_file = default_text_file_0;
        //printf("INFO: default_text_file_0: '%s' is_exists.\n\n", default_text_file_0);
    } else {
        bool is_exists = file_is_exists(default_text_file_root);
        if (is_exists) {
            default_text_file = default_text_file_root;
            //printf("INFO: default_text_file_root: '%s' is_exists.\n\n", default_text_file_root);
        } else {
            default_text_file = nullptr;
            //printf("INFO: default_text_file: not found.\n\n");
        }
    }

    return default_text_file;
}

int main(int argc, char * argv[])
{
    printf("\n");
    printf("Utf8-encoding benchmark v1.0.0\n");
    printf("\n");

    const char * text_file = nullptr;

    if (argc > 1) {
        text_file = argv[1];
    } else {
        text_file = get_default_text_file();
    }

    srand((unsigned)time(NULL));

#ifdef _DEBUG_
    const char * test_case = "x\xe2\x89\xa4(\xce\xb1+\xce\xb2)\xc2\xb2\xce\xb3\xc2\xb2";
    uint16_t dest[32] = { 0 };

    size_t unicode_len = utf8::utf8_decode_sse41(test_case, strlen(test_case), dest);
#else
    test::CPU::WarmUp warmUper(1000);
    benchmark(text_file);
#endif

#ifdef _DEBUG
    ::system("pause");
#endif
    return 0;
}
