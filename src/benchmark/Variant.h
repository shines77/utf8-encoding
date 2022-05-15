
#ifndef JSTD_VARIANT_H
#define JSTD_VARIANT_H

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

#include <iostream>
#include <memory>
#include <functional>
#include <tuple>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <exception>
#include <stdexcept>

//
// See: https://www.cnblogs.com/qicosmos/p/3559424.html
// See: https://www.jianshu.com/p/f16181f6b18d
// See: https://en.cppreference.com/w/cpp/utility/variant
// See: https://github.com/mpark/variant/blob/master/include/mpark/variant.hpp
//

//
// C++ 17 std::variant
//
// https://zhuanlan.zhihu.com/p/498648144
//

/*****************************************************************************

#include <cxxabi.h>

// Using abi demangle to print nice type name of instance of any holding.
template <typename T>
static void printType(const T & v) {
    int status;
    if (char * p = abi::__cxa_demangle(typeid(v).name(), 0, 0, &status)) {
        std::cout << p << '\n';
        std::free(p);
    }
}

******************************************************************************/

namespace jstd {

// std::variant_npos
static constexpr std::size_t VariantNPos = (std::size_t)-1;

// Like std::monostate
struct MonoState {
#if 1
    MonoState(void) noexcept {}
    //MonoState(const MonoState & src) noexcept {}

    //template <typename T>
    //MonoState(const T & src) noexcept {}

    //~MonoState() {}
#endif
};

#if 1
bool operator == (MonoState, MonoState) noexcept { return true;  }
bool operator != (MonoState, MonoState) noexcept { return false; }
bool operator  < (MonoState, MonoState) noexcept { return false; }
bool operator  > (MonoState, MonoState) noexcept { return false; }
bool operator <= (MonoState, MonoState) noexcept { return true;  }
bool operator >= (MonoState, MonoState) noexcept { return true;  }
#else
constexpr bool operator == (MonoState, MonoState) noexcept { return true;  }
constexpr bool operator != (MonoState, MonoState) noexcept { return false; }
constexpr bool operator  < (MonoState, MonoState) noexcept { return false; }
constexpr bool operator  > (MonoState, MonoState) noexcept { return false; }
constexpr bool operator <= (MonoState, MonoState) noexcept { return true;  }
constexpr bool operator >= (MonoState, MonoState) noexcept { return true;  }
#endif

// std::bad_variant_access
struct BadVariantAccess : public std::runtime_error {
    BadVariantAccess(const char * message = "Exception: Bad Variant<Types...> access") throw()
        : std::runtime_error(message) {
    }
    ~BadVariantAccess() noexcept {}
};

template <typename... Ts>
struct make_void {
    typedef void type;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <std::size_t I, typename... Args>
struct tuple_element_helper {
    static constexpr std::size_t arity = sizeof...(Args);

    static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
    using type = typename std::conditional<(arity == 0), MonoState,
                 typename std::tuple_element<I + ((arity != 0) ? 1 : 0), std::tuple<MonoState, Args...>>::type>::type;
};

//
// Specializing a template on a lambda in C++0x
// See: https://stackoverflow.com/questions/2562320/specializing-a-template-on-a-lambda-in-c0x
//
// See: https://www.cnblogs.com/qicosmos/p/4772328.html
//
// See: https://stackoverflow.com/questions/34283919/understanding-how-the-function-traits-template-works-in-particular-what-is-the
//
// C++ 11 Function Traits
// See: https://functionalcpp.wordpress.com/2013/08/05/function-traits/
//
template <typename Functor>
struct function_traits;

////////////////////////////////////////////////////////////////////////////

#if 0
#if 0
// For generic types, directly use the result of the signature of its 'operator()'
template <typename Functor>
struct function_traits : public function_traits<decltype(&Functor::operator ())> {
    // Arity is the number of arguments.
    static constexpr std::size_t arity = 0;

    typedef decltype(&Functor::operator ()) result_type;

    template <std::size_t I>
    struct arguments {
        typedef void type;
    };
};
#endif

#if 0
// We specialize for pointers to member function
template <typename Functor, typename ReturnType, typename... Args>
struct function_traits<ReturnType(Functor::*)(Args...) const> {
    // Arity is the number of arguments.
    static constexpr std::size_t arity = sizeof...(Args);

    typedef ReturnType result_type;

    template <std::size_t I>
    struct arguments {
        // The i-th argument is equivalent to the i-th tuple element of a tuple
        // composed of those arguments.
        static_assert((I < arity), "Error: function_traits<ReturnType(T::*)(Args...) const>: invalid parameter index.");
        using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    };

    typedef std::function<ReturnType(Args...)> func_type;
    typedef std::tuple<Args...> args_type;
};
#endif
#endif

///////////////////////////////////////////////////////////////////////////////////////

// Normal function
template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(Args...)> {
    // Arity is the number of arguments.
    static constexpr std::size_t arity = sizeof...(Args);

    typedef void                functor_type;
    typedef ReturnType          result_type;
    typedef std::tuple<Args...> args_type;

    typedef ReturnType(*type)(Args...);

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity) || (arity == 0), "Error: function_traits<ReturnType(Args...)>: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(Args...)> func_type;
};

// Function pointer
template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)> {
    // Arity is the number of arguments.
    static constexpr std::size_t arity = sizeof...(Args);

    typedef void                functor_type;
    typedef ReturnType          result_type;
    typedef std::tuple<Args...> args_type;

    typedef ReturnType(*type)(Args...);

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity) || (arity == 0), "Error: function_traits<ReturnType(*)(Args...)>: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(Args...)> func_type;
};

#if 0
template <typename Functor>
struct function_traits : public function_traits<decltype(&Functor::operator ())> {
    template <typename Functor, typename = void>
    struct __internal;

    template <typename Functor, typename ReturnType, typename... Args>
    struct __internal {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef MonoState           functor_type;
        typedef ReturnType          result_type;
        typedef std::tuple<Args...> args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename tuple_element_helper<I, Args...>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(Args...)> func_type;
    };

