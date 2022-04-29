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

#if 1 // ASCII optimize
        int asciiMask = _mm_movemask_epi8(chunk);
        if (!asciiMask) {
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest),     _mm_unpacklo_epi8(chunk, _mm_set1_epi8(0)));
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 8), _mm_unpackhi_epi8(chunk, _mm_set1_epi8(0)));
            dest += 16;
            src  += 16;
            continue;
        }
#endif
        __m128i chunk_signed = _mm_add_epi8(chunk, _mm_set1_epi8((signed char)0x80));
        __m128i cond2 = _mm_cmplt_epi8(_mm_set1_epi8((signed char)(0xc2 - 1 - 0x80)), chunk_signed);
        __m128i state = _mm_set1_epi8((signed char)(0x00 | 0x80));
        state = _mm_blendv_epi8(state , _mm_set1_epi8((signed char)(0x02 | 0xc0)), cond2);

        __m128i cond3 = _mm_cmplt_epi8(_mm_set1_epi8((signed char)(0xe0 - 1 - 0x80)), chunk_signed);

        // Possible improvement: create a separate processing when there are
        // only 2b ytes sequences
        //if (!_mm_movemask_epi8(cond3)) { /*process 2 max*/ }

        state = _mm_blendv_epi8(state, _mm_set1_epi8((signed char)(0x03 | 0xe0)), cond3);
        __m128i mask3 = _mm_slli_si128(cond3, 1);

        __m128i cond4 = _mm_cmplt_epi8(_mm_set1_epi8((signed char)(0xf0 - 1 - 0x80)), chunk_signed);

        // 4 bytes sequences are not vectorize. Fall back to the scalar processing
        if (_mm_movemask_epi8(cond4)) {
            break;
        }

        __m128i count =  _mm_and_si128(state, _mm_set1_epi8(0x07));
        __m128i count_sub1 = _mm_subs_epu8(count, _mm_set1_epi8(0x01));
        __m128i counts = _mm_add_epi8(count, _mm_slli_si128(count_sub1, 1));

        __m128i shifts = count_sub1;
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 1));
        counts = _mm_add_epi8(counts, _mm_slli_si128(_mm_subs_epu8(counts, _mm_set1_epi8(0x02)), 2));
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 2));

        if (asciiMask ^ _mm_movemask_epi8(_mm_cmpgt_epi8(counts, _mm_set1_epi8(0)))) {
            //break; // error
        }
        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 4));

        if (_mm_movemask_epi8(_mm_cmpgt_epi8(_mm_sub_epi8(_mm_slli_si128(counts, 1), counts), _mm_set1_epi8(0x01)))) {
            //break; // error
        }

        shifts = _mm_add_epi8(shifts, _mm_slli_si128(shifts, 8));

        __m128i mask = _mm_and_si128(state, _mm_set1_epi8((signed char)0xf8));
        shifts = _mm_and_si128(shifts, _mm_cmplt_epi8(counts, _mm_set1_epi8(0x02))); // <=1

        chunk = _mm_andnot_si128(mask, chunk); // from now on, we only have usefull bits

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 1),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 7), 1));

        __m128i chunk_right = _mm_slli_si128(chunk, 1);

        __m128i chunk_low = _mm_blendv_epi8(chunk,
                                  _mm_or_si128(chunk, _mm_and_si128(_mm_slli_epi16(chunk_right, 6), _mm_set1_epi8((signed char)0xc0))) ,
                                  _mm_cmpeq_epi8(counts, _mm_set1_epi8(1)) );

        __m128i chunk_high = _mm_and_si128(chunk , _mm_cmpeq_epi8(counts, _mm_set1_epi8(2)) );

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 2),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 6), 2));
        chunk_high = _mm_srli_epi32(chunk_high, 2);

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 4),
                                _mm_srli_si128(_mm_slli_epi16(shifts, 5), 4));
        chunk_high = _mm_or_si128(chunk_high,
                                _mm_and_si128(_mm_and_si128(_mm_slli_epi32(chunk_right, 4), _mm_set1_epi8((signed char)0xf0)),
                                                mask3));
        int c = _mm_extract_epi16(counts, 7);
        int source_advance = !(c & 0x0200) ? 16 : !(c & 0x02) ? 15 : 14;

        __m128i high_bits = _mm_and_si128(chunk_high, _mm_set1_epi8((signed char)0xf8));
        if (!_mm_testz_si128(mask3, _mm_or_si128(
                    _mm_cmpeq_epi8(high_bits, _mm_set1_epi8((signed char)0x00)),
                    _mm_cmpeq_epi8(high_bits, _mm_set1_epi8((signed char)0xd8))))) {
            //break;
        }

        shifts = _mm_blendv_epi8(shifts, _mm_srli_si128(shifts, 8),
                                 _mm_srli_si128(_mm_slli_epi16(shifts, 4), 8));

        chunk_high = _mm_slli_si128(chunk_high, 1);

        __m128i shuf = _mm_add_epi8(shifts, _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0));

        chunk_low  = _mm_shuffle_epi8(chunk_low,  shuf);
        chunk_high = _mm_shuffle_epi8(chunk_high, shuf);
        __m128i utf16_low  = _mm_unpacklo_epi8(chunk_low, chunk_high);
        __m128i utf16_high = _mm_unpackhi_epi8(chunk_low, chunk_high);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest),     utf16_low);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 8), utf16_high);

        int s = _mm_extract_epi32(shifts, 3);
        int dest_advance = source_advance - (0xff & (s >> 8 * (3 - 16 + source_advance)));

