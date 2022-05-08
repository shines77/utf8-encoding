
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

namespace jstd {

// std::variant_npos
static const std::size_t VariantNPos = (std::size_t)-1;

// Like std::monostate
struct MonoState {
#if 1
    MonoState() noexcept {}
    MonoState(const MonoState & src) noexcept {}

    template <typename T>
    MonoState(const T & src) noexcept {}

    ~MonoState() {}
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

// std::variant_alternative
struct VariantAlternative {
    //
};

// For generic types, directly use the result of the signature of its 'operator()'
template <typename T>
struct function_traits : public function_traits<decltype(&T::operator ())> {
};

// we specialize for pointers to member function
template <typename T, typename ReturnType, typename... Args>
struct function_traits<ReturnType(T::*)(Args...) const> {
    // arity is the number of arguments.
    enum {
        arity = sizeof...(Args)
    };

    typedef ReturnType result_type;

    template <std::size_t i>
    struct arg {
        // The i-th argument is equivalent to the i-th tuple element of a tuple
        // composed of those arguments.
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };

    typedef std::function<ReturnType(Args...)> FuncType;
    typedef std::tuple<Args...> ArgTupleType;
};

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
struct IsArray<T const[N]> : std::true_type {
};

template <typename T>
struct IsArray<T const[]> : std::true_type {
};

template <typename T, typename Base>
struct IsSame : std::is_same<T, Base> {
};

template <std::size_t Len>
struct IsSame<char[Len], char *> : std::true_type {
};

template <std::size_t Len>
struct IsSame<const char[Len], const char *> : std::true_type {
};

template <std::size_t Len>
struct IsSame<wchar_t[Len], wchar_t *> : std::true_type {
};

template <std::size_t Len>
struct IsSame<const wchar_t[Len], const wchar_t *> : std::true_type {
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
struct Index : std::integral_constant<std::size_t, sizeof...(Types) - GetLeftSize<T, Types...>::value - 1> {
};

// Forward declaration
template <std::size_t index, typename... Types>
struct IndexType;

// Declaration
template <std::size_t index, typename T, typename... Types>
struct IndexType<index, T, Types...> : IndexType<index - 1, Types...> {
};

// Specialized
template <typename T, typename... Types>
struct IndexType<0, T, Types...>
{
    typedef T DataType;
};

template <typename... Types>
struct GetTypeCount : std::integral_constant<std::size_t, sizeof...(Types)> {
};

template <typename T, std::size_t N, typename... Types>
struct GetIndex {
};

template <typename T, std::size_t N, typename First, typename... Types>
struct GetIndex<T, N, First, Types...> {
    static const std::size_t value = GetIndex<T, N + 1, Types...>::value;
};

template <typename T, std::size_t N, typename... Types>
struct GetIndex<T, N, T, Types...> {
    static const std::size_t value = N;
};

#if 1
template <std::size_t Len, std::size_t N, typename... Types>
struct GetIndex<char[Len], N, char *, Types...> {
    static const std::size_t value = N;
};

template <std::size_t Len, std::size_t N, typename... Types>
struct GetIndex<wchar_t[Len], N, wchar_t *, Types...> {
    static const std::size_t value = N;
};
#endif

#if 1
template <std::size_t Len, std::size_t N, typename... Types>
struct GetIndex<const char[Len], N, const char *, Types...> {
    static const std::size_t value = N;
};

template <std::size_t Len, std::size_t N, typename... Types>
struct GetIndex<const wchar_t[Len], N, const wchar_t *, Types...> {
    static const std::size_t value = N;
};
#endif

template <typename T, std::size_t N>
struct GetIndex<T, N> {
    static const std::size_t value = VariantNPos;
};

template <std::size_t N, typename... Types>
struct GetType {
};

template <std::size_t N, typename T, typename... Types>
struct GetType<N, T, Types...> {
    using type = typename GetType<N - 1, Types...>::type;
};

template <typename T, typename... Types>
struct GetType<0, T, Types...> {
    using type = T;
};

template <class T>
struct TypeFilter {
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

template <class T>
struct TypeFilterNoPtr {
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
    int compare(std::type_index old_type, void * old_val, void * new_val) {
        if (old_type == std::type_index(typeid(T))) {
            if (*reinterpret_cast<T *>(new_val) > *reinterpret_cast<T *>(old_val))
                return 1;
            else if (*reinterpret_cast<T *>(new_val) < *reinterpret_cast<T *>(old_val))
                return -1;
            else
                return 0;
        } else {
            return VariantHelper<Types...>::compare(old_type, old_val, new_val);
        }
    }
};

template <>
struct VariantHelper<>  {
    static inline void destroy(std::type_index type_id, void * data) {}
    static inline void copy(std::type_index old_type, const void * old_val, void * new_val) {}
    static inline void move(std::type_index old_type, void * old_val, void * new_val) {}

