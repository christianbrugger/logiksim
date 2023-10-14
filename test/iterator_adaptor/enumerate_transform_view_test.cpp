
#include "iterator_adaptor/enumerate.h"
#include "iterator_adaptor/transform_view.h"
#include "logging.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

TEST(IteratorAdaptorEnumerateTF, TransformViewNonOwning) {
    const auto container = std::vector<int> {2, 3, 4};

    const auto enumerated = enumerate<int>(container);

    const auto transformed =
        transform_view(enumerated, [](auto pair) { return pair.first * pair.second; });

    ASSERT_THAT(transformed, testing::ElementsAre(0, 3, 8));

    const auto result = std::vector<int>(transformed.begin(), transformed.end());

    ASSERT_THAT(result, testing::ElementsAre(0, 3, 8));
}

TEST(IteratorAdaptorEnumerateTF, TransformViewOwning) {
    const auto transformed = [] {
        return transform_view(enumerate<int>(std::vector<int> {2, 3, 4}),
                              [](auto pair) { return pair.first * pair.second; });
    }();

    ASSERT_THAT(transformed, testing::ElementsAre(0, 3, 8));

    const auto result = std::vector<int>(transformed.begin(), transformed.end());

    ASSERT_THAT(result, testing::ElementsAre(0, 3, 8));
}

}  // namespace logicsim