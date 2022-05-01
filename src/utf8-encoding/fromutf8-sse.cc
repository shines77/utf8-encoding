/****************************************************************************
 *
 * Copyright (C) 2012 Olivier Goffart <ogoffart@woboq.com>
 * http://woboq.com
 *
 * This is an experiment to process UTF-8 using SSE4 intrinscis.
 * Read: http://woboq.com/blog/utf-8-processing-using-simd.html
 *
 * This file may be used under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 *
 * For any question, please contact contact@woboq.com
 *
 ****************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>

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

#include "utf8-encoding/fromutf8-sse.h"

size_t fromUtf8_sse(const char * src, size_t len, uint16_t * dest)
{
#if defined(__SSE4_1__)
    const char * end = src + len;
    const uint16_t * dest_first = dest;

    while((src + 16) < end) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));

#if 0 // ASCII optimize
        int asciiMask = _mm_movemask_epi8(chunk);
        if (!asciiMask) {
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest),     _mm_unpacklo_epi8(chunk, _mm_set1_epi8(0x00)));
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 8), _mm_unpackhi_epi8(chunk, _mm_set1_epi8(0x00)));
            dest += 16;
            src  += 16;
            continue;
        }
#else
        int asciiMask = 0xFFFFu;
#endif
        __m128i chunk_signed = _mm_add_epi8(chunk, _mm_set1_epi8(0x80u));
        __m128i cond2 = _mm_cmplt_epi8(_mm_set1_epi8(0xC2u - 1 - 0x80u), chunk_signed);
        __m128i state = _mm_set1_epi8(0x00u | 0x80u);
        state = _mm_blendv_epi8(state, _mm_set1_epi8(0x02u | 0xC0u), cond2);

        __m128i cond3 = _mm_cmplt_epi8(_mm_set1_epi8(0xE0u - 1 - 0x80u), chunk_signed);

        // Possible improvement: create a separate processing when there are
        // only 2 bytes sequences
        //if (!_mm_movemask_epi8(cond3)) { /* process 2 max */ }

        state = _mm_blendv_epi8(state, _mm_set1_epi8(0x03u | 0xE0u), cond3);
        __m128i mask3 = _mm_slli_si128(cond3, 1);

        __m128i cond4 = _mm_cmplt_epi8(_mm_set1_epi8(0xF0u - 1 - 0x80u), chunk_signed);

        // 4 bytes sequences are not vectorize. Fall back to the scalar processing
        if (_mm_movemask_epi8(cond4)) {
            //break;
        }

        __m128i count =  _mm_and_si128(state, _mm_set1_epi8(0x07));
        __m128i count_sub1 = _mm_subs_epu8(count, _mm_set1_epi8(0x01));
        __m128i counts = _mm_add_epi8(count, _mm_slli_si128(count_sub1, 1));

        __m128i shifts = count_sub1;
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 1));
        counts = _mm_add_epi8(counts, _mm_slli_si128(_mm_subs_epu8(counts, _mm_set1_epi8(0x02)), 2));
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 2));

#if 0
        // ASCII characters (and only them) should have the corresponding byte of counts equal 0.
        if (asciiMask ^ _mm_movemask_epi8(_mm_cmpgt_epi8(counts, _mm_set1_epi8(0x00)))) {
            break; // error
        }
#endif
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 4));

#if 0
        // The difference between a byte in counts and the next one should be negative,
        // zero, or one. Any other value means there is not enough continuation bytes.
        if (_mm_movemask_epi8(_mm_cmpgt_epi8(_mm_sub_epi8(
                              _mm_slli_si128(counts, 1), counts),
                              _mm_set1_epi8(0x01)))) {
            break; // error
        }
