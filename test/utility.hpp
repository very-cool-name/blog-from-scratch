#pragma once

#include <string>
#include <variant>

namespace test {
template<template<typename...> typename V>
struct TestName;

template<>
struct TestName<std::variant> {
    inline static const std::string name = "std::variant";
};
} // namespace test
