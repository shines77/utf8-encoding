
#ifndef JSTD_VARIANT_H
#define JSTD_VARIANT_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif
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

#include "jstd/char_traits.h"
#include "jstd/function_traits.h"
#include "jstd/apply_visitor.h"

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

namespace jstd {

// std::variant_npos
static constexpr std::size_t VariantNPos = (std::size_t)-1;

///////////////////////////////////////////////////////////////////////////////////////

// Like std::monostate
struct MonoState {
    MonoState() noexcept {}
    MonoState(const MonoState & src) noexcept {}

    template <typename T>
    MonoState(const T & src) noexcept {}

    ~MonoState() {}
};

#if (JSTD_IS_CPP_14 == 0)
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

///////////////////////////////////////////////////////////////////////////////////////

// std::bad_variant_access
struct BadVariantAccess : public std::runtime_error {
    BadVariantAccess(const char * message = "Exception: Variant<Types...> Bad access") throw()
        : std::runtime_error(message) {
    }
    ~BadVariantAccess() noexcept {}
};

///////////////////////////////////////////////////////////////////////////////////////

template <std::size_t Val, std::size_t... Rest>
struct StaticMax;

template <std::size_t Val>
struct StaticMax<Val> : std::integral_constant<std::size_t, Val> {
};

template <std::size_t Val1, std::size_t Val2, std::size_t... Rest>
struct StaticMax<Val1, Val2, Rest...>
    : std::integral_constant<std::size_t, ((Val1 >= Val2) ?
                                           StaticMax<Val1, Rest...>::value :
                                           StaticMax<Val2, Rest...>::value) > {
};

template <typename... Types>
struct MaxAlign : std::integral_constant<std::size_t, StaticMax<std::alignment_of<Types>::value...>::value> {
};

///////////////////////////////////////////////////////////////////////////////////////

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
    : std::conditional<IsSame<T, Head>::value, std::true_type, ContainsType<T, Types...>>::type {
};

template <typename T>
struct ContainsType<T> : std::false_type {
};

///////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////

// std::variant_alternative

// Forward declaration
template <std::size_t I, typename... Types>
struct VariantAlternative;

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

///////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////

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
    int compare(std::type_index type_idx, void * left_val, void * right_val) {
        if (type_idx == std::type_index(typeid(T))) {
            if (*reinterpret_cast<T *>(left_val) > *reinterpret_cast<T *>(right_val))
                return 1;
            else if (*reinterpret_cast<T *>(left_val) < *reinterpret_cast<T *>(right_val))
                return -1;
            else
                return 0;
        } else {
            return VariantHelper<Types...>::compare(type_idx, left_val, right_val);
        }
    }

    static inline
    void add(std::type_index type_idx, const void * left_val, const void * right_val, void * out_val) {
        if (type_idx == std::type_index(typeid(T))) {
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
            VariantHelper<Types...>::add(type_idx, left_val, right_val, out_val);
        }
    }

    static inline
    void sub(std::type_index type_idx, const void * left_val, const void * right_val, void * out_val) {
        if (type_idx == std::type_index(typeid(T))) {
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
            VariantHelper<Types...>::sub(type_idx, left_val, right_val, out_val);
        }
    }

    template <typename ResultType, typename Visitor>
    static inline
    void apply_visitor(std::type_index type_idx, void * data, ResultType * result, Visitor & visitor) {
        if (type_idx == std::type_index(typeid(T))) {
            *result = visitor(*reinterpret_cast<T *>(data));
        } else {
            VariantHelper<Types...>::template apply_visitor<ResultType, Visitor>(
                type_idx, data, result, visitor);
        }
    }

    template <typename ResultType, typename Visitor>
    static inline
    void apply_move_visitor(std::type_index type_idx, void * data, ResultType * result, Visitor && visitor) {
        if (type_idx == std::type_index(typeid(T))) {
            *result = visitor(*reinterpret_cast<T *>(data));
        } else {
            VariantHelper<Types...>::template apply_move_visitor<ResultType, Visitor>(
                type_idx, data, result, std::forward<Visitor>(visitor));
        }
    }
};

