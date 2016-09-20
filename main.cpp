#include <iostream>
#include <utility>
#include <regex>

namespace naive
{
    template <int INDEX, typename C, typename ...TYPES>
    struct ArgAtIndex : public ArgAtIndex<INDEX - 1, TYPES...>
    {};

    template <typename C, typename ...TYPES>
    struct ArgAtIndex<0, C, TYPES...>
    {
        using type = C;
    };

    template <size_t INDEX, typename T>
    struct Argument
    {
        Argument(T&& v): value(v)
        {}
        T value;
    };

    template <typename T, typename ...ARGS>
    struct ArgHolderBuilder
    {};

    template <typename ...ARGS, size_t ...INDEXES>
    struct ArgHolderBuilder<std::index_sequence<INDEXES...>, ARGS...>
    {
        struct ArgsHolder: public Argument<INDEXES, ARGS>...
        {
            ArgsHolder(ARGS&& ...args): Argument<INDEXES, ARGS>(std::forward<ARGS>(args))...
            {}

        };
    };

    //-----------------------------------------------------------------------

    struct FunctorInvokeHelper
    {
        template <typename FUNC, typename ...ARGS>
        static auto invoke(FUNC& func, ARGS&& ...args) -> void /* decltype(func(std::forward<ARGS>(args)...))*/
        {
            return func(std::forward<ARGS>(args)...);
        }
    };

    struct MemberInvokeHelper
    {
        template <typename FUNC, typename OBJ, typename ...ARGS>
        static auto invoke(FUNC& fn, OBJ&& obj, ARGS&& ...args) -> decltype((obj.*fn)(std::forward<ARGS>(args)...))
        {
            return (obj.*fn)(std::forward<ARGS>(args)...);
        }
    };

    //-----------------------------------------------------------------------

    template <size_t INDEX, typename T, typename ...ARGS>
    struct TakeValue : public TakeValue<INDEX - 1, ARGS...>
    {
        using superclass_t = TakeValue<INDEX - 1, ARGS...>;
        using result_t = typename superclass_t::result_t;

        static result_t take(T&& t, ARGS&& ...other)
        {
            return superclass_t::take(std::forward<ARGS>(other)...);
        }

    };

    template <typename T, typename ...ARGS>
    struct TakeValue<0, T, ARGS...>
    {
        using result_t = T;

        static T take(T&& v, ARGS&& ...other)
        {
            return std::forward<T>(v);
        }
    };

    //-----------------------------------------------------------------------

    template <typename T>
    struct PlaceHolderIndex
    {};

    template <size_t INDEX>
    struct PlaceHolderIndex<std::placeholders::__ph<INDEX>> : public std::integral_constant<size_t, INDEX>
    {};

    //-----------------------------------------------------------------------

    template <size_t INDEX>
    struct TakePlaceholder
    {
        template <typename DUMMY, typename ...PHS>
        static auto take(const DUMMY&, PHS&& ...phs) -> typename ArgAtIndex<INDEX, PHS...>::type
        {
            return TakeValue<INDEX, PHS...>::take(std::forward<PHS>(phs)...);
        }
    };

    template <size_t INDEX, typename ...ARGS>
    struct TakeArgument
    {
        using args_holder_t = typename ArgHolderBuilder<std::make_index_sequence<sizeof...(ARGS)>, ARGS...>::ArgsHolder;
        using result_t = typename ArgAtIndex<INDEX, ARGS...>::type;

        template <typename ...DUMMY>
        static auto take(args_holder_t& holder, DUMMY&& ...) -> result_t
        {
            using arg_t = Argument<INDEX, result_t>;
            return static_cast<arg_t *>(&holder)->value;
        }
    };

    template <typename T, size_t INDEX, typename ...ARGS>
    struct Take : public std::conditional<std::is_placeholder<std::remove_reference_t<T>>::value,
            TakePlaceholder<PlaceHolderIndex<std::remove_reference_t<T>>::value - 1>, TakeArgument<INDEX, ARGS...>>::type
    {
    };

    //-----------------------------------------------------------------------

    template <typename FUNC, typename ...ARGS>
    struct Binder
    {
        using args_holder_t = typename ArgHolderBuilder<std::make_index_sequence<sizeof...(ARGS)>, ARGS...>::ArgsHolder;
        using invoker_t = typename std::conditional<std::is_member_pointer<FUNC>::value, MemberInvokeHelper, FunctorInvokeHelper>::type;

        Binder(FUNC& fn, ARGS&& ...args): m_func(fn), m_argsHolder(std::forward<ARGS>(args)...)
        {}

        template <typename ...PHS>
        void operator() (PHS&& ...phs)
        {
            invoke_helper(std::make_index_sequence<sizeof...(ARGS)>(), std::forward<PHS>(phs)...);
        }

        template <size_t ...ARGIDX, typename ...PHS>
        auto invoke_helper(std::index_sequence<ARGIDX...>, PHS&& ...phs) -> void
        {
            invoker_t::invoke(m_func, Take<typename ArgAtIndex<ARGIDX, ARGS...>::type, ARGIDX, ARGS...>::take(m_argsHolder, std::forward<PHS>(phs)...)...);
        };

    private:
        FUNC m_func;
        args_holder_t m_argsHolder;
    };

    template <typename FUNC, typename ...ARGS>
    auto bind(FUNC& fn, ARGS&& ...args) -> Binder<FUNC, ARGS...>
    {
        return Binder<FUNC, ARGS...>(fn, std::forward<ARGS>(args)...);
    };
}


int main()
{
//    auto foo = [] (int a, char b, std::string s) {
//        std::cout << "a=" << a << " b=" << b << " s=" << s << std::endl;
//    };
//    auto bar = naive::bind(foo, 42, 'b', "blah-blah-blah");
//    bar(1234);

    auto foo = [] (std::string a, std::string b, std::string c) {
        std::cout << "a=" << a << ", b=" << b << ", c=" << c << std::endl;
    };

    auto baz = naive::bind(foo, std::placeholders::_1, std::placeholders::_1, std::placeholders::_1);
    baz("zzzz");


    std::cout << "Hello, World!" << std::endl;
    return 0;
}