    static inline void swap(std::type_index old_type, void * old_val, void * new_val) {}
    static inline void compare(std::type_index old_type, void * old_val, void * new_val) {}
};

//
// See: https://www.cnblogs.com/qicosmos/p/3559424.html
// See: https://www.jianshu.com/p/f16181f6b18d
// See: https://en.cppreference.com/w/cpp/utility/variant
// See: https://github.com/mpark/variant/blob/master/include/mpark/variant.hpp
//

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

    static const std::size_t kMaxType = GetTypeCount<Types...>::value;

protected:
    data_t          data_;
    std::size_t     index_;
    std::type_index type_index_;

public:
    Variant() noexcept : index_(VariantNPos), type_index_(typeid(void)) {
    }

    template <typename T,
              typename = typename std::enable_if<ContainsType<typename jstd::TypeFilter<T>::type, Types...>::value>::type>
    Variant(const T & value) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename jstd::TypeFilter<T>::type U;
        this->print_type_info<T, U>("Variant(const T & value)");

        new (&this->data_) U(value);
        this->index_ = this->index_of<U>();
        this->type_index_ = typeid(U);
    }

    template <typename T,
              typename = typename std::enable_if<ContainsType<typename jstd::TypeFilter<T>::type, Types...>::value>::type>
    Variant(T && value) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename jstd::TypeFilter<T>::type U;
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

#if 1
    template <typename T, typename... Args>
    Variant(const T & type, Args &&... args) : index_(VariantNPos), type_index_(typeid(void)) {
        typedef typename jstd::TypeFilter<T>::type U;
        this->print_type_info<T, U>("Variant(const T & type, Args &&... args)");

        new (&this->data_) U(std::forward<Args>(args)...);
        this->index_ = this->index_of<U>();
        this->type_index_ = typeid(U);
    }
#endif

    Variant(const this_type & src) : index_(src.index_), type_index_(src.type_index_) {
        helper_type::copy(src.type_index_, &src.data_, &this->data_);
    }

    Variant(this_type && src) : index_(src.index_), type_index_(src.type_index_) {
        helper_type::move(src.type_index_, &src.data_, &this->data_);
    }

    virtual ~Variant() {
        helper_type::destroy(this->type_index_, &this->data_);
        this->index_ = VariantNPos;
        this->type_index_ = typeid(void);
    }

    template <typename T, typename U>
    void print_type_info(const std::string & title, bool T_is_main = true) {
#ifdef _DEBUG
        printf("%s;\n\n", title.c_str());
        printf("typeid(T).name() = %s\n", typeid(T).name());
        printf("typeid(U).name() = %s\n", typeid(U).name());
        printf("typeid(std::remove_const<T>::type).name() = %s\n", typeid(typename std::remove_const<T>::type).name());
        if (T_is_main) {
            printf("ContainsType<T>::value = %u\n", (uint32_t)ContainsType<typename jstd::TypeFilter<T>::type, Types...>::value);
            printf("std::is_array<T>::value = %u\n", (uint32_t)std::is_array<typename std::remove_const<T>::type>::value);
            printf("IsArray<T>::value = %u\n", (uint32_t)IsArray<T>::value);
        } else {
            printf("ContainsType<U>::value = %u\n", (uint32_t)ContainsType<typename jstd::TypeFilter<U>::type, Types...>::value);
            printf("std::is_array<U>::value = %u\n", (uint32_t)std::is_array<typename std::remove_const<U>::type>::value);
            printf("IsArray<U>::value = %u\n", (uint32_t)IsArray<U>::value);
        }
        printf("\n");
#endif
    }

#if 0
    template <typename T>
    this_type & operator = (const T & rhs) {
        typedef typename jstd::TypeFilter<T>::type U;
        helper_type::destroy(this->type_index_, &this->data_);
        std::type_index new_type_index = std::type_index(typeid(U));
        helper_type::copy(new_type_index, &rhs, &this->data_);
        this->index_ = this->index_of<U>();
        this->type_index_ = new_type_index;
        return *this;
    }

    template <typename T>
    this_type & operator = (T && rhs) {
        typedef typename jstd::TypeFilter<T>::type U;
        helper_type::destroy(this->type_index_, &this->data_);
        std::type_index new_type_index = std::type_index(typeid(U));
        helper_type::move(new_type_index, &rhs, &this->data_);
        this->index_ = this->index_of<U>();
        this->type_index_ = new_type_index;
        return *this;
    }
#endif

    this_type & operator = (const this_type & rhs) {
        helper_type::destroy(this->type_index_, &this->data_);
        helper_type::copy(rhs.type_index_, &rhs.data_, &this->data_);
        this->index_ = rhs.index_;
        this->type_index_ = rhs.type_index_;
        return *this;
    }

    this_type & operator = (this_type && rhs) {
        helper_type::destroy(this->type_index_, &this->data_);
        helper_type::move(rhs.type_index_, &rhs.data_, &this->data_);
        this->index_ = rhs.index_;
        this->type_index_ = rhs.type_index_;
        return *this;
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
        return GetIndex<T, 0, Types...>::value;
    }

    inline bool valueless_by_exception() const noexcept {
        std::size_t index = this->index();
        return (index == VariantNPos || index >= kMaxType);
    }