    template <typename Functor, typename ReturnType, typename... Args>
    struct __internal<ReturnType(Functor::*)(Args...)> {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef Functor             functor_type;
        typedef ReturnType          result_type;
        typedef std::tuple<Args...> args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename tuple_element_helper<I, Args...>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(Functor &, Args...)> func_type;
    };

    template <typename Functor, typename ReturnType, typename... Args>
    struct __internal<ReturnType (Functor::*)(Args...) const> {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef Functor             functor_type;
        typedef ReturnType          result_type;
        typedef std::tuple<Args...> args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename tuple_element_helper<I, Args...>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(const Functor &, Args...)> func_type;
    };

    template <typename Functor>
    struct __internal<Functor, void_t<decltype(&Functor::operator ())>>
        : public __internal<decltype(&Functor::operator ())> {
    };

    template <typename Functor>
    static constexpr std::size_t arity = __internal<decltype(&Functor::operator ())>::arity;

    template <typename Functor>
    using functor_type  = typename __internal<decltype(&Functor::operator ())>::functor_type;
    template <typename Functor>
    using result_type = typename __internal<decltype(&Functor::operator ())>::result_type;
    template <typename Functor>
    using args_type = typename __internal<decltype(&Functor::operator ())>::args_type;

    template <typename Functor>
    using arg0 = typename __internal<decltype(&Functor::operator ())>::arg0;
};
#endif

#if 0
template <typename Functor>
struct function_traits : public function_traits<decltype(&Functor::operator ())> {
    template <typename ReturnType, typename... Args>
    struct __internal {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef MonoState               functor_type;
        typedef ReturnType              result_type;
        typedef std::tuple<Args...>     args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename tuple_element_helper<I, Args...>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(Args...)> func_type;
    };

    template <typename ReturnType, typename... Args>
    struct __internal<ReturnType (Functor::*)(Args...)> {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef Functor             functor_type;
        typedef ReturnType          result_type;
        typedef std::tuple<Args...> args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename tuple_element_helper<I, Args...>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(Functor &, Args...)> func_type;
    };

    template <typename ReturnType, typename... Args>
    struct __internal<ReturnType (Functor::*)(Args...) const> {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef Functor             functor_type;
        typedef ReturnType          result_type;
        typedef std::tuple<Args...> args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename tuple_element_helper<I, Args...>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(const Functor &, Args...)> func_type;
    };

    static constexpr std::size_t arity = __internal<decltype(&Functor::operator ())>::arity;

    using functor_type  = typename __internal<decltype(&Functor::operator ())>::functor_type;
    using result_type = typename __internal<decltype(&Functor::operator ())>::result_type;
    using args_type = typename __internal<decltype(&Functor::operator ())>::args_type;

    using arg0 = typename __internal<decltype(&Functor::operator ())>::arg0;
};
#endif

// We specialize for member function pointer
template <typename ReturnType, typename Functor, typename... Args>
struct function_traits<ReturnType(Functor::*)(Args...)>
    /* : public function_traits<ReturnType(T &, Args...)> */ {
    // Arity is the number of arguments.
    static constexpr std::size_t arity = sizeof...(Args);

    typedef Functor             functor_type;
    typedef ReturnType          result_type;
    typedef std::tuple<Args...> args_type;

    typedef decltype(&Functor::operator ()) type;

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity), "Error: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(Functor &, Args...)> func_type;
};
 
// We specialize for const member function pointer
template <typename ReturnType, typename Functor, typename... Args>
struct function_traits<ReturnType(Functor::*)(Args...) const>
    /* : public function_traits<ReturnType(const T &, Args...)> */ {
    // Arity is the number of arguments.
    static constexpr std::size_t arity = sizeof...(Args);

    typedef Functor             functor_type;
    typedef ReturnType          result_type;
    typedef std::tuple<Args...> args_type;

    typedef decltype(&Functor::operator ()) const type;

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity), "Error: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(const Functor &, Args...)> func_type;
};
 
// We specialize for member object pointer
template <typename ReturnType, typename Functor>
struct function_traits<ReturnType(Functor::*)>
    /* : public function_traits<ReturnType(T &)> */ {
    static constexpr std::size_t arity = 0;

    typedef Functor      functor_type;
    typedef ReturnType   result_type;
    typedef std::tuple<> args_type;

    typedef typename Functor::ReturnType * type;

    template <std::size_t I>
    struct arguments {
        using type = void;
    };

    typedef MonoState arg0;
    typedef std::function<ReturnType(Functor &)> func_type;
};

// std::function<T>
template <typename ReturnType, typename... Args>
struct function_traits<std::function<ReturnType(Args...)>>
    : public function_traits<ReturnType(Args...)> {
};

#if 0
template <typename Functor>
struct function_traits {
    template <typename ReturnType, typename... Args>
    struct __internal {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef MonoState               functor_type;
        typedef ReturnType              result_type;
        typedef std::tuple<Args...>     args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename std::conditional<(arity == 0), void,
                         typename std::tuple_element<I + ((arity != 0) ? 1 : 0), std::tuple<void, Args...>>::type>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(Args...)> func_type;
    };

    template <typename ReturnType, typename... Args>
    struct __internal<ReturnType (Functor::*)(Args...)> {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef Functor             functor_type;
        typedef ReturnType          result_type;
        typedef std::tuple<Args...> args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename std::conditional<(arity == 0), void,
                         typename std::tuple_element<I + ((arity != 0) ? 1 : 0), std::tuple<void, Args...>>::type>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(Functor &, Args...)> func_type;
    };

    template <typename ReturnType, typename... Args>
    struct __internal<ReturnType (Functor::*)(Args...) const> {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef Functor             functor_type;
        typedef ReturnType          result_type;
        typedef std::tuple<Args...> args_type;

        template <std::size_t I>
        struct arguments {
            static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
            using type = typename std::conditional<(arity == 0), void,
                         typename std::tuple_element<I, std::tuple<Args...>>::type>::type;
        };

        typedef typename tuple_element_helper<0, Args...>::type arg0;
        typedef std::function<ReturnType(const Functor &, Args...)> func_type;
    };