#if defined(__SSE4_2__)
        const int check_mode = 5; /* _SIDD_UWORD_OPS | _SIDD_CMP_RANGES */
        if (_mm_cmpestrc(_mm_cvtsi64_si128(0xFDEFFDD0FFFFFFFE), 4, utf16_high, 8, check_mode) |
            _mm_cmpestrc(_mm_cvtsi64_si128(0xFDEFFDD0FFFFFFFE), 4, utf16_low,  8, check_mode)) {
            //break;
        }
#else
        if (!_mm_testz_si128(_mm_cmpeq_epi8(_mm_set1_epi8((signed char)0xfd), chunk_high),
               _mm_and_si128(_mm_cmplt_epi8(_mm_set1_epi8((signed char)0xd0), chunk_low),
                             _mm_cmpgt_epi8(_mm_set1_epi8((signed char)0xef), chunk_low))) ||
            !_mm_testz_si128(_mm_cmpeq_epi8(_mm_set1_epi8((signed char)0xff), chunk_high),
                _mm_or_si128(_mm_cmpeq_epi8(_mm_set1_epi8((signed char)0xfe), chunk_low),
                             _mm_cmpeq_epi8(_mm_set1_epi8((signed char)0xff), chunk_low)))) {
            break;
        }
#endif // __SSE4_2__

        dest += dest_advance;
        src  += source_advance;
    }

    len = dest - dest_first;
    return len;

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
    // Basically extracted from from  QUtf8::convertToUnicode in qutfcodec.c

    // required QChar API
    class QChar {
    public:
        enum SpecialCharacter {
            ReplacementCharacter = 0xfffd,
            ObjectReplacementCharacter = 0xfffc,
            LastValidCodePoint = 0x10ffff
        };

        static inline bool isNonCharacter(uint32_t ucs4) {
            return (ucs4 >= 0xfdd0) && (ucs4 <= 0xfdef || (ucs4 & 0xfffe) == 0xfffe);
        }

        static inline bool isSurrogate(uint32_t ucs4) {
            return ((ucs4 - 0xd800u) < 2048u);
        }

        static inline bool requiresSurrogates(uint32_t ucs4) {
            return (ucs4 >= 0x10000);
        }

        static inline uint16_t highSurrogate(uint32_t ucs4) {
            return uint16_t((ucs4 >> 10) + 0xd7c0);
        }

        static inline uint16_t lowSurrogate(uint32_t ucs4) {
            return uint16_t(ucs4 % 0x400 + 0xdc00);
        }
    };

    bool headerdone = false;
    uint16_t replacement = QChar::ReplacementCharacter;
    int need = 0;
    int error = -1;
    uint32_t uc = 0;
    uint32_t min_uc = 0;

    uint8_t ch;
    int invalid = 0;

    uint16_t * start = qch;

    int i;
    for (i = 0; i < len - need; ++i) {
        ch = chars[i];
        if (need) {
            if ((ch & 0xC0) == 0x80) {
                uc = (uc << 6) | (ch & 0x3f);
                --need;
                if (!need) {
                    // utf-8 bom composes into 0xfeff code point
                    bool nonCharacter;
                    if (!headerdone && uc == 0xfeff) {
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
                        *qch++ = ((uc & 0xff) << 8) | ((uc & 0xff00) >> 8);
                    }
                    headerdone = true;
                }
            } else {
                // error
                i = error;
                *qch++ = replacement;
                ++invalid;
                need = 0;
                headerdone = true;
            }
        } else {
            if (ch < 128) {
                *qch++ = uint16_t(ch) << 8;
                headerdone = true;
            } else if ((ch & 0xe0) == 0xc0) {
                uc = ch & 0x1f;
                need = 1;
                error = i;
                min_uc = 0x80;
                headerdone = true;
            } else if ((ch & 0xf0) == 0xe0) {
                uc = ch & 0x0f;
                need = 2;
                error = i;
                min_uc = 0x800;
            } else if ((ch & 0xf8) == 0xf0) {
                uc = ch & 0x07;
                need = 3;
                error = i;
                min_uc = 0x10000;
                headerdone = true;
            } else {
                // error
                *qch++ = replacement;
                ++invalid;
                headerdone = true;
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
