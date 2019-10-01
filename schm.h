#ifndef SCHM_H
#define SCHM_H

#include <string_view>

// FIXME enclosing namespace
template <template <typename...> typename Template, typename Type>
struct is_instance_of : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_instance_of<Template, Template<Args...>> : std::true_type {};

template<template<typename...> typename Template, typename Type>
constexpr bool is_instance_of_v = is_instance_of<Template, Type>::value;
//-----------------------------------------------------------------------------
#define ZUT_SCHM_MEMBER_INFO class

#define ZUT_SCHM_FORMER class
#define ZUT_SCHM_TEMPLATED_FORMER                                                                                                          \
  template <ZUT_SCHM_MEMBER_INFO>                                                                                                          \
  class
#define ZUT_SCHM_SCHEMA                                                                                                                    \
  template <ZUT_SCHM_FORMER>                                                                                                               \
  class // S<AsIs> - default constructible;
#define ZUT_SCHM_NAME_TYPE class
#define ZUT_SCHM_MEMBER_NAME(name) ::schm::Name<name>

//-----------------------------------------------------------------------------
// TODO concept IsFormer (has traits)
// TODO concept IsFormerTraits (defines type and init funcs)

//-----------------------------------------------------------------------------
namespace schm
{


struct Is;
template<ZUT_SCHM_FORMER F>
struct As;

template <ZUT_SCHM_SCHEMA S, class T, ZUT_SCHM_NAME_TYPE Name, class... Constraints>
struct MakeMemberInfo;


  template <class T>
  concept MemberInfo = /*is_instance_of_v<MakeMemberInfo, T>*/ true;

  template <template <class> class S>
  concept Schema = true; // TODO implement
} // namespace schm

namespace schm::detail
{
  /// constexpr string wrapper, to be used as template string arg
  template <std::size_t N>
  struct FixedString {
    constexpr explicit(false) FixedString(const char (&foo)[N]) {
      for (size_t i{0}; i < N; ++i) {
        m_data[i] = foo[i];
      }
    }
    constexpr std::string_view view() const { return {m_data}; }

  private:
    char m_data[N]{};
  };

  template <std::size_t N>
  FixedString(const char (&str)[N])->FixedString<N>;

} // namespace schm::detail

namespace schm
{
  /// Temporary workaround since FixedString cannot be used direclty due to gcc crash
  template <detail::FixedString input>
  struct Name : std::string_view {
    constexpr Name() : std::string_view(input.view()) {}
  };
  //-----------------------------------------------------------------------------
  template <ZUT_SCHM_SCHEMA S, ZUT_SCHM_FORMER F, class T, ZUT_SCHM_NAME_TYPE Name, class... Conatraints>
  using m_t = typename F::template type<MakeMemberInfo<S, T, Name, Conatraints...>>;
  //-----------------------------------------------------------------------------
} // namespace schm

#include "function_detect_traits.h"

namespace schm
{
//-----------------------------------------------------------------------------
// TODO rewrite using concept?
    namespace detail {
    struct has_init_func_impl
    {
        template<ZUT_SCHM_FORMER>
        struct Foo
        {};
        using DummyMI = MakeMemberInfo<Foo, int, Name<"memb">>;
        template<class T>
        auto require(T &) -> decltype(&T::template init<DummyMI>);
    };

    template <class T>
    constexpr bool has_init_func_v = zut::macro_detail::is_acceptable_v<detail::has_init_func_impl, T>;

    //-----------------------------------------------------------------------------
    template <typename...>
    struct typelist;

    template <class T, class... Args> // FIXME base class constraint
    constexpr bool HidesCopyMoveConstructors = std::is_same_v<typelist<T>, typelist<std::decay_t<Args>...>>;
  } // namespace detail

  //-----------------------------------------------------------------------------

  namespace detail
  {
    template <class T>
    struct PrimitiveHolder {
      T _{};
    };

    template <class B>
    using FormerWrapperBase = std::conditional_t<std::is_class_v<B>, B, PrimitiveHolder<B>>;
  } // namespace detail

  //! A former wrapper. Provides variadic template constructor, initialize base Former via its init method if exists
  template <class B, class MI> // TODO Former constraits: default constructible
  struct FormerWrapper : detail::FormerWrapperBase<B> {
    template <class... Args, typename = std::enable_if_t<!detail::HidesCopyMoveConstructors<FormerWrapper, Args...>>>
    constexpr explicit FormerWrapper(Args &&...) {
      //      detail::FormerWrapperBase<B> ::template init<MI>(*this); // TODO customization point via template specialization
      if constexpr (detail::has_init_func_v<detail::FormerWrapperBase<B>>) {
        detail::FormerWrapperBase<B>::template init<MI>(*this); // TODO customization point via template specialization
      };
    }

    FormerWrapper(const FormerWrapper &) = default;
    FormerWrapper(FormerWrapper &&) = default;
  };
} // namespace schm
  //-----------------------------------------------------------------------------