#endif

        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 8));

        __m128i mask = _mm_and_si128(state, _mm_set1_epi8(0xF8u));
        shifts = _mm_and_si128(shifts, _mm_cmplt_epi8(counts, _mm_set1_epi8(0x02))); // <= 1

        chunk = _mm_andnot_si128(mask, chunk); // from now on, we only have usefull bits

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 1),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 7), 1));

        __m128i chunk_right = _mm_slli_si128(chunk, 1);

        __m128i chunk_low = _mm_blendv_epi8(chunk,
                                  _mm_or_si128(chunk, _mm_and_si128(_mm_slli_epi16(chunk_right, 6), _mm_set1_epi8(0xC0u))),
                                  _mm_cmpeq_epi8(counts, _mm_set1_epi8(0x01)));

        __m128i chunk_high = _mm_and_si128(chunk , _mm_cmpeq_epi8(counts, _mm_set1_epi8(0x02)));

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 2),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 6), 2));
        chunk_high = _mm_srli_epi32(chunk_high, 2);

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 4),
                                _mm_srli_si128(_mm_slli_epi16(shifts, 5), 4));
        chunk_high = _mm_or_si128(chunk_high, _mm_and_si128(
                                _mm_and_si128(_mm_slli_epi32(chunk_right, 4),
                                              _mm_set1_epi8(0xF0u)), mask3));
        int c = _mm_extract_epi16(counts, 7);
        int source_advance = !(c & 0x0200) ? 16 : !(c & 0x02) ? 15 : 14;

#if 0
        // For the 3 bytes sequences we check the high byte to prevent
        // the over long sequence (0x00 - 0x07) or the UTF-16 surrogate (0xD8 - 0xDF)
        __m128i high_bits = _mm_and_si128(chunk_high, _mm_set1_epi8(0xF8u));
        if (!_mm_testz_si128(mask3, _mm_or_si128(
                    _mm_cmpeq_epi8(high_bits, _mm_set1_epi8(0x00u)),
                    _mm_cmpeq_epi8(high_bits, _mm_set1_epi8(0xD8u))))) {
            break;
        }
#endif

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 8),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 4), 8));

        chunk_high = _mm_slli_si128(chunk_high, 1);

        __m128i shuf = _mm_add_epi8(shifts, _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0));

        // Remove the gaps by shuffling
        chunk_low  = _mm_shuffle_epi8(chunk_low,  shuf);
        chunk_high = _mm_shuffle_epi8(chunk_high, shuf);

        // Now we can unpack and store
        __m128i utf16_low  = _mm_unpacklo_epi8(chunk_low, chunk_high);
        __m128i utf16_high = _mm_unpackhi_epi8(chunk_low, chunk_high);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest),     utf16_low);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 8), utf16_high);

        int s = _mm_extract_epi32(shifts, 3);
        int dest_advance = source_advance - (0xFFu & (s >> 8 * (3 - 16 + source_advance)));

#if 0
#if defined(__SSE4_2__)
        // Check for a few more invalid unicode using range comparison and _mm_cmpestrc
        const int check_mode = 5; /* _SIDD_UWORD_OPS | _SIDD_CMP_RANGES */
        if (_mm_cmpestrc(_mm_cvtsi64_si128(0xFDEFFDD0FFFFFFFE), 4, utf16_high, 8, check_mode) |
            _mm_cmpestrc(_mm_cvtsi64_si128(0xFDEFFDD0FFFFFFFE), 4, utf16_low,  8, check_mode)) {
            break;
        }
#else
        if (!_mm_testz_si128(_mm_cmpeq_epi8(_mm_set1_epi8(0xFD), chunk_high),
               _mm_and_si128(_mm_cmplt_epi8(_mm_set1_epi8(0xD0), chunk_low),
                             _mm_cmpgt_epi8(_mm_set1_epi8(0xEF), chunk_low))) ||
            !_mm_testz_si128(_mm_cmpeq_epi8(_mm_set1_epi8(0xFF), chunk_high),
                _mm_or_si128(_mm_cmpeq_epi8(_mm_set1_epi8(0xFE), chunk_low),
                             _mm_cmpeq_epi8(_mm_set1_epi8(0xFF), chunk_low)))) {
            break;
        }
#endif // __SSE4_2__
#endif

        dest += dest_advance;
        src  += source_advance;
    }

    size_t dest_len = dest - dest_first;
    return dest_len;

    // The rest will be handled sequencially.
    // Possible improvement: go back to the vectorized processing after the error or the 4 byte sequence
#endif // __SSE4_1__
}