template <>
struct VariantHelper<>  {
    static inline void destroy(std::type_index type_idx, void * data) {}
    static inline void copy(std::type_index old_type, const void * old_val, void * new_val) {}
    static inline void move(std::type_index old_type, void * old_val, void * new_val) {}
    static inline void swap(std::type_index old_type, void * old_val, void * new_val) {}

    static inline void compare(std::type_index type_idx, void * left_val, void * right_val) {}

    static inline void add(std::type_index type_idx, const void * left_val, const void * right_val, void * out_val) {}
    static inline void sub(std::type_index type_idx, const void * left_val, const void * right_val, void * out_val) {}

    template <typename ResultType, typename Visitor>
    static inline
    void apply_visitor(std::type_index type_idx, void * data, ResultType * result, Visitor & visitor) {}

    template <typename ResultType, typename Visitor>
    static inline
    void apply_move_visitor(std::type_index type_idx, void * data, ResultType * result, Visitor && visitor) {}
};

///////////////////////////////////////////////////////////////////////////////////////

template <typename... Types>
class Variant {
public:
    typedef Variant<Types...>           this_type;
    typedef VariantHelper<Types...>     helper_type;

    enum {
        kDataSize  = StaticMax<sizeof(Types)...>::value,
        kAlignment = MaxAlign<Types...>::value
    };
    using data_t = typename std::aligned_storage<kDataSize, kAlignment>::type;

    typedef std::size_t size_type;

    static constexpr size_type kMaxType = sizeof...(Types);

protected:
    data_t          data_;
    size_type       index_;
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

    template <typename T, size_type N>
    Variant(T (&value)[N]) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename std::remove_volatile<T>::type * U;
        this->print_type_info<T, U>("Variant(T (&value)[N])", false);

        new (&this->data_) U(value);
        this->index_ = this->index_of<U>();
        this->type_index_ = typeid(U);
    }

    template <typename T, size_type N>
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

    Variant(const this_type & other) : index_(other.index_), type_index_(other.type_index_) {
        helper_type::copy(other.type_index_, (void *)&other.data_, (void *)&this->data_);
    }

    Variant(this_type && other) : index_(other.index_), type_index_(other.type_index_) {
        helper_type::move(other.type_index_, (void *)&other.data_, (void *)&this->data_);
        other.index_ = VariantNPos;
        other.type_index_ = typeid(void);
    }

    Variant(const jstd::void_type & other) noexcept : index_(VariantNPos), type_index_(typeid(void)) {
    }

    Variant(jstd::void_type && other) noexcept : index_(VariantNPos), type_index_(typeid(void)) {
    }

    virtual ~Variant() {
        this->destroy();
    }

protected:
    inline void destroy() {
        helper_type::destroy(this->type_index_, (void *)&this->data_);
        this->index_ = VariantNPos;
        this->type_index_ = typeid(void);
    }

