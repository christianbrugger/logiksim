#include "test/container/value_pointer_pimpl.h"

namespace logicsim {

//
// Strong Ordering
//

namespace value_pointer_strong_ordering {

struct incomplete_type {
    int value;

    [[nodiscard]] auto operator==(const incomplete_type&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const incomplete_type&) const = default;
};

IncompleteTypeTest::IncompleteTypeTest(int value) : value_ {std::in_place, value} {}

auto IncompleteTypeTest::value() const -> int {
    return value_->value;
}

}  // namespace value_pointer_strong_ordering
template class value_pointer<value_pointer_strong_ordering::incomplete_type,
                             std::true_type, std::strong_ordering>;

//
// No Comparison
//

namespace value_pointer_no_comparison {

struct incomplete_type {
    int value;
};

IncompleteTypeTest::IncompleteTypeTest(int value) : value_ {std::in_place, value} {}

auto IncompleteTypeTest::value() const -> int {
    return value_->value;
}

}  // namespace value_pointer_no_comparison
template class value_pointer<value_pointer_no_comparison::incomplete_type>;

//
// Equality Only
//

namespace value_pointer_equality_only {

struct incomplete_type {
    int value;

    [[nodiscard]] auto operator==(const incomplete_type&) const -> bool = default;
};

IncompleteTypeTest::IncompleteTypeTest(int value) : value_ {std::in_place, value} {}

auto IncompleteTypeTest::value() const -> int {
    return value_->value;
}

}  // namespace value_pointer_equality_only
template class value_pointer<value_pointer_equality_only::incomplete_type, std::true_type,
                             void>;

}  // namespace logicsim
