#include "../variant_ii.hpp"

#include "test_i.hpp"
#include "test_ii.hpp"

namespace test {
namespace utility {
template<>
struct TestName<variant_ii::Variant> {
    inline static const std::string name = "variant_ii::Variant";
};
} // namespae utility
} // namespace test

int main() {
    test::TestSuiteI<variant_ii::Variant>::test();
    test::TestSuiteII<std::variant, variant_ii::Variant>::test();
}
