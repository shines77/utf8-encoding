
#ifndef APP_CMD_LINE_H
#define APP_CMD_LINE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(_WINDOWS_)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // WIN32_LEAN_AND_MEAN
#endif // _WIN32

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#if defined(_MSC_VER)
#include <tchar.h>
#endif

#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>

#include "jstd/char_traits.h"
#include "jstd/apply_visitor.h"
#include "jstd/function_traits.h"
#include "jstd/Variant.h"

#if defined(_MSC_VER)
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif // _MSC_VER

// Text is Unicode on Windows or wchar_t on Linux ?
#define USE_WIDE_CHAR_TEXT  0

#define _Ansi(x)        x
#define _LText(x)       L ## x

#ifndef _Text
  #if USE_WIDE_CHAR_TEXT
    #define _Text(x)    _LText(x)
  #else
    #define _Text(x)    _Ansi(x)
  #endif
#endif

#ifndef UNUSED_VAR
#define UNUSED_VAR(var)  ((void)var)
#endif

namespace app {

FILE * const LOG_FILE = stderr;

struct Error {
    typedef int error_code;

    enum {
        ErrorFirst = -20000,

        // Default Errors
        CmdLine_UnknownArgument,
        CmdLine_UnrecognizedArgument,
        CmdLine_EmptyArgumentName,
        CmdLine_ShortPrefixArgumentNameTooLong,
        CmdLine_LongPrefixArgumentNameTooShort,
        CmdLine_CouldNotParseArgumentValue,
        CmdLine_IllegalFormat,
        ExitProcess,

        // User errors
        UserErrorStart = -10000,

        NoError = 0,
    };

    static bool isSuccess(int error_id) {
        return (error_id == NoError);
    }

    static bool isError(int error_id) {
        return (error_id < NoError);
    }
};

struct Slice {
    std::size_t first;
    std::size_t last;

    Slice() noexcept : first(0), last(0) {
    }

    Slice(std::size_t _first, std::size_t _last) noexcept
        : first(_first), last(_last) {
    }

    Slice(const Slice & src) noexcept
        : first(src.first), last(src.last) {
    }

    bool is_empty() const {
        return ((std::intptr_t)last <= (std::intptr_t)first);
    }

    bool is_valid() const {
        return ((std::intptr_t)last >= (std::intptr_t)first);
    }

    std::intptr_t length() const  {
        return (std::intptr_t)(last - first);
    }

    std::size_t size() const  {
        return (std::size_t)(last - first);
    }

    std::size_t count() const  {
        return (this->is_valid()) ? this->size() : std::size_t(0);
    }

    bool is_last(std::size_t i) const {
        return ((last > 0) && (i >= (last - 1)));
    }
};

struct SliceEx : public Slice {
    std::size_t token;
    std::size_t count;

    SliceEx() noexcept : Slice(0, 0), token(0), count(0) {
    }

    SliceEx(std::size_t first, std::size_t last) noexcept
        : Slice(first, last), token(0), count(0) {
    }

    SliceEx(const SliceEx & src) noexcept
        : Slice(src.first, src.last),
          token(src.token), count(src.count) {
    }
};

template <typename CharT = char>
static inline
std::size_t find_first_not_of(const std::basic_string<CharT> & str, CharT ch,
                              std::size_t first, std::size_t last)
{
    std::size_t cur = first;
    while (cur < last) {
        if (str[cur] == ch)
            ++cur;
        else
            return cur;
    }
    return std::basic_string<CharT>::npos;
}

template <typename CharT = char>
static inline
std::size_t find_first_not_of(const std::basic_string<CharT> & str,
                              CharT ch, std::size_t offset = 0)
{
    std::size_t first = offset;
    std::size_t last = str.size();
    return find_first_not_of(str, ch, last, first);
}

template <typename CharT = char>
static inline
std::size_t find_last_not_of(const std::basic_string<CharT> & str, CharT ch,
                             std::size_t first, std::size_t last)
{
    std::size_t cur = --last;
    while (cur >= first) {
        if (str[cur] == ch)
            --cur;
        else
            return cur;
    }
    return std::basic_string<CharT>::npos;
}

template <typename CharT = char>
static inline
std::size_t find_last_not_of(const std::basic_string<CharT> & str, CharT ch,
                             std::size_t offset = std::basic_string<CharT>::npos)
{
    std::size_t last = (offset == std::basic_string<CharT>::npos) ? str.size() : offset;
    static const std::size_t first = 0;
    return find_last_not_of(str, ch, last, first);
}

template <typename CharT = char>
static inline
void string_trim_left(const std::basic_string<CharT> & str,
                      std::size_t & first, std::size_t last)
{
    // Trim left
    std::size_t ltrim = find_first_not_of<CharT>(str, CharT(' '), first, last);
    if (ltrim == std::basic_string<CharT>::npos)
        ltrim = last;

    first = ltrim;
}

template <typename CharT = char>
static inline
void string_trim_right(const std::basic_string<CharT> & str,
                       std::size_t first, std::size_t & last)
{
    // Trim right
    std::size_t rtrim = find_last_not_of<CharT>(str, CharT(' '), first, last);
    if (rtrim == std::basic_string<CharT>::npos)
        rtrim = first;

    last = rtrim;
}

template <typename CharT = char>
static inline
void string_trim(const std::basic_string<CharT> & str,
                 std::size_t & first, std::size_t & last)
{
    // Trim left
    std::size_t ltrim = find_first_not_of<CharT>(str, CharT(' '), first, last);
    if (ltrim == std::basic_string<CharT>::npos)
        ltrim = last;

    // Trim right
    std::size_t rtrim = find_last_not_of<CharT>(str, CharT(' '), ltrim, last);
    if (rtrim == std::basic_string<CharT>::npos)
        rtrim = ltrim;
    else
        rtrim++;

    first = ltrim;
    last = rtrim;
}

template <typename CharT = char>
static inline
bool is_empty_or_null(const std::basic_string<CharT> & str)
{
    if (str.empty()) {
        return true;
    } else {
        const CharT * data = str.c_str();
        if (data != nullptr) {
            // Trim left
            std::size_t ltrim = str.find_first_not_of(CharT(' '));
            if (ltrim == std::basic_string<CharT>::npos)
                ltrim = str.size();

            // Trim right
            std::size_t rtrim = str.find_last_not_of(CharT(' '));
            if (rtrim == std::basic_string<CharT>::npos)
                rtrim = 0;
            else
                rtrim++;

            return (ltrim >= rtrim);
        } else {
            return true;
        }
    }
}

template <typename CharT = char>
static inline
std::size_t string_copy(std::basic_string<CharT> & dest,
                        const std::basic_string<CharT> & src,
                        std::size_t first, std::size_t last)
{
    for (std::size_t i = first; i < last; i++) {
        dest.push_back(src[i]);
    }
    return (last > first) ? (last - first) : 0;
}

template <typename CharT = char>
static inline
std::size_t string_copy(std::basic_string<CharT> & dest,
                        const std::basic_string<CharT> & src,
                        const Slice & slice)
{
    for (std::size_t i = slice.first; i < slice.last; i++) {
        dest.push_back(src[i]);
    }
    return slice.count();
}

template <typename CharT = char>
static inline
std::size_t string_copy_n(std::basic_string<CharT> & dest,
                          const std::basic_string<CharT> & src,
                          std::size_t offset, std::size_t count)
{
    std::size_t last = offset + count;
    for (std::size_t i = offset; i < last; i++) {
        dest.push_back(src[i]);
    }
    return count;
}

template <typename CharT = char>
static inline
void split_to_list(const std::basic_string<CharT> & text, CharT delimiter,
                   std::vector<std::basic_string<CharT>> & word_list,
                   bool use_trim = true,
                   bool allow_empty = false)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find(delimiter, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        if (use_trim) {
            string_trim<CharT>(text, ltrim, rtrim);
        }

        if (allow_empty || ltrim < rtrim) {
            std::basic_string<CharT> word;
            std::size_t len = string_copy(word, text, ltrim, rtrim);
            assert(len > 0);
            word_list.push_back(word);
        }

        if (token_pos < text.size())
            last_pos = token_pos + 1;
        else
            break;
    } while (1);
}

template <typename CharT = char>
static inline
void split_to_list(const std::basic_string<CharT> & text,
                   const std::basic_string<CharT> & delimiters,
                   std::vector<std::basic_string<CharT>> & word_list,
                   bool use_trim = true,
                   bool allow_empty = false)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find(delimiters, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        if (use_trim) {
            string_trim<CharT>(text, ltrim, rtrim);
        }

        if (allow_empty || ltrim < rtrim) {
            std::basic_string<CharT> word;
            std::size_t len = string_copy(word, text, ltrim, rtrim);
            assert(len > 0);
            word_list.push_back(word);
        }

        if (token_pos < text.size())
            last_pos = token_pos + delimiters.size();
        else
            break;
    } while (1);
}