    static constexpr std::size_t arity = __internal<decltype(&Functor::operator ())>::arity;

    using functor_type  = typename __internal<decltype(&Functor::operator ())>::functor_type;
    using result_type = typename __internal<decltype(&Functor::operator ())>::result_type;
    using args_type = typename __internal<decltype(&Functor::operator ())>::args_type;

    using arg0 = typename __internal<decltype(&Functor::operator ()) const>::arg0;
};
#endif

template <typename Functor>
struct function_traits : public function_traits<decltype(&Functor::operator ())> {
};

#if 0
template <typename Functor>
struct function_traits<Functor, void_t<decltype(&Functor::operator ())>>
    : public function_traits<decltype(&Functor::operator ())> {
};
#endif

#if 0
template <typename Functor>
struct function_traits {
private:
    using this_type = function_traits<Functor>;
    //using call_type = function_traits<decltype(&(typename Func::type)::operator())>;

public:
    static constexpr std::size_t arity = this_type::arity - 1;

    using result_type = typename this_type::result_type;
    using args_type = typename this_type::args_type;

    template <std::size_t I>
    struct arguments {
        static_assert(I < arity, "error: invalid parameter index.");
        using type = typename std::tuple_element<I, args_type>::type;
    };

    typedef MonoState arg0;
    typedef typename this_type::func_type func_type;
};
#endif

template <typename Functor>
struct function_traits<Functor &> : public function_traits<Functor> {};
 
template <typename Functor>
struct function_traits<Functor &&> : public function_traits<Functor> {};

//////////////////////////////////////////////////////////////////////////////////////

template <std::size_t Val, std::size_t... Rest>
struct MaxInteger;

template <std::size_t Val>
struct MaxInteger<Val> : std::integral_constant<std::size_t, Val> {
};

template <std::size_t Val1, std::size_t Val2, std::size_t... Rest>
struct MaxInteger<Val1, Val2, Rest...>
    : std::integral_constant<std::size_t, ((Val1 >= Val2) ?
                                           MaxInteger<Val1, Rest...>::value :
                                           MaxInteger<Val2, Rest...>::value) > {
    /*
    static const std::size_t value = ((Val1 >= Val2) ? static_max<Val1, rest...>::value :
                                      static_max<Val2, rest...>::value);
    //*/
};

template <typename... Types>
struct MaxAlign : std::integral_constant<std::size_t, MaxInteger<std::alignment_of<Types>::value...>::value> {
};

/*
template <typename T, typename... Types>
struct MaxAlign : std::integral_constant<std::size_t,
    ((std::alignment_of<T>::value > MaxAlign<Types...>::value) ?
      std::alignment_of<T>::value : MaxAlign<Types...>::value)> {
};

template <typename T>
struct MaxAlign<T> : std::integral_constant<std::size_t, std::alignment_of<T>::value> {
};
//*/

template <typename T>
struct IsArray : std::is_array<T> {
};

template <typename T, std::size_t N>
struct IsArray<T[N]> : std::true_type {
};

template <typename T>
struct IsArray<T[]> : std::true_type {
};

template <typename T, std::size_t N>
struct IsArray<const T [N]> : std::true_type {
};

template <typename T>
struct IsArray<const T []> : std::true_type {
};

template <typename T, typename Base>
struct IsSame : std::is_same<T, Base> {
};

template <std::size_t N>
struct IsSame<char[N], char *> : std::true_type {
};

template <std::size_t N>
struct IsSame<const char[N], const char *> : std::true_type {
};

template <std::size_t N>
struct IsSame<wchar_t[N], wchar_t *> : std::true_type {
};

template <std::size_t N>
struct IsSame<const wchar_t[N], const wchar_t *> : std::true_type {
};

template <typename T, typename... Types>
struct ContainsType : std::true_type {
};

template <typename T, typename Head, typename... Types>
struct ContainsType<T, Head, Types...>
    : std::conditional<std::is_same<T, Head>::value, std::true_type, ContainsType<T, Types...>>::type {
};

template <typename T>
struct ContainsType<T> : std::false_type {
};

// Forward declaration
template <typename T, typename... Types>
struct GetLeftSize;

// Declaration
template <typename T, typename First, typename... Types>
struct GetLeftSize<T, First, Types...> : GetLeftSize<T, Types...> {
};

// Specialized
template <typename T, typename... Types>
struct GetLeftSize<T, T, Types...> : std::integral_constant<std::size_t, sizeof...(Types)> {
};

template <typename T>
struct GetLeftSize<T> : std::integral_constant<std::size_t, 0> {
};

template <typename T, typename ...Types>
struct GetIndex : std::integral_constant<std::size_t, sizeof...(Types) - GetLeftSize<T, Types...>::value - 1> {
};

// Forward declaration
template <typename... Types>
class Variant;

// std::variant_size
template <typename T>
struct VariantSize;

template <typename T>
struct VariantSize<const T> : VariantSize<T> {
};

template <typename T>
struct VariantSize<volatile T> : VariantSize<T> {
};

template <typename T>
struct VariantSize<const volatile T> : VariantSize<T> {
};

// std::variant_size
template <typename... Types>
struct VariantSize<Variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {
};

// Variable templates: require C++ 14 or MSVC 2015 update 3 and higher
#if defined(__cpp_variable_templates) || (defined(_MSC_VER) && (_MSC_FULL_VER >= 190024210))
template <typename T>
static constexpr std::size_t VariantSize_v = VariantSize<T>::value;
#endif

template <typename T>
using VariantSize_t = typename VariantSize<T>::type;

// TypeIndexOf<T, I, ...>

template <typename T, std::size_t I, typename... Types>
struct TypeIndexOf {
};

template <typename T, std::size_t I, typename First, typename... Types>
struct TypeIndexOf<T, I, First, Types...> {
    static constexpr std::size_t value = TypeIndexOf<T, I + 1, Types...>::value;
};

template <typename T, std::size_t I, typename... Types>
struct TypeIndexOf<T, I, T, Types...> {
    static constexpr std::size_t value = I;
};

#if 1
template <std::size_t N, std::size_t I, typename... Types>
struct TypeIndexOf<char[N], I, char *, Types...> {
    static constexpr std::size_t value = I;
};

