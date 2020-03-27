#pragma once

#define SCHM_PRP SCHM_PROPERY_PASCAL_CASE

//#include "schm/schm.h"

#include <cstring>
#include <type_traits>
#include <array>
#include <functional> //hash

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

template<class M, class T>
constexpr auto access(T &p)
{
    auto &pp = static_cast<typename M::parent>(p);
    return access<M>(pp);
}

template<class M, class T>
constexpr auto access(const T &p)
{
    auto &pp = static_cast<const typename M::parent>(p);
    return access<M>(pp);
}

template<class M>
constexpr auto name() -> const char *
{
    return M::name;
}

//-------------------------

struct creation_tag
{};

template<class S>
constexpr S create()
{
    return S{creation_tag{}};
}

//-------------------------
struct as_bool
{
    template<class M>
    struct type
    {
        type(...) {}
        bool value;
    };
};

struct as_is
{
    template<class M>
    using type = type<M>;
};

//-------------------------
template<template<class> class S>
constexpr bool is_schema_test(S<as_is> *)
{
    return true;
}
constexpr bool is_schema_test(...)
{
    return false;
}

template<class S>
constexpr bool is_schema = is_schema_test(null<S>);

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
template<class F, class R = void (*)(F &)>
struct as_visitor
{
    template<class M>
    struct type
    {
        static void accept(F &f) { f(M{}); }
        type(...) {}
        R v{&accept};
    };
};

template<class S, class F>
void visit(F f)
{
    using VT = reform<S, as_visitor<F>>;
    auto visitor_funcs = bit_cast<schm::members<S, void (*)(F &)>>(schm::create<VT>());
    for (auto func : visitor_funcs) {
        func(f);
    }
}

//-------------------------
} // namespace schm



//#include "schm/name.h"

namespace schm {

template<class T>
constexpr const char *schema_get_name(T *);

template<class T>
constexpr const char *schema_name = schema_get_name(null<T>);

}

//#include "schm/macro_definition.h"
#define SCHM_NAME(NAME) NAME##_

//-------------------------
#define SCHM_MEM(T, MEM_NAME, INIT) \
    struct SCHM_NAME(MEM_NAME) \
    { \
        using parent = _self; \
        using type = T; \
\
        static constexpr const char *name = #MEM_NAME; \
        static constexpr type &access(parent &p) { return p.MEM_NAME; } \
        static constexpr const type &access(const parent &p) { return p.MEM_NAME; } \
\
        template<class U = schm::as_is> \
        struct redirect \
        { \
            using type = typename schm::reform<parent, U>::SCHM_NAME(MEM_NAME); \
        }; \
    }; \
    typename former_::template type<typename SCHM_NAME(MEM_NAME)::template redirect<>::type> MEM_NAME INIT

//#include "schm/macro_class.h"

//-------------------------
#define SCHM_CLASS(NAME) \
    template<class _##NAME##_FORMER> \
    class SCHM_NAME(NAME); \
\
    using NAME = SCHM_NAME(NAME)<schm::as_is>; \
