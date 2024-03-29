#pragma once

#include <any>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <type_traits>
#include <variant>

#include "utility.hpp"

namespace test {
template <template<typename...> typename V>
void constructible_from_value() {
    using namespace std;
    V<int> x{1};
    assert(get<int>(x) == 1);
    assert(get<0>(x) == 1);
    assert(x.index() == 0);
    std::cout << "OK constructible_from_value\n";
}

template <template<typename...> typename V>
void calls_copy_constructor() {
    struct CopyOnlyValue {
        CopyOnlyValue(int& constructed)
        : constructed(constructed) {}

        CopyOnlyValue(const CopyOnlyValue& other)
        : constructed(other.constructed) {
            ++constructed;
        }

        int& constructed;
    };
    int constructed = 0;
    CopyOnlyValue value{constructed};
    V<char, CopyOnlyValue> x{value};
    assert(constructed == 1);
    assert(x.index() == 1);
    std::cout << "OK calls_copy_constructor\n";
}

template <template<typename...> typename V>
void calls_move_constructor() {
    struct MoveOnlyValue {
        MoveOnlyValue(int& constructed)
        : constructed(constructed) {}

        MoveOnlyValue(MoveOnlyValue&& other)
        : constructed(other.constructed) {
            ++constructed;
        }

        int& constructed;
    };
    int constructed = 0;
    V<char, MoveOnlyValue> x{MoveOnlyValue{constructed}};
    assert(constructed == 1);
    assert(x.index() == 1);
    std::cout << "OK calls_move_constructor\n";
}

template <template<typename...> typename V>
void calls_destructor() {
    struct Value {
        ~Value() {
            if (destructed) {
                *destructed = true;
            }
        }
        bool* destructed = nullptr;
    };
    bool destructed = false;
    {
        V<char, Value> x{Value{}};
        get<Value>(x).destructed = &destructed;
        assert(x.index() == 1);
    }
    assert(destructed);
    std::cout << "OK calls_destructor\n";
}

template <template<typename...> typename V>
void default_constructible() {
    V<int> x{};
    assert(get<int>(x) == 0);

    struct Value {
        Value() = delete;
    };
    V<std::monostate, Value> y{};
    using namespace std;
    get<0>(y);
    assert(y.index() == 0);
    std::cout << "OK default_constructible\n";
}

template <template<typename...> typename V>
void typed_get_works() {
    using namespace std;
    V<char, int> x{1};
    get<int>(x) = 2;
    assert(get<int>(x) == 2);
    try {
        get<char>(x);
        assert(false);
    } catch(const std::exception& e) {
        assert(true);
    }
    std::cout << "OK typed_get_works\n";
}


template <template<typename...> typename V>
void indexed_get_works() {
    using namespace std;
    V<char, int> x{1};
    get<1>(x) = 2;
    assert(get<1>(x) == 2);
    try {
        get<0>(x);
        assert(false);
    } catch(const std::exception& e) {
        assert(true);
    }
    std::cout << "OK indexed_get_works\n";
}

template <template<typename...> typename V>
void typed_get_if_works() {
    using namespace std;
    V<char, int> x{1};
    *get_if<int>(&x) = 2;
    assert(*get_if<int>(&x) == 2);
    assert(get_if<char>(&x) == nullptr);
    std::cout << "OK typed_get_if_works\n";
}

template <template<typename...> typename V>
void indexed_get_if_works() {
    using namespace std;
    V<char, int> x{1};
    *get_if<1>(&x) = 2;
    assert(*get_if<1>(&x) == 2);
    assert(get_if<0>(&x) == nullptr);
    std::cout << "OK indexed_get_if_works\n";
}

template <template<typename...> typename V>
void holds_alternative_works() {
    using namespace std;
    {
        V<char, int> x{1};
        assert(holds_alternative<int>(x));
        assert(!holds_alternative<char>(x));
    }
    {
        V<char, int> x{'a'};
        assert(holds_alternative<char>(x));
        assert(!holds_alternative<int>(x));
    }
    std::cout << "OK holds_alternative_works\n";
}

template <template<typename...> typename V>
struct TestI {
    static void test () {
        std::cout << "start TestI " << utility::TestName<V>::name << "\n";
        static_test();
        calls_copy_constructor<V>();
        calls_move_constructor<V>();
        calls_destructor<V>();
        typed_get_if_works<V>();
        indexed_get_if_works<V>();
        typed_get_works<V>();
        indexed_get_works<V>();
        constructible_from_value<V>();
        default_constructible<V>();
        holds_alternative_works<V>();
        std::cout << "end TestI " << utility::TestName<V>::name << "\n" << std::endl;
    }

private:
    static V<int>& lvalue_ref();
    static const V<int>& const_lvalue_ref();
    static V<int> rvalue_ref();
    static const V<int> const_rvalue_ref();

    static void static_test() {
    using namespace std;
        static_assert(std::is_same_v<decltype(get<0>(lvalue_ref())), int&>);
        static_assert(std::is_same_v<decltype(get<0>(const_lvalue_ref())), const int&>);
        static_assert(std::is_same_v<decltype(get<0>(std::declval<V<int>>())), int&&>);
        static_assert(std::is_same_v<decltype(get<0>(const_rvalue_ref())), const int&&>);
        static_assert(std::is_default_constructible_v<V<int>>);

        struct Value {
            Value() = delete;
        };
        static_assert(std::is_default_constructible_v<V<std::monostate, Value>>);
        // static_assert(!std::is_default_constructible_v<V<Value>>);
        std::cout << "OK static_test\n";
    }
};

template <template<typename...> typename V, template<typename...> typename... Variants>
struct TestSuiteI {
    static void test() {
        TestI<V>::test();
        TestSuiteI<Variants...>::test();
    }
};

template<template<typename...> typename V>
struct TestSuiteI<V> {
    static void test() {
        TestI<V>::test();
    }
};
} // namespace test