template <typename CharT = char>
static inline
void split_to_list_of(const std::basic_string<CharT> & text, CharT delimiter,
                      std::vector<Slice> & word_list,
                      bool use_trim = true,
                      bool allow_empty = false)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find_first_of(delimiter, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        if (use_trim) {
            string_trim<CharT>(text, ltrim, rtrim);
        }

        if (allow_empty || ltrim < rtrim) {
            Slice word(ltrim, rtrim);
            std::size_t len = rtrim - ltrim;
            assert(len > 0);
            word_list.push_back(word);
        }

        if (token_pos < text.size())
            last_pos = token_pos + 1;
        else
            break;
    } while (1);
}

template <typename CharT = char>
static inline
void split_to_list_of(const std::basic_string<CharT> & text,
                      const std::basic_string<CharT> & delimiters,
                      std::vector<Slice> & word_list,
                      bool use_trim = true,
                      bool allow_empty = false)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find_first_of(delimiters, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        if (use_trim) {
            string_trim<CharT>(text, ltrim, rtrim);
        }

        if (allow_empty || ltrim < rtrim) {
            Slice word(ltrim, rtrim);
            std::size_t len = rtrim - ltrim;
            assert(len > 0);
            word_list.push_back(word);
        }

        if (token_pos < text.size())
            last_pos = token_pos + 1;
        else
            break;
    } while (1);
}

template <typename CharT = char>
static inline
void split_to_list_of(const std::basic_string<CharT> & text, CharT delimiter,
                      std::vector<std::basic_string<CharT>> & word_list,
                      bool use_trim = true,
                      bool allow_empty = false)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find_first_of(delimiter, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        if (use_trim) {
            string_trim<CharT>(text, ltrim, rtrim);
        }

        if (allow_empty || ltrim < rtrim) {
            std::basic_string<CharT> word;
            std::size_t len = string_copy(word, text, ltrim, rtrim);
            assert(len > 0);
            word_list.push_back(word);
        }

        if (token_pos < text.size())
            last_pos = token_pos + 1;
        else
            break;
    } while (1);
}

template <typename CharT = char>
static inline
void split_to_list_of(const std::basic_string<CharT> & text,
                      const std::basic_string<CharT> & delimiters,
                      std::vector<std::basic_string<CharT>> & word_list,
                      bool use_trim = true,
                      bool allow_empty = false)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find_first_of(delimiters, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        if (use_trim) {
            string_trim<CharT>(text, ltrim, rtrim);
        }

        if (allow_empty || ltrim < rtrim) {
            std::basic_string<CharT> word;
            std::size_t len = string_copy(word, text, ltrim, rtrim);
            assert(len > 0);
            word_list.push_back(word);
        }

        if (token_pos < text.size())
            last_pos = token_pos + 1;
        else
            break;
    } while (1);
}

///////////////////////////////////////////////////////////////////////////////////////

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
        > Variant;

///////////////////////////////////////////////////////////////////////////////////////

template <typename CharT>
struct Converter {
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type> string_type;

    template <typename... Types>
    static inline
    bool try_convert(jstd::Variant<Types...> & dest,
                     const string_type & value,
                     const char_type * value_start) {
        bool convertible = false;
        std::size_t index = dest.index();
        std::type_index type_index = dest.type_index();
        try {
            if (0) {
                // Do nothing !!
            } else if (type_index == typeid(char_type *)) {
                dest = (char_type *)value_start;
                convertible = true;
            } else if (type_index == typeid(const char_type *)) {
                dest = (const char_type *)value_start;
                convertible = true;
            } else if (type_index == typeid(void *)) {
                dest = (void *)value_start;
                convertible = true;
            } else if (type_index == typeid(const void *)) {
                dest = (const void *)value_start;
                convertible = true;
            } else if (type_index == typeid(bool)) {
                dest = (bool)(std::stoi(value) != 0);
                convertible = true;
            } else if (type_index == typeid(char)) {
                dest = (char)std::stoi(value);
                convertible = true;
            } else if (type_index == typeid(signed char)) {
                dest = (signed char)std::stoi(value);
                convertible = true;
            } else if (type_index == typeid(unsigned char)) {
                dest = (unsigned char)std::stoul(value);
                convertible = true;
            } else if (type_index == typeid(short)) {
                dest = (short)std::stoi(value);
                convertible = true;
            } else if (type_index == typeid(signed short)) {
                dest = (signed short)std::stoi(value);
                convertible = true;
            } else if (type_index == typeid(unsigned short)) {
                dest = (unsigned short)std::stoul(value);
                convertible = true;
            } else if (type_index == typeid(int)) {
                dest = (int)std::stoi(value);
                convertible = true;
            } else if (type_index == typeid(int32_t)) {
                dest = (int32_t)std::stoi(value);
                convertible = true;
            } else if (type_index == typeid(long)) {
                dest = (long)std::stol(value);
                convertible = true;
            } else if (type_index == typeid(long long)) {
                dest = (long long)std::stoll(value);
                convertible = true;
            } else if (type_index == typeid(int64_t)) {
                dest = (int64_t)std::stoll(value);
                convertible = true;
            } else if (type_index == typeid(unsigned int)) {
                dest = (unsigned int)std::stoul(value);
                convertible = true;
            } else if (type_index == typeid(uint32_t)) {
                dest = (uint32_t)std::stoul(value);
                convertible = true;
            } else if (type_index == typeid(unsigned long)) {
                dest = (unsigned long)std::stoul(value);
                convertible = true;
            } else if (type_index == typeid(unsigned long long)) {
                dest = (unsigned long long)std::stoull(value);
                convertible = true;
            } else if (type_index == typeid(uint64_t)) {
                dest = (uint64_t)std::stoull(value);
                convertible = true;
            } else if (type_index == typeid(size_t)) {
                dest = (size_t)std::stoull(value);
                convertible = true;
            } else if (type_index == typeid(intptr_t)) {
                dest = (intptr_t)std::stoll(value);
                convertible = true;
            } else if (type_index == typeid(uintptr_t)) {
                dest = (uintptr_t)std::stoull(value);
                convertible = true;
            } else if (type_index == typeid(ptrdiff_t)) {
                dest = (ptrdiff_t)std::stoll(value);
                convertible = true;
            } else if (type_index == typeid(float)) {
                dest = (float)std::stof(value);
                convertible = true;
            } else if (type_index == typeid(double)) {
                dest = (double)std::stod(value);
                convertible = true;
            } else if (type_index == typeid(long double)) {
                dest = (long double)std::stold(value);
                convertible = true;
            } else if (type_index == typeid(string_type)) {
                dest = value;
                convertible = true;
            }
        } catch(const std::invalid_argument & ex) {
            std::cout << "std::invalid_argument::what(): " << ex.what() << '\n';
        } catch(const std::out_of_range & ex) {
            std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
        } catch (const jstd::BadVariantAccess & ex) {
            std::cout << "jstd::BadVariantAccess::what(): " << ex.what() << '\n';
        }
        return convertible;
    }

