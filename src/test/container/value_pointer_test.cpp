
#include "container/value_pointer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

static_assert(sizeof(value_pointer<int>) == sizeof(int*));
static_assert(sizeof(value_pointer<std::string>) == sizeof(std::string*));

TEST(ContainerValuePointer, ConstIntDefault) {
    const auto x = value_pointer<int> {};

    ASSERT_EQ(x.value(), 0);
    ASSERT_EQ(*x, 0);

    static_assert(!std::is_assignable_v<decltype(x.value()), int>);
    static_assert(!std::is_assignable_v<decltype(*x), int>);
}

TEST(ContainerValuePointer, MutableInt) {
    auto x = value_pointer<int> {1};

    ASSERT_EQ(x.value(), 1);
    *x = 2;
    ASSERT_EQ(*x, 2);
    x.value() = 3;
    ASSERT_EQ(x.value(), 3);

    static_assert(std::is_assignable_v<decltype(x.value()), int>);
    static_assert(std::is_assignable_v<decltype(*x), int>);
}

TEST(ContainerValuePointer, SwapInt) {
    auto a = value_pointer<int> {2};
    auto b = value_pointer<int> {4};

    swap(a, b);

    ASSERT_EQ(*a, 4);
    ASSERT_EQ(*b, 2);
}

TEST(ContainerValuePointer, CopyConstructInt) {
    auto a = value_pointer<int> {2};
    auto b = value_pointer<int> {a};

    ASSERT_EQ(*a, 2);
    ASSERT_EQ(*b, 2);
}

TEST(ContainerValuePointer, CopyAssignInt) {
    auto a = value_pointer<int> {2};
    auto b = value_pointer<int> {};

    b = a;

    ASSERT_EQ(*a, 2);
    ASSERT_EQ(*b, 2);
}

//
// Move Only Type
//

struct MoveOnlyType {
    MoveOnlyType() = default;
    explicit MoveOnlyType(int v) : value {v} {};
    ~MoveOnlyType() = default;

    MoveOnlyType(const MoveOnlyType&) = delete;
    auto operator=(const MoveOnlyType&) -> MoveOnlyType& = delete;

    MoveOnlyType(MoveOnlyType&&) = default;
    auto operator=(MoveOnlyType&&) -> MoveOnlyType& = default;

    int value {};
};

TEST(ContainerValuePointer, MoveOnlyConstructDefault) {
    const auto val = value_pointer<MoveOnlyType> {};

    ASSERT_EQ(val->value, 0);

    static_assert(!std::is_assignable_v<decltype(*val), MoveOnlyType>);
}

TEST(ContainerValuePointer, MoveOnlyConstructWithMoved) {
    auto obj = MoveOnlyType {2};
    const auto val = value_pointer<MoveOnlyType> {std::move(obj)};

    ASSERT_EQ(val->value, 2);
}

TEST(ContainerValuePointer, MoveOnlyConstruct) {
    auto val = value_pointer<MoveOnlyType> {MoveOnlyType {2}};

    ASSERT_EQ(val->value, 2);

    *val = MoveOnlyType {0};
    ASSERT_EQ(val->value, 0);

    val->value = 1;
    ASSERT_EQ(val->value, 1);

    static_assert(std::is_assignable_v<decltype(*val), MoveOnlyType>);
}

TEST(ContainerValuePointer, MoveOnlyInplaceConstruct) {
    static_assert(std::is_constructible_v<value_pointer<MoveOnlyType>, MoveOnlyType>);
    static_assert(!std::is_constructible_v<value_pointer<MoveOnlyType>, int>);

    auto a = value_pointer<MoveOnlyType> {std::in_place, 2};

    ASSERT_EQ(a->value, 2);
}

TEST(ContainerValuePointer, MoveConstructMoveOnly) {
    auto a = value_pointer<MoveOnlyType> {MoveOnlyType {2}};
    auto b = value_pointer<MoveOnlyType> {std::move(a)};

    ASSERT_EQ(b->value, 2);
}

TEST(ContainerValuePointer, MoveAssignMoveOnly) {
    auto a = value_pointer<MoveOnlyType> {MoveOnlyType {2}};
    auto b = value_pointer<MoveOnlyType> {};

    b = std::move(a);

    ASSERT_EQ(b->value, 2);
}

TEST(ContainerValuePointer, MoveValueMoveOnly1) {
    auto a = value_pointer<MoveOnlyType> {MoveOnlyType {2}};

    auto b_val = MoveOnlyType {std::move(a.value())};

    ASSERT_EQ(b_val.value, 2);
}

TEST(ContainerValuePointer, MoveValueMoveOnly2) {
    auto a = value_pointer<MoveOnlyType> {MoveOnlyType {2}};

    auto b_val = MoveOnlyType {};
    b_val = std::move(a.value());

    ASSERT_EQ(b_val.value, 2);
}

TEST(ContainerValuePointer, MoveValueMoveOnly3) {
    auto a = value_pointer<MoveOnlyType> {MoveOnlyType {}};

    auto b_val = MoveOnlyType {2};

    a.value() = std::move(b_val);

    ASSERT_EQ(a->value, 2);
}

TEST(ContainerValuePointer, MoveValueMoveOnly4) {
    auto a = value_pointer<MoveOnlyType> {MoveOnlyType {}};

    auto b_val = MoveOnlyType {2};

    *a = std::move(b_val);

    ASSERT_EQ(a->value, 2);
}

//
// Comparable Struct
//

struct ComparableType {
    ComparableType() = default;
    explicit ComparableType(std::string v) : value {std::move(v)} {};

    [[nodiscard]] auto operator==(const ComparableType&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const ComparableType&) const = default;

    std::string value {};
};

TEST(ContainerValuePointer, ComparableTypeConstruction) {
    auto a = value_pointer<ComparableType> {ComparableType {"abc"}};

    ASSERT_EQ(a->value, "abc");
}

TEST(ContainerValuePointer, ComparableTypeEqual) {
    const auto a = value_pointer_complete<ComparableType> {ComparableType {"abc"}};
    const auto b = value_pointer_complete<ComparableType> {ComparableType {"abc"}};
    const auto c = value_pointer_complete<ComparableType> {ComparableType {"efg"}};

    ASSERT_EQ(a == b, true);
    ASSERT_EQ(b == a, true);
    ASSERT_EQ(a == c, false);
    ASSERT_EQ(b == c, false);

    ASSERT_EQ(a != c, true);
    ASSERT_EQ(b != c, true);
}

TEST(ContainerValuePointer, ComparableTypeThreeWay) {
    const auto a = value_pointer_complete<ComparableType> {ComparableType {"abc"}};
    const auto b = value_pointer_complete<ComparableType> {ComparableType {"efg"}};

    ASSERT_EQ(a < b, true);
    ASSERT_EQ(a > b, false);

    ASSERT_EQ(a <= b, true);
    ASSERT_EQ(a >= b, false);
}

// TODO test swap

}  // namespace logicsim
