
#ifndef JSTD_APPLY_VISITOR_H
#define JSTD_APPLY_VISITOR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <type_traits>

namespace jstd {

///////////////////////////////////////////////////////////////////////////////////////

namespace detail {

struct faked_return_void
{
    faked_return_void() noexcept {
    }

    template <typename T>
    faked_return_void(const T &) noexcept {
    }

    template <typename T>
    faked_return_void(T &&) noexcept {
    }
};

#if (JSTD_IS_CPP_14 == 0)
bool operator == (faked_return_void, faked_return_void) noexcept { return true;  }
bool operator != (faked_return_void, faked_return_void) noexcept { return false; }
bool operator  < (faked_return_void, faked_return_void) noexcept { return false; }
bool operator  > (faked_return_void, faked_return_void) noexcept { return false; }
bool operator <= (faked_return_void, faked_return_void) noexcept { return true;  }
bool operator >= (faked_return_void, faked_return_void) noexcept { return true;  }
#else
constexpr bool operator == (faked_return_void, faked_return_void) noexcept { return true;  }
constexpr bool operator != (faked_return_void, faked_return_void) noexcept { return false; }
constexpr bool operator  < (faked_return_void, faked_return_void) noexcept { return false; }
constexpr bool operator  > (faked_return_void, faked_return_void) noexcept { return false; }
constexpr bool operator <= (faked_return_void, faked_return_void) noexcept { return true;  }
constexpr bool operator >= (faked_return_void, faked_return_void) noexcept { return true;  }
#endif

template <typename T>
struct return_type_wrapper
{
    typedef T type;
};

template <>
struct return_type_wrapper<void>
{
    typedef faked_return_void type;
};

template <typename T>
struct has_result_type
{
private:
    typedef char Yes;
    typedef struct {
        char array[2];
    } No;

    template <typename Class>
    static Yes test(typename std::remove_reference<typename Class::result_type>::type *);

    template <typename Class>
    static No  test(...);

public:
    static constexpr bool value = (sizeof(test<T>(0)) == sizeof(Yes));
};

struct is_static_visitor_tag {
};

template <typename T>
struct is_static_visitor_impl
{
    static constexpr bool value = std::is_base_of<is_static_visitor_tag, T>::value;
};

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////

template <typename ReturnType = void>
struct static_visitor : public detail::is_static_visitor_tag
{
    typedef ReturnType result_type;

    static_visitor() = default;
    ~static_visitor() = default;
};

template <typename T>
struct is_static_visitor : public std::integral_constant<bool, detail::is_static_visitor_impl<T>::value> {
};

///////////////////////////////////////////////////////////////////////////////////////

template <typename BinaryVisitor, typename Value1, bool IsMoveSemantics>
class apply_visitor_binary_invoke
{
public:
    typedef typename BinaryVisitor::result_type result_type;

private:
    BinaryVisitor & visitor_;
    Value1 &        value1_;

public:
    apply_visitor_binary_invoke(BinaryVisitor & visitor, Value1 & value1) noexcept
        : visitor_(visitor), value1_(value1) {
    }

    apply_visitor_binary_invoke(BinaryVisitor && visitor, Value1 && value1) noexcept
        : visitor_(std::forward<BinaryVisitor>(visitor)), value1_(std::forward<Value1>(value1)) {
    }

    template <typename Value2>
    typename std::enable_if<(IsMoveSemantics && std::is_same<Value2, Value2>::value),
        typename detail::return_type_wrapper<result_type>::type
    >::type
    operator () (Value2 && value2)
    {
        return visitor_(std::move(value1_), std::forward<Value2>(value2));
    }

    template <typename Value2>
    typename std::enable_if<!(IsMoveSemantics && std::is_same<Value2, Value2>::value),
        typename detail::return_type_wrapper<result_type>::type
    >::type
    operator () (Value2 && value2)
    {
        return visitor_(value1_, std::forward<Value2>(value2));
    }

private:
    apply_visitor_binary_invoke & operator = (const apply_visitor_binary_invoke &);
};

///////////////////////////////////////////////////////////////////////////////////////

template <typename BinaryVisitor, typename Visitable2, bool IsMoveSemantics>
class apply_visitor_binary_unwrap
{
public:
    typedef typename BinaryVisitor::result_type result_type;

private:
    BinaryVisitor & visitor_;
    Visitable2 &    visitable2_;

public:
    apply_visitor_binary_unwrap(BinaryVisitor & visitor, Visitable2 & visitable2) noexcept
        : visitor_(visitor), visitable2_(visitable2) {
    }