    template <typename... Types>
    static inline
    bool try_to_string(const jstd::Variant<Types...> & src,
                       string_type & dest) {
        bool convertible = false;
        std::size_t index = src.index();
        std::type_index type_index = src.type();
        try {
            if (0) {
                // Do nothing !!
            } else if (type_index == typeid(char_type *)) {
                dest = string_type(src.template get<char_type *>());
                convertible = true;
            } else if (type_index == typeid(const char_type *)) {
                dest = string_type(src.template get<const char_type *>());
                convertible = true;
            } else if (type_index == typeid(void *)) {
                dest = string_type((char_type *)src.template get<void *>());
                convertible = true;
            } else if (type_index == typeid(const void *)) {
                dest = string_type((const char_type *)src.template get<const void *>());
                convertible = true;
            } else if (type_index == typeid(bool)) {
                dest = std::to_string((uint32_t)src.template get<bool>());
                convertible = true;
            } else if (type_index == typeid(char)) {
                dest = std::to_string((int32_t)src.template get<char>());
                convertible = true;
            } else if (type_index == typeid(signed char)) {
                dest = std::to_string((int32_t)src.template get<signed char>());
                convertible = true;
            } else if (type_index == typeid(unsigned char)) {
                dest = std::to_string((uint32_t)src.template get<unsigned char>());
                convertible = true;
            } else if (type_index == typeid(short)) {
                dest = std::to_string((int32_t)src.template get<short>());
                convertible = true;
            } else if (type_index == typeid(signed short)) {
                dest = std::to_string((int32_t)src.template get<signed short>());
                convertible = true;
            } else if (type_index == typeid(unsigned short)) {
                dest = std::to_string((uint32_t)src.template get<unsigned short>());
                convertible = true;
            } else if (type_index == typeid(int)) {
                dest = std::to_string(src.template get<int>());
                convertible = true;
            } else if (type_index == typeid(int32_t)) {
                dest = std::to_string(src.template get<int32_t>());
                convertible = true;
            } else if (type_index == typeid(long)) {
                dest = std::to_string(src.template get<long>());
                convertible = true;
            } else if (type_index == typeid(long long)) {
                dest = std::to_string(src.template get<long long>());
                convertible = true;
            } else if (type_index == typeid(int64_t)) {
                dest = std::to_string(src.template get<int64_t>());
                convertible = true;
            } else if (type_index == typeid(unsigned int)) {
                dest = std::to_string(src.template get<unsigned int>());
                convertible = true;
            } else if (type_index == typeid(uint32_t)) {
                dest = std::to_string(src.template get<uint32_t>());
                convertible = true;
            } else if (type_index == typeid(unsigned long)) {
                dest = std::to_string(src.template get<unsigned long>());
                convertible = true;
            } else if (type_index == typeid(unsigned long long)) {
                dest = std::to_string(src.template get<unsigned long long>());
                convertible = true;
            } else if (type_index == typeid(uint64_t)) {
                dest = std::to_string(src.template get<uint64_t>());
                convertible = true;
            } else if (type_index == typeid(size_t)) {
                dest = std::to_string(src.template get<size_t>());
                convertible = true;
            } else if (type_index == typeid(intptr_t)) {
                dest = std::to_string(src.template get<intptr_t>());
                convertible = true;
            } else if (type_index == typeid(uintptr_t)) {
                dest = std::to_string(src.template get<uintptr_t>());
                convertible = true;
            } else if (type_index == typeid(ptrdiff_t)) {
                dest = std::to_string(src.template get<ptrdiff_t>());
                convertible = true;
            } else if (type_index == typeid(float)) {
                dest = std::to_string(src.template get<float>());
                convertible = true;
            } else if (type_index == typeid(double)) {
                dest = std::to_string(src.template get<double>());
                convertible = true;
            } else if (type_index == typeid(long double)) {
                dest = std::to_string(src.template get<long double>());
                convertible = true;
            } else if (type_index == typeid(string_type)) {
                dest = src.template get<string_type>();
                convertible = true;
            }
        } catch(const std::invalid_argument & ex) {
            std::cout << "std::invalid_argument::what(): " << ex.what() << '\n';
        } catch(const std::out_of_range & ex) {
            std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
        } catch (const jstd::BadVariantAccess & ex) {
            std::cout << "jstd::BadVariantAccess::what(): " << ex.what() << '\n';
        }
        return convertible;
    }
};

///////////////////////////////////////////////////////////////////////////////////////

template <typename VariantT, typename CharT = char>
struct StringConverter : public ::jstd::static_visitor<VariantT> {
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type> string_type;
    typedef VariantT                     result_type;

    const string_type & value;
    const char_type *   value_start;

    StringConverter(const string_type & arg_value,
                    const char_type * arg_start) noexcept
        : value(arg_value), value_start(arg_start) {
    }

    result_type operator () (char_type * p) {
        UNUSED_VAR(p);
        result_type variant((char_type *)value_start);
        return variant;
    }

    result_type operator () (const char_type * p) {
        UNUSED_VAR(p);
        result_type variant((const char_type *)value_start);
        return variant;
    }

    result_type operator () (char_type * const p) const {
        UNUSED_VAR(p);
        result_type variant((char_type * const)value_start);
        return variant;
    }

    result_type operator () (const char_type * const p) const {
        UNUSED_VAR(p);
        result_type variant((const char_type * const)value_start);
        return variant;
    }

    result_type operator () (void * p) const {
        UNUSED_VAR(p);
        result_type variant((void *)value_start);
        return variant;
    }

    result_type operator () (const void * p) const {
        UNUSED_VAR(p);
        result_type variant((const void *)value_start);
        return variant;
    }

    result_type operator () (bool i) const {
        UNUSED_VAR(i);
        result_type variant((bool)(std::stoi(value) != 0));
        return variant;
    }

    result_type operator () (char i) const {
        UNUSED_VAR(i);
        result_type variant((char)std::stoi(value));
        return variant;
    }

    result_type operator () (signed char i) const {
        UNUSED_VAR(i);
        result_type variant((signed char)std::stoi(value));
        return variant;
    }

    result_type operator () (unsigned char i) const {
        UNUSED_VAR(i);
        result_type variant((unsigned char)std::stoul(value));
        return variant;
    }

    result_type operator () (short i) const {
        UNUSED_VAR(i);
        result_type variant((short)std::stoi(value));
        return variant;
    }

    result_type operator () (unsigned short i) const {
        UNUSED_VAR(i);
        result_type variant((unsigned short)std::stoul(value));
        return variant;
    }

    result_type operator () (int i) const {
        UNUSED_VAR(i);
        result_type variant((int)std::stoi(value));
        return variant;
    }

    result_type operator () (long i) const {
        UNUSED_VAR(i);
        result_type variant((long)std::stol(value));
        return variant;
    }

    result_type operator () (long long i) const {
        UNUSED_VAR(i);
        result_type variant((long long)std::stoll(value));
        return variant;
    }

    result_type operator () (unsigned int i) const {
        UNUSED_VAR(i);
        result_type variant((unsigned int)std::stoul(value));
        return variant;
    }

    result_type operator () (unsigned long i) const {
        UNUSED_VAR(i);
        result_type variant((unsigned long)std::stoul(value));
        return variant;
    }

    result_type operator () (unsigned long long i) const {
        UNUSED_VAR(i);
        result_type variant((unsigned long long)std::stoull(value));
        return variant;
    }

    result_type operator () (float f) const {
        UNUSED_VAR(f);
        result_type variant((float)std::stof(value));
        return variant;
    }

    result_type operator () (double d) const {
        UNUSED_VAR(d);
        result_type variant((double)std::stod(value));
        return variant;
    }

    result_type operator () (long double d) const {
        UNUSED_VAR(d);
        result_type variant((long double)std::stold(value));
        return variant;
    }

    result_type operator () (const string_type & str) const {
        UNUSED_VAR(str);
        result_type variant(value);
        return variant;
    }

    result_type operator () (const result_type & var) const {
        result_type variant(var);
        return variant;
    }
};

template <typename CharT = char>
struct StringFormatter : public ::jstd::static_visitor< std::basic_string<CharT> > {
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type> string_type;
    typedef string_type                  result_type;
    typedef Variant                      variant_t;

    result_type operator () (char_type * src, const variant_t & var) const {
        return result_type();
    }
};

///////////////////////////////////////////////////////////////////////////////////////

template <typename CharT = char>
struct BasicConfig {
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type> string_type;

    BasicConfig() {
    }

    bool assert_check(bool condition, const char * format, ...)
        __attribute__((format(printf, 3, 4))) {
        if (!condition) {
            va_list args;
            va_start(args, format);
            vfprintf(LOG_FILE, format, args);
            va_end(args);
        }
        return condition;
    }

#if defined(_MSC_VER)
    bool assert_check(bool condition, const wchar_t * format, ...)
        __attribute__((format(printf, 3, 4))) {
        if (!condition) {
            va_list args;
            va_start(args, format);
            vfwprintf(LOG_FILE, format, args);
            va_end(args);
        }
        return condition;
    }
#endif

    virtual int validate() {
        return Error::NoError;
    }
};

///////////////////////////////////////////////////////////////////////////////////////

struct OptType {
    enum {
        Unknown,
        Text,
        Section,
        Option,
        Command,
        Value,
        Max
    };
};

struct GenericError {
    typedef typename Error::error_code error_code;

    error_code err;

    GenericError(error_code _err = Error::NoError) noexcept : err(_err) {
    }

    GenericError(const GenericError & src) noexcept : err(src.err) {
    }

    bool isSuccess(error_code error_id) {
        return (Error::isSuccess(this->err));
    }

    bool isError() {
        return (Error::isError(this->err));
    }

    error_code err_code() const {
        return this->err;
    }

    void setError(error_code err) {
        this->err = err;
    }
};

struct ParseError {
    typedef typename Error::error_code error_code;

    error_code      err;
    std::int32_t    value;
    std::uint32_t   begin;
    std::uint32_t   end;

    ParseError(error_code _err = Error::NoError) noexcept
        : err(_err), value(0), begin(0), end(0) {
    }

