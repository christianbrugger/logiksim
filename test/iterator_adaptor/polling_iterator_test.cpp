
#include "iterator_adaptor/polling_iterator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ranges>
#include <vector>

namespace logicsim {

TEST(IteratorAdaptorPollingIterator, SimpleRange) {
    struct State {
        int start;
        int stop;

        auto operator==(const State&) const -> bool = default;
    };

    const auto mutator = [](State& s) -> polling_status {
        ++s.start;
        return s.start > s.stop ? polling_status::stop : polling_status::iterate;
    };

    const auto getter = [](const State& s) -> int { return s.start; };

    {
        const auto view = polling_view<int, State>(
            mutator, getter, State {.start = 0, .stop = 3}, polling_status::iterate);
        ASSERT_THAT(view, testing::ElementsAre(0, 1, 2, 3));
    }

    {
        const auto view = polling_view<int, State>(
            mutator, getter, State {.start = 1, .stop = 2}, polling_status::iterate);

        auto vector = std::vector<int> {};
        std::ranges::copy(view.begin(), view.end(), std::back_inserter(vector));

        ASSERT_THAT(vector, testing::ElementsAre(1, 2));
    }

    {
        const auto view = polling_view<int, State>(
            mutator, getter, State {.start = 1, .stop = 0}, polling_status::stop);

        ASSERT_THAT(view, testing::ElementsAre());
    }
}

}  // namespace logicsim