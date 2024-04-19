#ifndef LOGICSIM_TEST_CONTAINER_VALUE_POINTER_PIMPL_H
#define LOGICSIM_TEST_CONTAINER_VALUE_POINTER_PIMPL_H

#include "container/value_pointer.h"

namespace logicsim {

namespace value_pointer_test {

struct incomplete_type;

class IncompleteTypeTest {
   public:
    IncompleteTypeTest() = default;
    IncompleteTypeTest(int value);

    [[nodiscard]] auto value() const -> int;

    [[nodiscard]] auto operator==(const IncompleteTypeTest&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const IncompleteTypeTest&) const = default;

    friend auto swap(IncompleteTypeTest& a, IncompleteTypeTest& b) noexcept -> void;

   private:
    value_pointer<incomplete_type, std::strong_ordering> value_;
};

inline auto swap(IncompleteTypeTest& a, IncompleteTypeTest& b) noexcept -> void {
    ::logicsim::swap(a.value_, b.value_);
}

}  // namespace value_pointer_test

extern template class value_pointer<value_pointer_test::incomplete_type,
                                    std::strong_ordering>;

}  // namespace logicsim

#endif