    ParseError(error_code _err, std::size_t _value, std::size_t _begin = 0, std::size_t _end= 0) noexcept
        : err(_err), value((std::int32_t)_value), begin((std::uint32_t)_begin), end((std::uint32_t)_end) {
    }

    ParseError(const ParseError & src) noexcept
        : err(src.err), value(src.value), begin(src.begin), end(src.end) {
    }

    bool isSuccess(error_code error_id) {
        return (Error::isSuccess(this->err));
    }

    bool isError() {
        return (Error::isError(this->err));
    }

    error_code err_code() const {
        return this->err;
    }

    void setError(error_code err, std::size_t value, std::size_t begin = 0, std::size_t end = 0) {
        this->err   = err;
        this->value = (std::int32_t)value;
        this->begin = (std::uint32_t)begin;
        this->end   = (std::uint32_t)end;
    }
};

template <typename CharT = char>
class BasicResult {
public:
    typedef typename GenericError::error_code error_code;
    typedef std::size_t size_type;

protected:
    std::vector<GenericError> errors_;

public:
    BasicResult() {
    }
    BasicResult(const BasicResult & src) noexcept
        : errors_(src.errors_) {
    }
    BasicResult(BasicResult && src) noexcept
        : errors_(std::move(src.errors_)) {
    }
    ~BasicResult() {}

    size_type size() const {
        return errors_.size();
    }

    bool is_empty() const {
        return (errors_.size() == 0);
    }

    bool any_errors() const {
        return (errors_.size() > 0);
    }

    std::vector<GenericError> & error_list() {
        return errors_;
    }

    const std::vector<GenericError> & error_list() const {
        return errors_;
    }

    void clear() {
        errors_.clear();
    }

    void reserve(size_type count) {
        errors_.reserve(count);
    }

    void resize(size_type newCapacity) {
        errors_.resize(newCapacity);
    }

    void add_error(const GenericError & err) {
        errors_.push_back(err);
    }

    void add_error(error_code err_code) {
        GenericError err(err_code);
        errors_.push_back(err);
    }

    template <typename OutputStreamT>
    void print_errors(OutputStreamT & os) {
        //
    }

    void print_errors() {
        this->print_errors(std::cout);
    }
};

template <typename CharT = char>
class BasicParseResult {
public:
    typedef BasicParseResult<CharT>         this_type;
    typedef typename ParseError::error_code error_code;
    typedef std::size_t size_type;

protected:
    std::vector<ParseError> errors_;

public:
    BasicParseResult() {
    }
    BasicParseResult(const BasicParseResult & src) noexcept
        : errors_(src.errors_) {
    }
    BasicParseResult(BasicParseResult && src) noexcept
        : errors_(std::move(src.errors_)) {
    }
    ~BasicParseResult() {}

    size_type size() const {
        return errors_.size();
    }

    bool is_empty() const {
        return (errors_.size() == 0);
    }

    bool any_errors() const {
        return (errors_.size() > 0);
    }

    std::vector<ParseError> & error_list() {
        return errors_;
    }

    const std::vector<ParseError> & error_list() const {
        return errors_;
    }

    void clear() {
        errors_.clear();
    }

    void reserve(size_type count) {
        errors_.reserve(count);
    }

    void resize(size_type newCapacity) {
        errors_.resize(newCapacity);
    }

    void add_error(const ParseError & err_info) {
        errors_.push_back(err_info);
    }

    void add_error(error_code err, size_type value, size_type begin, size_type end) {
        ParseError parseError(err, value, begin, end);
        errors_.push_back(parseError);
    }

    template <typename OutputStreamT>
    void print_errors(OutputStreamT & os) {
        //
    }

    void print_errors() {
        this->print_errors(std::cout);
    }
};

template <typename CharT = char>
struct BasicPrintStyle {
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef std::basic_string<char_type>                    string_type;

    typedef std::size_t size_type;

    bool compact_style;
    bool auto_fit;
    size_type tab_size;
    size_type ident_spaces;
    size_type min_padding;
    size_type max_left_column;
    size_type max_column;

    string_type separator;

    BasicPrintStyle() : compact_style(true), auto_fit(false),
                        tab_size(4), ident_spaces(2), min_padding(1),
                        max_left_column(32), max_column(80) {
        this->separator.push_back(char_type(':'));
    }
};

template <typename CharT = char>
class BasicVirtualTextArea {
    typedef BasicVirtualTextArea<CharT>                     this_type;
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type>    string_type;
    typedef BasicPrintStyle<char_type>      PrintStyle;

    typedef std::size_t size_type;

    static const size_type kMaxReservedSize = 4096;

protected:
    const PrintStyle & print_style_;
    string_type text_buf_;
    size_type   lines_;

public:
    BasicVirtualTextArea(const PrintStyle & print_style)
        : print_style_(print_style), lines_(0) {
    }

public:
    size_type size() const {
        return text_buf_.size();
    }

    size_type lines() const {
        return lines_;
    }

    size_type acutal_lines() const {
        return (text_buf_.size() / (print_style_.max_column + 1));
    }

    size_type capacity_lines() const {
        return (text_buf_.capacity() / (print_style_.max_column + 1));
    }

    size_type virtual_pos(size_type rows, size_type cols) const {
        return (rows * (print_style_.max_column + 1) + cols);
    }

    PrintStyle & print_style() {
        return const_cast<PrintStyle &>(this->print_style_);
    }

    const PrintStyle & print_style() const {
        return this->print_style_;
    }

    void clear() {
        text_buf_.clear();
        lines_ = 0;
    }

    void reset(char_type ch = char_type(' ')) {
        if (text_buf_.size() <= kMaxReservedSize) {
            std::fill_n(&text_buf_[0], text_buf_.size(), ch);
        } else {
            text_buf_.resize(kMaxReservedSize, ch);
        }
        lines_ = 0;
    }

    void reserve_lines(size_type lines) {
        text_buf_.reserve((print_style_.max_column + 1) * lines);
    }

    void resize_lines(size_type lines, char_type ch = char_type(' ')) {
        text_buf_.resize((print_style_.max_column + 1) * lines, ch);
    }

    void prepare_add_lines(size_type lines) {
        size_type n_acutal_lines = this->acutal_lines();
        size_type new_lines = this->lines_ + lines;
        if (new_lines > n_acutal_lines) {
            this->resize_lines(new_lines * 3 / 2 + 1);
        }
    }

    void append_text(size_type offset, const string_type & text,
                     const std::vector<Slice> & line_list,
                     bool update_lines = true) {
        prepare_add_lines(line_list.size());

        size_type lines = this->lines_;
        for (auto const & line : line_list) {
            size_type pos = virtual_pos(lines, offset);
            for (size_type i = line.first; i < line.last; i++) {
                text_buf_[pos++] = text[i];
            }
            lines++;
        }

        if (update_lines) {
            this->lines_ = lines;
        }
    }

    void append_text(size_type offset_x, size_type offset_y,
                     size_type total_lines,
                     const string_type & text,
                     const std::vector<Slice> & line_list,
                     bool update_lines = true) {
        assert(total_lines >= line_list.size());
        prepare_add_lines(total_lines);

        size_type lines = this->lines_ + offset_y;
        for (auto const & line : line_list) {
            size_type pos = virtual_pos(lines, offset_x);
            for (size_type i = line.first; i < line.last; i++) {
                text_buf_[pos++] = text[i];
            }
            lines++;
        }

        if (update_lines) {
            this->lines_ = lines;
        }
    }

    void append_text(size_type offset,
                     const std::vector<string_type> & line_list,
                     bool update_lines = true) {
        prepare_add_lines(line_list.size());

        size_type lines = this->lines_;
        for (auto const & line : line_list) {
            size_type pos = virtual_pos(lines, offset);
            for (size_type i = 0; i < line.size(); i++) {
                text_buf_[pos++] = line[i];
            }
            lines++;
        }

        if (update_lines) {
            this->lines_ = lines;
        }
    }

    void append_text(size_type offset_x, size_type offset_y,
                     size_type total_lines,
                     const std::vector<string_type> & line_list,
                     bool update_lines = true) {
        assert(total_lines >= line_list.size());
        prepare_add_lines(total_lines);

        size_type lines = this->lines_ + offset_y;
        for (auto const & line : line_list) {
            size_type pos = virtual_pos(lines, offset_x);
            for (size_type i = 0; i < line.size(); i++) {
                text_buf_[pos++] = line[i];
            }
            lines++;
        }

        if (update_lines) {
            this->lines_ = lines;
        }
    }

    void append_new_line(size_type n = 1) {
        if (n > 0) {
            prepare_add_lines(n);

            for (size_type i = 0; i < n; i++) {
                size_type pos = virtual_pos(this->lines_, 0);
                text_buf_[pos] = char_type('\n');
                this->lines_++;
            }
        }
    }