    template <typename Value1>
    typename std::enable_if<(IsMoveSemantics && std::is_same<Value1, Value1>::value),
        typename detail::return_type_wrapper<result_type>::type
    >::type
    operator () (Value1 && value1)
    {
        apply_visitor_binary_invoke<BinaryVisitor, Value1,
            !std::is_lvalue_reference<Value1>::value> invoker(visitor_, value1);
        return apply_visitor(invoker, std::move(visitable2_));
    }

    template <typename Value1>
    typename std::enable_if<!(IsMoveSemantics && std::is_same<Value1, Value1>::value),
        typename detail::return_type_wrapper<result_type>::type
    >::type
    operator () (Value1 && value1)
    {
        apply_visitor_binary_invoke<BinaryVisitor, Value1,
            !std::is_lvalue_reference<Value1>::value> invoker(visitor_, value1);
        return apply_visitor(invoker, visitable2_);
    }

private:
    apply_visitor_binary_unwrap & operator = (const apply_visitor_binary_unwrap &);
};

///////////////////////////////////////////////////////////////////////////////////////

//
// non-const visitor version:
//
template <typename BinaryVisitor, typename Visitable1, typename Visitable2>
inline
typename std::enable_if<!std::is_const<BinaryVisitor>::value,
                        typename detail::return_type_wrapper<
                            typename BinaryVisitor::result_type
                          >::type
                       >::type
apply_visitor(BinaryVisitor & visitor, Visitable1 && visitable1, Visitable2 && visitable2)
{
    apply_visitor_binary_unwrap<BinaryVisitor, Visitable2,
        !std::is_lvalue_reference<Visitable2>::value> unwrapper(visitor, visitable2);
    return apply_visitor(unwrapper, std::forward<Visitable1>(visitable1));
}

//
// const visitor version:
//
template <typename BinaryVisitor, typename Visitable1, typename Visitable2>
inline
typename detail::return_type_wrapper<typename BinaryVisitor::result_type>::type
apply_visitor(const BinaryVisitor & visitor, Visitable1 && visitable1, Visitable2 && visitable2)
{
    apply_visitor_binary_unwrap<const BinaryVisitor, Visitable2,
        !std::is_lvalue_reference<Visitable2>::value> unwrapper(visitor, visitable2);
    return apply_visitor(unwrapper, std::forward<Visitable1>(visitable1));
}

///////////////////////////////////////////////////////////////////////////////////////

//
// non-const visitor version:
//
template <typename Visitor, typename Visitable>
inline
typename std::enable_if<!std::is_const<Visitor>::value,
                        typename detail::return_type_wrapper<
                            typename Visitor::result_type
                          >::type
                       >::type
apply_visitor(Visitor & visitor, Visitable && visitable)
{
    return std::forward<Visitable>(visitable).apply_visitor(visitor);
}

//
// const visitor version:
//
template <typename Visitor, typename Visitable>
inline
typename detail::return_type_wrapper<typename Visitor::result_type>::type
apply_visitor(const Visitor & visitor, Visitable && visitable)
{
    return std::forward<Visitable>(visitable).apply_visitor(visitor);
}

///////////////////////////////////////////////////////////////////////////////////////

template <typename Visitor>
class apply_visitor_delayed_t
{
public:
    typedef typename Visitor::result_type result_type;

private:
    Visitor & visitor_;

public:
    explicit apply_visitor_delayed_t(Visitor & visitor) noexcept
      : visitor_(visitor) {
    }

    template <typename... Visitables>
    result_type operator () (Visitables &&... visitables) const
    {
        return apply_visitor(visitor_, std::forward<Visitables>(visitables)...);
    }

private:
    apply_visitor_delayed_t & operator = (const apply_visitor_delayed_t &);
};

///////////////////////////////////////////////////////////////////////////////////////

} // namespace jstd

#endif // JSTD_APPLY_VISITOR_H
