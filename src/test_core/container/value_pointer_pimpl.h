#ifndef LOGICSIM_TEST_CONTAINER_VALUE_POINTER_PIMPL_H
#define LOGICSIM_TEST_CONTAINER_VALUE_POINTER_PIMPL_H

#include "core/container/value_pointer.h"

namespace logicsim {

//
// Strong Ordering
//

namespace value_pointer_strong_ordering {

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
    value_pointer<incomplete_type, equality_comparable, std::strong_ordering> value_;
};

inline auto swap(IncompleteTypeTest& a, IncompleteTypeTest& b) noexcept -> void {
    ::logicsim::swap(a.value_, b.value_);
}

}  // namespace value_pointer_strong_ordering

extern template class value_pointer<value_pointer_strong_ordering::incomplete_type,
                                    equality_comparable, std::strong_ordering>;

//
// No Comparison
//

namespace value_pointer_no_comparison {

struct incomplete_type;

class IncompleteTypeTest {
   public:
    IncompleteTypeTest() = default;
    IncompleteTypeTest(int value);

    [[nodiscard]] auto value() const -> int;

    friend auto swap(IncompleteTypeTest& a, IncompleteTypeTest& b) noexcept -> void;

   private:
    value_pointer<incomplete_type> value_;
};

inline auto swap(IncompleteTypeTest& a, IncompleteTypeTest& b) noexcept -> void {
    ::logicsim::swap(a.value_, b.value_);
}

}  // namespace value_pointer_no_comparison

extern template class value_pointer<value_pointer_no_comparison::incomplete_type>;

//
// Equality Only
//

namespace value_pointer_equality_only {

struct incomplete_type;

class IncompleteTypeTest {
   public:
    IncompleteTypeTest() = default;
    IncompleteTypeTest(int value);

    [[nodiscard]] auto value() const -> int;

    [[nodiscard]] auto operator==(const IncompleteTypeTest&) const -> bool = default;

    friend auto swap(IncompleteTypeTest& a, IncompleteTypeTest& b) noexcept -> void;

   private:
    value_pointer<incomplete_type, equality_comparable> value_;
};

inline auto swap(IncompleteTypeTest& a, IncompleteTypeTest& b) noexcept -> void {
    ::logicsim::swap(a.value_, b.value_);
}

}  // namespace value_pointer_equality_only

extern template class value_pointer<value_pointer_equality_only::incomplete_type,
                                    equality_comparable>;

}  // namespace logicsim

#endif
