
#ifndef UTF8_ENCODE_SSE_41_H
#define UTF8_ENCODE_SSE_41_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstdbool>

#ifndef __SSE4_1__
#define __SSE4_1__
#endif

#ifndef __SSE4_2__
#define __SSE4_2__
#endif

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif
#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif

namespace utf8 {

/*******************************************************************************

    UTF-8 encoding

    Bits     Unicode 32                     UTF-8
            (hexadecimal)                  (binary)

     7   0000 0000 - 0000 007F   1 byte:   0xxxxxxx
    11   0000 0080 - 0000 07FF   2 bytes:  110xxxxx 10xxxxxx
    16   0000 0800 - 0000 FFFF   3 bytes:  1110xxxx 10xxxxxx 10xxxxxx
    21   0001 0000 - 001F FFFF   4 bytes:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    26   0020 0000 - 03FF FFFF   5 bytes:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    31   0400 0000 - 7FFF FFFF   6 bytes:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

    RFC3629：UTF-8, a transformation format of ISO 10646

    https://www.ietf.org/rfc/rfc3629.txt

    if ((str[i] & 0xC0) != 0x80)
        length++;

    Utf-8 encoding online: https://tool.oschina.net/encode?type=4

*******************************************************************************/

//
// "x\e2\89\a4(\ce\b1+\ce\b2)\c2\b2\ce\b3\c2\b2"
//
static inline
std::size_t utf8_decode_sse41(const char * src, std::size_t len, unsigned short * dest)
{
    std::size_t size = 0;
    const char * end = src + len;

    while (src + 16 < end) {
        __m128i chunk = _mm_loadu_si128((const __m128i *)src);
        uint32_t asciiMask = _mm_movemask_epi8(chunk);

        // The state for ascii or contiuation bit: mask the most significant bit.
        __m128i state = _mm_set1_epi8(0x00u | 0x80u);

        // Add an offset of 0x80 to work around the fact that we don't have unsigned comparison.
        __m128i chunk_signed = _mm_add_epi8(chunk, _mm_set1_epi8(0x80u));

        // If the bytes are greater or equal than 0xc0, we have the start of a
        // multi-bytes sequence of at least 2.
        // We use 0xc2 for error detection, see later.
        __m128i cond2 = _mm_cmplt_epi8(_mm_set1_epi8(0xC2u - 1 - 0x80u), chunk_signed);

        state = _mm_blendv_epi8(state, _mm_set1_epi8(0x02u | 0xC0u), cond2);

        __m128i cond3 = _mm_cmplt_epi8(_mm_set1_epi8(0xE0u - 1 - 0x80u), chunk_signed);

        // We could optimize the case when there is no sequence longer than 2 bytes.
        // But i did not do it in this version.
        if (!_mm_movemask_epi8(cond3)) {
            /* process max 2 bytes sequences */
        }

        state = _mm_blendv_epi8(state, _mm_set1_epi8(0x03u | 0xE0u), cond3);
        __m128i mask3 = _mm_slli_si128(cond3, 1);

        // In this version I do not handle sequences of 4 bytes. So if there is one
        // we break and do a classic processing byte per byte.
        __m128i cond4 = _mm_cmplt_epi8(_mm_set1_epi8(0xF0u - 1 - 0x80u), chunk_signed);
        if (_mm_movemask_epi8(cond4)) {
            break;
        }

        // Separate the count and mask from the state vector
        __m128i count = _mm_and_si128(state, _mm_set1_epi8(0x07u));
        __m128i mask  = _mm_and_si128(state, _mm_set1_epi8(0xF8u));

        // Substract 1, shift 1 byte and add
        __m128i count_subs1 = _mm_subs_epu8(count, _mm_set1_epi8(0x01));
        __m128i counts = _mm_add_epi8(count, _mm_slli_si128(count_subs1, 1));

        // Substract 2, shift 2 bytes and add
        counts = _mm_add_epi8(counts, _mm_slli_si128(_mm_subs_epu8(counts, _mm_set1_epi8(0x02)), 2));

        // Mask away our control bits with ~mask (and not)
        chunk = _mm_andnot_si128(mask, chunk);
        // from now on, we only have usefull bits

        // Shift by one byte on the left
        __m128i chunk_right = _mm_slli_si128(chunk, 1);

        // If counts == 1, compute the low byte using 2 bits from chunk_right
        __m128i chunk_low = _mm_blendv_epi8(chunk,
                        _mm_or_si128(chunk, _mm_and_si128(
                        _mm_slli_epi16(chunk_right, 6), _mm_set1_epi8(0xC0u))),
                        _mm_cmpeq_epi8(counts, _mm_set1_epi8(0x01)));

        // in chunk_high, only keep the bits if counts == 2
        __m128i chunk_high = _mm_and_si128(chunk, _mm_cmpeq_epi8(counts, _mm_set1_epi8(0x02)));
        // and shift that by 2 bits on the right.
        // (no unwanted bits are comming because of the previous mask)
        chunk_high = _mm_srli_epi32(chunk_high, 2);

        // Add the bits from the bytes for which counts == 3
        mask3 = _mm_slli_si128(cond3, 1); // re-use cond3 (shifted)
        chunk_high = _mm_or_si128(chunk_high, _mm_and_si128(
                     _mm_and_si128(_mm_slli_epi32(chunk_right, 4), _mm_set1_epi8(0xF0u)), mask3));

        __m128i shifts = count_subs1;
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 2));
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 4));
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 8));

        // Keep only if the corresponding byte should stay
        // that is, if counts is 1 or 0  (so < 2).
        shifts  = _mm_and_si128 (shifts, _mm_cmplt_epi8(counts, _mm_set1_epi8(0x02)));

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 1),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 7) , 1));

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 2),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 6) , 2));

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 4),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 5) , 4));

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 8),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 4) , 8));

        __m128i shuf = _mm_add_epi8(shifts, _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0));

        // Remove the gaps by shuffling
        __m128i shuffled_low  = _mm_shuffle_epi8(chunk_low,  shuf);
        __m128i shuffled_high = _mm_shuffle_epi8(chunk_high, shuf);

        // Now we can unpack and store
        __m128i utf16_low  = _mm_unpacklo_epi8(shuffled_low, shuffled_high);
        __m128i utf16_high = _mm_unpackhi_epi8(shuffled_low, shuffled_high);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest),     utf16_low);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 8), utf16_high);

        // ASCII characters (and only them) should have the corresponding byte of counts equal 0.
        if (asciiMask ^ _mm_movemask_epi8(_mm_cmpgt_epi8(counts, _mm_set1_epi8(0x00)))) {
            break;
        }

        // The difference between a byte in counts and the next one should be negative,
        // zero, or one. Any other value means there is not enough continuation bytes.
        if (_mm_movemask_epi8(_mm_cmpgt_epi8(_mm_sub_epi8(
                              _mm_slli_si128(counts, 1), counts),
                              _mm_set1_epi8(0x01)))) {
            break;
        }

        // For the 3 bytes sequences we check the high byte to prevent
        // the over long sequence (0x00 - 0x07) or the UTF-16 surrogate (0xD8 - 0xDF)
        __m128i high_bits = _mm_and_si128(chunk_high, _mm_set1_epi8(0xF8u));
        if (!_mm_testz_si128(mask3, _mm_or_si128(
                    _mm_cmpeq_epi8(high_bits, _mm_set1_epi8(0x00)),
                    _mm_cmpeq_epi8(high_bits, _mm_set1_epi8(0xD8u))))) {
            break;
        }

        // Check for a few more invalid unicode using range comparison and _mm_cmpestrc
        const int check_mode = _SIDD_UWORD_OPS | _SIDD_CMP_RANGES;
        if (_mm_cmpestrc(_mm_cvtsi64_si128(0xfdeffdd0fffffffe), 4, utf16_high, 8, check_mode) |
            _mm_cmpestrc(_mm_cvtsi64_si128(0xfdeffdd0fffffffe), 4, utf16_low,  8, check_mode)) {
            break;
        }
    }

    return size;
}

} // namespace utf8

#endif // UTF8_ENCODE_SSE_41_H
