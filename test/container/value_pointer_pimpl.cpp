#include "test/container/value_pointer_pimpl.h"

namespace logicsim {

namespace value_pointer_test {

struct incomplete_type {
    int value;
};

}  // namespace value_pointer_test

template class value_pointer<value_pointer_test::incomplete_type>;

namespace value_pointer_test {

IncompleteTypeTest::IncompleteTypeTest(int value) : value_ {std::in_place, value} {}

auto IncompleteTypeTest::value() const -> int {
    return value_->value;
}

}  // namespace value_pointer_test

}  // namespace logicsim
