
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

#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>

#include "jstd/char_traits.h"
#include "jstd/function_traits.h"
#include "jstd/apply_visitor.h"
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
                ++rtrim;

            return (ltrim >= rtrim);
        } else {
            return true;
        }
    }
}

template <typename CharT = char>
static inline
std::size_t find_first_not_of(const std::basic_string<CharT> & str, CharT token,
                              std::size_t first, std::size_t last)
{
    std::size_t cur = first;
    while (cur < last) {
        if (str[cur] == token)
            ++cur;
        else
            return cur;
    }
    return std::basic_string<CharT>::npos;
}

template <typename CharT = char>
static inline
std::size_t find_last_not_of(const std::basic_string<CharT> & str, CharT token,
                             std::size_t first, std::size_t last)
{
    std::size_t cur = --last;
    while (cur >= first) {
        if (str[cur] == token)
            --cur;
        else
            return (cur + 1);
    }
    return std::basic_string<CharT>::npos;
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

    first = ltrim;
    last = rtrim;
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

    std::size_t size() const  {
        return (last > first) ? (last - first - 1) : std::size_t(0);
    }

    bool is_last(std::size_t i) const {
        return ((last > 0) && (i >= (last - 1)));
    }
};

struct SliceEx {
    std::size_t first;
    std::size_t last;
    std::size_t token;
    std::size_t count;

    SliceEx() noexcept : first(0), last(0), token(0), count(0) {
    }

    SliceEx(std::size_t _first, std::size_t _last) noexcept
        : first(_first), last(_last), token(0), count(0) {
    }

    SliceEx(const SliceEx & src) noexcept
        : first(src.first), last(src.last),
          token(src.token), count(src.count) {
    }

    std::size_t size() const {
        return (last > first) ? (last - first - 1) : std::size_t(0);
    }

    bool is_last(std::size_t i) const {
        return ((last > 0) && (i >= (last - 1)));
    }
};