    inline bool is_assigned() const noexcept {
#if 1
        std::size_t index = this->index();
        return (index != VariantNPos);
#else
        return (this->type_index_ != type_index(typeid(void)));
#endif
    }

    template <typename T>
    inline bool is_valid_type() const noexcept {
        using U = typename jstd::TypeFilter<T>::type;
        std::size_t index = this->index_of<U>();
        if (index == this->index()) {
            return !this->valueless_by_exception();
        } else {
            return false;
        }
    }

    template <typename T>
    inline bool check_type_index() const noexcept {
        using U = typename jstd::TypeFilter<T>::type;
        if (this->type_index_ == std::type_index(typeid(U))) {
            return !this->valueless_by_exception();
        } else {
            return false;
        }
    }

    template <typename T>
    inline void check_valid_type(const char * name) {
        if (!this->is_valid_type<T>()) {
            std::cout << "Variant<Types...>::" << name << " exception:" << std::endl << std::endl;
            std::cout << "Type [" << typeid(T).name() << "] is not defined. " << std::endl;
            std::cout << "Current type is [" << this->type_index().name() << "], index is "
                      << (std::intptr_t)this->index() << "." << std::endl << std::endl;
            throw std::bad_cast();
        }
    }

    template <typename T, std::size_t N>
    inline void check_valid_type(char (&name)[N]) {
        return this->check_valid_type<T>(name);
    }

    template <std::size_t N>
    void init() {
        if (this->index_ == VariantNPos) {
            using T = typename GetType<N, Types...>::type;
            typedef typename jstd::TypeFilter<T>::type U;
            if (this->is_valid_type<U>()) {
                helper_type::destroy(this->type_index_, &this->data_);

                new (&this->data_) U();
                this->index_ = N;
                this->type_index_ = typeid(U);
            }
        }
    }

    template <typename T>
    typename jstd::TypeFilter<T>::type & get() {
        using U = typename jstd::TypeFilter<T>::type;
        this->check_valid_type<U>("get<T>()");
        return *((U *)(&this->data_));
    }

    template <typename T>
    const typename jstd::TypeFilter<T>::type & get() const {
        using U = typename jstd::TypeFilter<T>::type;
        this->check_valid_type<U>("get<T>()");
        return *((U *)(&this->data_));
    }

    template <std::size_t N>
    typename GetType<N, Types...>::type & get() {
        using U = typename GetType<N, Types...>::type;
        this->check_valid_type<U>("get<N>()");
        return *((U *)(&this->data_));
    }

    template <std::size_t N>
    const typename GetType<N, Types...>::type & get() const {
        using U = typename GetType<N, Types...>::type;
        this->check_valid_type<U>("get<N>()");
        return *((U *)(&this->data_));
    }

    template <typename T>
    void set(const T & value) {
        using U = typename jstd::TypeFilter<T>::type;
        this->check_valid_type<U>("set<T>(const T & value)");
        *((U *)(&this->data_)) = value;
    }

    template <typename T>
    void set(T && value) {
        using U = typename jstd::TypeFilter<T>::type;
        this->check_valid_type<U>("set<T>(T && value)");
        *((U *)(&this->data_)) = std::forward<U>(value);
    }

    template <std::size_t N>
    void set(const typename GetType<N, Types...>::type & value) {
        using U = typename GetType<N, Types...>::type;
        this->check_valid_type<U>("set<N>(const T & value)");
        *((U *)(&this->data_)) = value;
    }

    template <std::size_t N>
    void set(typename GetType<N, Types...>::type && value) {
        using U = typename GetType<N, Types...>::type;
        this->check_valid_type<U>("set<N>(T && value)");
        *((U *)(&this->data_)) = std::forward<U>(value);
    }

    template <typename Func>
    void visit(Func && func) {
        using T = typename function_traits<Func>::arg<0>::type;
        if (this->is_valid_type<T>()) {
            func(this->get<T>());
        }
    }

    template <typename Func, typename... Args>
    void visit(Func && func, Args &&... args) {
        using T = typename function_traits<Func>::arg<0>::type;
        if (this->is_valid_type<T>())
            this->visit(std::forward<Func>(func));
        else
            this->visit(std::forward<Args>(args)...);
    }
};

template <typename T, typename... Types>
static bool holds_alternative(const Variant<Types...> & var) noexcept {
    using U = typename jstd::TypeFilter<T>::type;
    return var.template is_valid_type<U>();
}

template <std::size_t N, typename... Types>
typename GetType<N, Types...>::type & get(Variant<Types...> & var) {
    return var.template get<N>();
}

template <std::size_t N, typename... Types>
const typename GetType<N, Types...>::type & get(const Variant<Types...> & var) {
    return var.template get<N>();
}

// std::variant_size
template <typename T>
struct VariantSize;

// std::variant_size
template <typename... Types>
struct VariantSize<Variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {
};

template <typename T>
constexpr std::size_t VariantSize_v = VariantSize<T>::value;

template <typename T>
using VariantSize_t = typename VariantSize<T>::type;

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