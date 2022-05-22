
#ifndef JSTD_FUNCTION_TRAITS_H
#define JSTD_FUNCTION_TRAITS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <cstdint>
#include <cstddef>
#include <cstdbool>

#include <tuple>
#include <functional>
#include <type_traits>

#include "jstd/apply_visitor.h"

namespace jstd {

///////////////////////////////////////////////////////////////////////////////////////

struct void_type {
    void_type() noexcept {}

    template <typename T>
    void_type(const T &) noexcept {
    }

    template <typename T>
    void_type(T &&) noexcept {
    }
};

#if (JSTD_IS_CPP_14 == 0)
bool operator == (void_type, void_type) noexcept { return true;  }
bool operator != (void_type, void_type) noexcept { return false; }
bool operator  < (void_type, void_type) noexcept { return false; }
bool operator  > (void_type, void_type) noexcept { return false; }
bool operator <= (void_type, void_type) noexcept { return true;  }
bool operator >= (void_type, void_type) noexcept { return true;  }
#else
constexpr bool operator == (void_type, void_type) noexcept { return true;  }
constexpr bool operator != (void_type, void_type) noexcept { return false; }
constexpr bool operator  < (void_type, void_type) noexcept { return false; }
constexpr bool operator  > (void_type, void_type) noexcept { return false; }
constexpr bool operator <= (void_type, void_type) noexcept { return true;  }
constexpr bool operator >= (void_type, void_type) noexcept { return true;  }
#endif

template <typename T>
struct return_type_wrapper
{
    typedef T type;
};

template <>
struct return_type_wrapper<void>
{
    typedef void_type type;
};

template <typename T>
struct return_type_traits {
    typedef T result_type;
    typedef typename std::remove_cv<
                typename std::remove_reference<result_type
                >::type
            >::type type;
};

///////////////////////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
struct remove_cvr {
    typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////

template <typename... Ts>
struct make_void {
    typedef void type;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

///////////////////////////////////////////////////////////////////////////////////////

template <std::size_t I, typename... Args>
struct tuple_element_helper {
    static constexpr std::size_t arity = sizeof...(Args);

    static_assert(((I < arity) || (arity == 0)), "Error: invalid parameter index.");
    using type = typename std::conditional<(arity == 0), void_type,
                 typename std::tuple_element<I + ((arity != 0) ? 1 : 0), std::tuple<void_type, Args...>>::type>::type;
};

///////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////

// Specialize for normal function
template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(Args...)> {
    static constexpr std::size_t arity = sizeof...(Args);

    typedef void_type           functor_type;
    typedef std::tuple<Args...> args_type;

    typedef typename return_type_wrapper<ReturnType>::type result_type;

    typedef ReturnType(*type)(Args...);

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity) || (arity == 0),
                      "Error: function_traits<ReturnType(Args...)>: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(Args...)> func_type;
};

// Specialize for function pointer
template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)> {
    static constexpr std::size_t arity = sizeof...(Args);

    typedef void_type           functor_type;
    typedef std::tuple<Args...> args_type;

    typedef typename return_type_wrapper<ReturnType>::type result_type;

    typedef ReturnType(*type)(Args...);

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity) || (arity == 0),
                      "Error: function_traits<ReturnType(*)(Args...)>: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(Args...)> func_type;
};

// Specialize for member function pointer
template <typename ReturnType, typename Functor, typename... Args>
struct function_traits<ReturnType(Functor::*)(Args...)>
    /* : public function_traits<ReturnType(T &, Args...)> */ {
    static constexpr std::size_t arity = sizeof...(Args);

    typedef Functor             functor_type;
    typedef std::tuple<Args...> args_type;

    typedef typename return_type_wrapper<ReturnType>::type result_type;

    typedef decltype(&Functor::operator ()) type;

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity), "Error: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(Functor &, Args...)> func_type;
};

// Specialize for const member function pointer
template <typename ReturnType, typename Functor, typename... Args>
struct function_traits<ReturnType(Functor::*)(Args...) const>
    /* : public function_traits<ReturnType(const T &, Args...)> */ {
    static constexpr std::size_t arity = sizeof...(Args);

    typedef Functor             functor_type;
    typedef std::tuple<Args...> args_type;

    typedef typename return_type_wrapper<ReturnType>::type result_type;

    typedef decltype(&Functor::operator ()) const type;

    template <std::size_t I>
    struct arguments {
        static_assert((I < arity), "Error: invalid parameter index.");
        using type = typename tuple_element_helper<I, Args...>::type;
    };

    typedef typename tuple_element_helper<0, Args...>::type arg0;
    typedef std::function<ReturnType(const Functor &, Args...)> func_type;
};

// Specialize for member object pointer
template <typename ReturnType, typename Functor>
struct function_traits<ReturnType(Functor::*)>
    /* : public function_traits<ReturnType(T &)> */ {
    static constexpr std::size_t arity = 0;

    typedef Functor      functor_type;
    typedef std::tuple<> args_type;

    typedef typename return_type_wrapper<ReturnType>::type result_type;

    typedef typename Functor::ReturnType * type;

    template <std::size_t I>
    struct arguments {
        using type = void_type;
    };

    typedef void_type arg0;
    typedef std::function<ReturnType(Functor &)> func_type;
};

// Specialize for std::function<T>
template <typename ReturnType, typename... Args>
struct function_traits<std::function<ReturnType(Args...)>>
    : public function_traits<ReturnType(Args...)> {
};

// Specialize for lambda or function object
template <typename Functor>
struct function_traits : public function_traits<decltype(&Functor::operator ())> {
};

template <typename Functor>
struct function_traits<Functor &> : public function_traits<Functor> {};

template <typename Functor>
struct function_traits<Functor &&> : public function_traits<Functor> {};

///////////////////////////////////////////////////////////////////////////////////////

} // namespace jstd

#endif // JSTD_FUNCTION_TRAITS_H
