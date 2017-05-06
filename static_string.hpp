#include <type_traits>
#include <cassert>
#include <cstddef>

// # Based on value of macro AK_TOOLKIT_CONFIG_USING_STRING_VIEW we decide if and how
//   we want to handle a conversion to string_view

# if defined AK_TOOLKIT_CONFIG_USING_STRING_VIEW
#   if AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 0
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS()
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 1
#     include <string_view>
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() constexpr operator ::std::string_view () const { return ::std::string_view(c_str(), N); }
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 2
#     include <experimental/string_view>
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() constexpr operator ::std::experimental::string_view () const { return ::std::experimental::string_view(c_str(), N); }
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 3
#     include <boost/utility/string_ref.hpp> 
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() constexpr operator ::boost::string_ref () const { return ::boost::string_ref(c_str(), N); }
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 4
#     include <string> 
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() operator ::std::string () const { return ::std::string(c_str(), N); }
#   endif
# else
#   define AK_TOOLKIT_STRING_VIEW_OPERATIONS()
# endif

namespace ak_toolkit { namespace static_str {
 
// # Implementation of a subset of C++14 std::integer_sequence and std::make_integer_sequence
 
namespace detail
{
  template <int... I>
  struct int_sequence
  {};
 
  template <int i, typename T>
  struct cat
  {
    static_assert (sizeof(T) < 0, "bad use of cat");
  };
 
  template <int i, int... I>
  struct cat<i, int_sequence<I...>>
  {
    using type = int_sequence<I..., i>;
  };
 
  template <int I>
  struct make_int_sequence_
  {
    static_assert (I >= 0, "bad use of make_int_sequence: negative size");
    using type = typename cat<I - 1, typename make_int_sequence_<I - 1>::type>::type;
  };
 
  template <>
  struct make_int_sequence_<0>
  {
    using type = int_sequence<>;
  };
 
  template <int I>
  using make_int_sequence = typename make_int_sequence_<I>::type;
}


// # Implementation of a constexpr-compatible assertion

#if defined NDEBUG
# define AK_TOOLKIT_ASSERTED_EXPRESSION(CHECK, EXPR) (EXPR)
#else
# define AK_TOOLKIT_ASSERTED_EXPRESSION(CHECK, EXPR) ((CHECK) ? (EXPR) : ([]{assert(!#CHECK);}(), (EXPR)))
#endif


// # A wraper over a string literal with alternate interface. No ownership management

template <int N>
class string_literal
{
    const char (&_lit)[N + 1];
public:
    constexpr string_literal(const char (&lit)[N + 1]) : _lit(AK_TOOLKIT_ASSERTED_EXPRESSION(lit[N] == 0, lit)) {}
    constexpr char operator[](int i) const { return AK_TOOLKIT_ASSERTED_EXPRESSION(i >= 0 && i < N, _lit[i]); }
    AK_TOOLKIT_STRING_VIEW_OPERATIONS()
    constexpr ::std::size_t size() const { return N; };
    constexpr const char* c_str() const { return _lit; }
    constexpr operator const char * () const { return c_str(); }
};


// # A function that converts raw string literal into string_literal and deduces the size.

template <int N_PLUS_1>
constexpr string_literal<N_PLUS_1 - 1> literal(const char (&lit)[N_PLUS_1])
{
    return string_literal<N_PLUS_1 - 1>(lit);
}


// # This implements a null-terminated array that stores elements on stack.

template <int N>
class array_string
{
    char _array[N + 1];
    struct private_ctor {};
   
       
    template <int M, int... Il, int... Ir>
    constexpr explicit array_string(private_ctor, string_literal<M> l, string_literal<N - M> r, detail::int_sequence<Il...>, detail::int_sequence<Ir...>)
      : _array{l[Il]..., r[Ir]..., 0}
    {
    }
   
    template <int... Il>
    constexpr explicit array_string(private_ctor, string_literal<N> l, detail::int_sequence<Il...>)
      : _array{l[Il]..., 0}
    {
    }
   
        template <int M, int... Il, int... Ir>
    constexpr explicit array_string(private_ctor, array_string<M> const& l, array_string<N - M> const& r, detail::int_sequence<Il...>, detail::int_sequence<Ir...>)
      : _array{l[Il]..., r[Ir]..., 0}
    {
    }
   
public:
    template <int M, typename std::enable_if<(M < N), bool>::type = true>
    constexpr explicit array_string(string_literal<M> l, string_literal<N - M> r)
    : array_string(private_ctor{}, l, r, detail::make_int_sequence<M>{}, detail::make_int_sequence<N - M>{})
    {
    }

    constexpr array_string(string_literal<N> l) // converting
    : array_string(private_ctor{}, l, detail::make_int_sequence<N>{})
    {
    }
   
    template <int M, typename std::enable_if<(M < N), bool>::type = true>
    constexpr explicit array_string(array_string<M> const& l, array_string<N - M> const& r)
    : array_string(private_ctor{}, l, r, detail::make_int_sequence<M>{}, detail::make_int_sequence<N - M>{})
    {
    }
   
    constexpr ::std::size_t size() const { return N; }
  
    constexpr const char* c_str() const { return _array; }
    constexpr operator const char * () const { return c_str(); }
    AK_TOOLKIT_STRING_VIEW_OPERATIONS()
    constexpr char operator[] (int i) const { return AK_TOOLKIT_ASSERTED_EXPRESSION(i >= 0 && i < N, _array[i]); }
};


// # A set of concatenating operators, for different combinations of raw literals, string_literal<>, and array_string<>

template <int N1, int N2>
constexpr array_string<N1 + N2> operator+(string_literal<N1> l, string_literal<N2> r)
{
    return array_string<N1 + N2>(l, r);
}

template <int N1_1, int N2>
constexpr array_string<N1_1 - 1 + N2> operator+(const char (&l)[N1_1], string_literal<N2> r)
{
    return array_string<N1_1 - 1 + N2>(string_literal<N1_1 - 1>(l), r);
}

template <int N1, int N2_1>
constexpr array_string<N1 + N2_1 - 1> operator+(string_literal<N1> l, const char (&r)[N2_1])
{
    return array_string<N1 + N2_1 - 1>(l, string_literal<N2_1 - 1>(r));
}


template <int N1, int N2>
constexpr array_string<N1 + N2> operator+(array_string<N1> const& l, string_literal<N2> r)
{
    return array_string<N1 + N2>(l, array_string<N2>(r));
}

template <int N1, int N2>
constexpr array_string<N1 + N2> operator+(string_literal<N1> l, array_string<N2> const& r)
{
    return array_string<N1 + N2>(l, r);
}

template <int N1, int N2>
constexpr array_string<N1 + N2> operator+(array_string<N1> const& l, array_string<N2> const& r)
{
    return array_string<N1 + N2>(l, r);
}

template <int N1_1, int N2>
constexpr array_string<N1_1 - 1 + N2> operator+(const char (&l)[N1_1], array_string<N2> const& r)
{
    return array_string<N1_1 - 1 + N2>(string_literal<N1_1 - 1>(l), r);
}

template <int N1, int N2_1>
constexpr array_string<N1 + N2_1 - 1> operator+(array_string<N1> const& l, const char (&r)[N2_1])
{
    return array_string<N1 + N2_1 - 1>(l, array_string<N2_1 - 1>(string_literal<N2_1 - 1>(r)));
}

}} // namespace ak_toolkit::static_str
