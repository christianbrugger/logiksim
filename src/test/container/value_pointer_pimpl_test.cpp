

#include "test/container/value_pointer_pimpl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

//
// Strong Ordering
//

namespace logicsim {

namespace value_pointer_strong_ordering {

TEST(ContainerValuePointerPimpl, SOConstructDefault) {
    const auto x = IncompleteTypeTest {};
    ASSERT_EQ(x.value(), 0);
}

TEST(ContainerValuePointerPimpl, SOConstructValue) {
    const auto x = IncompleteTypeTest {2};
    ASSERT_EQ(x.value(), 2);
}

TEST(ContainerValuePointerPimpl, SOMoveCopy) {
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

TEST(ContainerValuePointerPimpl, SOEquality) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    ASSERT_EQ(a == b, false);
    ASSERT_EQ(a != b, true);
}

TEST(ContainerValuePointerPimpl, SOComparison) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    ASSERT_EQ(a < b, true);
    ASSERT_EQ(a > b, false);
}

TEST(ContainerValuePointerPimpl, SOSwap) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    swap(a, b);
}

}  // namespace value_pointer_strong_ordering

//
// No Comparison
//

namespace value_pointer_no_comparison {

TEST(ContainerValuePointerPimpl, NCConstructDefault) {
    const auto x = IncompleteTypeTest {};
    ASSERT_EQ(x.value(), 0);
}

TEST(ContainerValuePointerPimpl, NCConstructValue) {
    const auto x = IncompleteTypeTest {2};
    ASSERT_EQ(x.value(), 2);
}

TEST(ContainerValuePointerPimpl, NCMoveCopy) {
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

TEST(ContainerValuePointerPimpl, NCSwap) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    swap(a, b);
}

}  // namespace value_pointer_no_comparison

//
// Equality Only
//

namespace value_pointer_equality_only {

TEST(ContainerValuePointerPimpl, EOConstructDefault) {
    const auto x = IncompleteTypeTest {};
    ASSERT_EQ(x.value(), 0);
}

TEST(ContainerValuePointerPimpl, EOConstructValue) {
    const auto x = IncompleteTypeTest {2};
    ASSERT_EQ(x.value(), 2);
}

TEST(ContainerValuePointerPimpl, EOMoveCopy) {
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

TEST(ContainerValuePointerPimpl, EOEquality) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    ASSERT_EQ(a == b, false);
    ASSERT_EQ(a != b, true);
}

TEST(ContainerValuePointerPimpl, EOSwap) {
    auto a = IncompleteTypeTest {1};
    auto b = IncompleteTypeTest {2};

    swap(a, b);
}

}  // namespace value_pointer_equality_only

}  // namespace logicsim