template <std::size_t N, std::size_t I, typename... Types>
struct TypeIndexOf<wchar_t[N], I, wchar_t *, Types...> {
    static constexpr std::size_t value = I;
};
#endif

#if 1
template <std::size_t N, std::size_t I, typename... Types>
struct TypeIndexOf<const char[N], I, const char *, Types...> {
    static constexpr std::size_t value = I;
};

template <std::size_t N, std::size_t I, typename... Types>
struct TypeIndexOf<const wchar_t[N], I, const wchar_t *, Types...> {
    static constexpr std::size_t value = I;
};
#endif

template <typename T, std::size_t I>
struct TypeIndexOf<T, I> {
    static constexpr std::size_t value = VariantNPos;
};

// std::variant_alternative

// Forward declaration
template <std::size_t I, typename... Types>
struct VariantAlternative {
};

template <std::size_t I, typename T, typename... Types>
struct VariantAlternative<I, T, Types...> {
    static_assert((I < sizeof...(Types)),
                  "Error: index out of bounds in `jstd::VariantAlternative<>`");
    using type = typename VariantAlternative<I - 1, Types...>::type;
};

template <typename T, typename... Types>
struct VariantAlternative<0, T, Types...> {
    using type = T;
};

template <typename T, typename... Types>
struct VariantAlternative<0, const T, Types...> {
    using type = typename std::add_const<T>::type;
};

template <typename T, typename... Types>
struct VariantAlternative<0, volatile T, Types...> {
    using type = typename std::add_volatile<T>::type;
};

template <typename T, typename... Types>
struct VariantAlternative<0, const volatile T, Types...> {
    using type = typename std::add_cv<T>::type;
};

template <typename T>
struct TypeErasure {
    typedef typename std::remove_reference<T>::type _T0;

    typedef typename std::conditional<
                IsArray<_T0>::value,
                typename std::remove_extent<_T0>::type *,
                typename std::conditional<
                    std::is_function<_T0>::value,
                    typename std::add_pointer<_T0>::type,
                    typename std::remove_cv<_T0>::type
                >::type
            >::type   type;
};

template <typename T>
struct TypeErasureNoPtr {
    typedef typename std::remove_reference<T>::type _T0;

    typedef typename std::conditional<
                IsArray<_T0>::value,
                typename std::remove_extent<_T0>::type,
                typename std::conditional<
                    std::is_function<_T0>::value,
                    typename std::add_pointer<_T0>::type,
                    typename std::remove_cv<_T0>::type
                >::type
            >::type   type;
};

/*
  std::size_t kMask = (std::is_integral<T>::value ?
                      (!std::is_pointer<T>::value ? 0 : 1) :
                      (!std::is_pointer<T>::value ? 2 : 3));
*/
template <typename T, std::size_t Mask>
struct MathHelper;

template <typename T>
struct MathHelper<T, std::size_t(0)> {
    static T add(T a, T b) {
        return (a + b);
    }

    static T sub(T a, T b) {
        return (a - b);
    }
};

template <>
struct MathHelper<bool, std::size_t(0)> {
    static bool add(bool a, bool b) {
        return (a || b);
    }

    static bool sub(bool a, bool b) {
        return (a && b);
    }
};

template <typename T>
struct MathHelper<T, std::size_t(1)> {
    static T add(T a, T b) {
        T tmp(a);
        return tmp;
    }

    static T sub(T a, T b) {
        T tmp(a);
        return tmp;
    }
};

template <typename T>
struct MathHelper<T, std::size_t(2)> {
    static T add(const T & a, const T & b) {
        return (a + b);
    }

    static T sub(const T & a, const T & b) {
        return (a - b);
    }
};

template <typename T>
struct MathHelper<T, std::size_t(3)> {
    static T add(T a, T b) {
        T tmp(a);
        return tmp;
    }

    static T sub(T a, T b) {
        T tmp(a);
        return tmp;
    }
};

template <typename... Types>
struct VariantHelper;

template <typename T, typename... Types>
struct VariantHelper<T, Types...> {
    static inline
    void destroy(std::type_index type_id, void * data) {
        if (type_id == std::type_index(typeid(T)))
            reinterpret_cast<T *>(data)->~T();
        else
            VariantHelper<Types...>::destroy(type_id, data);
    }

    static inline
    void copy(std::type_index old_type, const void * old_val, void * new_val) {
        if (old_type == std::type_index(typeid(T)))
            new (new_val) T(*reinterpret_cast<const T *>(old_val));
        else
            VariantHelper<Types...>::copy(old_type, old_val, new_val);
    }

    static inline
    void move(std::type_index old_type, void * old_val, void * new_val) {
        if (old_type == std::type_index(typeid(T)))
            new (new_val) T(std::move(*reinterpret_cast<T *>(old_val)));
        else
            VariantHelper<Types...>::move(old_type, old_val, new_val);
    }

    static inline
    void swap(std::type_index old_type, void * old_val, void * new_val) {
        if (old_type == std::type_index(typeid(T)))
            std::swap(*reinterpret_cast<T *>(new_val), *reinterpret_cast<T *>(old_val));
        else
            VariantHelper<Types...>::swap(old_type, old_val, new_val);
    }

    static inline
    int compare(std::type_index now_type, void * left_val, void * right_val) {
        if (now_type == std::type_index(typeid(T))) {
            if (*reinterpret_cast<T *>(left_val) > *reinterpret_cast<T *>(right_val))
                return 1;
            else if (*reinterpret_cast<T *>(left_val) < *reinterpret_cast<T *>(right_val))
                return -1;
            else
                return 0;
        } else {
            return VariantHelper<Types...>::compare(now_type, left_val, right_val);
        }
    }