\
    constexpr const char *schema_get_name(NAME *) { return #NAME; } \
\
    template<class former_> \
    class SCHM_NAME(NAME)

//-------------------------
#define SCHM_DEF_COMMON(NAME) \
\
    template<class T> \
    friend class SCHM_NAME(NAME); \
\
    using _self = SCHM_NAME(NAME)<schm::as_is>; \
\
    /*To access private base class*/ \
    template<class, class T_> \
    friend constexpr auto schm::access(T_ &); \
\
    template<class, class T_> \
    friend constexpr auto schm::access(const T_ &); \
\
    /*To access private schema constructor*/ \
    template<class S_> \
    friend constexpr S_ schm::create();

//-------------------------
#define SCHM_BASE(NAME) SCHM_NAME(NAME)<former_>

//-------------------------
#define SCHM_DEF(NAME) \
    SCHM_DEF_COMMON(NAME) \
\
protected: \
    SCHM_NAME(NAME)(schm::creation_tag) {} \
\
private:

//-------------------------
#define SCHM_DEF1(NAME, BASE) \
    SCHM_DEF_COMMON(NAME) \
\
protected: \
    SCHM_NAME(NAME)(schm::creation_tag tag) : SCHM_BASE(BASE){tag} {} \
\
private:

//-------------------------
#define SCHM_DEF2(NAME, BASE1, BASE2) \
    SCHM_DEF_COMMON(NAME) \
\
protected: \
    SCHM_NAME(NAME)(schm::creation_tag tag) : SCHM_BASE(BASE1){tag}, SCHM_BASE(BASE2){tag} {} \
\
private:
//-------------------------
#define SCHM_DEF3(NAME, BASE1, BASE2, BASE3) \
    SCHM_DEF_COMMON(NAME) \
\
protected: \
    SCHM_NAME(NAME) \
    (schm::creation_tag tag) : SCHM_BASE(BASE1){tag}, SCHM_BASE(BASE2){tag}, SCHM_BASE(BASE3){tag} \
    {} \
\
private:
//-------------------------

//#include "schm/macro_property.h"

#define SCHM_PROP_IMPL(GET_NAME, SET_NAME, T, MEM_NAME, INIT) \
\
public: \
    T &GET_NAME() { return MEM_NAME; } \
    auto &Set##PROP_NAME(T value) \
    { \
        MEM_NAME = std::move(value); \
        return *this; \
    } \
\
private: \
    SCHM_MEM(T, MEM_NAME, INIT);

#define SCHM_PROPERY_PASCAL_CASE(T, PROP_NAME, MEM_NAME, INIT) \
    SCHM_PROP_IMPL(Get##PROP_NAME, Set##PROP_NAME, T, MEM_NAME, INIT)

#define SCHM_PROPERY_CAMAL_CASE(T, PROP_NAME, MEM_NAME, INIT) \
    SCHM_PROP_IMPL(get##PROP_NAME, set##PROP_NAME, T, MEM_NAME, INIT)

#define SCHM_PROPERY_SNAKE_CASE(T, PROP_NAME, MEM_NAME, INIT) \
    SCHM_PROP_IMPL(get_##PROP_NAME, set_##PROP_NAME, T, MEM_NAME, INIT)

//-------------------------

//#include "schm/operator.h"
namespace schm
{
template<class S, typename = std::enable_if_t<schm::is_schema<S>>>
constexpr bool operator==(const S &lhs, const S &rhs)
{
    bool res = true;
    visit<S>([&](auto m) { res &= m.access(lhs) == m.access(rhs); });
    return res;
}

template<class S, typename = std::enable_if_t<schm::is_schema<S>>>
constexpr bool operator!=(const S &lhs, const S &rhs)
{
    return !(lhs == rhs);
}

template<class S, typename = std::enable_if_t<schm::is_schema<S>>>
constexpr bool operator<(const S &lhs, const S &rhs)
{
    bool res = true;
    visit<S>([&](auto m) { res &= m.access(lhs) < m.access(rhs); });
    return res;
}

}

//#include "schm/hash.h"
namespace std
{
template<template<class> class SC>
struct hash<SC<schm::as_is>>
{
    using argument_type = SC<schm::as_is>;
    using result_type = std::size_t;

    constexpr result_type operator()(const argument_type &s) const noexcept
    {

        result_type seed{0};
        schm::visit<SC<schm::as_is>>([&](auto m) {
            //https://www.nullptr.me/2018/01/15/hashing-stdpair-and-stdtuple/
            seed ^= std::hash<schm::type<decltype (m)>>{}(m.access(s)) + 0x9e3779b9 + (seed << 6)
                    + (seed >> 2);
        });
        return seed;
    }

};

} // namespace std

//#include "schm/cout.h"
#include <iostream>

template<class S, class = std::enable_if_t<schm::is_schema<S>>>
std::ostream &operator<<(std::ostream &out, const S &s)
{
    out << "(" << schm::schema_name<S>;
    schm::visit<S>([&out, &s](auto m) { out << '(' << m.name << ":" << m.access(s) << ')'; });
    out << ")";
    return out;
}


//qdebug
#include <QDebug>
template<class S, class = std::enable_if_t<schm::is_schema<S>>>
QDebug operator<< (QDebug out, const S &s) {
    out << "(" << schm::schema_name<S>;
    schm::visit<S>([&out, &s](auto m) { out << '(' << m.name << ":" << m.access(s) << ')'; });
    out << ")";
    return out;
}


