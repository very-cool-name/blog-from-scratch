#include "../variant_i.hpp"
#include "../variant_ii.hpp"

#include <any>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <type_traits>
#include <variant>

namespace test {
namespace static_test {
using namespace std;
using namespace variant_i;

static_assert(std::is_same_v<VariantAlternativeT<0, Variant<int, char>>, int>);
static_assert(std::is_same_v<VariantAlternativeT<1, Variant<int, char>>, char>);

static_assert(VariantSizeV<Variant<>> == 0);
static_assert(VariantSizeV<Variant<int>> == 1);
static_assert(VariantSizeV<Variant<int, int>> == 2);
static_assert(VariantSizeV<Variant<int, int, int>> == 3);
static_assert(VariantSizeV<Variant<int, float, double>> == 3);
static_assert(VariantSizeV<Variant<std::monostate, void>> == 2);
static_assert(VariantSizeV<Variant<const int, const float>> == 2);
static_assert(VariantSizeV<Variant<std::variant<std::any>>> == 1);
} // namespace static_test

template <template<typename...> typename V>
void constructible_from_value() {
    using namespace std;
    using namespace variant_i;
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
    using namespace variant_i;
    get<0>(y);
    assert(y.index() == 0);
    std::cout << "OK default_constructible\n";
}

template <template<typename...> typename V>
void typed_get_works() {
    using namespace std;
    using namespace variant_i;
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
    using namespace variant_i;
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
    using namespace variant_i;
    V<char, int> x{1};
    *get_if<int>(&x) = 2;
    assert(*get_if<int>(&x) == 2);
    assert(get_if<char>(&x) == nullptr);
    std::cout << "OK typed_get_if_works\n";
}

template <template<typename...> typename V>
void indexed_get_if_works() {
    using namespace std;
    using namespace variant_i;
    V<char, int> x{1};
    *get_if<1>(&x) = 2;
    assert(*get_if<1>(&x) == 2);
    assert(get_if<0>(&x) == nullptr);
    std::cout << "OK indexed_get_if_works\n";
}

template <template<typename...> typename V>
void holds_alternative_works() {
    using namespace std;
    using namespace variant_i;
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
void swap_works() {
    // assert(false);
}

template<template<typename...> typename V>
struct TestName;

template<>
struct TestName<std::variant> {
    inline static const std::string name = "std::variant";
};

template<>
struct TestName<variant_i::Variant> {
    inline static const std::string name = "variant_i::Variant";
};

template<>
struct TestName<variant_ii::Variant> {
    inline static const std::string name = "variant_ii::Variant";
};

template <template<typename...> typename V>
struct TestI {
    static void test () {
        std::cout << "start TestI " << TestName<V>::name << "\n";
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
        swap_works<V>();
        std::cout << "end TestI " << TestName<V>::name << "\n" << std::endl;
    }

private:
    static V<int>& lvalue_ref();
    static const V<int>& const_lvalue_ref();
    static V<int> rvalue_ref();
    static const V<int> const_rvalue_ref();

    static void static_test() {
        using namespace std;
        using namespace variant_i;
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

template <template<typename...> typename V>
void copy_constructible() {
    using namespace std;
    using namespace variant_i;
    auto orig = make_shared<int>(1);
    V<shared_ptr<int>> x{orig};
    assert(get<shared_ptr<int>>(x).use_count() == 2);
    assert(get<shared_ptr<int>>(x) == orig);
    {
        auto y = x;
        assert(get<shared_ptr<int>>(x) == get<shared_ptr<int>>(y));
        assert(orig.use_count() == 3);
        assert(get<shared_ptr<int>>(x).use_count() == 3);
        assert(get<shared_ptr<int>>(y).use_count() == 3);
    }
    assert(orig.use_count() == 2);
    assert(get<shared_ptr<int>>(x).use_count() == 2);
    std::cout << "OK copy_constructible\n";
}

template <template<typename...> typename V>
void move_constructible() {
    using namespace std;
    using namespace variant_i;
    auto orig = make_unique<int>(1);
    V<unique_ptr<int>> x{std::move(orig)};
    assert(*get<0>(x) == 1);
    {
        auto y = std::move(x);
        assert(*get<0>(y) == 1);
    }
    std::cout << "OK move_constructible\n";
}

template <template<typename...> typename V>
void copy_assignable() {
    using namespace std;
    using namespace variant_i;
    struct Value {
        ~Value() {
            if (destructed) {
                *destructed = true;
            }
        }
        bool* destructed = nullptr;
    };
    struct CopyAssignableValue {
        CopyAssignableValue(int& constructed)
        : constructed(constructed) {}

        CopyAssignableValue& operator=(const CopyAssignableValue& other) {
            ++constructed;
            return *this;
        }

        int& constructed;
    };
    bool destructed = false;
    V<Value, CopyAssignableValue> x {};
    get<0>(x).destructed = &destructed;
    int constructed = 0;
    V<Value, CopyAssignableValue> y {CopyAssignableValue{constructed}};
    x = y;
    assert(destructed);
    assert(x.index() == 1);
    // assert(constructed == 1);
    std::cout << "OK copy_assignable\n";
}

template <template<typename...> typename V>
void move_assignable() {
    using namespace std;
    using namespace variant_i;
    struct Value {
        ~Value() {
            if (destructed) {
                *destructed = true;
            }
        }
        bool* destructed = nullptr;
    };
    struct MoveAssignableValue {
        MoveAssignableValue(int& constructed)
        : constructed(constructed) {}

        MoveAssignableValue(MoveAssignableValue&& other)
        :constructed{other.constructed} {}

        MoveAssignableValue& operator=(MoveAssignableValue&& other) {
            ++constructed;
            return *this;
        }

        int& constructed;
    };
    bool destructed = false;
    V<Value, MoveAssignableValue> x {};
    int constructed = 0;
    get<0>(x).destructed = &destructed;
    V<Value, MoveAssignableValue> y{MoveAssignableValue{constructed}};
    x = std::move(y);
    assert(destructed);
    assert(x.index() == 1);
    std::cout << "OK move_assignable\n";
}

template <template<typename...> typename V>
void emplace_valueless_by_exception() {
    struct Value {
        Value(int, char) {
            throw std::runtime_error("Throw on construct");
        }
    };
    {
        V<int, Value> x {1};
        try {
            x.template emplace<1>(1, 'a');
            assert(false);
        } catch(const std::runtime_error&) {
            assert(x.valueless_by_exception());
        }
    }
    {
        V<int, Value> x {1};
        try {
            x.template emplace<Value>(1, 'a');
            assert(false);
        } catch(const std::runtime_error&) {
            assert(x.valueless_by_exception());
        }
    }
    std::cout << "OK emplace_valueless_by_exception\n";
}

template <template<typename...> typename V>
struct TestII {
    static void test() {
        std::cout << "start TestII " << TestName<V>::name << "\n";
        copy_constructible<V>();
        move_constructible<V>();
        copy_assignable<V>();
        move_assignable<V>();
        emplace_valueless_by_exception<V>();
        std::cout << "end TestII " << TestName<V>::name << "\n";
    }

    static void static_test() {
        struct Value {
            Value() = delete;
        };
        static_assert(!std::is_default_constructible_v<V<Value>>);
    }
};

template <template<typename...> typename V, template<typename...> typename... Variants>
struct TestSuiteII {
    static void test() {
        TestII<V>::test();
        TestSuiteII<Variants...>::test();
    }
};

template<template<typename...> typename V>
struct TestSuiteII<V> {
    static void test() {
        TestII<V>::test();
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

// template<typename T>
// struct TestGet {
//     static constexpr auto value = true;
// };

// template<template<typename...> typename V>
// struct TestGet<V<int>> {
//     static constexpr auto value = std::get<char>(std::declval<V<int>>());
// };

// template<class...>
// constexpr std::false_type always_false{};

// template<typename T>
// void f(T&& x) {
//     TypeCheck<T> y;
// }

int main() {
    // TypeCheck<decltype()> x;
    test::TestSuiteI<std::variant, variant_i::Variant, variant_ii::Variant>::test();
    test::TestSuiteII<std::variant, variant_ii::Variant>::test();
    // f(x);
    // variant_ii::Variant<int> x{1};
    // auto y = x;
}