    static inline
    void add(std::type_index now_type, const void * left_val, const void * right_val, void * out_val) {
        if (now_type == std::type_index(typeid(T))) {
            typedef typename std::remove_cv<T>::type U;
            const U & left  = (*reinterpret_cast<U *>(const_cast<void *>(left_val)));
            const U & right = (*reinterpret_cast<U *>(const_cast<void *>(right_val)));
            static constexpr std::size_t kMask = (std::is_integral<T>::value ?
                                                 (!std::is_pointer<T>::value ? 0 : 1) :
                                                 (!std::is_pointer<T>::value ? 2 : 3));
            U sum = MathHelper<U, kMask>::add(left, right);
            *reinterpret_cast<U *>(out_val) = std::move(sum);
        }
        else {
            VariantHelper<Types...>::add(now_type, left_val, right_val, out_val);
        }
    }

    static inline
    void sub(std::type_index now_type, const void * left_val, const void * right_val, void * out_val) {
        if (now_type == std::type_index(typeid(T))) {
            typedef typename std::remove_cv<T>::type U;
            const U & left  = (*reinterpret_cast<U *>(const_cast<void *>(left_val)));
            const U & right = (*reinterpret_cast<U *>(const_cast<void *>(right_val)));
            static constexpr std::size_t kMask = (std::is_integral<T>::value ?
                                                 (!std::is_pointer<T>::value ? 0 : 1) :
                                                 (!std::is_pointer<T>::value ? 2 : 3));
            U sub = MathHelper<U, kMask>::sub(left, right);
            *reinterpret_cast<U *>(out_val) = std::move(sub);
        }
        else {
            VariantHelper<Types...>::sub(now_type, left_val, right_val, out_val);
        }
    }
};

template <>
struct VariantHelper<>  {
    static inline void destroy(std::type_index type_id, void * data) {}
    static inline void copy(std::type_index old_type, const void * old_val, void * new_val) {}
    static inline void move(std::type_index old_type, void * old_val, void * new_val) {}
    static inline void swap(std::type_index old_type, void * old_val, void * new_val) {}

    static inline void compare(std::type_index now_type, void * left_val, void * right_val) {}

    static inline void add(std::type_index now_type, const void * left_val, const void * right_val, void * out_val) {}
    static inline void sub(std::type_index now_type, const void * left_val, const void * right_val, void * out_val) {}
};

template <typename... Types>
class Variant {
public:
    typedef Variant<Types...>           this_type;
    typedef VariantHelper<Types...>     helper_type;

    enum {
        kDataSize = MaxInteger<sizeof(Types)...>::value,
        kAlignment = MaxAlign<Types...>::value
    };
    using data_t = typename std::aligned_storage<kDataSize, kAlignment>::type;

    static const std::size_t kMaxType = sizeof...(Types);

protected:
    data_t          data_;
    std::size_t     index_;
    std::type_index type_index_;

public:
    Variant() noexcept : index_(VariantNPos), type_index_(typeid(void)) {
    }

    template <typename T,
              typename = typename std::enable_if<ContainsType<typename std::remove_reference<T>::type, Types...>::value>::type>
    Variant(const T & value) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename std::remove_reference<T>::type U;
        this->print_type_info<T, U>("Variant(const T & value)");

        new (&this->data_) U(value);
        this->index_ = this->index_of<U>();
        this->type_index_ = typeid(U);
    }

    template <typename T,
              typename = typename std::enable_if<ContainsType<typename std::remove_reference<T>::type, Types...>::value>::type>
    Variant(T && value) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename std::remove_reference<T>::type U;
        this->print_type_info<T, U>("Variant(T && value)");

        new (&this->data_) U(std::forward<T>(value));
        this->index_ = this->index_of<U>();
        this->type_index_ = typeid(U);
    }

    template <typename T, std::size_t N>
    Variant(T (&value)[N]) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename std::remove_volatile<T>::type * U;
        this->print_type_info<T, U>("Variant(T (&value)[N])", false);

        new (&this->data_) U(value);
        this->index_ = this->index_of<U>();
        this->type_index_ = typeid(U);
    }

    template <typename T, std::size_t N>
    Variant(const T (&value)[N]) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename std::add_const<typename std::remove_volatile<T>::type>::type * U;
        this->print_type_info<T, U>("Variant(const T (&value)[N])", false);

        new (&this->data_) U(value);
        this->index_ = this->index_of<U>();
        this->type_index_ = typeid(U);
    }

#if 0
    template <typename T, typename... Args>
    Variant(const T & type, Args &&... args) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename std::remove_reference<T>::type U;
        this->print_type_info<T, U>("Variant(const T & type, Args &&... args)");

        static constexpr bool is_constructible = std::is_constructible<T, Args...>::value;
        if (is_constructible) {
            new (&this->data_) U(std::forward<Args>(args)...);
            this->index_ = this->index_of<U>();
            this->type_index_ = typeid(U);
        } else {
            throw BadVariantAccess("Exception: Bad emplace assignment access.");
        }

        //static_assert(is_constructible, "Error: Bad emplace assignment access.");
    }
#endif

    Variant(const this_type & src) : index_(src.index_), type_index_(src.type_index_) {
        helper_type::copy(src.type_index_, &src.data_, &this->data_);
    }

    Variant(this_type && src) : index_(src.index_), type_index_(src.type_index_) {
        helper_type::move(src.type_index_, &src.data_, &this->data_);
    }

    virtual ~Variant() {
        this->destroy();
    }

    inline void destroy() {
        helper_type::destroy(this->type_index_, &this->data_);
        this->index_ = VariantNPos;
        this->type_index_ = typeid(void);
    }