private:
    template <typename T, typename U>
    inline void print_type_info(const std::string & title, bool T_is_main = true) const {
#ifdef _DEBUG
        if (0) {
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
        }
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

        helper_type::copy(rhs.type_index_, (void *)&rhs.data_, (void *)&this->data_);
        this->index_ = rhs.index_;
        this->type_index_ = rhs.type_index_;
        return *this;
    }

    this_type & operator = (this_type && rhs) {
        // For the safety of exceptions, reset the index and type index.
        this->destroy();

        helper_type::move(rhs.type_index_, (void *)&rhs.data_, (void *)&this->data_);
        this->index_ = rhs.index_;
        this->type_index_ = rhs.type_index_;

        // Reset the rhs index
        rhs.index_ = VariantNPos;
        rhs.type_index_ = typeid(void);
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
            helper_type::add(this->type_index_, (const void *)&this->data_,
                             (const void *)&rhs.data_, (void *)&tmp.data_);
        }
        return tmp;
    }

    this_type sub(const this_type & rhs) const  {
        this_type tmp(*this);
        if ((this->index() == rhs.index()) &&
            !this->valueless_by_exception() &&
            !rhs.valueless_by_exception()) {
            helper_type::sub(this->type_index_, (const void *)&this->data_,
                             (const void *)&rhs.data_, (void *)&tmp.data_);
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
            int cmp = helper_type::compare(this->type_index_, (void *)&this->data_, (void *)&rhs.data_);
            return (cmp == 0);
        }
        return false;
    }

    bool operator != (const this_type & rhs) const {
        if (this->index_ != rhs.index_) {
            return true;
        } else {
            int cmp = helper_type::compare(this->type_index_, (void *)&this->data_, (void *)&rhs.data_);
            return (cmp != 0);
        }
    }

    bool operator < (const this_type & rhs) const {
        if (this->index_ == rhs.index_) {
            int cmp = helper_type::compare(this->type_index_, (void *)&this->data_, (void *)&rhs.data_);
            return (cmp < 0);
        }
        return (this->index_ < rhs.index_);
    }

    bool operator > (const this_type & rhs) const {
        if (this->index_ == rhs.index_) {
            int cmp = helper_type::compare(this->type_index_, (void *)&this->data_, (void *)&rhs.data_);
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

    inline constexpr size_type size() const noexcept {
        return sizeof...(Types);
    }

    inline size_type index() const noexcept {
        return this->index_;
    }

    inline std::type_index & type_index() noexcept {
        return this->type_index_;
    }

    inline const std::type_index type_index() const noexcept {
        return this->type_index_;
    }

    inline bool has_assigned() const noexcept {
#if 1
        size_type index = this->index();
        return (index != VariantNPos);
#else
        return (this->type_index_ != type_index(typeid(void)));
#endif
    }

    inline bool valueless_by_exception() const noexcept {
        size_type index = this->index();
        return (index >= this->size() || index == VariantNPos);
    }

    inline bool is_valid_index() const noexcept {
        size_type index = this->index();
        return (index < this->size() && index != VariantNPos);
    }

    static inline
    constexpr bool is_valid_index(size_type index) noexcept {
        return (index < kMaxType && index != VariantNPos);
    }

    template <typename T>
    inline bool is_valid_type_index() const noexcept {
        using U = typename std::remove_reference<T>::type;
        if (this->type_index_ == std::type_index(typeid(U))) {
            return this->is_valid_index();
        } else {
            return false;
        }
    }

    template <typename T>
    static inline
    constexpr size_type index_of() noexcept {
        using U = typename std::remove_reference<T>::type;
        return TypeIndexOf<U, 0, Types...>::value;
    }

    template <typename T>
    inline bool holds_alternative() const noexcept {
        using U = typename std::remove_reference<T>::type;
        constexpr size_type index = this->index_of<U>();
        if (index == this->index()) {
            return this->is_valid_index();
        } else {
            return false;
        }
    }

    // Using abi demangle to print nice type name of instance of any holding.
    std::string pretty_type() {
        std::string strType;
#if defined(_MSC_VER)
        strType = this->type_index_.name();
#else
        int status;
        if (char * p = abi::__cxa_demangle(this->type_index_.name(), 0, 0, &status)) {
            strType = p;
            std::free(p);
        }
#endif
        return strType;
    }

    template <size_type I>
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

    template <size_type I>
    typename VariantAlternative<I, Types...>::type & get() {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("T & get<I>()");
        return *((U *)(&this->data_));
    }

    template <size_type I>
    const typename VariantAlternative<I, Types...>::type & get() const {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("const T & get<I>()");
        return *((const U *)(&this->data_));
    }

#if 0
    template <size_type I>
    typename VariantAlternative<I, Types...>::type && get() {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("T && get<I>()");
        return std::move(*((U *)(&this->data_)));
    }

    template <size_type I>
    const typename VariantAlternative<I, Types...>::type && get() const {
        using U = typename VariantAlternative<I, Types...>::type;
        this->check_valid_type<U>("const T && get<I>()");
        return std::move(*((const U *)(&this->data_)));
    }
#endif

    template <typename T>
    void set(const T & value) {
        using U = typename std::remove_reference<T>::type;
        size_type new_index = this->index_of<U>();
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
        size_type new_index = this->index_of<U>();
        if (this->has_assigned()) {
            if (new_index == this->index()) {
                assert(new_index == this->index_);
                //assert(typeid(U) == this->type_index_);
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

    template <typename T, size_type N>
    void set(T (&value)[N]) {
        using U = typename std::remove_reference<T>::type *;
        size_type new_index = this->index_of<U>();
        if (this->has_assigned()) {
            if (new_index == this->index()) {
                assert(new_index == this->index_);
                //assert(typeid(U) == this->type_index_);
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

    template <size_type I>
    void set(const typename VariantAlternative<I, Types...>::type & value) {
        using U = typename std::remove_reference<typename VariantAlternative<I, Types...>::type>::type;
        size_type new_index = this->index_of<U>();
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

    template <size_type I>
    void set(typename VariantAlternative<I, Types...>::type && value) {
        using T = typename VariantAlternative<I, Types...>::type;
        using U = typename std::remove_reference<typename VariantAlternative<I, Types...>::type>::type;
        size_type new_index = this->index_of<U>();
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

    template <typename Visitor>
    inline
    typename std::enable_if<!std::is_const<Visitor>::value,
                            typename detail::return_type_wrapper<
                                typename Visitor::result_type
                              >::type
                           >::type
    apply_visitor(Visitor & visitor) {
        using result_type = typename Visitor::result_type;
        result_type result;
        static constexpr size_type index = this->index_of<result_type>();
        if (this_type::is_valid_index(index)) {
            result = visitor((this->template get<result_type>()));
        } else if (this->holds_alternative<result_type>() || std::is_same<result_type, this_type>::value) {
            helper_type::template apply_visitor<result_type, Visitor>(
                this->type_index_, (void *)&this->data_, &result, visitor);
        }
        return result;
    }

    template <typename Visitor>
    inline
    typename std::enable_if<!std::is_const<Visitor>::value,
                            typename detail::return_type_wrapper<
                                typename Visitor::result_type
                              >::type
                           >::type
    apply_visitor(Visitor && visitor) {
        using result_type = typename Visitor::result_type;
        result_type result;
        static constexpr size_type index = this->index_of<result_type>();
        if (this_type::is_valid_index(index)) {
            result = std::forward<Visitor>(visitor)((this->template get<result_type>()));
        } else if (this->holds_alternative<result_type>() || std::is_same<result_type, this_type>::value) {
            helper_type::template apply_move_visitor<result_type, Visitor>(
                this->type_index_, (void *)&this->data_, &result, std::forward<Visitor>(visitor));
        }
        return result;
    }

    template <typename Visitor>
    inline
    typename detail::return_type_wrapper<typename Visitor::result_type>::type
    apply_visitor(const Visitor & visitor) {
        using result_type = typename Visitor::result_type;
        result_type result;
        static constexpr size_type index = this->index_of<result_type>();
        if (this_type::is_valid_index(index)) {
            result = visitor((this->template get<result_type>()));
        } else if (this->holds_alternative<result_type>() || std::is_same<result_type, this_type>::value) {
            helper_type::template apply_visitor<result_type, Visitor>(
                this->type_index_, (void *)&this->data_, &result, visitor);
        }
        return result;
    }
}; // class Variant<Types...>

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

template <typename Arg0, typename Visitor>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor) {
    using result_type = typename function_traits<Visitor>::result_type;
    return result_type();
}

#if 1
template <typename Arg0, typename Visitor, typename Arg>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value
                        && std::is_lvalue_reference<Arg>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor, Arg && arg) {
    using result_type = typename function_traits<Visitor>::result_type;
    using T = typename std::remove_reference<Arg0>::type;
    using U = typename std::remove_reference<Arg>::type;

    result_type result;
    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
        static_assert(true, "Error: visit_impl_return(visitor, arg) invalid return type.");
    } else {
        static constexpr bool result_is_same = std::is_same<result_type, U>::value;
        static constexpr bool arg_is_same = std::is_same<T, U>::value;
        static constexpr bool arg_is_constructible = std::is_constructible<T, U>::value;
        static constexpr bool result_is_same_as_arg = std::is_same<result_type, T>::value;

        static constexpr bool isMoveSemantics = !std::is_lvalue_reference<Arg>::value;

        if (!std::is_same<result_type, void_type>::value &&
            !std::is_same<result_type, MonoState>::value) {
            if (arg_is_same || arg_is_constructible) {
                if (isMoveSemantics)
                    result = std::forward<Visitor>(visitor)(std::move(std::forward<U>(arg)));
                else
                    result = std::forward<Visitor>(visitor)(std::forward<U>(arg));
            } else {
                throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch. [visit_impl_return]");
            }

            if (result_is_same || result_is_same_as_arg) {
                arg = result;
            }
        } else {
            if (arg_is_same || arg_is_constructible) {
                if (isMoveSemantics)
                    std::forward<Visitor>(visitor)(std::move(std::forward<U>(arg)));
                else
                    std::forward<Visitor>(visitor)(std::forward<U>(arg));
            } else {
                throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch. [visit_impl_return]");
            }
        }
    }

    return result;
}
#endif

template <typename Arg0, typename Visitor, typename Arg>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value
                        && !std::is_lvalue_reference<Arg>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor, Arg && arg) {
    using result_type = typename function_traits<Visitor>::result_type;
    using T = typename std::remove_reference<Arg0>::type;
    using U = typename std::remove_reference<Arg>::type;

    result_type result;
    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
        static_assert(true, "Error: visit_impl_return(visitor, arg) invalid return type.");
    } else {
        static constexpr bool result_is_same = std::is_same<result_type, U>::value;
        static constexpr bool arg_is_same = std::is_same<T, U>::value;
        static constexpr bool arg_is_constructible = std::is_constructible<T, U>::value;
        static constexpr bool result_is_same_as_arg = std::is_same<result_type, T>::value;

        static constexpr bool isMoveSemantics = !std::is_lvalue_reference<Arg>::value;

        if (!std::is_same<result_type, void_type>::value &&
            !std::is_same<result_type, MonoState>::value) {
            if (arg_is_same || arg_is_constructible) {
                if (isMoveSemantics)
                    result = std::forward<Visitor>(visitor)(std::move(std::forward<U>(arg)));
                else
                    result = std::forward<Visitor>(visitor)(std::forward<U>(arg));
            } else {
                throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch. [visit_impl_return]");
            }

            if (result_is_same || result_is_same_as_arg) {
                arg = result;
            }
        } else {
            if (arg_is_same || arg_is_constructible) {
                if (isMoveSemantics)
                    std::forward<Visitor>(visitor)(std::move(std::forward<U>(arg)));
                else
                    std::forward<Visitor>(visitor)(std::forward<U>(arg));
            } else {
                throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch. [visit_impl_return]");
            }
        }
    }

    return result;
}

template <typename Arg0, typename Visitor, typename... Types>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor, Variant<Types...> && variant) {
    using result_type = typename function_traits<Visitor>::result_type;
    using T = typename std::remove_reference<Arg0>::type;

    result_type result;
    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
        static_assert(true, "Error: visit_impl_return(visitor, variant) invalid return type.");
    } else {
        static constexpr bool result_is_variant = std::is_same<result_type, Variant<Types...>>::value;
        static constexpr bool arg_is_variant = std::is_same<T, Variant<Types...>>::value;

        static constexpr bool isMoveSemantics = !std::is_lvalue_reference<Variant<Types...>>::value;

        bool result_is_holds_alternative = holds_alternative<result_type, Types...>(std::forward<Variant<Types...>>(variant));
        bool arg_is_holds_alternative = holds_alternative<T, Types...>(std::forward<Variant<Types...>>(variant));

        if (!std::is_same<result_type, void_type>::value &&
            !std::is_same<result_type, MonoState>::value) {
            if (arg_is_variant) {
                result = std::forward<Variant<Types...>>(variant).visit(std::forward<Visitor>(visitor));
            } else if (arg_is_holds_alternative) {
                result = std::forward<Visitor>(visitor)(get<T>(std::forward<Variant<Types...>>(variant)));
            } else {
                throw BadVariantAccess("Exception: jstd::visit(visitor, variant): Type T is dismatch. [visit_impl_return]");
            }

            if (result_is_variant || result_is_holds_alternative) {
                if (!isMoveSemantics) {
                    variant = result;
                }
            }
        } else {
            if (arg_is_variant) {
                std::forward<Variant<Types...>>(variant).visit(std::forward<Visitor>(visitor));
            } else if (arg_is_holds_alternative) {
                std::forward<Visitor>(visitor)(get<T>(std::forward<Variant<Types...>>(variant)));
            } else {
                throw BadVariantAccess("Exception: jstd::visit(visitor, variant): Type T is dismatch. [visit_impl_return]");
            }
        }
    }
    return result;
}

template <typename Arg0, typename Visitor, typename Arg, typename... Args>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor, Arg && arg, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    static constexpr bool isMoveSemantics = !std::is_lvalue_reference<Arg>::value;
    visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Arg>(arg));
    return visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Args>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor, Arg0 && arg0, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    static constexpr bool isMoveSemantics = !std::is_lvalue_reference<Arg0>::value;
    visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Arg0>(arg0));
    return visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Args, typename... Types>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor, Variant<Types...> && variant, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    static constexpr bool isMoveSemantics = !std::is_lvalue_reference<Variant<Types...>>::value;
    visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Variant<Types...>>(variant));
    return visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Args>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit_impl_return(Visitor && visitor, Args &&... args) {
    return std::forward<Visitor>(visitor)(std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(Visitor && visitor) {
}

template <typename Arg0, typename Visitor, typename Arg>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(Visitor & visitor, Arg & arg) {
    using T = typename std::remove_reference<Arg0>::type;
    using U = typename std::remove_reference<Arg>::type;

    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
    } else if (std::is_same<T, U>::value || std::is_constructible<T, U>::value) {
        visitor(arg);
    } else {
        throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch.");
    }
}

template <typename Arg0, typename Visitor, typename Arg>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(const Visitor & visitor, Arg & arg) {
    using T = typename std::remove_reference<Arg0>::type;
    using U = typename std::remove_reference<Arg>::type;

    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
    } else if (std::is_same<T, U>::value || std::is_constructible<T, U>::value) {
        visitor(arg);
    } else {
        throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch.");
    }
}