    size_type prepare_display_text(const string_type & text,
                                   string_type & display_text,
                                   std::vector<string_type> & line_list,
                                   size_type max_column,
                                   bool lineCrLf = true,
                                   bool endCrLf = true) const {
        std::vector<Slice> word_list;
        split_to_list_of(text, char_type(' '), word_list);

        line_list.clear();

        string_type line;
        size_type column = 0;
        size_type last_column = 0;
        bool is_end;
        for (size_type i = 0; i < word_list.size(); i++) {
            Slice sword = word_list[i];
            string_type word;
            do {
                bool need_wrap = false;
                bool new_line = false;
                is_end = false;
                while (sword.first < sword.last) {
                    char_type ch = text[sword.first];
                    if (ch != char_type('\t')) {
                        if (ch != char_type('\n')) {
                            if (ch >= char_type(' ')) {
                                word.push_back(ch);
                            } else if (ch == char_type('\0')) {
                                is_end = true;
                                break;
                            } else {
                                // Skip the another control chars: /b /v /f /a /r
                            }
                        } else {
                            need_wrap = true;
                            break;
                        }
                    } else {
                        for (size_type i = 0; i < print_style_.tab_size; i++) {
                            word.push_back(char_type(' '));
                        }
                    }
                    sword.first++;
                }

                if ((column + word.size()) <= max_column) {
                    size_type pre_column = column + word.size();
                    if (pre_column < max_column) {
                        if (i < (word_list.size() - 1)) {
                            Slice word_next = word_list[i + 1];
                            assert(word_next.first > sword.last);
                            size_type num_space = word_next.first - sword.last;
                            assert(num_space > 0);
                            if ((pre_column + num_space) >= max_column) {
                                // If after add the space chars, it's exceed the max column,
                                // push the '\n' directly.
                                if (lineCrLf) {
                                    word.push_back(char_type('\n'));
                                }
                                new_line = true;
                            } else {
                                if (!need_wrap) {
                                    for (size_type i = 0; i < num_space; i++) {
                                        word.push_back(char_type(' '));
                                    }
                                    pre_column += num_space;
                                } else {
                                    column += word.size();
                                    if (lineCrLf) {
                                        word.push_back(char_type('\n'));
                                    }

                                    line += word;
                                    line_list.push_back(line);
                                    line.clear();

                                    last_column = pre_column;
                                    pre_column = 0;

                                    word.clear();
                                    sword.first++;
                                    continue;
                                }
                            }
                        } else {
                            if (need_wrap) {
                                if (lineCrLf) {
                                    word.push_back(char_type('\n'));
                                }

                                line += word;
                                line_list.push_back(line);
                                line.clear();

                                last_column = pre_column;
                                pre_column = 0;

                                word.clear();
                            }

                            if (endCrLf) {
                                word.push_back(char_type('\n'));
                            }
                            is_end = true;
                        }
                    } else {
                        if (lineCrLf) {
                            word.push_back(char_type('\n'));
                        }
                        new_line = true;
                    }

                    line += word;
                    column = pre_column;

                    if (new_line || need_wrap || is_end) {
                        line_list.push_back(line);
                        line.clear();

                        last_column = column;
                        column = 0;

                        if (need_wrap) {
                            word.clear();
                        }
                    }
                } else {
                    if (lineCrLf) {
                        line.push_back(char_type('\n'));
                    }
                    line_list.push_back(line);
                    line.clear();

                    last_column = column;
                    column = word.size();

                    line = word;

                    if (need_wrap) {
                        line.push_back(char_type('\n'));
                        line_list.push_back(line);
                        line.clear();

                        last_column = column;
                        column = 0;

                        sword.first++;
                    }

                    bool is_last = ((i >= (word_list.size() - 1)) && (sword.first >= sword.last));
                    if (is_last || is_end) {
                        if (endCrLf) {
                            line.push_back(char_type('\n'));
                        }
                        line_list.push_back(line);
                        line.clear();

                        last_column = column;
                        column = 0;

                        is_end = true;
                    } else if (need_wrap) {
                        word.clear();
                    }
                }

                if ((sword.first >= sword.last) || is_end) {
                    break;
                }
            } while (1);

            if (is_end)
                break;
        }

        display_text.clear();
        for (size_type i = 0; i < line_list.size(); i++) {
            display_text += line_list[i];
        }

        return last_column;
    }

    string_type to_string() {
        string_type real_text;
        for (size_type l = 0; l < this->lines(); l++) {
            size_type first_pos = virtual_pos(l, 0);
            size_type last_pos = virtual_pos(l, print_style_.max_column);
            do {
                char_type ch = text_buf_[last_pos];
                if (ch == char_type(' ')) {
                    if (last_pos == first_pos) {
                        // It's a empty line.
                        break;
                    }
                    last_pos--;
                } else {
                    if (first_pos <= last_pos) {
                        for (size_type i = first_pos; i <= last_pos; i++) {
                            real_text.push_back(text_buf_[i]);
                        }
                        if (ch != char_type('\n')) {
                            real_text.push_back(char_type('\n'));
                        }
                    } else {
                        // It's a empty line.
                        //real_text.push_back(char_type('\n'));
                    }
                    break;
                }
            } while (first_pos <= last_pos);
        }
        return real_text;
    }
};

template <typename CharT = char>
class BasicCmdLine {
public:
    typedef BasicCmdLine<CharT>                             this_type;
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type>    string_type;
    typedef BasicParseResult<char_type>     ParseResult;
    typedef BasicPrintStyle<char_type>      PrintStyle;
    typedef BasicVirtualTextArea<char_type> VirtualTextArea;

    typedef std::size_t size_type;
    typedef Variant     variant_t;

#if defined(_MSC_VER)
    static const char_type kPathDelimiter = char_type('\\');
#else
    static const char_type kPathDelimiter = char_type('/');
#endif

    union ParamState {
        struct {
            std::uint16_t   type;
            std::uint16_t   order;
            std::uint32_t   required   : 1;
            std::uint32_t   is_default : 1;
            std::uint32_t   visited    : 1;
            std::uint32_t   assigned   : 1;
            std::uint32_t   unused     : 28;
        };
        uint64_t value;

        ParamState() : value(0) {
        }
    };

    struct Param {
        enum {
            Normal,
            ShortLabel,
            LongLabel
        };
        ParamState  state;
        string_type label;
        variant_t   value;
    };

    struct ParamName {
        string_type label;
        size_type   type;

        ParamName() noexcept : type(Param::Normal) {
        }
        ParamName(const string_type & _label, int _type) noexcept
            : label(_label), type((size_type)_type) {
        }
        ParamName(const string_type & _label, size_type _type) noexcept
            : label(_label), type(_type) {
        }

        ParamName(const ParamName & src) noexcept
            : label(src.label), type(src.type) {
        }

        ParamName(ParamName && src) noexcept
            : label(std::move(src.label)), type(src.type) {
        }

        ParamName & operator = (const ParamName & rhs) noexcept {
            this->label = rhs.label;
            this->type = rhs.type;
            return *this;
        }

        ParamName & operator = (ParamName && rhs) noexcept {
            this->label = std::move(rhs.label);
            this->type = rhs.type;
            return *this;
        }
    };

    struct Option {
        std::uint32_t   type;
        std::uint32_t   desc_id;
        string_type     labels;
        string_type     display_text;
        string_type     desc;
        Param           param;

        static const size_type NotFound       = (size_type)-1;
        static const std::uint32_t NotFound32 = (std::uint32_t)-1;
        static const std::uint16_t NotFound16 = (std::uint16_t)-1;

        static const size_type Unknown       = (size_type)-1;
        static const std::uint32_t Unknown32 = (std::uint32_t)-1;

        Option(std::uint32_t _type = OptType::Unknown)
            : type(_type), desc_id(Unknown32) {
        }

        virtual ~Option() {
            //
        }
    };

    class Group {
        //
    };

    class OptionDesc {
    public:
        string_type label;

        std::vector<Option>                        option_list;
        std::unordered_map<string_type, size_type> option_map;

        OptionDesc() {
        }

        OptionDesc(const string_type & _label) : label(_label) {
        }