namespace schm
{
  //-----------------------------------------------------------------------------
  template <ZUT_SCHM_FORMER F>
  struct As {
    template <MemberInfo MI>
    using type = FormerWrapper<F, MI>;
  };
  //-----------------------------------------------------------------------------
  template <ZUT_SCHM_TEMPLATED_FORMER F>
  struct AsT {
    template <MemberInfo MI>
    using type = FormerWrapper<F<MI>, MI>;
  };

  //-----------------------------------------------------------------------------
  struct Is {};

  template <>
  struct As<Is> {
    template <MemberInfo MI>
    using type = typename MI::type;
  };

  using AsIs = As<Is>;
  //-----------------------------------------------------------------------------
} // namespace schm

//-----------------------------------------------------------------------------
//                         Usage
//-----------------------------------------------------------------------------

#include <array>
#include <cstddef>

namespace schm
{
  //-----------------------------------------------------------------------------
  //! Provides the number of membes in the schema
  template <ZUT_SCHM_SCHEMA S>
  constexpr std::size_t MembersCount = sizeof(S<As<std::byte>>);

  //-----------------------------------------------------------------------------
  //! An array of type T with the size of members count of the schame S
  template <ZUT_SCHM_SCHEMA S, class T>
  using Members = std::array<T, MembersCount<S>>;
} // namespace schm

namespace schm::detail
{
  struct SizeHolder {
    std::size_t size;

    template <MemberInfo MI>
    constexpr static void init(SizeHolder &sh) {
      sh.size = sizeof(typename MI::type);
    }
  };

} // namespace schm::detail

namespace schm::detail
{
  template <ZUT_SCHM_SCHEMA S>
  struct OffsetCalculator {
    template <class... Args>
    OffsetCalculator(Args &&...) {
      new (&un.src) Src(); // object creation forces offsets computatuin
    }

    Members<S, std::ptrdiff_t> calculate() { return offsets; }

  private:
    template <class MI>
    struct AddressProvider; // fwd

    using Src = S<AsT<AddressProvider>>;

    inline static Members<S, std::ptrdiff_t> offsets{}; // a collection of member offsets
    union Un {
      Src src;
      Un() {}
      ~Un() { src.~Src(); } // assumes Un is always inialized
    };
    inline static Un un;
    inline static void *startAddress = &un;
    inline static std::size_t idx{0};

    template <class MI>
    struct AddressProvider {
      union {
        typename MI::type v;
      };

      AddressProvider() {
        if (idx < offsets.size()) {
          offsets[idx++] = reinterpret_cast<const std::byte *>(this) - reinterpret_cast<const std::byte *>(startAddress);
        }
      }
    };
  };
} // namespace schm::detail

//-----------------------------------------------------------------------------
#include <cstring> //std::memcpy
namespace schm
{
  //! helper function for convering the schema instance to array of underlying type;
  template <ZUT_SCHM_SCHEMA S, ZUT_SCHM_FORMER F, class Res = F>
  Members<S, Res> AsMembers() {
    Members<S, Res> res;
    S<As<F>> src;
    // FIXME bit_cast here
    std::memcpy((void *)&res, (const void *)&src, sizeof(src));
    return res;
  }

  template <ZUT_SCHM_SCHEMA S>
  Members<S, std::ptrdiff_t> Offsets = detail::OffsetCalculator<S>().calculate(); // a collection of member offsets

  template <ZUT_SCHM_SCHEMA S>
  Members<S, std::size_t> Sizes = AsMembers<S, detail::SizeHolder, std::size_t>(); // a collection of member sizes

} // namespace schm

namespace schm
{
  //-----------------------------------------------------------------------------
  //! Describes the member's position inside it's schema
  struct MemberLayout {
    std::ptrdiff_t offset;
    std::size_t size;
    std::size_t index;
  };

  namespace detail
  {
    template <ZUT_SCHM_SCHEMA S>
    struct MemberLayoutOf : MemberLayout {
      template <MemberInfo MI>
      static void init(MemberLayoutOf &self) {
        self.size = sizeof(MI::type);
      }
    };

  } // namespace detail

  //-----------------------------------------------------------------------------
  template <ZUT_SCHM_SCHEMA S>
  const Members<S, MemberLayout> MembersLayout = [] {
    using namespace detail;
    Members<S, MemberLayout> res{};

    for (std::size_t i = 0; i < res.size(); ++i) {
      res[i].index = i;
      res[i].size = Sizes<S>[i];
      res[i].offset = Offsets<S>[i];
    }
    return res;
  }();
} // namespace schm

namespace schm
{
  //-----------------------------------------------------------------------------
  struct Member {
    constexpr Member() = default;
    constexpr Member(MemberLayout data, void *obj) : mData{data}, mMember{(obj + data.offset)} {}

    // FIXME is non-const overload needed?
    constexpr MemberLayout *operator->() { return &mData; }
    constexpr const MemberLayout *operator->() const { return &mData; }

    template <class T>
    T &as() {
      return *static_cast<T *>(mMember);
    }

    template <class T>
    const T &as() const {
      return *static_cast<const T *>(mMember);
    }

  private:
    MemberLayout mData{}; // NOTE can be a ref since data is static
    void *mMember{};
  };

