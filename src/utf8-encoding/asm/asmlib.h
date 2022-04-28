/*************************** asmlib.h ***************************************
* Author:        shines77
* Date created:  2022-04-28
* Last modified: 2022-04-28
* Project:       utf8-encoding
* Source URL:    https://gitee.com/shines77/utf8-encoding, https://github.com/shines77/utf8-encoding
*
* Description:
* Header file for the asmlib function library.
* This library is available in many versions for different platforms.
*
* (c) Copyright 2022 by shines77(Guo XiongHui). 
* GNU General Public License http://www.gnu.org/licenses/gpl.html
*****************************************************************************/

#ifndef UTF8_ENCODING_ASMLIB_H
#define UTF8_ENCODING_ASMLIB_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/***********************************************************************
Define compiler-specific types and directives
***********************************************************************/

// Turn off name mangling
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
Function prototypes, memory and string functions
***********************************************************************/

// Search for substring in string
unsigned int utf8_decode_sse41(const char * utf8, int * skip);

// Tell which instruction set is supported
int    InstructionSet(void);

#ifdef __cplusplus
}  // end of extern "C"
#endif

#ifdef __cplusplus

static inline
unsigned int utf8_decode_sse41(char * utf8, int * skip) {
   return utf8_decode_sse41((const char *)utf8, skip);
}  // Overload utf8_decode_sse4_1() with const char * version

#endif

// Test if emmintrin.h is included and __m128i defined
#if defined(__GNUC__) && defined(_EMMINTRIN_H_INCLUDED) && !defined(__SSE2__)
#error Please compile with -sse2 or higher 
#endif

#if defined(_INCLUDED_EMM) || (defined(_EMMINTRIN_H_INCLUDED) && defined(__SSE2__))
#define VECTORDIVISIONDEFINED
#endif

#endif // UTF8_ENCODING_ASMLIB_H