    private:
        size_type parseOptionLabel(const string_type & labels,
                                   std::vector<ParamName> & param_list,
                                   std::vector<string_type> & text_list) {
            param_list.clear();
            text_list.clear();

            std::vector<string_type> word_list;
            string_type delimiters;
            delimiters.push_back(char_type(','));
            delimiters.push_back(char_type(' '));
            split_to_list_of(labels, delimiters, word_list);

            size_type nums_word = word_list.size();
            if (nums_word > 0) {
                for (auto const & word : word_list) {
                    assert(!word.empty());
                    string_type label;
                    char_type ch_0 = word[0];
                    if (ch_0 == char_type('-')) {
                        char_type ch_1 = (word.size() > 1) ? word[1] : char_type('\0');
                        if (word.size() > 1 && ch_1 == char_type('-')) {
                            label = word.substr(2, word.size() - 2);
                            if (!label.empty()) {
                                ParamName param(label, Param::LongLabel);
                                param_list.push_back(param);
                            }
                        } else {
                            label = word.substr(1, word.size() - 1);
                            if (!label.empty()) {
                                ParamName param(label, Param::ShortLabel);
                                param_list.push_back(param);
                            }
                        }
                    } else if (ch_0 == char_type('<')) {
                        if ((word.size() >= 2) &&
                            (word[word.size() - 1] == char_type('>'))) {
                            text_list.push_back(word);
                        }
                    } else if (ch_0 == char_type('=')) {
                        // Skip
                    } else {
                        ParamName param(word, Param::Normal);
                        param_list.push_back(param);
                    }
                }
            }
            return param_list.size();
        }

        // Accept format: --name=abc, or -n=10, or -n 10
        int addOptionImpl(std::uint32_t type, const string_type & labels,
                          const string_type & desc,
                          const variant_t & value,
                          bool is_default) {
            int err_code = Error::NoError;

            Option option(type);
            option.labels = labels;
            option.display_text = labels;
            option.desc = desc;
            option.param.state.is_default = is_default;
            option.param.value = value;

            std::vector<ParamName> param_list;
            std::vector<string_type> text_list;
            size_type nums_label = this->parseOptionLabel(labels, param_list, text_list);
            if (nums_label > 0) {
                // Sort all the option labels asc
                for (size_type i = 0; i < (param_list.size() - 1); i++) {
                    for (size_type j = i + 1; j < param_list.size(); j++) {
                        if (param_list[i].label.size() > param_list[j].label.size()) {
                            std::swap(param_list[i], param_list[j]);
                        } else if (param_list[i].label.size() == param_list[j].label.size()) {
                            if (param_list[i].label > param_list[j].label)
                                std::swap(param_list[i], param_list[j]);
                        }
                    }
                }

                // Format short label text
                string_type s_labels;
                for (size_type i = 0; i < param_list.size(); i++) {
                    s_labels += param_list[i].label;
                    if ((i + 1) < param_list.size())
                        s_labels += char_type(',');
                }
                option.labels = s_labels;

                // Format display label text
                string_type display_text;
                for (size_type i = 0; i < param_list.size(); i++) {
                    const string_type & label = param_list[i].label;
                    if (label.size() == 1) {
                        display_text += char_type('-');
                    } else if (label.size() > 1) {
                        display_text += char_type('-');
                        display_text += char_type('-');
                    } else {
                        continue;
                    }
                    display_text += label;
                    if ((i + 1) < param_list.size()) {
                        display_text += char_type(',');
                        display_text += char_type(' ');
                    }
                }
                if (text_list.size() > 0) {
                    display_text += char_type(' ');
                }
                for (size_type i = 0; i < text_list.size(); i++) {
                    display_text += text_list[i];
                    if ((i + 1) < text_list.size())
                        display_text += char_type(' ');
                }
                option.display_text = display_text;

                size_type option_id = this->option_list.size();
                this->option_list.push_back(option);

                for (auto iter = param_list.begin(); iter != param_list.end(); ++iter) {
                    const string_type & label = iter->label;
                    // Only the first option with the same name is valid.
                    if (this->option_map.count(label) == 0) {
                        this->option_map.insert(std::make_pair(label, option_id));
                    } else {
                        // Warning
                        printf("Warning: desc: \"%s\", option_id: %u, arg_label = \"%s\" already exists.\n\n",
                               this->label.c_str(), (uint32_t)option_id, label.c_str());
                    }
                }
            }
            return (err_code == Error::NoError) ? (int)nums_label : err_code;
        }

        template <typename OutputStreamT>
        void print_compact_style(OutputStreamT & os, VirtualTextArea & text_buf) const {
            const PrintStyle & ps = text_buf.print_style();
            std::vector<string_type> line_list;
            if (!is_empty_or_null(this->label)) {
                string_type title_text;
                text_buf.prepare_display_text(this->label,
                                              title_text,
                                              line_list,
                                              ps.max_column);
                text_buf.append_new_line();
                text_buf.append_text(0, line_list);
                text_buf.append_new_line();
            }

            for (auto const & option : this->option_list) {
                if (option.type == OptType::Text) {
                    if (!is_empty_or_null(option.desc)) {
                        string_type desc_text;
                        text_buf.prepare_display_text(option.desc,
                                                      desc_text,
                                                      line_list,
                                                      ps.max_column);
                        text_buf.append_text(0, line_list);
                    }
                } else {
                    if (!is_empty_or_null(option.display_text)) {
                        string_type display_text;
                        size_type column = text_buf.prepare_display_text(
                                                        option.display_text,
                                                        display_text, line_list,
                                                        ps.max_column - ps.ident_spaces,
                                                        true, false);
                        size_type lines = ((line_list.size() > 0) ? (line_list.size() - 1) : 0);
                        text_buf.append_text(ps.ident_spaces, line_list, false);
                        if (!is_empty_or_null(option.desc)) {
                            string_type desc_text;
                            text_buf.prepare_display_text(option.desc,
                                                          desc_text,
                                                          line_list,
                                                          ps.max_column - ps.max_left_column);
                            if ((column + ps.ident_spaces) > (ps.max_left_column - ps.min_padding)) {
                                lines++;
                            }
                            size_type total_lines = lines + line_list.size();
                            text_buf.append_text(ps.max_left_column, lines, total_lines, line_list);
                        }
                    } else {
                        if (!is_empty_or_null(option.desc)) {
                            string_type desc_text;
                            text_buf.prepare_display_text(option.desc,
                                                          desc_text,
                                                          line_list,
                                                          ps.max_column);
                            text_buf.append_text(ps.ident_spaces, line_list);
                        }
                    }
                }
            }

            string_type out_text = text_buf.to_string();
            os << out_text;
        }

        template <typename OutputStreamT>
        void print_relaxed_style(OutputStreamT & os) const {
            if (!is_empty_or_null(this->label)) {
                printf("%s:\n\n", this->label.c_str());
            }

            for (auto const & option : this->option_list) {
                if (option.type == OptType::Text) {
                    if (!is_empty_or_null(option.desc)) {
                        printf("%s\n\n", option.desc.c_str());
                    }
                } else {
                    if (!is_empty_or_null(option.display_text)) {
                        printf("  %s :\n\n", option.display_text.c_str());
                        if (!is_empty_or_null(option.desc)) {
                            printf("    %s\n\n", option.desc.c_str());
                        }
                    } else {
                        if (!is_empty_or_null(option.desc)) {
                            printf("%s\n\n", option.desc.c_str());
                        }
                    }
                }
            }
        }

    public:
        void find_all_name(size_type target_id, std::vector<string_type> & name_list) const {
            name_list.clear();
            for (auto iter = this->option_map.begin(); iter != this->option_map.end(); ++iter) {
                const string_type & arg_name = iter->first;
                size_type option_id = iter->second;
                if (option_id == target_id) {
                    name_list.push_back(arg_name);
                }
            }
        }

        void addText(const char * format, ...) __attribute__((format(printf, 2, 3))) {
            static const size_type kMaxTextSize = 4096;
            char text[kMaxTextSize];
            va_list args;
            va_start(args, format);
            int text_size = vsnprintf(text, kMaxTextSize, format, args);
            assert(text_size < (int)kMaxTextSize);
            va_end(args);

            Option option(OptType::Text);
            option.desc = text;
            this->option_list.push_back(option);
        }

#if defined(_MSC_VER)
        void addText(const wchar_t * format, ...) __attribute__((format(printf, 2, 3))) {
            static const size_type kMaxTextSize = 4096;
            wchar_t text[kMaxTextSize];
            va_list args;
            va_start(args, format);
            int text_size = _vsnwprintf(text, kMaxTextSize, format, args);
            assert(text_size < (int)kMaxTextSize);
            va_end(args);

            Option option(OptType::Text);
            option.desc = text;
            this->option_list.push_back(option);
        }
#endif

        int addOption(const string_type & labels, const string_type & desc, const variant_t & value) {
            return this->addOptionImpl(OptType::Option, labels, desc, value, true);
        }

        int addOption(const string_type & labels, const string_type & desc) {
            Variant default_value(size_type(0));
            return this->addOptionImpl(OptType::Option, labels, desc, default_value, false);
        }