// Same signature as match iconv
size_t fromUtf8(const char ** in_buf, size_t * in_bytesleft, char ** out_buf, size_t * out_bytesleft)
{
    uint16_t *& qch = *reinterpret_cast<uint16_t **>(out_buf);
    const char *& chars = *const_cast<const char **>(in_buf);
    size_t len = std::min<size_t>(*in_bytesleft, *out_bytesleft / 2);

    // First process the bytes using SSE
    fromUtf8_sse(chars, len, qch);

    // Then handle the remaining bytes using scalar algorith.
    // Basically extracted from from QUtf8::convertToUnicode in qutfcodec.c

    // required QChar API
    class QChar {
    public:
        enum SpecialCharacter {
            ReplacementCharacter = 0xFFFDu,
            ObjectReplacementCharacter = 0xFFFCu,
            LastValidCodePoint = 0x10FFFFu
        };

        static inline bool isNonCharacter(uint32_t ucs4) {
            return (ucs4 >= 0xFDD0u) && (ucs4 <= 0xFDEFu || (ucs4 & 0xFFFEu) == 0xFFFEu);
        }

        static inline bool isSurrogate(uint32_t ucs4) {
            return ((ucs4 - 0xD800u) < 2048u);
        }

        static inline bool requiresSurrogates(uint32_t ucs4) {
            return (ucs4 >= 0x10000u);
        }

        static inline uint16_t highSurrogate(uint32_t ucs4) {
            return uint16_t((ucs4 >> 10) + 0xD7C0u);
        }

        static inline uint16_t lowSurrogate(uint32_t ucs4) {
            return uint16_t((ucs4 % 0x0400u) + 0xDC00u);
        }
    };

    bool header_done = false;
    uint16_t replacement = QChar::ReplacementCharacter;
    int need = 0;
    int error = -1;
    int invalid = 0;

    uint8_t ch;
    uint32_t uc = 0;
    uint32_t min_uc = 0;

    uint16_t * start = qch;

    int i;
    for (i = 0; i < len - need; ++i) {
        ch = (uint8_t)chars[i];
        if (need != 0) {
            if ((ch & 0xC0u) == 0x80u) {
                uc = (uc << 6) | (ch & 0x3Fu);
                --need;
                if (need == 0) {
                    // utf-8 bom composes into 0xFEFF code point
                    bool nonCharacter;
                    if (!header_done && uc == 0xFEFFu) {
                        // don't do anything, just skip the BOM
                    } else if (!(nonCharacter = QChar::isNonCharacter(uc)) && QChar::requiresSurrogates(uc) && uc <= QChar::LastValidCodePoint) {
                        // surrogate pair
                        *qch++ = QChar::highSurrogate(uc);
                        *qch++ = QChar::lowSurrogate(uc);
                    } else if ((uc < min_uc) || QChar::isSurrogate(uc) || nonCharacter || uc > QChar::LastValidCodePoint) {
                        // error: overlong sequence, UTF16 surrogate or non-character
                        *qch++ = replacement;
                        ++invalid;
                    } else {
                        *qch++ = ((uc & 0xFFu) << 8) | ((uc & 0xFF00u) >> 8);
                    }
                    header_done = true;
                }
            } else {
                // error
                i = error;
                *qch++ = replacement;
                ++invalid;
                need = 0;
                header_done = true;
            }
        } else {
            if (ch < 128) {
                *qch++ = uint16_t(ch) << 8;
                header_done = true;
            } else if ((ch & 0xE0u) == 0xC0u) {
                uc = ch & 0x1Fu;
                need = 1;
                error = i;
                min_uc = 0x80u;
                header_done = true;
            } else if ((ch & 0xF0u) == 0xE0u) {
                uc = ch & 0x0Fu;
                need = 2;
                error = i;
                min_uc = 0x800u;
            } else if ((ch & 0xF8u) == 0xF0u) {
                uc = ch & 0x07u;
                need = 3;
                error = i;
                min_uc = 0x10000u;
                header_done = true;
            } else {
                // error
                *qch++ = replacement;
                ++invalid;
                header_done = true;
            }
        }
    }

    if (need) {
        i--;
    }

    *in_bytesleft = len - i;
    chars += i;

    size_t r = (qch - start);
    *out_bytesleft -= r;

    return (need ? -1 : 0);
}
