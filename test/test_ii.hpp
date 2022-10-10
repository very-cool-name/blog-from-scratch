#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <utility>
#include <variant>

#include "utility.hpp"

namespace test {
template <template<typename...> typename V>
void converting_constructor_uses_correct_type() {
    struct IntConvertibleThrows {
        IntConvertibleThrows(int) {
            throw utility::TestThrow{"IntConvertibleThrows throws"};
        }
    };
    try {
        V<char, IntConvertibleThrows> x{1};
        assert(false);
    } catch(const utility::TestThrow&) {
    }
    std::cout << "OK converting_constructor_uses_correct_type\n";
}

template <template<typename...> typename V>
void copy_constructible() {
    using namespace std;
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
void move_assignment_calls_destructor() {
    using namespace std;
    using namespace utility;
    {
        V<DestructCountable, MoveAssignable> x{};
        int destructed = 0;
        get<0>(x).destructed = &destructed;
        int move_assigned = 0;
        V<DestructCountable, MoveAssignable> y{MoveAssignable{move_assigned}};
        x = std::move(y);
        assert(destructed == 1);
        assert(x.index() == 1);
    }
    {
        V<DestructCountable, int, ThrowConstructible> x{};
        int destructed = 0;
        get<0>(x).destructed = &destructed;
        V<DestructCountable, int, ThrowConstructible> y{1};
        try {
            y. template emplace<2>();
            assert(false);
        } catch(const std::runtime_error&) {
        }
        x = std::move(y);
        assert(destructed == 1);
        assert(x.valueless_by_exception());
    }
    std::cout << "OK move_assignment_calls_destructor\n";
}

template <template<typename...> typename V>
void move_assignment_calls_move_assignment() {
    using namespace std;
    using namespace utility;
    int move_assigned = 0;
    V<int, MoveAssignable> x{MoveAssignable{move_assigned}};
    int other = 0;
    V<int, MoveAssignable> y{MoveAssignable{other}};
    x = std::move(y);
    assert(move_assigned == 1);
    assert(other == 0);
    std::cout << "OK move_assignment_calls_move_assignment\n";
}

template <template<typename...> typename V>
void copy_assignment_calls_destructor() {
    using namespace std;
    using namespace utility;
    {
        V<DestructCountable, CopyAssignable> x{};
        int destructed = 0;
        get<0>(x).destructed = &destructed;
        int copy_assigned = 0;
        V<DestructCountable, CopyAssignable> y{CopyAssignable{copy_assigned}};
        x = y;
        assert(destructed == 1);
        assert(x.index() == 1);
    }
    {
        V<DestructCountable, int, ThrowConstructible> x{};
        int destructed = 0;
        get<0>(x).destructed = &destructed;
        V<DestructCountable, int, ThrowConstructible> y{1};
        try {
            y. template emplace<2>();
            assert(false);
        } catch(const std::runtime_error&) {
        }
        x = y;
        assert(destructed == 1);
        assert(x.valueless_by_exception());
    }
    std::cout << "OK copy_assignment_calls_destructor\n";
}

template <template<typename...> typename V>
void copy_assignment_calls_copy_assignment() {
    using namespace std;
    using namespace utility;
    int copy_assigned = 0;
    V<int, CopyAssignable> x{CopyAssignable{copy_assigned}};
    int other = 0;
    V<int, CopyAssignable> y{CopyAssignable{other}};
    x = y;
    assert(copy_assigned == 1);
    assert(other == 0);
    std::cout << "OK copp_assignment_calls_copy_assignment\n";
}

template <template<typename...> typename V>
void copy_assignment_calls_copy_construct() {
    using namespace std;
    using namespace utility;
    {
        struct NothrowCopyConstructible {
            NothrowCopyConstructible() = default;

            NothrowCopyConstructible(const NothrowCopyConstructible&) noexcept
            : copy_constructed {true} {}

            NothrowCopyConstructible(NothrowCopyConstructible&&) noexcept
            : move_constructed {true} {}

            NothrowCopyConstructible& operator=(const NothrowCopyConstructible&) {
                return *this;
            }

            NothrowCopyConstructible& operator=(NothrowCopyConstructible&&) {
                return *this;
            }

            bool copy_constructed = false;
            bool move_constructed = false;
        };
        V<int, NothrowCopyConstructible> x{1};
        V<int, NothrowCopyConstructible> y{NothrowCopyConstructible{}};
        x = y;
        assert(get<1>(x).copy_constructed);
        assert(!get<1>(x).move_constructed);
    }
    {
        struct CopyConstructible {
            CopyConstructible() = default;

            CopyConstructible(const CopyConstructible&)
            : copy_constructed {true} {}

            CopyConstructible(CopyConstructible&&)
            : move_constructed {true} {}

            CopyConstructible& operator=(const CopyConstructible&) {
                return *this;
            }

            CopyConstructible& operator=(CopyConstructible&&) {
                return *this;
            }

            bool copy_constructed = false;
            bool move_constructed = false;
        };
        V<int, CopyConstructible> x{1};
        V<int, CopyConstructible> y{CopyConstructible{}};
        x = y;
        assert(get<1>(x).copy_constructed);
        assert(!get<1>(x).move_constructed);
    }
    std::cout << "OK copy_assignment_calls_copy_construct\n";
}

template <template<typename...> typename V>
void copy_assignment_calls_move_construct() {
    using namespace std;
    using namespace utility;
    {
        struct NothrowMoveConstructible {
            NothrowMoveConstructible() = default;

            NothrowMoveConstructible(const NothrowMoveConstructible&)
            : copy_constructed {true} {}

            NothrowMoveConstructible(NothrowMoveConstructible&&) noexcept
            : move_constructed {true} {}

            NothrowMoveConstructible& operator=(const NothrowMoveConstructible&) {
                return *this;
            }

            NothrowMoveConstructible& operator=(NothrowMoveConstructible&&) {
                return *this;
            }

            bool copy_constructed = false;
            bool move_constructed = false;
        };
        V<int, NothrowMoveConstructible> x{1};
        V<int, NothrowMoveConstructible> y{NothrowMoveConstructible{}};
        x = y;
        assert(get<1>(x).move_constructed);
        assert(!get<1>(x).copy_constructed);
    }
    std::cout << "OK copy_assignment_calls_move_construct\n";
}

template <template<typename...> typename V>
void copy_assignment_copies_valueless() {
    using namespace std;
    struct Value {
        Value(int, char) {
            throw std::runtime_error("Throw on construct");
        }
    };
    V<int, Value> x {1};
    try {
        x.template emplace<1>(1, 'a');
        assert(false);
    } catch(const std::runtime_error&) {
        assert(x.valueless_by_exception());
    }
    V<int, Value> y {1};
    y = x;
    assert(y.valueless_by_exception());
    assert(y.index() == variant_npos);
    std::cout << "OK copy_assignment_copies_valueless\n";
}

template <template<typename...> typename V>
void copy_assignment_copy_leaves_variant_valueless() {
    using namespace std;
    struct CopyConstructThrows {
        CopyConstructThrows() = default;

        CopyConstructThrows(const CopyConstructThrows&)
        : copy_constructed {true} {
            throw std::runtime_error("CopyConstructThrows throws on copy");
        }

        CopyConstructThrows(CopyConstructThrows&&)
        : move_constructed {true} {}

        CopyConstructThrows& operator=(const CopyConstructThrows&) {
            return *this;
        }

        CopyConstructThrows& operator=(CopyConstructThrows&&) {
            return *this;
        }

        bool copy_constructed = false;
        bool move_constructed = false;
    };
    V<int, CopyConstructThrows> x{1};
    V<int, CopyConstructThrows> y{CopyConstructThrows{}};
    try {
        x = y;
        assert(false);
    } catch (const std::runtime_error&) {
    }
    assert(x.valueless_by_exception());
    assert(!y.valueless_by_exception());
    std::cout << "OK copy_assignment_copy_leaves_variant_valueless\n";
}

template <template<typename...> typename V>
void converting_assignment_calls_assignment() {
    using namespace std;
    struct IntConvertible {
        IntConvertible() = default;

        IntConvertible(int) {}

        IntConvertible(const IntConvertible&) {}
        IntConvertible(IntConvertible&&) {}

        IntConvertible& operator=(const IntConvertible&) { return *this; }

        IntConvertible& operator=(int) {
            assigned = true;
            return *this;
        }

        IntConvertible& operator=(IntConvertible&&) { return *this; }

        bool assigned = false;
    };
    V<char, IntConvertible> x{3};
    x = 1;
    assert(get<1>(x).assigned);
    std::cout << "OK converting_assignment_calls_assignment\n";
}

template <template<typename...> typename V>
void converting_assignment_calls_inplace() {
    using namespace std;
    struct IntConvertible {
        IntConvertible() = default;

        IntConvertible(int)
        :assigned{true} {}

        IntConvertible(const IntConvertible&) {}
        IntConvertible(IntConvertible&&) {}

        IntConvertible& operator=(const IntConvertible&) { return *this; }

        IntConvertible& operator=(IntConvertible&&) { return *this; }

        bool assigned = false;
    };
    V<char, IntConvertible> x{'3'};
    x = 1;
    assert(get<1>(x).assigned);
    std::cout << "OK converting_assignment_calls_inplace\n";
}

template <template<typename...> typename V>
void converting_assignment_calls_move() {
    using namespace std;
    struct IntConvertible {
        IntConvertible() = default;

        IntConvertible(int) {}

        IntConvertible(const IntConvertible&) {}
        IntConvertible(IntConvertible&&) noexcept
        :moved{true} {}

        IntConvertible& operator=(const IntConvertible&) { return *this; }

        IntConvertible& operator=(IntConvertible&&) { return *this; }

        bool moved = false;
    };
    V<char, IntConvertible> x{'3'};
    x = 1;
    assert(get<1>(x).moved);
    std::cout << "OK converting_assignment_calls_move\n";
}

template <template<typename...> typename V>
void converting_assignment_leaves_variant_valueless() {
    using namespace std;
    struct IntConvertible {
        IntConvertible() = default;

        IntConvertible(int)
        :assigned{true} {
            throw utility::TestThrow("IntConvertible throws on construction");
        }

        IntConvertible(const IntConvertible&) {}
        IntConvertible(IntConvertible&&) {}

        IntConvertible& operator=(const IntConvertible&) { return *this; }

        IntConvertible& operator=(IntConvertible&&) { return *this; }

        bool assigned = false;
    };
    V<char, IntConvertible> x{'3'};
    try {
        x = 1;
        assert(false);
    } catch (const utility::TestThrow&) {
    }
    assert(x.valueless_by_exception());
    std::cout << "OK converting_assignment_leaves_variant_valueless\n";
}

template <template<typename...> typename V>
void emplace_leaves_variant_valueless() {
    using namespace std;
    using namespace utility;
    {
        V<int, ThrowConstructible> x {1};
        try {
            x.template emplace<1>();
            assert(false);
        } catch(const std::runtime_error&) {
            assert(x.valueless_by_exception());
            assert(x.index() == variant_npos);
        }
    }
    {
        V<int, ThrowConstructible> x {1};
        try {
            x.template emplace<ThrowConstructible>();
            assert(false);
        } catch(const std::runtime_error&) {
            assert(x.valueless_by_exception());
            assert(x.index() == variant_npos);
        }
    }
    std::cout << "OK emplace_leaves_variant_valueless\n";
}

template <template<typename...> typename V>
void emplace_destructs_and_calls_constructor() {
    using namespace std;
    using namespace utility;
    struct MoveAssignableValue {
        MoveAssignableValue(int& copied)
        : copied(copied) {}

        MoveAssignableValue(MoveAssignableValue&& other)
        :copied{other.copied} {
            ++copied;
        }

        MoveAssignableValue& operator=(MoveAssignableValue&& other) {
            ++copied;
            return *this;
        }

        int& copied;
    };
    int copied = 0;
    int destructed = 0;
    V<DestructCountable, MoveAssignableValue> x {};
    get<0>(x).destructed = &destructed;
    x.template emplace<1>(copied);
    assert(copied == 0);
    assert(destructed == 1);
    std::cout << "OK emplace_destructs_and_calls_constructor\n";
}

template <template<typename...> typename V>
void swap_works() {
    using namespace std;
    V<char, int> x{1};
    V<char, int> y{'a'};
    x.swap(y);
    assert(get<int>(y) == 1);
    assert(get<char>(x) == 'a');
    std::cout << "OK swap_works\n";
}

template <template<typename...> typename V>
void swap_calls_swap() {
    using namespace std;
    using namespace utility;
    V<int, Swappable> x {Swappable{}};
    V<int, Swappable> y {Swappable{}};
    x.swap(y);
    assert(get<Swappable>(x).swapped);
    assert(get<Swappable>(y).swapped);
    std::cout << "OK swap_calls_swap\n";
}

template <template<typename...> typename V>
void swap_calls_move() {
    using namespace std;
    using namespace utility;
    V<int, Swappable> x{1};
    V<int, Swappable> y{Swappable{}};
    x.swap(y);
    assert(get<int>(y) == 1);
    assert(get<Swappable>(x).moved);
    std::cout << "OK swap_calls_move\n";
}

template <template<typename...> typename V>
void swap_throws() {
    using namespace std;
    using namespace utility;
    {
        V<MoveThrowsSwappable, MovedOut> x{MovedOut{}};
        V<MoveThrowsSwappable, MovedOut> y{};
        try {
            x.swap(y);
            assert(false);
        } catch(const utility::TestThrow&) {}
        assert(x.index() == 1);
        assert(y.index() == 0);
        assert(!get<MovedOut>(x).moved_out);
    }
    {
        V<MoveThrowsSwappable, MovedOut> x{MovedOut{}};
        V<MoveThrowsSwappable, MovedOut> y{};
        try {
            y.swap(x);
            assert(false);
        } catch(const utility::TestThrow&) {}
        assert(x.valueless_by_exception());
        assert(y.index() == 0);
    }
    std::cout << "OK swap_throws\n";
}

template <template<typename...> typename V>
void inplace_constructs() {
    using namespace std;
    struct IntConvertible {
        IntConvertible(int){}
    };
    {
        V<int, IntConvertible> v{std::in_place_index<1>, 3};
        assert(v.index() == 1);
    }
    {
        V<int, IntConvertible> v{std::in_place_type<IntConvertible>, 3};
        assert(v.index() == 1);
    }
    std::cout << "OK inplace_constructs\n";
}

template <template<typename...> typename V>
struct TestII {
    static void test() {
        std::cout << "start TestII " << utility::TestName<V>::name << "\n";
        static_test();
        converting_constructor_uses_correct_type<V>();
        copy_constructible<V>();
        move_constructible<V>();
        copy_assignable<V>();
        move_assignment_calls_destructor<V>();
        move_assignment_calls_move_assignment<V>();

        copy_assignment_calls_destructor<V>();
        copy_assignment_calls_copy_assignment<V>();
        copy_assignment_calls_copy_construct<V>();
        copy_assignment_calls_move_construct<V>();
        copy_assignment_copies_valueless<V>();
        copy_assignment_copy_leaves_variant_valueless<V>();

        converting_assignment_calls_assignment<V>();
        converting_assignment_calls_inplace<V>();
        converting_assignment_calls_move<V>();
        converting_assignment_leaves_variant_valueless<V>();

        emplace_leaves_variant_valueless<V>();
        emplace_destructs_and_calls_constructor<V>();

        swap_works<V>();
        swap_calls_swap<V>();
        swap_calls_move<V>();
        swap_throws<V>();

        inplace_constructs<V>();

        std::cout << "end TestII " << utility::TestName<V>::name << "\n";
    }

    static void static_test() {
        struct Value {
            Value() = delete;
            Value(const Value&) = delete;
        };
        struct ThrowDestructible {
            ~ThrowDestructible() noexcept(false) {}
        };
        struct NothrowIntAssignable {
            NothrowIntAssignable(int) noexcept {}
            NothrowIntAssignable& operator=(int) noexcept {return *this;}
        };
        struct ThrowIntAssignable {
            ThrowIntAssignable(int) {}
            ThrowIntAssignable& operator=(int) noexcept {return *this;}
        };
        static_assert(!std::is_default_constructible_v<V<Value>>);
        static_assert(std::is_default_constructible_v<V<int>>);
        static_assert(std::is_nothrow_default_constructible_v<V<int>>);
        static_assert(!std::is_nothrow_default_constructible_v<V<utility::ThrowConstructible>>);

        static_assert(std::is_trivially_destructible_v<V<int>>);
        static_assert(!std::is_trivially_destructible_v<V<int, std::shared_ptr<int>>>);

        static_assert(!std::is_copy_constructible_v<V<Value>>);
        static_assert(std::is_copy_constructible_v<V<std::shared_ptr<int>>>);
        static_assert(!std::is_copy_constructible_v<V<int, std::unique_ptr<int>>>);
        static_assert(!std::is_nothrow_copy_constructible_v<V<int, utility::ThrowConstructible>>);
        static_assert(std::is_trivially_copy_constructible_v<V<int>>);
        static_assert(!std::is_trivially_copy_constructible_v<V<int, std::shared_ptr<int>>>);

        static_assert(!std::is_move_constructible_v<V<Value>>);
        static_assert(std::is_move_constructible_v<V<int, std::unique_ptr<int>>>);
        static_assert(std::is_nothrow_move_constructible_v<V<int, std::unique_ptr<int>>>);
        static_assert(!std::is_nothrow_move_constructible_v<V<int, utility::ThrowConstructible>>);
        static_assert(std::is_trivially_move_constructible_v<V<int>>);
        static_assert(!std::is_trivially_move_constructible_v<V<int, std::shared_ptr<int>>>);

        static_assert(!std::is_move_assignable_v<V<Value>>);
        static_assert(std::is_move_assignable_v<V<int, std::unique_ptr<int>>>);
        static_assert(std::is_nothrow_move_assignable_v<V<int, std::unique_ptr<int>>>);
        static_assert(!std::is_nothrow_move_assignable_v<V<ThrowDestructible>>);
        static_assert(!std::is_nothrow_move_assignable_v<V<int, utility::ThrowConstructible>>);
        static_assert(!std::is_trivially_move_assignable_v<V<int, std::unique_ptr<int>>>);
        static_assert(std::is_trivially_move_assignable_v<V<int>>);

        static_assert(!std::is_copy_assignable_v<V<Value>>);
        static_assert(std::is_copy_assignable_v<V<int, std::shared_ptr<int>>>);
        static_assert(!std::is_nothrow_copy_assignable_v<V<int, utility::ThrowConstructible>>);
        static_assert(!std::is_trivially_copy_assignable_v<V<int, std::unique_ptr<int>>>);
        static_assert(std::is_trivially_copy_assignable_v<V<int>>);

        static_assert(!std::is_assignable_v<V<Value>, int>);
        static_assert(std::is_nothrow_assignable_v<V<NothrowIntAssignable>, int>);
        static_assert(std::is_assignable_v<V<NothrowIntAssignable>, char>);
        static_assert(!std::is_nothrow_assignable_v<V<ThrowIntAssignable>, int>);
        static_assert(std::is_assignable_v<V<ThrowIntAssignable>, int>);

        static_assert(std::is_swappable_v<V<utility::MoveThrowsSwappable>>);
        static_assert(std::is_nothrow_swappable_v<V<utility::Swappable>>);
        static_assert(!std::is_swappable_v<V<utility::Swappable, Value>>);
        static_assert(!std::is_nothrow_swappable_v<V<utility::MoveThrowsSwappable>>);
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