  //-----------------------------------------------------------------------------
  template <ZUT_SCHM_SCHEMA S>
  constexpr auto members(const S<AsIs> &s) {
    using namespace detail;
    Members<S, Member> res{};
    for (std::size_t i = 0; i < MembersCount<S>; ++i) {
      res[i] = Member{MembersLayout<S>[i], const_cast<void *>(static_cast<const void *>(&s))};
    }
    return res;
  }
} // namespace schm

//-----------------------------------------------------------------------------
//                         Extensions
//-----------------------------------------------------------------------------

#include <functional>
#include <iostream>
namespace schm
{
  //-----------------------------------------------------------------------------

  class OStreamPrinter {
    struct Serializer;
    using SerialzerFuncProto = void (*)(const Member &, OStreamPrinter &);

  public:
    template <ZUT_SCHM_SCHEMA S>
    static inline Members<S, SerialzerFuncProto> SerialzerFunc = AsMembers<S, Serializer, SerialzerFuncProto>();

    explicit OStreamPrinter(std::ostream &os) : mOs{os} { delimeter("{"); }
    ~OStreamPrinter() { delimeter("}"); }

    template <class T>
    void delimeter(T v) {
      mOs << v;
    }
    template <class T>
    void value(T &v) {
      mOs << v;
    }

    template <class T>
    OStreamPrinter &operator<<(const T &v) {
      return mPrevMember = false, value(v), *this;
    }

    template <Schema S>
    OStreamPrinter &operator<<(const S<AsIs> &s) {
      mPrevMember = false; // FIXME check flag
      auto funcIt = SerialzerFunc<S>.begin();
      for (auto ms = members(s); auto &m : ms) {
        std::invoke(*(funcIt++), m, *this);
      }
      return *this;
    }

    operator std::ostream &() { return mOs; }

  private:
    struct Serializer {
      template <MemberInfo MI>
      static void init(Serializer &self) {
        self.func = [](const Member &m, OStreamPrinter &out) {
          if (std::exchange(out.mPrevMember, true)) {
            out << ", ";
          }
          out.delimeter(MI::name);
          out.delimeter(": ");
          out.value(m.as<typename MI::type>());
        };
      }

    private:
      using Func = void (*)(const Member &, OStreamPrinter &);
      Func func;
    };

  private:
    std::ostream &mOs;
    bool mPrevMember{false};
  };

  template <ZUT_SCHM_SCHEMA S> // TOOD IsSchame concept
  std::ostream &operator<<(std::ostream &ss, const S<AsIs> &s) {
    return OStreamPrinter{ss} << s;
  }

} // namespace schm


#include <exception>
#include <stdexcept>
namespace schm
{
  class IStreamPrinter {
    struct Serializer;
    using SerialzerFuncProto = void (*)(Member &, IStreamPrinter &);

  public:
    template <ZUT_SCHM_SCHEMA S>
    static inline Members<S, SerialzerFuncProto> SerialzerFunc = AsMembers<S, Serializer, SerialzerFuncProto>();

    explicit IStreamPrinter(std::istream &os) : mOs{os} { delimeter("{"); }
    ~IStreamPrinter() { delimeter("}"); } // FIXME prevent exception throw

    void delimeter(std::string_view str) {
      for (char s : str) {
        if (auto c = mOs.get(); s != c) {
          panic();
        }
      }
    }
    template <class T>
    void value(T &v) {
      mOs >> v;
    }

    void panic() {
      throw std::runtime_error("invalid format");
      // invalidate cin
    }
    template <class T>
    IStreamPrinter &operator>>(T &v) {
      return mPrevMember = false, value(v), *this;
    }

    template <Schema S>
    IStreamPrinter &operator>>(S<AsIs> &s) {
      mPrevMember = false; // FIXME check flag
      auto funcIt = SerialzerFunc<S>.begin();
      for (auto ms = members(s); auto &m : ms) {
        std::invoke(*(funcIt++), m, *this);
      }
      return *this;
    }

    operator std::istream &() { return mOs; }

  private:
    struct Serializer {
      template <MemberInfo MI>
      static void init(Serializer &self) {
        self.func = [](Member &m, IStreamPrinter &out) {
          if (std::exchange(out.mPrevMember, true)) {
            out.delimeter(", ");
          }
          out.delimeter(MI::name);
          out.delimeter(": ");
          out.value(m.as<typename MI::type>());
        };
      }

    private:
      using Func = void (*)(Member &, IStreamPrinter &);
      Func func;
    };

  private:
    std::istream &mOs;
    bool mPrevMember{false};
  };

  template <ZUT_SCHM_SCHEMA S> // TOOD IsSchame concept
  std::istream &operator>>(std::istream &ss, S<schm::AsIs> &s) {
    return schm::IStreamPrinter{ss} >> s;
  }

  //! Holds member's data
  template<ZUT_SCHM_SCHEMA S, class T, ZUT_SCHM_NAME_TYPE Name, class... Constraints>
  struct MakeMemberInfo
  {
      using type = T;
      constexpr static inline std::string_view name{Name{}};
      using constraints = std::tuple<Constraints...>;
      using parent = S<As<Is>>;
      static type &value(parent &p) { members(p);
      }
  };
} // namespace schm
#endif // SCHM_H
