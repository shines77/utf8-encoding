
#ifndef FROMUTF8_SSE_41_H
#define FROMUTF8_SSE_41_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t fromUtf8_sse(const char * src, size_t len, uint16_t * dest);
size_t fromUtf8(const char ** in_buf, size_t * in_bytesleft, char ** out_buf, size_t * out_bytesleft);

#ifdef __cplusplus
}
#endif

#endif // FROMUTF8_SSE_41_H