template <typename Arg0, typename Visitor, typename Arg>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type, void_type>::value
                        && std::is_lvalue_reference<Arg>::value, void>::type
visit_impl(Visitor && visitor, Arg && arg) {
    using T = typename std::remove_reference<Arg0>::type;
    using U = typename std::remove_reference<Arg>::type;

    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
    } else if (std::is_same<T, U>::value || std::is_constructible<T, U>::value) {
        std::forward<Visitor>(visitor)(std::forward<U>(arg));
    } else {
        throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch.");
    }
}

template <typename Arg0, typename Visitor, typename Arg>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type, void_type>::value
                        && !std::is_lvalue_reference<Arg>::value, void>::type
visit_impl(Visitor && visitor, Arg && arg) {
    using T = typename std::remove_reference<Arg0>::type;
    using U = typename std::remove_reference<Arg>::type;

    static constexpr bool isMoveSemantics = !std::is_lvalue_reference<Arg>::value;

    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
    } else if (std::is_same<T, U>::value || std::is_constructible<T, U>::value) {
        std::forward<Visitor>(visitor)(std::move(std::forward<U>(arg)));
    } else {
        throw BadVariantAccess("Exception: jstd::visit(visitor, arg): Type Arg is dismatch.");
    }
}

template <typename Arg0, typename Visitor, typename... Types>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(Visitor && visitor, Variant<Types...> && variant) {
    using T = typename std::remove_reference<Arg0>::type;
    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
    } else if (std::is_same<T, Variant<Types...>>::value) {
        std::forward<Variant<Types...>>(variant).visit(std::forward<Visitor>(visitor));
    } else if (holds_alternative<T, Types...>(std::forward<Variant<Types...>>(variant))) {
        std::forward<Visitor>(visitor)(get<T>(std::forward<Variant<Types...>>(variant)));
    } else {
        throw BadVariantAccess("Exception: jstd::visit(visitor, variant): Type T is dismatch.");
    }
}

