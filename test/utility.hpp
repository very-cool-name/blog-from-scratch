#pragma once

#include <string>
#include <type_traits>
#include <variant>

namespace test {
namespace utility {
template<template<typename...> typename V>
struct TestName;

template<>
struct TestName<std::variant> {
    inline static const std::string name = "std::variant";
};

struct MoveAssignable {
    MoveAssignable(int& move_assigned)
    :move_assigned{move_assigned} {}

    MoveAssignable(MoveAssignable&& other) = default;

    MoveAssignable& operator=(MoveAssignable&& other) {
        ++move_assigned;
        return *this;
    }

    int& move_assigned;
};

struct CopyAssignable {
    CopyAssignable(int& copy_assigned)
    :copy_assigned{copy_assigned} {}

    CopyAssignable(const CopyAssignable&) = default;
    CopyAssignable(CopyAssignable&&) = default;

    CopyAssignable& operator=(const CopyAssignable& other) {
        ++copy_assigned;
        return *this;
    }

    CopyAssignable& operator=(CopyAssignable&&) = delete;

    int& copy_assigned;
};

struct DestructCountable {
    DestructCountable() = default;

    DestructCountable(const DestructCountable& other) = default;
    DestructCountable(DestructCountable&& other) = default;

    DestructCountable& operator=(const DestructCountable& other) {
        destructed = other.destructed;
        return *this;
    }
    DestructCountable& operator=(DestructCountable&& other) {
        destructed = other.destructed;
        return *this;
    }

    ~DestructCountable() {
        if (destructed) {
            ++(*destructed);
        }
    }

    int* destructed = nullptr;
};

struct ThrowConstructible {
    ThrowConstructible() {
        throw std::runtime_error("ThrowConstructbile throws on construction");
    }

    ThrowConstructible(const ThrowConstructible&) noexcept(false) {}
    ThrowConstructible(ThrowConstructible&&) noexcept(false) {}

    ThrowConstructible& operator=(const ThrowConstructible&) noexcept(false) {return *this;}
    ThrowConstructible& operator=(ThrowConstructible&&) noexcept(false) { return *this; }
};

struct TestThrow: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Swappable {
    Swappable() {}
    Swappable(Swappable&&) noexcept
    :moved(true) {}

    bool moved = false;
    bool swapped = false;
};

void swap(Swappable& a, Swappable& b) noexcept {
    a.swapped = true;
    b.swapped = true;
}

struct MoveThrowsSwappable {
    MoveThrowsSwappable() = default;
    MoveThrowsSwappable(MoveThrowsSwappable&&) {
        throw utility::TestThrow("MoveThrowsSwappable throws on move");
    }
};

void swap(MoveThrowsSwappable& a, MoveThrowsSwappable& b) {}

struct MovedOut {
    MovedOut() = default;

    MovedOut(MovedOut&& other) {
        other.moved_out = true;
    }

    bool moved_out = false;
};

void swap(MovedOut& a, MovedOut& b) {
    std::swap(a.moved_out, b.moved_out);
}

} // namespace utility
} // namespace test