private:
    template <typename T, typename U>
    inline void print_type_info(const std::string & title, bool T_is_main = true) const {
#ifdef _DEBUG
        printf("%s;\n\n", title.c_str());
        printf("typeid(T).name() = %s\n", typeid(T).name());
        printf("typeid(U).name() = %s\n", typeid(U).name());
        printf("typeid(std::remove_const<T>::type).name() = %s\n", typeid(typename std::remove_const<T>::type).name());
        if (T_is_main) {
            printf("ContainsType<T>::value = %u\n", (uint32_t)ContainsType<typename std::remove_reference<T>::type, Types...>::value);
            printf("std::is_array<T>::value = %u\n", (uint32_t)std::is_array<typename std::remove_const<T>::type>::value);
            printf("IsArray<T>::value = %u\n", (uint32_t)IsArray<T>::value);
        } else {
            printf("ContainsType<U>::value = %u\n", (uint32_t)ContainsType<typename jstd::TypeErasure<U>::type, Types...>::value);
            printf("std::is_array<U>::value = %u\n", (uint32_t)std::is_array<typename std::remove_const<U>::type>::value);
            printf("IsArray<U>::value = %u\n", (uint32_t)IsArray<U>::value);
        }
        printf("\n");
#endif
    }

    template <typename T>
    inline void check_valid_type(const char * name) const {
        if (!this->holds_alternative<T>()) {
            std::cout << "Variant<Types...>::" << name << " exception:" << std::endl << std::endl;
            std::cout << "Type [" << typeid(T).name() << "] is not defined. " << std::endl;
            std::cout << "Current type is [" << this->type_index().name() << "], index is "
                      << (std::intptr_t)this->index() << "." << std::endl << std::endl;
            throw BadVariantAccess();
        }
    }

    template <typename T>
    inline void check_valid_type(char * name) const {
        return this->check_valid_type<T>((const char *)name);
    }

public:
    this_type & operator = (const this_type & rhs) {
        // For the safety of exceptions, reset the index and type index.
        this->destroy();

        helper_type::copy(rhs.type_index_, &rhs.data_, &this->data_);
        this->index_ = rhs.index_;
        this->type_index_ = rhs.type_index_;
        return *this;
    }

    this_type & operator = (this_type && rhs) {
        // For the safety of exceptions, reset the index and type index.
        this->destroy();

        helper_type::move(rhs.type_index_, &rhs.data_, &this->data_);
        this->index_ = rhs.index_;
        this->type_index_ = rhs.type_index_;
        return *this;
    }

    template <typename T, typename... Args>
    T & emplace(Args &&... args) {
        typedef typename std::remove_reference<T>::type U;
        this->print_type_info<T, U>("T & Variant::emplace(Args &&... args)");

        static constexpr bool is_constructible = std::is_constructible<T, Args...>::value;
        if (is_constructible) {
            // For the safety of exceptions, reset the index and type index.
            this->destroy();

            new (&this->data_) U(std::forward<Args>(args)...);
            this->index_ = this->index_of<U>();
            this->type_index_ = typeid(U);
        } else {
            throw BadVariantAccess("Exception: Bad emplace assignment access.");
        }

        //static_assert(is_constructible, "Error: Bad Variant::emplace(Args &&... args) access.");

        return *(reinterpret_cast<T *>(this->data_));
    }

    template <typename T, typename... Args>
    const T & emplace(Args &&... args) const {
        typedef typename std::remove_reference<T>::type U;
        this->print_type_info<T, U>("const T & Variant::emplace(Args &&... args)");

        static constexpr bool is_constructible = std::is_constructible<T, Args...>::value;
        if (is_constructible) {
            // For the safety of exceptions, reset the index and type index.
            this->destroy();

            new (&this->data_) U(std::forward<Args>(args)...);
            this->index_ = this->index_of<U>();
            this->type_index_ = typeid(U);
        } else {
            throw BadVariantAccess("Exception: Bad emplace assignment access.");
        }

        //static_assert(is_constructible, "Error: Bad Variant::emplace(Args &&... args) access.");

        return *(const_cast<const T *>(reinterpret_cast<T *>(this->data_)));
    }

    template <typename T, typename V, typename... Args>
    T & emplace(std::initializer_list<V> il, Args &&... args) {
        typedef typename std::remove_reference<T>::type U;
        this->print_type_info<T, U>("T & Variant::emplace(std::initializer_list<V> il, Args &&... args)");

        static constexpr bool is_constructible = std::is_constructible<T, std::initializer_list<V> &, Args...>::value;
        if (is_constructible) {
            // For the safety of exceptions, reset the index and type index.
            this->destroy();

            new (&this->data_) U(il, std::forward<Args>(args)...);
            this->index_ = this->index_of<U>();
            this->type_index_ = typeid(U);
        } else {
            throw BadVariantAccess("Exception: Bad emplace assignment access.");
        }

        //static_assert(is_constructible, "Error: Bad Variant::emplace(std::initializer_list<V> il, Args &&... args) access.");

        return *(reinterpret_cast<T *>(this->data_));
    }

    this_type add(const this_type & rhs) const  {
        this_type tmp(*this);
        if ((this->index() == rhs.index()) &&
            !this->valueless_by_exception() &&
            !rhs.valueless_by_exception()) {
            helper_type::add(this->type_index_, &this->data_, &rhs.data_, &tmp.data_);
        }
        return tmp;
    }

    this_type sub(const this_type & rhs) const  {
        this_type tmp(*this);
        if ((this->index() == rhs.index()) &&
            !this->valueless_by_exception() &&
            !rhs.valueless_by_exception()) {
            helper_type::sub(this->type_index_, &this->data_, &rhs.data_, &tmp.data_);
        }
        return tmp;
    }

    this_type operator + (const this_type & rhs) const {
        return this->add(rhs);
    }

    this_type operator - (const this_type & rhs) const {
        return this->sub(rhs);
    }

    bool operator == (const this_type & rhs) const {
        if (this->index_ == rhs.index_) {
            int cmp = helper_type::compare(this->type_index_, &this->data_, &rhs.data_);
            return (cmp == 0);
        }
        return false;
    }

    bool operator != (const this_type & rhs) const {
        if (this->index_ != rhs.index_) {
            return true;
        } else {
            int cmp = helper_type::compare(this->type_index_, &this->data_, &rhs.data_);
            return (cmp != 0);
        }
    }

    bool operator < (const this_type & rhs) const {
        if (this->index_ == rhs.index_) {
            int cmp = helper_type::compare(this->type_index_, &this->data_, &rhs.data_);
            return (cmp < 0);
        }
        return (this->index_ < rhs.index_);
    }

    bool operator > (const this_type & rhs) const {
        if (this->index_ == rhs.index_) {
            int cmp = helper_type::compare(this->type_index_, &this->data_, &rhs.data_);
            return (cmp > 0);
        }
        return (this->index_ > rhs.index_);
    }

    bool operator <= (const this_type & rhs) const {
        return !(*this > rhs);
    }

    bool operator >= (const this_type & rhs) const {
        return !(*this < rhs);
    }

    inline constexpr std::size_t size() const noexcept {
        return sizeof...(Types);
    }

    inline std::size_t index() const noexcept {
        return this->index_;
    }

    inline std::type_index & type_index() noexcept {
        return this->type_index_;
    }

    inline const std::type_index type_index() const noexcept {
        return this->type_index_;
    }

    template <typename T>
    inline std::size_t index_of() const noexcept {
        using U = typename std::remove_reference<T>::type;
        return TypeIndexOf<U, 0, Types...>::value;
    }

    inline bool valueless_by_exception() const noexcept {
        std::size_t index = this->index();
        return (index == VariantNPos || index >= this->size());
    }

    inline bool has_assigned() const noexcept {
#if 1
        std::size_t index = this->index();
        return (index != VariantNPos);
#else
        return (this->type_index_ != type_index(typeid(void)));
#endif
    }

    template <typename T>
    inline bool holds_alternative() const noexcept {
        using U = typename std::remove_reference<T>::type;
        std::size_t index = this->index_of<U>();
        if (index == this->index()) {
            return !this->valueless_by_exception();
        } else {
            return false;
        }
    }

    template <typename T>
    inline bool is_valid_type_index() const noexcept {
        using U = typename std::remove_reference<T>::type;
        if (this->type_index_ == std::type_index(typeid(U))) {
            return !this->valueless_by_exception();
        } else {
            return false;
        }
    }

    template <std::size_t I>
    void init() {
        if (this->index_ == VariantNPos) {
            using T = typename VariantAlternative<I, Types...>::type;
            typedef typename std::remove_reference<T>::type U;
            if (this->holds_alternative<U>()) {
                // For the safety of exceptions, reset the index and type index.
                this->destroy();

                new (&this->data_) U();
                this->index_ = I;
                this->type_index_ = typeid(U);
            }
        }
    }

    template <typename T>
    typename std::remove_reference<T>::type & get() {
        using U = typename std::remove_reference<T>::type;
        this->check_valid_type<U>("T & get<T>()");
        return *((U *)(&this->data_));
    }

    template <typename T>
    const typename std::remove_reference<T>::type & get() const {
        using U = typename std::remove_reference<T>::type;
        this->check_valid_type<U>("const T & get<T>()");
        return *((const U *)(&this->data_));
    }

