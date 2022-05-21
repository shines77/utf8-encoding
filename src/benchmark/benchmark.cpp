
#if defined(_MSC_VER) && defined(_DEBUG)
#include <vld.h>
#endif

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
#include <type_traits>

#ifndef __SSE4_1__
#define __SSE4_1__
#endif

#ifndef __SSE4_2__
#define __SSE4_2__
#endif

#include "utf8-encoding/fromutf8-sse.h"
#include "utf8-encoding/utf8_utils.h"
#include "utf8-encoding/utf8_decode_sse.h"

#include "CmdLine.h"
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
            // High Surrogate (0xD800 - 0xDBFF) & Low Surrogate (0xDC00 - 0xDFFF)
            if (code_point >= 0xD800u && code_point <= 0xDFFFu)
                goto Retry;
            if (code_point >= 0xE000u && code_point <= 0xF8FFu)
                goto Retry;
            if (code_point >= 0xFDD0u && code_point <= 0xFDEFu)
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
            // High Surrogate (0xD800 - 0xDBFF) & Low Surrogate (0xDC00 - 0xDFFF)
            if (code_point >= 0xD800u && code_point <= 0xDFFFu)
                goto Retry;
            if (code_point >= 0xE000u && code_point <= 0xF8FFu)
                goto Retry;
            if (code_point >= 0xFDD0u && code_point <= 0xFDEFu)
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
    size_t unicode_len = utf8::utf8_decode_sse((const char *)buf, size, (uint16_t *)output);
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
            printf("elapsed time: %0.2f ms, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
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
            printf("elapsed time: %0.2f ms, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
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

            printf("utf8::utf8_decode_sse():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed time: %0.2f ms, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
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
    test::StopWatch sw;

    void * utf8_text = nullptr;
    printf("read_text_file() begin.\n");
    sw.start();
    size_t text_capacity = read_text_file(text_file, &utf8_text);
    sw.stop();
    if (text_capacity == 0 || utf8_text == nullptr) {
        printf("ERROR: text_file: %s, text_capacity: %" PRIuPTR " bytes\n\n", text_file, text_capacity);
        return;
    }
    printf("read_text_file() done. time: %0.2f ms\n\n", sw.getElapsedMillisec());

    printf("----------------------------------------------------------------------\n\n");
    printf("text_mb3_benchmark(): text_capacity = %0.2f MiB (%" PRIuPTR " bytes)\n",
           (double)text_capacity / MiB, text_capacity);
    printf("text_file: \"%s\"\n\n", text_file);

    if (text_capacity >= 20 * MiB) {
        save_to_file = false;
    }

    size_t textSize         = text_capacity;
    size_t utf8_BufSize     = textSize * sizeof(char);
    size_t utf16_BufSize    = textSize * sizeof(uint16_t);

    size_t unicode_len_0, unicode_len_1, unicode_len_2;

    if (utf8_text != nullptr) {
        printf("buffer0 init begin.\n");
        void * unicode_text_0 = (void *)malloc(utf16_BufSize);
        if (unicode_text_0 != nullptr) {
            std::memset(unicode_text_0, 0, utf16_BufSize);
            printf("buffer0 init done.\n\n");

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
            printf("elapsed time: %0.2f us, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMicrosecs, throughput, tick);

            if (save_to_file)
                unicode16_buffer_save("unicode_text_0.txt", (const uint16_t *)unicode_text_0, unicode_len_0);
            free(unicode_text_0);
            unicode_text_0 = nullptr;
        }

        printf("buffer1 init begin.\n");
        void * unicode_text_1   = (void *)malloc(utf16_BufSize);
        if (unicode_text_1 != nullptr) {
            std::memset(unicode_text_1, 0, utf16_BufSize);
            printf("buffer1 init done.\n\n");

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
            printf("elapsed time: %0.2f us, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMicrosecs, throughput, tick);

            if (save_to_file)
                unicode16_buffer_save("unicode_text_1.txt", (const uint16_t *)unicode_text_1, unicode_len_1);
            free(unicode_text_1);
            unicode_text_1 = nullptr;
        }

        printf("buffer2 init begin.\n");
        void * unicode_text_2   = (void *)malloc(utf16_BufSize);
        if (unicode_text_2 != nullptr) {
            std::memset(unicode_text_2, 0, utf16_BufSize);
            printf("buffer2 init done.\n\n");

            sw.start();
            std::size_t unicode_len = mb3_buffer_decode_sse2(utf8_text, utf8_BufSize, unicode_text_2);
            sw.stop();

            unicode_len_2 = unicode_len;
            std::size_t unicode_bytes = unicode_len * sizeof(uint16_t);
            double elapsed_time = sw.getElapsedSecond();
            double throughput = (double)utf8_BufSize / elapsed_time / MiB;
            double tick = elapsed_time * kNanosecs / utf8_BufSize;

            uint64_t check_sum = unicode16_buffer_checksum((uint16_t *)unicode_text_2, unicode_len);

            printf("utf8::utf8_decode_sse():\n\n");
            printf("check_sum = %" PRIuPTR ", unicode_len = %0.2f MiB (%" PRIuPTR ")\n\n",
                   check_sum, (double)unicode_len / MiB, unicode_len);
            printf("elapsed time: %0.2f us, throughput: %0.2f MiB/s, tick = %0.3f ns/byte\n\n",
                   elapsed_time * kMicrosecs, throughput, tick);

            if (save_to_file)
                unicode16_buffer_save("unicode_text_2.txt", (const uint16_t *)unicode_text_2, unicode_len_2);
            free(unicode_text_2);
            unicode_text_2 = nullptr;
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

template <typename T>
void is_array_char(const T & src)
{
    typedef typename std::remove_reference<T>::type U;
    std::cout << "is_array_char(const T & src);" << "\n";
    std::cout << "typeid(T).name() = " << typeid(T).name() << "\n";
    std::cout << "typeid(U).name() = " << typeid(U).name() << "\n";
    std::cout << "std::is_array<T>::value: " << std::is_array<T>::value << '\n';
    std::cout << "std::is_array<U>::value: " << std::is_array<U>::value << '\n';
    std::cout << "\n";
}

template <typename T>
void is_array_char(T && src)
{
    typedef typename std::remove_reference<T>::type U;
    std::cout << "is_array_char(T && src);" << "\n";
    std::cout << "typeid(T).name() = " << typeid(T).name() << "\n";
    std::cout << "typeid(U).name() = " << typeid(U).name() << "\n";
    std::cout << "std::is_array<T>::value: " << std::is_array<T>::value << '\n';
    std::cout << "std::is_array<U>::value: " << std::is_array<U>::value << '\n';
    std::cout << "\n";
}

void is_array_test()
{
    class A {};

    std::cout << std::boolalpha;
    std::cout << "std::is_array<A>::value: " << std::is_array<A>::value << '\n';
    std::cout << "std::is_array<A[]>::value: " << std::is_array<A[]>::value << '\n';
    std::cout << "std::is_array<A[3]>::value: " << std::is_array<A[3]>::value << '\n';
    std::cout << "std::is_array<float>::value: " << std::is_array<float>::value << '\n';
    std::cout << "std::is_array<int>::value: " << std::is_array<int>::value << '\n';
    std::cout << "std::is_array<int[]>::value: " << std::is_array<int[]>::value << '\n';
    std::cout << "std::is_array<int[3]>::value: " << std::is_array<int[3]>::value << '\n';
    std::cout << "std::is_array<std::array<int, 3>::value: " << std::is_array<std::array<int, 3>>::value << '\n';

    char buff[128] = "";
    const char cbuff[64] = "dfffffffffffffff";
    static const char sbuff[64] = "dfffffffffffffff";
    std::cout << "std::is_array<char [19]>::value: " << std::is_array<char [19]>::value << '\n';
    std::cout << "std::is_array<const char[19]>::value: " << std::is_array<const char[19]>::value << '\n';
    std::cout << "std::is_array<char *[19]>::value: " << std::is_array<char *[19]>::value << '\n';
    std::cout << "std::is_array<const char *[19]>::value: " << std::is_array<const char *[19]>::value << '\n';
    std::cout << "\n";

    is_array_char(buff);
    is_array_char(cbuff);
    is_array_char(sbuff);
}

template <typename ReturnType, typename... Args>
void visit(std::function<ReturnType(Args...)> && func)
{
    typedef typename jstd::function_traits<std::function<ReturnType(Args...)>>::arg0 arg0;
    std::cout << "visit<std::function<ReturnType(Args...) && func>(): " << typeid(func).name() << "\n";
    std::cout << "\n";
    std::cout << "arg0: " << typeid(arg0).name() << "\n";
    std::cout << "\n";
}

template <typename ReturnType, typename Functor, typename... Args, typename... Args2>
void visit(ReturnType(Functor::*functor)(Args...), Args2 &&... args)
{
    typedef typename jstd::function_traits<ReturnType(Functor::*)(Args...)>::type type;
    typedef typename jstd::function_traits<ReturnType(Functor::*)(Args...)>::arg0 arg0;
    std::cout << "visit<Functor && functor>(): " << typeid(functor).name() << "\n";
    std::cout << "\n";
    std::cout << "type: " << typeid(type).name() << "\n";
    std::cout << "\n";
    std::cout << "arg0: " << typeid(arg0).name() << "\n";
    std::cout << "\n";
}

template <typename Functor, typename... Args>
void visit(decltype(&Functor::operator()) && functor, Args &&... args)
{
    typedef typename jstd::function_traits<decltype(&Functor::operator())>::type type;
    typedef typename jstd::function_traits<decltype(&Functor::operator())>::arg0 arg0;
    std::cout << "visit<Functor && functor>(): " << typeid(std::forward<Functor>(functor)).name() << "\n";
    std::cout << "\n";
    std::cout << "type: " << typeid(type).name() << "\n";
    std::cout << "\n";
    std::cout << "arg0: " << typeid(arg0).name() << "\n";
    std::cout << "\n";
}

#if 1
template <typename Functor, typename... Args>
void visit(Functor && functor, Args &&... args)
{
    typedef typename jstd::function_traits<Functor>::type type;
    typedef typename jstd::function_traits<Functor>::arg0 arg0;
    std::cout << "visit<Functor && functor>(): " << typeid(std::forward<Functor>(functor)).name() << "\n";
    std::cout << "\n";
    std::cout << "type: " << typeid(type).name() << "\n";
    std::cout << "\n";
    std::cout << "arg0: " << typeid(arg0).name() << "\n";
    std::cout << "\n";
}
#endif

void variant_test()
{

    typedef app::Variant variant_t;

    typedef jstd::Variant<unsigned long long, long long,
                          bool, char, short, int, long,
                          unsigned char, unsigned short, unsigned int, unsigned long,
                          signed char, wchar_t, char16_t, char32_t,
                          float, double, long double,
                          std::string, std::wstring,
                          void *, const void *,
                          char *, const char *,
                          short *, const short *,
                          wchar_t *, const wchar_t *,
                          char16_t *, const char16_t *,
                          char32_t *, const char32_t *
            > variant2_t;

    printf("\n");
    printf("variant_t::kDataSize = %u, variant_t::kAlignment = %u\n\n",
           (uint32_t)variant_t::kDataSize,
           (uint32_t)variant_t::kAlignment);

    try {
        char buf[128] = "abcdefg";
        const char cbuf[128] = "ABCDEFG";
        variant_t ctor;
        variant_t str0(std::string("str"));
        variant_t str1 = std::string("text");
        variant_t str2 = (const char *)"fixed string";
        variant_t str3 = buf; // "fixed string array";
        variant_t int0 = 123;
        ctor = cbuf; // "fixed string array";

        printf("str0 = \"%s\", \t\t str0.index() = %u\n", str0.get<std::string>().c_str(), (uint32_t)str0.index());
        printf("str1 = \"%s\", \t\t str1.index() = %u\n", str1.get<std::string>().c_str(), (uint32_t)str1.index());
        printf("str2 = \"%s\", \t str2.index() = %u\n",   str2.get<const char *>(),        (uint32_t)str2.index());
        printf("str3 = \"%s\", \t str3.index() = %u\n",   str3.get<char *>(),              (uint32_t)str3.index());
        printf("int0 = %d,     \t int0.index() = %u\n",   int0.get<int>(),                 (uint32_t)int0.index());
        printf("ctor = \"%s\", \t ctor.index() = %u\n",   ctor.get<const char *>(),        (uint32_t)ctor.index());
        printf("\n");

        ctor.set<int>(1234567);
        str0.set(std::string("1234"));
        str2.set(buf);
        str3.set(cbuf);

        printf("str0 = \"%s\", \t\t str0.index() = %u\n", str0.get<std::string>().c_str(), (uint32_t)str0.index());
        printf("str1 = \"%s\", \t\t str1.index() = %u\n", str1.get<std::string>().c_str(), (uint32_t)str1.index());
        printf("str2 = \"%s\", \t str2.index() = %u\n",   str2.get<char *>(),              (uint32_t)str2.index());
        printf("str3 = \"%s\", \t str3.index() = %u\n",   str3.get<const char *>(),        (uint32_t)str3.index());
        printf("int0 = %d,     \t int0.index() = %u\n",   int0.get<int>(),                 (uint32_t)int0.index());
        printf("ctor = %d,     \t ctor.index() = %u\n",   ctor.get<int>(),                 (uint32_t)ctor.index());
        printf("\n");

        ctor = buf;
        str0.set(cbuf);
        str2.set(std::string("1234"));
        str3.set(buf);

        printf("str0 = \"%s\", \t str0.index() = %u\n",   str0.get<const char *>(),        (uint32_t)str0.index());
        printf("str1 = \"%s\", \t\t str1.index() = %u\n", str1.get<std::string>().c_str(), (uint32_t)str1.index());
        printf("str2 = \"%s\", \t\t str2.index() = %u\n", str2.get<std::string>().c_str(), (uint32_t)str2.index());
        printf("str3 = \"%s\", \t str3.index() = %u\n",   str3.get<char *>(),              (uint32_t)str3.index());
        printf("int0 = %d,     \t int0.index() = %u\n",   int0.get<int>(),                 (uint32_t)int0.index());
        printf("ctor = \"%s\", \t ctor.index() = %u\n",   ctor.get<char *>(),              (uint32_t)ctor.index());
        printf("\n");

        printf("str0 = \"%s\", \t str0.type() = \"%s\"\n",   str0.get<const char *>(),        str0.pretty_type().c_str());
        printf("str1 = \"%s\", \t\t str1.type() = \"%s\"\n", str1.get<std::string>().c_str(), str1.pretty_type().c_str());
        printf("str2 = \"%s\", \t\t str2.type() = \"%s\"\n", str2.get<std::string>().c_str(), str2.pretty_type().c_str());
        printf("str3 = \"%s\", \t str3.type() = \"%s\"\n",   str3.get<char *>(),              str3.pretty_type().c_str());
        printf("int0 = %d,     \t int0.type() = \"%s\"\n",   int0.get<int>(),                 int0.pretty_type().c_str());
        printf("ctor = \"%s\", \t ctor.type() = \"%s\"\n",   ctor.get<char *>(),              ctor.pretty_type().c_str());
        printf("\n");

    } catch(const std::bad_cast & ex) {
        std::cout << "Exception: " << ex.what() << std::endl << std::endl;
    }

    // See: https://en.cppreference.com/w/cpp/utility/variant/visit

    {
        std::vector<variant_t> vec = { 10, 15L, 1.5, "hello" };
        for (auto & v : vec) {
            // 1. void visitor, only called for side-effects (here, for I/O)
            jstd::visit([](variant_t & arg) -> void {
                std::string str;
                app::Converter<char>::try_to_string(arg, str);
                std::cout << str;
            }, v);

            // 2. value-returning visitor, demonstrates the idiom of returning another variant
            jstd::visit([](variant_t & arg) -> variant_t {
                return (arg + arg);
            }, v, v);

            // 3. type-matching visitor: a lambda that handles each type differently
            std::cout << ". After doubling, variant holds ";
            jstd::visit([](variant_t & arg) {
                using T = typename std::decay<decltype(arg)>::type;

                std::string str;
                app::Converter<char>::try_to_string(arg, str);
                std::cout << str << " ";

#if (JSTD_IS_CPP_14 == 0)
                if (arg.is_type<int>())
                    std::cout << "int with value " << str << '\n';
                else if (arg.is_type<long>())
                    std::cout << "long with value " << str << '\n';
                else if (arg.is_type<double>())
                    std::cout << "double with value " << str << '\n';
                else if (arg.is_type<std::string>())
                    std::cout << "std::string with value \"" << str << "\"\n";
                else if (arg.is_type<T>())
                    std::cout << "Variant<...> with value \"" << str << "\"\n";
                else {
                    std::cout << "unknown type with value \"" << str << "\"\n";
                }
#else
                if (std::is_same<T, int>::value)
                    std::cout << "int with value " << str << '\n';
                else if (std::is_same<T, long>::value)
                    std::cout << "long with value " << str << '\n';
                else if (std::is_same<T, double>::value)
                    std::cout << "double with value " << str << '\n';
                else if (std::is_same<T, std::string>::value)
                    std::cout << "std::string with value \"" << str << "\"\n";
                else {
                    //static_assert(false, "non-exhaustive visitor!");
                    std::cout << "unknown type with value \"" << str << "\"\n";
                }
#endif
            }, v);
        }
        std::cout << '\n';
    }

    {
        std::vector<variant_t> vec = { 10, 15L, 1.5, "hello" };
        for (auto & v : vec) {
            // 1. void visitor, only called for side-effects (here, for I/O)
            v.visit([](variant_t && arg) -> void {
                std::string str;
                app::Converter<char>::try_to_string(arg, str);
                std::cout << str;
            }, v);

            // 2. value-returning visitor, demonstrates the idiom of returning another variant
            v.visit([](variant_t && arg) -> variant_t {
                return (arg + arg);
            }, v);

            // 3. type-matching visitor: a lambda that handles each type differently
            std::cout << ". After doubling, variant holds ";
            v.visit([](variant_t & arg) {
                using T = typename std::decay<decltype(arg)>::type;

                std::string str;
                app::Converter<char>::try_to_string(arg, str);
                std::cout << str << " ";

#if (JSTD_IS_CPP_14 == 0)
                if (arg.is_type<int>())
                    std::cout << "int with value " << str << '\n';
                else if (arg.is_type<long>())
                    std::cout << "long with value " << str << '\n';
                else if (arg.is_type<double>())
                    std::cout << "double with value " << str << '\n';
                else if (arg.is_type<std::string>())
                    std::cout << "std::string with value \"" << str << "\"\n";
                else if (arg.is_type<T>())
                    std::cout << "Variant<...> with value \"" << str << "\"\n";
                else {
                    std::cout << "unknown type with value \"" << str << "\"\n";
                }
#else
                if (std::is_same<T, int>::value)
                    std::cout << "int with value " << str << '\n';
                else if (std::is_same<T, long>::value)
                    std::cout << "long with value " << str << '\n';
                else if (std::is_same<T, double>::value)
                    std::cout << "double with value " << str << '\n';
                else if (std::is_same<T, std::string>::value)
                    std::cout << "std::string with value \"" << str << "\"\n";
                else {
                    //static_assert(false, "non-exhaustive visitor!");
                    std::cout << "unknown type with value \"" << str << "\"\n";
                }
#endif
            });
        }
        std::cout << '\n';
    }

    variant_t v1 = 100;
    variant2_t v1a = "12345";
    variant2_t v2 = "abcde";
    v1.visit([](variant_t & arg) -> void {
        std::string str;
        app::Converter<char>::try_to_string(arg, str);
        std::cout << str << '\n';
    });

    v2.visit([](variant2_t & arg) -> void {
        std::string str;
        app::Converter<char>::try_to_string(arg, str);
        std::cout << str << '\n';
    });

    std::cout << '\n';
}

void print_version()
{
    printf("\n");
    printf("Utf8-encoding benchmark v1.0.0\n");
    printf("\n");
}

struct UserError : public app::Error {
    enum {
        UserErrorFirst = app::Error::UserErrorStart,

        // User errors define from here
        TextFileIsNull,

        NoError = app::Error::NoError
    };
};

struct AppConfig : public app::Config {
    const char * text_file;

    AppConfig() : text_file(nullptr) {
    }

    int validate() override {
        bool condition;

        condition = this->assert_check((this->text_file != nullptr), _Text("[text_file] must be specified.\n"));
        if (!condition) {
            return UserError::TextFileIsNull;
        }

        return UserError::NoError;
    }
};

int parse_command_line(const app::CmdLine & cmdLine, AppConfig & config)
{
    int err_code = app::Error::NoError;

    app::Variant variant;

#if 0
    if (cmdLine.getVar("i", variant)) {
        config.text_file = variant.get<const char *>();
    } else if (cmdLine.getVar("input-file", variant)) {
        config.text_file = variant.get<const char *>();
    } else {
        config.text_file = get_default_text_file();
    }
#endif

    if (!cmdLine.getVar("i", config.text_file) && !cmdLine.getVar("input-file", config.text_file)) {
        config.text_file = get_default_text_file();
    }

    if (cmdLine.visited("v") || cmdLine.visited("version")) {
        cmdLine.printVersion();
        return app::Error::ExitProcess;
    }

    if (cmdLine.visited("h") || cmdLine.visited("help")) {
        cmdLine.printUsage();
        return app::Error::ExitProcess;
    }

    return err_code;
}

void read_any_key()
{
#if defined(_MSC_VER) && defined(_DEBUG)
    ::system("pause");
#endif
}

int main(int argc, char * argv[])
{
    srand((unsigned)time(NULL));

    app::CmdLine cmdLine;
    cmdLine.setDisplayName("Utf8-encoding benchmark");
    cmdLine.setVersion("1.0.0");

    std::string appName = cmdLine.getAppName(argv);

    app::CmdLine::OptionDesc usage_desc;
    usage_desc.addText(
        "Usage:\n"
        "  %s [input_file_path]",
        appName.c_str()
    );
#if 0
    usage_desc.addText("Specify a source directory to (re-)generate a build system for it in the current working directory. "
                       "Specify an existing build directory tore-generate its build system.  "
                       "Specify a source directory to (re-)generate a build system for it in the current working directory. "
                       "Specify an existing build directory tore-generate its build system.  "
                       "Specify a source directory to (re-)generate a build system for it in the current working directory. "
                       "Specify an existing build directory tore-generate its build system.  "
                       "Specify a source directory to (re-)generate a build system for it in the current working directory. "
                       "Specify an existing build directory tore-generate its build system.  "
                       "Specify a source directory to (re-)generate a build system for it in the current working directory. "
                       "Specify an existing build directory tore-generate its build system.  "
    );
#endif
    cmdLine.addDesc(usage_desc);

    app::CmdLine::OptionDesc desc("Options");
    desc.addText("file argument options:");
    desc.addOption("-i, --input-file <file_path>", "Input UTF-8 text file path",   get_default_text_file());
    desc.addOption("-v, --version",                "Display version info");
    desc.addOption("-h, --help",                   "Display help info");
    cmdLine.addDesc(desc);

    int err_code = cmdLine.parseArgs(argc, argv);
    //printf("err_code = cmdLine.parseArgs(argc, argv) = %d\n\n", err_code);
    if (app::Error::isError(err_code)) {
        cmdLine.printUsage();
        read_any_key();
        return EXIT_FAILURE;
    } else {
        if (!(cmdLine.visited("h") || cmdLine.visited("help")) &&
            !(cmdLine.visited("v") || cmdLine.visited("version"))) {
            print_version();
        }
    }

    AppConfig config;
    err_code = parse_command_line(cmdLine, config);
    if (err_code == app::Error::ExitProcess) {
        read_any_key();
        return EXIT_FAILURE;
    }

    err_code = config.validate();
    if (UserError::isError(err_code)) {
        cmdLine.printUsage();
        read_any_key();
        return EXIT_FAILURE;
    }

    //is_array_test();
    variant_test();

    printf("--input-file: \"%s\"\n\n", config.text_file);

    if (0) {
#ifdef _DEBUG
        const char * test_case = "x\xe2\x89\xa4(\xce\xb1+\xce\xb2)\xc2\xb2\xce\xb3\xc2\xb2";
        uint16_t dest[32] = { 0 };

        size_t unicode_len = utf8::utf8_decode_sse(test_case, strlen(test_case), dest);
#else
        test::CPU::WarmUp warmUper(1000);
        benchmark(config.text_file);
#endif
    }

    read_any_key();
    return EXIT_SUCCESS;
}