template <typename CharT = char>
static inline
void split_string_by_token(const std::basic_string<CharT> & text, CharT token,
                           std::vector<Slice> & word_list)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find_first_of(token, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        string_trim<CharT>(text, ltrim, rtrim);

        if (ltrim < rtrim) {
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
void split_string_by_token(const std::basic_string<CharT> & text, CharT token,
                           std::vector<std::basic_string<CharT>> & word_list)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find_first_of(token, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        string_trim<CharT>(text, ltrim, rtrim);

        if (ltrim < rtrim) {
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
void split_string_by_token(const std::basic_string<CharT> & text,
                           const std::basic_string<CharT> & tokens,
                           std::vector<std::basic_string<CharT>> & word_list)
{
    word_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = text.find_first_of(tokens, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = text.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        string_trim<CharT>(text, ltrim, rtrim);

        if (ltrim < rtrim) {
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
        Void,
        String,
    };
};

template <typename CharT = char>
struct PrintStyle {
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef std::basic_string<char_type>                    string_type;

    typedef std::size_t size_type;

    bool compact_style;
    bool auto_fit;
    size_type tab_size;
    size_type ident_spaces;
    size_type min_padding;
    size_type max_start_pos;
    size_type max_column;

    string_type separator;

    PrintStyle() : compact_style(true), auto_fit(false),
                   tab_size(4), ident_spaces(2), min_padding(1),
                   max_start_pos(35), max_column(80) {
        this->separator.push_back(char_type(':'));
    }
};

template <typename CharT = char>
class VirtualTextArea {
    typedef VirtualTextArea<CharT>                          this_type;
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type>                    string_type;

    typedef std::size_t size_type;

    static const size_type kMaxReservedSize = 4096;

public:
    VirtualTextArea(const PrintStyle<char_type> & print_style)
        : print_style_(print_style), lines_(0) {
    }

protected:
    const PrintStyle<char_type> & print_style_;
    string_type text_buf_;
    size_type lines_;

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

     PrintStyle<char_type> & print_style() {
        return const_cast<PrintStyle<char_type> &>(this->print_style_);
    }

    const PrintStyle<char_type> & print_style() const {
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
        split_string_by_token(text, char_type(' '), word_list);

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

    string_type output_real() {
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

    typedef std::basic_string<char_type>                    string_type;

    typedef std::size_t size_type;
    typedef Variant     variant_t;

#if defined(_MSC_VER)
    static const char_type kPathSeparator = char_type('\\');
#else
    static const char_type kPathSeparator = char_type('/');
#endif

    union VariableState {
        struct {
            std::uint32_t   order;
            std::uint32_t   required   : 1;
            std::uint32_t   is_default : 1;
            std::uint32_t   visited    : 1;
            std::uint32_t   assigned   : 1;
            std::uint32_t   unused     : 28;
        };
        uint64_t value;

        VariableState() : value(0) {
        }
    };

    struct Variable {
        VariableState state;
        string_type   name;
        variant_t     value;
    };

    struct Option {
        std::uint32_t   type;
        std::uint32_t   desc_id;
        string_type     names;
        string_type     display_text;
        string_type     desc;
        Variable        variable;

        static const size_type NotFound       = (size_type)-1;
        static const std::uint32_t NotFound32 = (std::uint32_t)-1;
        static const std::uint16_t NotFound16 = (std::uint16_t)-1;

        static const size_type Unknown       = (size_type)-1;
        static const std::uint32_t Unknown32 = (std::uint32_t)-1;

        Option(std::uint32_t _type = OptType::Unknown)
            : type(_type), desc_id(Unknown32) {
        }
    };

    class OptionDesc {
    public:
        string_type title;

        std::vector<Option>                        option_list;
        std::unordered_map<string_type, size_type> option_map;

        OptionDesc() {
        }

        OptionDesc(const string_type & _title) : title(_title) {
        }

    private:
        size_type parseOptionName(const string_type & names,
                                  std::vector<string_type> & name_list,
                                  std::vector<string_type> & text_list) {
            std::vector<string_type> token_list;
            string_type tokens = _Text(", ");
            split_string_by_token(names, tokens, token_list);
            size_type nums_token = token_list.size();
            if (nums_token > 0) {
                name_list.clear();
                text_list.clear();
                for (auto iter = token_list.begin(); iter != token_list.end(); ++iter) {
                    const string_type & token = *iter;
                    string_type name;
                    size_type pos = token.find(_Text("--"));
                    if (pos == 0) {
                        for (size_type i = pos + 2; i < token.size(); i++) {
                            name.push_back(token[i]);
                        }
                        if (!is_empty_or_null(name)) {
                            name_list.push_back(name);
                        }
                    } else {
                        pos = token.find(char_type('-'));
                        if (pos == 0) {
                            for (size_type i = pos + 1; i < token.size(); i++) {
                                name.push_back(token[i]);
                            }
                            if (!is_empty_or_null(name)) {
                                name_list.push_back(name);
                            }
                        } else {
                            assert(!is_empty_or_null(token));
                            text_list.push_back(token);
                        }
                    }
                }
            }
            return name_list.size();
        }

        // Accept format: --name=abc, or -n=10, or -n 10
        int addOptionImpl(std::uint32_t type, const string_type & names,
                          const string_type & desc,
                          const variant_t & value,
                          bool is_default) {
            int err_code = Error::NoError;

            Option option(type);
            option.names = names;
            option.display_text = names;
            option.desc = desc;
            option.variable.state.is_default = is_default;
            option.variable.value = value;

            std::vector<string_type> name_list;
            std::vector<string_type> text_list;
            size_type nums_name = this->parseOptionName(names, name_list, text_list);
            if (nums_name > 0) {
                // Sort all the option names asc
                for (size_type i = 0; i < (name_list.size() - 1); i++) {
                    for (size_type j = i + 1; j < name_list.size(); j++) {
                        if (name_list[i].size() > name_list[j].size()) {
                            std::swap(name_list[i], name_list[j]);
                        } else if (name_list[i].size() == name_list[j].size()) {
                            if (name_list[i] > name_list[j])
                                std::swap(name_list[i], name_list[j]);
                        }
                    }
                }

                // Format short name text
                string_type s_names;
                for (size_type i = 0; i < name_list.size(); i++) {
                    s_names += name_list[i];
                    if ((i + 1) < name_list.size())
                        s_names += _Text(",");
                }
                option.names = s_names;

                // Format display name text
                string_type display_text;
                for (size_type i = 0; i < name_list.size(); i++) {
                    const string_type & name = name_list[i];
                    if (name.size() == 1)
                        display_text += _Text("-");
                    else if (name.size() > 1)
                        display_text += _Text("--");
                    else
                        continue;
                    display_text += name;
                    if ((i + 1) < name_list.size())
                        display_text += _Text(", ");
                }
                if (text_list.size() > 0) {
                    display_text += _Text(" ");
                }
                for (size_type i = 0; i < text_list.size(); i++) {
                    display_text += text_list[i];
                    if ((i + 1) < text_list.size())
                        display_text += _Text(" ");
                }
                option.display_text = display_text;

                size_type option_id = this->option_list.size();
                this->option_list.push_back(option);

                for (auto iter = name_list.begin(); iter != name_list.end(); ++iter) {
                    const string_type & name = *iter;
                    // Only the first option with the same name is valid.
                    if (this->option_map.count(name) == 0) {
                        this->option_map.insert(std::make_pair(name, option_id));
                    } else {
                        // Warning
                        printf("Warning: desc: \"%s\", option_id: %u, arg_name = \"%s\" already exists.\n\n",
                               this->title.c_str(), (uint32_t)option_id, name.c_str());
                    }
                }
            }
            return (err_code == Error::NoError) ? (int)nums_name : err_code;
        }

        void print_compact_style(VirtualTextArea<char_type> & text_buf) const {
            const PrintStyle<char_type> & ps = text_buf.print_style();
            std::vector<string_type> line_list;
            if (!is_empty_or_null(this->title)) {
                string_type title_text;
                text_buf.prepare_display_text(this->title,
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
                                                          ps.max_column - ps.max_start_pos);
                            if ((column + ps.ident_spaces) > (ps.max_start_pos - ps.min_padding)) {
                                lines++;
                            }
                            size_type total_lines = lines + line_list.size();
                            text_buf.append_text(ps.max_start_pos, lines, total_lines, line_list);
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

            string_type out_text = text_buf.output_real();
            std::cout << out_text;
        }

        void print_relaxed_style() const {
            if (!is_empty_or_null(this->title)) {
                printf("%s:\n\n", this->title.c_str());
            }

            for (auto iter = this->option_list.begin(); iter != this->option_list.end(); ++iter) {
                const Option & option = *iter;
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

        int addOption(const string_type & names, const string_type & desc, const variant_t & value) {
            return this->addOptionImpl(OptType::String, names, desc, value, true);
        }

        int addOption(const string_type & names, const string_type & desc) {
            Variant default_value(size_type(0));
            return this->addOptionImpl(OptType::Void, names, desc, default_value, false);
        }

        void print(VirtualTextArea<char_type> & text_buf) const {
            if (text_buf.print_style().compact_style) {
                this->print_compact_style(text_buf);
            } else {
                this->print_relaxed_style();
            }
        }
    }; // class OptionsDescription

protected:
    std::vector<OptionDesc>                         option_desc_list_;
    std::vector<Option>                             option_list_;
    std::unordered_map<string_type, size_type>    option_map_;

    std::vector<string_type> arg_list_;

    string_type app_name_;
    string_type display_name_;
    string_type version_;

    PrintStyle<char_type> print_style_;

    Variable    empty_variable_;

public:
    BasicCmdLine() : print_style_() {
        this->version_ = _Text("1.0.0");
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

    static string_type getAppName(char_type * argv[]) {
        string_type strModuleName = argv[0];
        size_type lastSepPos = strModuleName.find_last_of(kPathSeparator);
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

    size_type argn() const {
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

    void setDisplayName(const string_type & display_name) {
        this->display_name_ = display_name;
    }

    void setVersion(const string_type & version) {
        this->version_ = version;
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
        return this->print_style_.max_start_pos;
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

    void setCompactStyle(bool compact_style) {
        this->print_style_.compact_style = compact_style;
    }

    void setAutoFit(bool auto_fit) {
        this->print_style_.auto_fit = auto_fit;
    }

    void setMinPadding(size_type min_padding) {
        this->print_style_.min_padding = min_padding;
    }

    void setMaxStartPos(size_type start_pos) {
        this->print_style_.max_start_pos = start_pos;
    }

    void setMaxColumn(size_type max_column) {
        this->print_style_.max_column = max_column;
    }

    void setSeparator(const string_type & separator) {
        this->print_style_.separator = separator;
    }

    size_type order(const string_type & name) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (size_type)option->variable.state.order;
        } else {
            return Option::NotFound;
        }
    }

    bool isRequired(const string_type & name) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (option->variable.state.required != 0);
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
            return (option->variable.state.visited != 0);
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
            return (option->variable.state.assigned != 0);
        } else {
            return false;
        }
    }

    bool assigned(const string_type & name) const {
        return this->isAssigned(name);
    }

    bool getVariableState(const string_type & name, VariableState & variable_state) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            variable_state = option->variable.state;
            return true;
        } else {
            return false;
        }
    }

    Variable & getVariable(const string_type & name) {
        Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return option->variable;
        } else {
            return this->empty_variable_;
        }
    }

    const Variable & getVariable(const string_type & name) const {
        Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return option->variable;
        } else {
            return this->empty_variable_;
        }
    }

    bool hasVar(const string_type & name) const {
        return this->hasOption(name);
    }

    bool getVar(const string_type & name, Variable & variable) const {
        const Option * option = nullptr;
        size_type option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            variable = option->variable;
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
            value = option->variable.value;
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
            value = option->variable.value.template get<T>();
            return true;
        } else {
            return false;
        }
    }

    void setVar(const string_type & name, const Variant & value) {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            size_type option_id = iter->second;
            if (option_id < this->option_list_.size()) {
                Option & option = this->getOptionById(option_id);
                option.variable.value = value;
            }
        }
    }

    template <typename T>
    void setVar(const string_type & name, const T & value) {
        this->setVar(name, value);
    }

    size_type addDesc(const OptionDesc & desc) {
        size_type desc_id = this->option_desc_list_.size();
        this->option_desc_list_.push_back(desc);
        size_type index = 0;
        for (auto iter = desc.option_list.begin(); iter != desc.option_list.end(); ++iter) {
            const Option & option = *iter;
            size_type old_option_id = index;
            std::vector<string_type> arg_names;
            desc.find_all_name(old_option_id, arg_names);
            if (arg_names.size() != 0) {
                size_type new_option_id = this->option_list_.size();
                this->option_list_.push_back(option);
                // Actual desc id
                this->option_list_[new_option_id].desc_id = (std::uint32_t)desc_id;
                for (size_type i = 0; i < arg_names.size(); i++) {
                    if (this->option_map_.count(arg_names[i]) == 0) {
                        this->option_map_.insert(std::make_pair(arg_names[i], new_option_id));
                    } else {
                        printf("Warning: desc_id: %u, desc: \"%s\", option_id: %u, arg_name = \"%s\" already exists.\n\n",
                               (uint32_t)desc_id, desc.title.c_str(), (uint32_t)new_option_id, arg_names[i].c_str());
                    }
                };
            }
            index++;
        }
        return desc_id;
    }

    void printVersion() const {
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
    }

    void printUsage() const {
        static const size_type kMaxOutputSize = 4096;
        VirtualTextArea<char_type> text_buf(this->print_style_);
        // Pre allocate 16 lines text
        text_buf.reserve_lines(16);
        for (auto iter = this->option_desc_list_.begin();
             iter != this->option_desc_list_.end(); ++iter) {
            const OptionDesc & desc = *iter;
            desc.print(text_buf);
            text_buf.reset();
        }

        if (this->print_style_.compact_style) {
            std::cout << std::endl;
        }
    }

    int parseArgs(int argc, char_type * argv[], bool strict = false) {
        int err_code = Error::NoError;

        this->arg_list_.clear();
        this->arg_list_.push_back(argv[0]);

        this->app_name_ == this_type::getAppName(argv);

        int i = 1;
        bool need_delay_assign = false;
        string_type last_arg;

        while (i < argc) {
            bool has_equal_sign = false;
            bool is_delay_assign = false;
            size_type start_pos, end_pos;

            string_type arg = argv[i];
            this->arg_list_.push_back(argv[i]);

            size_type separator_pos = arg.find(char_type('='));
            if (separator_pos != string_type::npos) {
                if (separator_pos == 0) {
                    // Skip error format
                    err_code = Error::CmdLine_IllegalFormat;
                    i++;
                    continue;
                }
                assert(separator_pos > 0);
                end_pos = separator_pos;
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
                                err_code = Error::CmdLine_ShortPrefixArgumentNameTooLong;
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
                                err_code = Error::CmdLine_LongPrefixArgumentNameTooShort;
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
                        err_code = Error::CmdLine_EmptyArgumentName;
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
                        err_code = Error::CmdLine_UnrecognizedArgument;
                    }
                }

                need_delay_assign = false;
                last_arg = "";
            }

            if ((arg_name_type > 0) && (arg_name.size() > 0)) {
                bool exists = this->hasVar(arg_name);
                if (exists) {
                    Variable & variable = this->getVariable(arg_name);
                    variable.state.order = i;
                    variable.state.visited = 1;
                    variable.name = arg_name;
                    if (!has_equal_sign && !is_delay_assign) {
                        // If arg name no contains "=" and it's not delay assign.
                        if (variable.state.is_default == 0) {
                            need_delay_assign = false;
                            last_arg = "";
                        }
                    } else {
#if 1
                        bool convertible = false;
                        try {
                            StringConverter<variant_t, char_type> stringConverter(arg_value, value_start);
                            variable.value = ::jstd::apply_visitor(stringConverter, variable.value);
                            convertible = true;
                        } catch(const std::invalid_argument & ex) {
                            std::cout << "std::invalid_argument::what(): " << ex.what() << '\n';
                        } catch(const std::out_of_range & ex) {
                            std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
                        } catch (const jstd::BadVariantAccess & ex) {
                            std::cout << "jstd::BadVariantAccess::what(): " << ex.what() << '\n';
                        }
                        if (convertible)
                            variable.state.assigned = 1;
                        else
                            err_code = Error::CmdLine_CouldNotParseArgumentValue;
#else
                        bool convertible = Converter<char_type>::try_convert(variable.value, arg_value, value_start);
                        if (convertible)
                            variable.state.assigned = 1;
                        else
                            err_code = Error::CmdLine_CouldNotParseArgumentValue;
#endif
                    }
                } else {
                    need_delay_assign = false;
                    last_arg = "";
                    if (strict) {
                        // Unknown argument: It's a argument, but can't be found in variable map.
                        err_code = Error::CmdLine_UnknownArgument;
                    }
                }
            }

            if (err_code != Error::NoError)
                break;

            // Scan next argument
            i++;
        }
        return err_code;
    }
};

typedef BasicConfig<char>       Config;
typedef BasicCmdLine<char>      CmdLine;

typedef BasicConfig<wchar_t>    ConfigW;
typedef BasicCmdLine<wchar_t>   CmdLineW;

} // namespace app

#endif // APP_CMD_LINE_H
