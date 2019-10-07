#pragma once
#include <cstring>
#include <type_traits>
#include <array>

//-------------------------
namespace schm {

template<class To, class From>
auto bit_cast(const From &src) noexcept
    -> std::enable_if_t<(sizeof(To) == sizeof(From)) && std::is_trivially_copyable<From>::value
                            && std::is_trivial<To>::value,
                        To>
{
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}
//-------------------------
template<class T>
constexpr T *null = nullptr;
//-------------------------
// Former F;
// Schema S
// Member M;
//-------------------------

template<class M>
using type = typename M::type;

template<class M>
using parent = typename M::parent;

template<class M>
constexpr auto access(parent<M> &p) -> type<M> &
{
    return M::access(p);
}

template<class M>
constexpr auto access(const parent<M> &p) -> const type<M> &
{
    return M::access(p);
}

template<class M>
constexpr auto name() -> const char *
{
    return M::name;
}

//-------------------------
struct as_bool
{
    template<class M>
    using type = bool;
};

struct as_is
{
    template<class M>
    using type = type<M>;
};

//-------------------------
template<template<class F> class SC>
constexpr std::size_t get_member_number(SC<as_is> * = nullptr)
{
    return sizeof(SC<as_bool>);
}

template<class S>
constexpr std::size_t member_number = get_member_number(null<S>);

//-------------------------
template<class S, class T>
using members = std::array<T, member_number<S>>;


template<class S, class F>
struct reformator
{
    template<template<class> class SC>
    static auto test(SC<as_is> * = nullptr) -> SC<F>;
    using type = decltype(test(null<S>));
};

template<class S, class F>
using reform = typename reformator<S, F>::type;

//-------------------------
template<class R, class F>
struct make_transfomer
{
    struct former
    {
        template<class M>
        struct type
        {
            R v{F{}(M{})};
        };
    };
};
template<class T>
using former = typename T::former;

template<class S, class R, class F>
auto tranform(const F &/*f*/) -> members<S, R>
{
    return bit_cast<members<S, R>>(reform<S, former<make_transfomer<R, F>>>{});
}
//-------------------------

} // namespace schm

#define SCHM_REF(TYPE, NAME) \
struct _##NAME \
{ \
    using parent = _self; \
    using type = TYPE; \
    static constexpr const char *name = #NAME; \
    static constexpr type &access(parent& p) { return p.NAME; } \
    static constexpr const type & access(const parent & p) { return p.NAME; } \
}; \

//-------------------------
#define SCHM_BEG(NAME) \
    template<class F> \
    struct _##NAME; \
\
    using NAME = _##NAME<schm::as_is>; \
\
    template<class F> \
    struct _##NAME{ using _self = _##NAME<schm::as_is>;\

#define SCHM_MEM(T, NAME) SCHM_REF(T, NAME) typename F:: template type<_##NAME> NAME;
#define SCHM_END };
