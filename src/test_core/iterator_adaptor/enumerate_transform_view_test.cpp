
#include "core/algorithm/to_vector.h"
#include "core/iterator_adaptor/enumerate.h"
#include "core/iterator_adaptor/transform_view.h"
#include "core/logging.h"

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
    ASSERT_THAT(to_vector(transformed), testing::ElementsAre(0, 3, 8));
}

TEST(IteratorAdaptorEnumerateTF, TransformViewOwning) {
    const auto transformed = [] {
        return transform_view(enumerate<int>(std::vector<int> {2, 3, 4}),
                              [](auto pair) { return pair.first * pair.second; });
    }();

    ASSERT_THAT(transformed, testing::ElementsAre(0, 3, 8));
    ASSERT_THAT(to_vector(transformed), testing::ElementsAre(0, 3, 8));
}

}  // namespace logicsim