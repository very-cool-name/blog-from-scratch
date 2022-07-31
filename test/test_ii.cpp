#include "../variant_i.hpp"
#include "../variant_ii.hpp"

#include <cassert>
#include <iostream>
#include <variant>

namespace test {
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
            assert(x. valueless_by_exception());
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
void emplace_calls_copy() {
    struct MoveAssignableValue {
        MoveAssignableValue(int& constructed)
        : constructed(constructed) {}

        MoveAssignableValue(MoveAssignableValue&& other)
        :constructed{other.constructed} {
            ++constructed;
        }

        MoveAssignableValue& operator=(MoveAssignableValue&& other) {
            ++constructed;
            return *this;
        }

        int& constructed;
    };
    int constructed = 0;
    V<MoveAssignableValue> x {MoveAssignableValue(constructed)};
    assert(constructed == 1);
    x.template emplace<0>(constructed);
    assert(constructed == 1);
    std::cout << "OK emplace_calls_copy\n";
}

template<template<typename...> typename V>
struct TestName;

template<>
struct TestName<std::variant> {
    inline static const std::string name = "std::variant";
};

template<>
struct TestName<variant_ii::Variant> {
    inline static const std::string name = "variant_ii::Variant";
};

template <template<typename...> typename V>
struct TestII {
    static void test() {
        std::cout << "start TestII " << TestName<V>::name << "\n";
        copy_constructible<V>();
        move_constructible<V>();
        copy_assignable<V>();
        move_assignable<V>();
        emplace_valueless_by_exception<V>();
        emplace_calls_copy<V>();
        std::cout << "end TestII " << TestName<V>::name << "\n";
    }

    static void static_test() {
        struct Value {
            Value() = delete;
        };
        static_assert(!std::is_default_constructible_v<V<Value>>);
        static_assert(std::is_default_constructible_v<V<int>>);
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


int main() {
    test::TestSuiteII<std::variant, variant_ii::Variant>::test();
}