        template <typename OutputStreamT>
        void print(OutputStreamT & os, VirtualTextArea & text_buf) const {
            if (text_buf.print_style().compact_style) {
                this->print_compact_style(os, text_buf);
            } else {
                this->print_relaxed_style(os);
            }
        }
    }; // class OptionsDesc

protected:
    std::vector<OptionDesc>                     desc_list_;
    std::vector<Option>                         option_list_;
    std::unordered_map<string_type, size_type>  option_map_;

    std::vector<Param>       param_list_;
    std::vector<string_type> arg_list_;

    string_type app_name_;
    string_type display_name_;
    string_type version_;

    PrintStyle print_style_;

    Param empty_param_;

public:
    BasicCmdLine() : print_style_() {
        this->version_.push_back(char_type('1'));
        this->version_.push_back(char_type('.'));
        this->version_.push_back(char_type('0'));
        this->version_.push_back(char_type('.'));
        this->version_.push_back(char_type('0'));
    }

protected:
    Option & getOptionById(size_type option_id) {
        assert(option_id >= 0 && option_id < this->option_list_.size());
        return this->option_list_[option_id];
    }

    const Option & getOptionById(size_type option_id) const {
        assert(option_id >= 0 && option_id < this->option_list_.size());
        return this->option_list_[option_id];
    }

    bool hasOption(const string_type & name) const {
        auto iter = this->option_map_.find(name);
        return (iter != this->option_map_.end());
    }

    size_type getOption(const string_type & name, Option *& option) {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            size_type option_id = iter->second;
            if (option_id < this->option_list_.size()) {
                option = (Option *)&this->getOptionById(option_id);
                return option_id;
            }
        }
        return Option::NotFound;
    }

    size_type getOption(const string_type & name, const Option *& option) const {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            size_type option_id = iter->second;
            if (option_id < this->option_list_.size()) {
                option = (const Option *)&this->getOptionById(option_id);
                return option_id;
            }
        }
        return Option::NotFound;
    }

