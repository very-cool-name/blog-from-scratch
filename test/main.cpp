#include "../variant_i.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <variant>

union S
{
    std::int32_t big_int;   // occupies 4 bytes
    std::uint8_t small_int; // occupies 1 byte
};

// namespace test_internal {
//     static_assert(
//         IndexOf<int, int, char, double>::value == 0
//     );
//     static_assert(
//         IndexOf<int, char, int, double>::value == 1
//     );
//         static_assert(
//         IndexOf<int, char, double, int>::value == 2
//     );
// }

namespace test {

namespace static_test {
using namespace std;
using namespace variant_i;

template<typename Get, typename Variant, typename Check=void>
struct TestGet {
    static void test() {
        std::cout << "ok";
    }
};

template<
    typename Get,
    typename... Args
>
struct TestGet<Get, std::variant<Args...>, decltype(get<Get>(std::declval<std::variant<Args...>>()))> {
    static void test() {
        std::cout << "notok";
        assert(false);
    }
};
} // namespace static_test

struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly& operator=(MoveOnly&&) = default;
};

template <template<typename...> typename V>
void constructible_from_value() {
    {
        V<int> x{1};
        using namespace std;
        using namespace variant_i;
        assert(get<int>(x) == 1);
        assert(get<0>(x) == 1);
    }
    {
        MoveOnly moveOnly;
        V<MoveOnly> z{std::move(moveOnly)};
    }
}

template <template<typename...> typename V>
struct Test {
    static void test () {
        // static_test::TestGet<int, V<char>> x;
        constructible_from_value<V>();
    }
};

template <template<typename...> typename V, template<typename...> typename... Variants>
struct TestAll {
    static void test() {
        Test<V>::test();
        TestAll<Variants...>::test();
    }
};

template<template<typename...> typename V>
struct TestAll<V> {
    static void test() {
        Test<V>::test();
    }
};
} // namespace test

template<class T>
struct TypeCheck;

// template<typename... Types>
// struct Variant{
//     template<typename U, typename... Args>
//     friend int get(const Variant<Args...>& x);
// private:
//     int x;
// };

// template<typename U, typename... Args>
// int get(const Variant<Args...>& x) {
//     U u;
//     return x.x;
// }

// const int& clref(const int& x) {
//     return x;
// }

// const int&& crref(const int&& x) {
//     return std::move(clref(x));
// }

// const int crref_x() {
//     return 23;
// }

template<typename T, typename U=void>
struct TestNegative {
};

template<
    template<typename...> typename V,
    typename... Args
>
struct TestNegative<V<Args...>, decltype(std::get<char>(std::declval<V<Args...>>()))> {
    static_assert("this is bad");
};

// template<template<typename...> typename V, typename... Args>
// struct TestNegative<V<Args...>> {
//     static void test(V<Args...>& x) {
//         std::get<char>(x);
//         assert(false);
//     }
// };


// template<typename T>
// void test_(const T& x) {}

// template<template<typename...> typename V, typename... Args>
// void test_(const V<Args...>& x) {
//     std::get<char>(x);
// }


int main() {
    // TypeCheck<decltype(get<0>(std::declval<std::variant<const int>>()))> t;
    // TypeCheck<decltype(std::declval<variant_i::Variant<const int>>().get<0>())> t;
    // TypeCheck<std::invoke_result_t<std::get<0>, std::variant<const int>>> t;
    // std::variant<> x;
    test::TestAll<std::variant, variant_i::Variant>::test();

    TypeCheck<decltype(variant_i::get<int>(std::declval<variant_i::Variant<char>>()))> x;
    test::static_test::TestGet<char, std::variant<char>>::test();
    // const variant_i::Variant<int> x{1};
    // x.get<int>();
    // std::variant<int&&> x;
    // variant_i::get<int>(crref());
    // variant_i::get<int>(variant_i::Variant<int>{1});
    // variant_i::Variant<int> x{1};
    // variant_i::get<0>(x) = 25;

    // std::variant<int> x{1};
    // TestNegative<std::variant<int>> y;

}
