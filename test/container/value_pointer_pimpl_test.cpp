

#include "test/container/value_pointer_pimpl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim::value_pointer_test {

TEST(ContainerValuePointerPimpl, ConstructDefault) {
    const auto x = IncompleteTypeTest {};
    ASSERT_EQ(x.value(), 0);
}

TEST(ContainerValuePointerPimpl, ConstructValue) {
    const auto x = IncompleteTypeTest {2};
    ASSERT_EQ(x.value(), 2);
}

TEST(ContainerValuePointerPimpl, MoveCopy) {
    auto a = IncompleteTypeTest {2};

    ASSERT_EQ(a.value(), 2);

    auto b = IncompleteTypeTest {std::move(a)};
    auto c = IncompleteTypeTest {b};

    ASSERT_EQ(b.value(), 2);
    ASSERT_EQ(c.value(), 2);

    auto d = IncompleteTypeTest {};
    auto e = IncompleteTypeTest {};

    d = std::move(b);
    e = c;

    ASSERT_EQ(d.value(), 2);
    ASSERT_EQ(e.value(), 2);
}

TEST(ContainerValuePointerPimpl, Equality) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    ASSERT_EQ(a == b, false);
    ASSERT_EQ(a != b, true);
}

TEST(ContainerValuePointerPimpl, Comparison) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    ASSERT_EQ(a < b, true);
    ASSERT_EQ(a > b, false);
}

TEST(ContainerValuePointerPimpl, Swap) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    swap(a, b);
}

}  // namespace logicsim::value_pointer_test