#if 0
    template <typename T>
    typename std::remove_reference<T>::type && get() {
        using U = typename std::remove_reference<T>::type;
        this->check_valid_type<U>("T && get<T>()");
        return std::move(*((U *)(&this->data_)));
    }

    template <typename T>
    const typename std::remove_reference<T>::type && get() const {
        using U = typename std::remove_reference<T>::type;
        this->check_valid_type<U>("const T && get<T>()");
        return std::move(*((const U *)(&this->data_)));
    }
#endif

    template <std::size_t I>
    typename VariantAlternative<I, Types...>::type & get() {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("T & get<I>()");
        return *((U *)(&this->data_));
    }

    template <std::size_t I>
    const typename VariantAlternative<I, Types...>::type & get() const {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("const T & get<I>()");
        return *((const U *)(&this->data_));
    }

#if 0
    template <std::size_t I>
    typename VariantAlternative<I, Types...>::type && get() {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("T && get<I>()");
        return std::move(*((U *)(&this->data_)));
    }

    template <std::size_t I>
    const typename VariantAlternative<I, Types...>::type && get() const {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("const T && get<I>()");
        return std::move(*((const U *)(&this->data_)));
    }
#endif

    template <typename T>
    void set(const T & value) {
        using U = typename std::remove_reference<T>::type;
        std::size_t new_index = this->index_of<U>();
        if (this->has_assigned()) {
            if (new_index == this->index()) {
                assert(new_index == this->index_);
                assert(typeid(U) == this->type_index_);
                *((U *)(&this->data_)) = value;
                return;
            } else {
                // For the safety of exceptions, reset the index and type index.
                this->destroy();
            }
        }

        new (&this->data_) U(value);
        this->index_ = new_index;
        this->type_index_ = typeid(U);
    }

    template <typename T>
    void set(T && value) {
        using U = typename std::remove_reference<T>::type;
        std::size_t new_index = this->index_of<U>();
        if (this->has_assigned()) {
            if (new_index == this->index()) {
                assert(new_index == this->index_);
                assert(typeid(U) == this->type_index_);
                *((U *)(&this->data_)) = std::forward<U>(value);
                return;
            } else {
                // For the safety of exceptions, reset the index and type index.
                this->destroy();
            }
        }

        new (&this->data_) U(std::forward<T>(value));
        this->index_ = new_index;
        this->type_index_ = typeid(U);
    }

    template <typename T, std::size_t N>
    void set(T (&value)[N]) {
        using U = typename std::remove_reference<T>::type *;
        std::size_t new_index = this->index_of<U>();
        if (this->has_assigned()) {
            if (new_index == this->index()) {
                assert(new_index == this->index_);
                assert(typeid(U) == this->type_index_);
                *((U *)(&this->data_)) = value;
                return;
            } else {
                // For the safety of exceptions, reset the index and type index.
                this->destroy();
            }
        }

        new (&this->data_) U(value);
        this->index_ = new_index;
        this->type_index_ = typeid(U);
    }

    template <std::size_t I>
    void set(const typename VariantAlternative<I, Types...>::type & value) {
        using U = typename std::remove_reference<typename VariantAlternative<I, Types...>::type>::type;
        std::size_t new_index = this->index_of<U>();
        if (this->has_assigned()) {
            if (new_index == this->index()) {
                assert(new_index == this->index_);
                assert(typeid(U) == this->type_index_);
                *((U *)(&this->data_)) = value;
                return;
            } else {
                // For the safety of exceptions, reset the index and type index.
                this->destroy();
            }
        }

        new (&this->data_) U(value);
        this->index_ = new_index;
        this->type_index_ = typeid(U);
    }

    template <std::size_t I>
    void set(typename VariantAlternative<I, Types...>::type && value) {
        using T = typename VariantAlternative<I, Types...>::type;
        using U = typename std::remove_reference<typename VariantAlternative<I, Types...>::type>::type;
        std::size_t new_index = this->index_of<U>();
        if (this->has_assigned()) {
            if (new_index == this->index()) {
                assert(new_index == this->index_);
                assert(typeid(U) == this->type_index_);
                *((U *)(&this->data_)) = std::forward<U>(value);
                return;
            } else {
                // For the safety of exceptions, reset the index and type index.
                this->destroy();
            }
        }

        new (&this->data_) U(std::forward<T>(value));
        this->index_ = new_index;
        this->type_index_ = typeid(U);
    }

    template <typename Visitor>
    void visit(Visitor && visitor) {
        using Arg0 = typename function_traits<Visitor>::template arguments<0>::type;
        using T = typename std::remove_reference<Arg0>::type;
        if (this->holds_alternative<T>()) {
            std::forward<Visitor>(visitor)(this->get<T>());
        } else if (std::is_same<T, this_type>::value) {
            std::forward<Visitor>(visitor)(*this);
        }
    }

    template <typename Visitor, typename... Args>
    void visit(Visitor && visitor, Args &&... args) {
        using Arg0 = typename function_traits<Visitor>::template arguments<0>::type;
        using T = typename std::remove_reference<Arg0>::type;
        if (this->holds_alternative<T>())
            this->visit(std::forward<Visitor>(visitor));
        else
            this->visit(std::forward<Args>(args)...);
    }
}; // class Variant<...>

