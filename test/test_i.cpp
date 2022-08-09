#include "../variant_i.hpp"

#include "test_i.hpp"

namespace static_test {
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

namespace test {
template<>
struct TestName<variant_i::Variant> {
    inline static const std::string name = "variant_i::Variant";
};
}

int main() {
    test::TestSuiteI<std::variant, variant_i::Variant>::test();
}