template <typename Arg0, typename Visitor, typename Arg, typename... Args>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(Visitor && visitor, Arg && arg, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Arg>(arg));
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Args>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(Visitor && visitor, Arg0 && arg0, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Arg0>(arg0));
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Args, typename... Types>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(Visitor && visitor, Variant<Types...> && variant, Args &&... args) {
    using T = typename std::remove_reference<Arg0>::type;
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Variant<Types...>>(variant));
    visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
}

template <typename Arg0, typename Visitor, typename... Args>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit_impl(Visitor && visitor, Args &&... args) {
    std::forward<Visitor>(visitor)(std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit(Visitor && visitor, Args &&... args) {
    using Arg0 = typename function_traits<Visitor>::arg0;
    using result_type = typename function_traits<Visitor>::result_type;
    using T = typename std::remove_reference<Arg0>::type;
    static constexpr bool has_result_type = !std::is_same<result_type, void>::value &&
                                            !std::is_same<result_type, void_type>::value;
    result_type result;
    if (std::is_same<T, void>::value) {
        // No return
    } else if (std::is_same<T, void_type>::value ||
               std::is_same<T, MonoState>::value) {
        // No return
    } else {
        if (std::is_same<result_type, void_type>::value) {
            visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
        } else {
            result = visit_impl_return<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
        }
    }
    return result;
}

template <typename Visitor, typename... Args>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit(Visitor && visitor, Args &&... args) {
    using Arg0 = typename function_traits<Visitor>::template arguments<0>::type;
    using result_type = typename function_traits<Visitor>::result_type;
    using T = typename std::remove_reference<Arg0>::type;
    static constexpr bool has_result_type = !std::is_same<result_type, void>::value &&
                                            !std::is_same<result_type, void_type>::value;
    if (std::is_same<T, void>::value ||
        std::is_same<T, void_type>::value ||
        std::is_same<T, MonoState>::value) {
        // No return
    } else {
        visit_impl<T>(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
    }
}

template <typename Visitor>
typename std::enable_if<!std::is_same<typename function_traits<Visitor>::result_type, void_type>::value,
                        typename function_traits<Visitor>::result_type>::type
visit(Visitor && visitor) {
    using result_type = typename function_traits<Visitor>::result_type;
    return result_type();
}

template <typename Visitor>
typename std::enable_if<std::is_same<typename function_traits<Visitor>::result_type,
                        void_type>::value, void>::type
visit(Visitor && visitor) {
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
