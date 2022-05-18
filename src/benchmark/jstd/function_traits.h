
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

namespace jstd {

///////////////////////////////////////////////////////////////////////////////////////

struct MonoObject {
    MonoObject() noexcept {}
};

#if 1
bool operator == (MonoObject, MonoObject) noexcept { return true;  }
bool operator != (MonoObject, MonoObject) noexcept { return false; }
bool operator  < (MonoObject, MonoObject) noexcept { return false; }
bool operator  > (MonoObject, MonoObject) noexcept { return false; }
bool operator <= (MonoObject, MonoObject) noexcept { return true;  }
bool operator >= (MonoObject, MonoObject) noexcept { return true;  }
#else
constexpr bool operator == (MonoObject, MonoObject) noexcept { return true;  }
constexpr bool operator != (MonoObject, MonoObject) noexcept { return false; }
constexpr bool operator  < (MonoObject, MonoObject) noexcept { return false; }
constexpr bool operator  > (MonoObject, MonoObject) noexcept { return false; }
constexpr bool operator <= (MonoObject, MonoObject) noexcept { return true;  }
constexpr bool operator >= (MonoObject, MonoObject) noexcept { return true;  }
#endif

///////////////////////////////////////////////////////////////////////////////////////

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
    using type = typename std::conditional<(arity == 0), MonoObject,
                 typename std::tuple_element<I + ((arity != 0) ? 1 : 0), std::tuple<MonoObject, Args...>>::type>::type;
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

    typedef MonoObject          functor_type;
    typedef ReturnType          result_type;
    typedef std::tuple<Args...> args_type;

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

    typedef MonoObject          functor_type;
    typedef ReturnType          result_type;
    typedef std::tuple<Args...> args_type;

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
 
// Specialize for const member function pointer
template <typename ReturnType, typename Functor, typename... Args>
struct function_traits<ReturnType(Functor::*)(Args...) const>
    /* : public function_traits<ReturnType(const T &, Args...)> */ {
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
 
// Specialize for member object pointer
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

    typedef MonoObject arg0;
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

#if 0
template <typename Functor>
struct function_traits : public function_traits<decltype(&Functor::operator ())> {
    template <typename Functor, typename = void>
    struct __internal;

    template <typename Functor, typename ReturnType, typename... Args>
    struct __internal {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef MonoObject          functor_type;
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

        typedef VoidObject               functor_type;
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

///////////////////////////////////////////////////////////////////////////////////////

#if 0
template <typename Functor>
struct function_traits {
    template <typename ReturnType, typename... Args>
    struct __internal {
        static constexpr std::size_t arity = sizeof...(Args);

        typedef MonoObject              functor_type;
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

///////////////////////////////////////////////////////////////////////////////////////

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

} // namespace jstd

#endif // JSTD_FUNCTION_TRAITS_H