template <typename... Types>
inline
Variant<Types...> operator + (const Variant<Types...> & lhs, const Variant<Types...> & rhs) {
    return lhs.add(rhs);
}

template <typename... Types>
inline
Variant<Types...> operator - (const Variant<Types...> & lhs, const Variant<Types...> & rhs) {
    return lhs.sub(rhs);
}

template <std::size_t I, typename... Types>
static bool holds_alternative(const Variant<Types...> & variant) noexcept {
    std::size_t index = variant.index();
    return ((I == index) && variant.valueless_by_exception());
}

template <typename T, typename... Types>
static bool holds_alternative(const Variant<Types...> & variant) noexcept {
    using U = typename std::remove_reference<T>::type;
    return variant.template holds_alternative<U>();
}

template <typename T, typename... Types>
T & get(Variant<Types...> & variant) {
    return variant.template get<T>();
}

template <typename T, typename... Types>
const T & get(const Variant<Types...> & variant) {
    return variant.template get<T>();
}

template <std::size_t I, typename... Types>
typename VariantAlternative<I, Types...>::type & get(Variant<Types...> & variant) {
    return variant.template get<I>();
}

template <std::size_t I, typename... Types>
const typename VariantAlternative<I, Types...>::type & get(const Variant<Types...> & variant) {
    return variant.template get<I>();
}

template <typename Arg0, typename Visitor, typename Arg>
void visit_impl(Visitor && visitor, Arg && arg) {
    //typedef typename std::remove_const<Visitor>::type non_const_visitor;
    using T = typename std::remove_reference<Arg0>::type;
    using U = typename std::remove_reference<Arg>::type;

    if (std::is_same<T, void>::value) {
        //
    } else if (std::is_same<T, MonoState>::value) {
        //
    } else if (std::is_same<T, U>::value) {
        std::forward<Visitor>(visitor)(std::forward<Arg>(arg));
        //(*const_cast<non_const_visitor *>(&visitor))(std::move(std::forward<Arg>(arg)));
    } else if (std::is_constructible<T, U>::value && (!std::is_same<T, void>::value &&
                                                      !std::is_same<T, MonoState>::value)) {
        std::forward<Visitor>(visitor)(std::forward<Arg>(arg));
        //(*const_cast<non_const_visitor *>(&visitor))(std::move(std::forward<Arg>(arg)));
    } else {
        throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch.");
    }
}

template <typename Arg0, typename Visitor, typename... Types>
void visit_impl(Visitor && visitor, Variant<Types...> && variant) {
    using T = typename std::remove_reference<Arg0>::type;
    if (std::is_same<T, void>::value) {
        //
    } else if (std::is_same<T, MonoState>::value) {
        //
    } else if (holds_alternative<T, Types...>(std::forward<Variant<Types...>>(variant))) {
        std::forward<Visitor>(visitor)(std::move(get<T>(std::forward<Variant<Types...>>(variant))));
    } else if (std::is_same<T, Variant<Types...>>::value) {
        std::forward<Variant<Types...>>(variant).visit(std::forward<Visitor>(visitor));
    } else {
        throw BadVariantAccess("Exception: jstd::visit(visitor, variant): Type T is dismatch.");
    }
}

template <typename Arg0, typename Visitor, typename Arg, typename... Args>
void visit_impl(Visitor && visitor, Arg && arg, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Arg>(arg));
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Args>
void visit_impl(Visitor && visitor, Arg0 && arg0, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Arg0>(arg0));
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Types, typename... Args>
void visit_impl(Visitor && visitor, Variant<Types...> && variant, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Variant<Types...>>(variant));
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
void visit(Visitor && visitor, Args &&... args) {
    using Arg0 = typename function_traits<Visitor>::arg0;
    using result_type = typename function_traits<Visitor>::result_type;
    using T = typename std::remove_reference<Arg0>::type;
    static constexpr bool has_result_type = !std::is_same<result_type, void>::value &&
                                            !std::is_same<result_type, MonoState>::value;
    if (std::is_same<T, void>::value) {
        //
    } else if (std::is_same<T, MonoState>::value) {
        //
    } else {
        visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
    }
}

template <typename Visitor>
void visit(Visitor && visitor) {
}

} // namespace jstd

template <>
struct std::hash<jstd::MonoState> {
    using argument_type = jstd::MonoState;
    using result_type = std::size_t;

    inline result_type operator () (const argument_type &) const noexcept {
        // return a fundamentally attractive random value.
        return 66740831;
    }
};

#endif // JSTD_VARIANT_H