public:
    static bool isWideString() {
        return (sizeof(char_type) != 1);
    }

    static string_type getAppName(char_type * arg) {
        string_type strModuleName = arg;
        size_type lastSepPos = strModuleName.find_last_of(kPathDelimiter);
        if (lastSepPos != string_type::npos) {
            string_type appName;
            for (size_type i = lastSepPos + 1; i < strModuleName.size(); i++) {
                appName.push_back(strModuleName[i]);
            }
            return appName;
        } else {
            return strModuleName;
        }
    }

    static string_type getAppName(char_type * argv[]) {
        return this_type::getAppName(argv[0]);
    }

    size_type argc() const {
        return this->arg_list_.size();
    }

    std::vector<string_type> & argv() {
        return this->arg_list_;
    }

    const std::vector<string_type> & argv() const {
        return this->arg_list_;
    }

    string_type & getAppName() {
        return this->app_name_;
    }

    const string_type & getAppName() const {
        return this->app_name_;
    }

    string_type & getDisplayName() {
        return this->display_name_;
    }

    const string_type & getDisplayName() const {
        return this->display_name_;
    }

    string_type & getVersion() {
        return this->version_;
    }

    const string_type & getVersion() const {
        return this->version_;
    }

    this_type &  setDisplayName(const string_type & display_name) {
        this->display_name_ = display_name;
        return *this;
    }

    this_type &  setVersion(const string_type & version) {
        this->version_ = version;
        return *this;
    }

    bool isCompactStyle() const {
        return this->print_style_.compact_style;
    }

    bool isAutoFit() const {
        return this->print_style_.auto_fit;
    }

    size_type getMinPadding() const {
        return this->print_style_.min_padding;
    }

    size_type getMaxStartPos() const {
        return this->print_style_.max_left_column;
    }

    size_type getMaxColumn() const {
        return this->print_style_.max_column;
    }

    string_type & getSeparator() {
        return this->print_style_.separator;
    }

    const string_type & getSeparator() const {
        return this->print_style_.separator;
    }

    this_type & setCompactStyle(bool compact_style) {
        this->print_style_.compact_style = compact_style;
        return *this;
    }

    this_type & setAutoFit(bool auto_fit) {
        this->print_style_.auto_fit = auto_fit;
        return *this;
    }

    this_type & setMinPadding(size_type min_padding) {
        this->print_style_.min_padding = min_padding;
        return *this;
    }

    this_type & setMaxLeftColumn(size_type left_column) {
        this->print_style_.max_left_column = left_column;
        return *this;
    }

    this_type & setMaxColumn(size_type max_column) {
        this->print_style_.max_column = max_column;
        return *this;
    }

    this_type & setSeparator(const string_type & separator) {
        this->print_style_.separator = separator;
        return *this;
    }

    PrintStyle & getPrintStyle() {
        return this->print_style_;
    }

    const PrintStyle & getPrintStyle() const {
        return this->print_style_;
    }

    this_type & setPrintStyle(const PrintStyle & ps) {
        this->print_style_ = ps;
        return *this;
    }

    this_type & clear() {
        this->arg_list_.clear();
        this->param_list_.clear();

        this->option_list_.clear();
        this->option_map_.clear();
        this->desc_list_.clear();

        return *this;
    }

    size_type order(const string_type & name) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (size_type)option->param.state.order;
        } else {
            return Option::NotFound;
        }
    }

    bool isRequired(const string_type & name) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (option->param.state.required != 0);
        } else {
            return false;
        }
    }

    bool required(const string_type & name) const {
        return this->isRequired(name);
    }

    bool isVisited(const string_type & name) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (option->param.state.visited != 0);
        } else {
            return false;
        }
    }

    bool visited(const string_type & name) const {
        return this->isVisited(name);
    }

    bool isAssigned(const string_type & name) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (option->param.state.assigned != 0);
        } else {
            return false;
        }
    }

    bool assigned(const string_type & name) const {
        return this->isAssigned(name);
    }

    bool getParamState(const string_type & name, ParamState & param_state) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            param_state = option->param.state;
            return true;
        } else {
            return false;
        }
    }

    Param & getParam(const string_type & name) {
        Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return option->param;
        } else {
            return this->empty_param_;
        }
    }

    const Param & getParam(const string_type & name) const {
        Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return option->param;
        } else {
            return this->empty_param_;
        }
    }

    size_type parseNames(const string_type & names,
                         std::vector<ParamName> & param_list) {
        param_list.clear();

        std::vector<string_type> word_list;
        char_type delimiter = char_type(',');
        split_to_list(labels, delimiter, word_list);

        size_type nums_word = word_list.size();
        if (nums_word > 0) {
            for (auto const & word : word_list) {
                assert(!word.empty());
                string_type label;
                char_type ch_0 = word[0];
                if (ch_0 == char_type('-')) {
                    char_type ch_1 = (word.size() > 1) ? word[1] : char_type('\0');
                    if (word.size() > 1 && ch_1 == char_type('-')) {
                        label = word.substr(2, word.size() - 2);
                        if (!label.empty()) {
                            ParamName param(label, Param::LongLabel);
                            param_list.push_back(param);
                        }
                    } else {
                        label = word.substr(1, word.size() - 1);
                        if (!label.empty()) {
                            ParamName param(label, Param::ShortLabel);
                            param_list.push_back(param);
                        }
                    }
                } else if (((ch_0 >= char_type('!')) && (ch_0 <= char_type('/'))) ||
                           ((ch_0 >= char_type(':')) && (ch_0 <= char_type('@'))) ||
                           ((ch_0 >= char_type('[')) && (ch_0 <= char_type('`'))) ||
                           ((ch_0 >= char_type('{')) && (ch_0 <= char_type('~')))) {
                    // Skip
                } else {
                    ParamName param(word, Param::Normal);
                    param_list.push_back(param);
                }
            }
        }
        return param_list.size();
    }

    bool hasVar(const string_type & name) const {
        return this->hasOption(name);
    }

    bool getVar(const string_type & name, Param & variable) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            variable = option->param;
            return true;
        } else {
            return false;
        }
    }

    bool getVar(const string_type & name, variant_t & value) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            value = option->param.value;
            return true;
        } else {
            return false;
        }
    }

    template <typename T>
    bool getVar(const string_type & name, T & value) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            value = option->param.value.template get<T>();
            return true;
        } else {
            return false;
        }
    }

    this_type & setVar(const string_type & name, const Variant & value) {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            size_type option_id = iter->second;
            if (option_id < this->option_list_.size()) {
                Option & option = this->getOptionById(option_id);
                option.param.value = value;
            }
        }
        return *this;
    }

    template <typename T>
    this_type & setVar(const string_type & name, const T & value) {
        return this->setVar(name, value);
    }

    this_type & addDesc(const OptionDesc & desc) {
        size_type desc_id = this->desc_list_.size();
        this->desc_list_.push_back(desc);
        size_type index = 0;
        for (auto const & option : desc.option_list) {
            std::vector<string_type> arg_names;
            desc.find_all_name(index, arg_names);
            if (arg_names.size() != 0) {
                size_type option_id = this->option_list_.size();
                this->option_list_.push_back(option);
                // Actual desc id
                this->option_list_[option_id].desc_id = (std::uint32_t)desc_id;
                for (size_type i = 0; i < arg_names.size(); i++) {
                    if (this->option_map_.count(arg_names[i]) == 0) {
                        this->option_map_.insert(std::make_pair(arg_names[i], option_id));
                    } else {
                        printf("Warning: desc_id: %u, desc: \"%s\", option_id: %u, arg_name = \"%s\" already exists.\n\n",
                               (uint32_t)desc_id, desc.label.c_str(), (uint32_t)option_id, arg_names[i].c_str());
                    }
                };
            }
            index++;
        }
        return *this;
    }

    this_type & printVersion() const {
        printf("\n");
        if (!is_empty_or_null(this->display_name_)) {
            printf("%s v%s\n", this->display_name_.c_str(), this->version_.c_str());
        } else {
            if (!is_empty_or_null(this->app_name_)) {
                printf("%s v%s\n", this->app_name_.c_str(), this->version_.c_str());
            } else {
                printf("No-name program v%s\n", this->version_.c_str());
            }
        }
        printf("\n");
        return *(const_cast<this_type *>(this));
    }

    template <typename OutputStreamT>
    this_type & printUsage(OutputStreamT & os) const {
        static constexpr size_type kMaxOutputSize = 4096;
        VirtualTextArea text_buf(this->print_style_);
        // Pre allocate 16 lines text
        text_buf.reserve_lines(16);
        for (auto const & desc : this->desc_list_) {
            desc.print(os, text_buf);
            text_buf.reset();
        }

        if (this->print_style_.compact_style) {
            os << char_type('\n');
        }
        return *(const_cast<this_type *>(this));
    }

    this_type & printUsage() const {
        return this->printUsage(std::cout);
    }

    ParseResult parseArgs(int argc, char_type * argv[], bool strict = false) {
        ParseResult result;

        this->arg_list_.clear();
        this->arg_list_.push_back(argv[0]);
        this->param_list_.clear();

        this->app_name_ = this_type::getAppName(argv[0]);

        int i = 1;
        bool need_delay_assign = false;
        string_type last_arg;

        while (i < argc) {
            bool has_equal_sign = false;
            bool is_delay_assign = false;
            size_type start_pos, end_pos;

            string_type arg = argv[i];
            this->arg_list_.push_back(argv[i]);

            size_type token_pos = arg.find(char_type('='));
            if (token_pos != string_type::npos) {
                if (token_pos == 0) {
                    // Skip error format
                    result.add_error(Error::CmdLine_IllegalFormat, i, 0, 1);
                    i++;
                    continue;
                }
                assert(token_pos > 0);
                end_pos = token_pos;
                has_equal_sign = true;
            } else {
                end_pos = arg.size();
            }

            string_type arg_name, arg_value;
            const char_type * value_start = nullptr;
            int arg_name_type = 0;
            start_pos = 0;
            char head_char = arg[0];
            assert(head_char != char_type(' '));
            if (head_char == char_type('-')) {
                if (arg[1] == char_type('-')) {
                    // --name=abc
                    start_pos = 2;
                } else {
                    // -n=abc
                    start_pos = 1;
                }

                // Parse the arg name or value
                string_copy(arg_name, arg, start_pos, end_pos);
                if (arg_name.size() > 0) {
                    if (start_pos == 1) {
                        // -n=abc
                        if (arg_name.size() == 1) {
                            arg_name_type = 1;
                            // short prefix arg name format can be not contains "="
                            if (has_equal_sign) {
                                need_delay_assign = false;
                                last_arg = "";
                            } else {
                                need_delay_assign = true;
                                last_arg = arg_name;
                            }
                        } else {
                            if (strict) {
                                result.add_error(Error::CmdLine_ShortPrefixArgumentNameTooLong, i, 0, 1);
                            }
                        }
                    } else if (start_pos == 2) {
                        // --name=abc
                        if (arg_name.size() > 1) {
                            // long prefix arg name format
                            arg_name_type = 2;
                            need_delay_assign = false;
                            last_arg = "";
                        } else {
                            if (strict) {
                                result.add_error(Error::CmdLine_LongPrefixArgumentNameTooShort, i, 0, 1);
                            }
                        }
                    }

                    if ((arg_name_type > 0) && has_equal_sign) {
                        // Parse the arg value
                        value_start = &argv[i][0] + end_pos + 1;
                        string_copy(arg_value, arg, end_pos + 1, arg.size());
                    }
                } else {
                    if (strict) {
                        result.add_error(Error::CmdLine_EmptyArgumentName, i, 0, 1);
                    }
                }
            } else {
                // Maybe is a argument value.
            }

            if (arg_name_type == 0) {
                // The arg value equal full arg[i]
                value_start = &argv[i][0];
                arg_value = arg;
                if (need_delay_assign) {
                    assert(last_arg.size() == 1);
                    arg_name = last_arg;
                    arg_name_type = 1;
                    is_delay_assign = true;
                } else {
                    // Unrecognized argument
                    arg_name_type = -1;
                    if (strict) {
                        result.add_error(Error::CmdLine_UnrecognizedArgument, i, 0, 1);
                    }
                }

                need_delay_assign = false;
                last_arg = "";
            }

            if ((arg_name_type > 0) && (arg_name.size() > 0)) {
                bool exists = this->hasVar(arg_name);
                if (exists) {
                    Param & param = this->getParam(arg_name);
                    param.state.order = i;
                    param.state.visited = 1;
                    param.label = arg_name;
                    if (!has_equal_sign && !is_delay_assign) {
                        // If arg name no contains "=" and it's not delay assign.
                        if (param.state.is_default == 0) {
                            need_delay_assign = false;
                            last_arg = "";
                        }
                    } else {
#if 1
                        bool convertible = false;
                        try {
                            StringConverter<variant_t, char_type> stringConverter(arg_value, value_start);
                            param.value = ::jstd::apply_visitor(stringConverter, param.value);
                            convertible = true;
                        } catch(const std::invalid_argument & ex) {
                            std::cout << "std::invalid_argument::what(): " << ex.what() << '\n';
                        } catch(const std::out_of_range & ex) {
                            std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
                        } catch (const jstd::BadVariantAccess & ex) {
                            std::cout << "jstd::BadVariantAccess::what(): " << ex.what() << '\n';
                        }
                        if (convertible)
                            param.state.assigned = 1;
                        else
                            result.add_error(Error::CmdLine_CouldNotParseArgumentValue, i, 0, 1);
#else
                        bool convertible = Converter<char_type>::try_convert(param.value, arg_value, value_start);
                        if (convertible)
                            param.state.assigned = 1;
                        else
                            result.add_error(Error::CmdLine_CouldNotParseArgumentValue, i, 0, 1);
#endif
                    }
                } else {
                    need_delay_assign = false;
                    last_arg = "";
                    if (strict) {
                        // Unknown argument: It's a argument, but can't be found in variable map.
                        result.add_error(Error::CmdLine_UnknownArgument, i, 0, 1);
                    }
                }
            }

            if (result.any_errors())
                break;

            // Scan next argument
            i++;
        }
        return result;
    }
};

typedef BasicResult<char>                   Result;
typedef BasicResult<wchar_t>                ResultW;

typedef BasicParseResult<char>              ParseResult;
typedef BasicParseResult<wchar_t>           ParseResultW;

typedef BasicPrintStyle<char>               PrintStyle;
typedef BasicPrintStyle<wchar_t>            PrintStyleW;

typedef BasicCmdLine<char>::ParamState      ParamState;
typedef BasicCmdLine<wchar_t>::ParamState   ParamStateW;

typedef BasicCmdLine<char>::Param           Param;
typedef BasicCmdLine<wchar_t>::Param        ParamW;

typedef BasicCmdLine<char>::Option          Option;
typedef BasicCmdLine<wchar_t>::Option       OptionW;

typedef BasicCmdLine<char>::OptionDesc      OptionDesc;
typedef BasicCmdLine<wchar_t>::OptionDesc   OptionDescW;

typedef BasicConfig<char>       Config;
typedef BasicConfig<wchar_t>    ConfigW;

typedef BasicCmdLine<char>      CmdLine;
typedef BasicCmdLine<wchar_t>   CmdLineW;

} // namespace app

#endif // APP_CMD_LINE_H
