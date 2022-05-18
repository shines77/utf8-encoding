
#ifndef JSTD_CHAR_TRAITS_H
#define JSTD_CHAR_TRAITS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <type_traits>

namespace jstd {

template <typename T>
struct no_signed_char_trait {
    typedef T type;
};

template <>
struct no_signed_char_trait<unsigned char> {
    typedef char type;
};

template <>
struct no_signed_char_trait<signed char> {
    typedef char type;
};

template <>
struct no_signed_char_trait<unsigned short> {
    typedef short type;
};

template <>
struct no_signed_char_trait<signed short> {
    typedef short type;
};

template <>
struct no_signed_char_trait<wchar_t> {
    typedef wchar_t type;
};

template <>
struct no_signed_char_trait<char16_t> {
    typedef char16_t type;
};

template <>
struct no_signed_char_trait<char32_t> {
    typedef char32_t type;
};

template <>
struct no_signed_char_trait<unsigned int> {
    typedef int type;
};

template <>
struct no_signed_char_trait<signed int> {
    typedef int type;
};

template <>
struct no_signed_char_trait<unsigned long> {
    typedef long type;
};

template <>
struct no_signed_char_trait<signed long> {
    typedef long type;
};

template <>
struct no_signed_char_trait<unsigned long long> {
    typedef long long type;
};

template <>
struct no_signed_char_trait<signed long long> {
    typedef long long type;
};

template <typename T>
struct char_traits {
	static_assert(
		((std::is_integral<T>::value || std::is_enum<T>::value) &&
			!std::is_same<T, bool>::value),
        "jstd::char_trait<T> require that T shall be a (possibly "
		"cv-qualified) integral type or enumeration but not a bool type.");

    typedef typename std::remove_reference<
                typename std::remove_pointer<
                    typename std::remove_extent<
                        typename std::remove_cv<T>::type
                    >::type
                >::type
            >::type CharT;

    typedef typename std::make_unsigned<CharT>::type    Unsigned;
    typedef typename std::make_signed<CharT>::type      Signed;
    typedef typename no_signed_char_trait<CharT>::type  NoSigned;
};

} // namespace jstd

#endif // JSTD_CHAR_TRAITS_H
