
#include "component/simulation/history_view.h"

#include "component/simulation/history_buffer.h"
#include "vocabulary/time.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace simulation {

// size

TEST(SimulationTest, HistoryViewSize) {
    auto time = time_t {100us};
    auto history_length = delay_t {7us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 2);
}

TEST(SimulationTest, HistoryViewSizeExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 2);
}

TEST(SimulationTest, HistoryViewSizeLast) {
    auto time = time_t {100us};
    auto history_length = delay_t {20us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 3);
}

TEST(SimulationTest, HistoryViewSizeEmpty) {
    auto time = time_t {10us};
    auto history_length = delay_t {20us};
    auto history = HistoryBuffer {};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 1);
}

TEST(SimulationTest, HistoryViewSizeNegative) {
    auto time = time_t {10us};
    auto history_length = delay_t {20us};
    auto history = HistoryBuffer {time_t {5us}, time_t {7us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 3);
}

TEST(SimulationTest, HistoryViewEmpty) {
    const auto view = HistoryView {};

    ASSERT_THAT(view.size(), 1);
    ASSERT_THAT(view.end() - view.begin(), 1);

    ASSERT_THAT(view.last_value(), false);
    ASSERT_THAT(view.value(time_t {0us}), false);

    const auto value = *view.begin();
    ASSERT_THAT(value.first_time, time_t::min());
    ASSERT_THAT(value.last_time, time_t::max());
    ASSERT_THAT(value.value, false);

    ASSERT_THAT(view.until(time_t {100us}) - view.from(time_t {0us}), 1);
}

// begin end iteration

TEST(SimulationTest, HistoryViewBeginEndExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    auto begin = view.begin();
    auto end = view.end();

    ASSERT_THAT(view.size(), 2);
    ASSERT_THAT(end - begin, 2);

    ASSERT_THAT(begin == end, false);
    auto value0 = *(begin++);
    ASSERT_THAT(begin == end, false);
    auto value1 = *(begin++);
    ASSERT_THAT(begin == end, true);

    ASSERT_THAT(value0.first_time, time_t::min());
    ASSERT_THAT(value0.last_time, time_t {95us});
    ASSERT_THAT(value0.value, true);

    ASSERT_THAT(value1.first_time, time_t {95us});
    ASSERT_THAT(value1.last_time, time_t {100us});
    ASSERT_THAT(value1.value, false);
}

TEST(SimulationTest, HistoryViewBeginEndFull) {
    auto time = time_t {100us};
    auto history_length = delay_t {50us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    auto begin = view.begin();
    auto end = view.end();

    ASSERT_THAT(view.size(), 3);
    ASSERT_THAT(end - begin, 3);

    ASSERT_THAT(begin == end, false);
    auto value0 = *(begin++);
    ASSERT_THAT(begin == end, false);
    auto value1 = *(begin++);
    ASSERT_THAT(begin == end, false);
    auto value2 = *(begin++);
    ASSERT_THAT(begin == end, true);

    ASSERT_THAT(value0.first_time, time_t::min());
    ASSERT_THAT(value0.last_time, time_t {90us});
    ASSERT_THAT(value0.value, false);

    ASSERT_THAT(value1.first_time, time_t {90us});
    ASSERT_THAT(value1.last_time, time_t {95us});
    ASSERT_THAT(value1.value, true);

    ASSERT_THAT(value2.first_time, time_t {95us});
    ASSERT_THAT(value2.last_time, time_t {100us});
    ASSERT_THAT(value2.value, false);
}

// before

TEST(SimulationTest, HistoryViewFromExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {95us});
    ASSERT_THAT(view.end() - from, 1);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t {95us});
    ASSERT_THAT(value.last_time, time_t {100us});
    ASSERT_THAT(value.value, false);
}

TEST(SimulationTest, HistoryViewFrom) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {96us});
    ASSERT_THAT(view.end() - from, 1);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t {95us});
    ASSERT_THAT(value.last_time, time_t {100us});
    ASSERT_THAT(value.value, false);
}

TEST(SimulationTest, HistoryViewFromSecond) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {90us});
    ASSERT_THAT(view.end() - from, 2);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t::min());
    ASSERT_THAT(value.last_time, time_t {95us});
    ASSERT_THAT(value.value, true);
}

TEST(SimulationTest, HistoryViewFromSmall) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {50us});
    ASSERT_THAT(view.end() - from, 2);
}

// until

TEST(SimulationTest, HistoryViewUntil) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    auto from = view.from(time_t {90us});
    auto until = view.until(time_t {96us});
    ASSERT_THAT(view.end() - from, 2);
    ASSERT_THAT(until - from, 2);
}

TEST(SimulationTest, HistoryViewUntilExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto epsilon = time_t::epsilon();

    auto view = HistoryView {history, time, last_value, history_length};

    auto from = view.from(time_t {90us});
    ASSERT_THAT(view.end() - from, 2);

    ASSERT_THAT(view.until(time_t {95us} + epsilon) - from, 2);
    ASSERT_THAT(view.until(time_t {95us}) - from, 1);
}

TEST(SimulationTest, HistoryViewFromUntilBounds) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.end() - view.begin(), 2);

    ASSERT_THAT(view.from(time_t::min()) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {-100us}) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {0us}) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {50us}) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {99us}) - view.begin(), 1);
    ASSERT_THAT(view.from(time_t {100us}) - view.begin(), 1);

    ASSERT_THAT(view.until(time_t::min()) - view.begin(), 1);
    ASSERT_THAT(view.until(time_t {50us}) - view.begin(), 1);
    ASSERT_THAT(view.until(time_t {100us}) - view.begin(), 2);
}

// value

TEST(SimulationTest, HistoryViewValueFull) {
    auto time = time_t {100us};
    auto history_length = delay_t {50us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto epsilon = time_t::epsilon();

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.value(time_t::min()), false);
    ASSERT_THAT(view.value(time_t {-100us}), false);
    ASSERT_THAT(view.value(time_t {0us}), false);

    ASSERT_THAT(view.value(time_t {90us} - epsilon), false);
    ASSERT_THAT(view.value(time_t {90us}), true);

    ASSERT_THAT(view.value(time_t {95us} - epsilon), true);
    ASSERT_THAT(view.value(time_t {95us}), false);

    ASSERT_THAT(view.value(time_t {100us}), false);
}

TEST(SimulationTest, HistoryViewValuePartialHistory) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto epsilon = time_t::epsilon();

    auto view = HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.value(time_t::min()), true);
    ASSERT_THAT(view.value(time_t {-100us}), true);
    ASSERT_THAT(view.value(time_t {0us}), true);

    ASSERT_THAT(view.value(time_t {90us} - epsilon), true);
    ASSERT_THAT(view.value(time_t {90us}), true);

    ASSERT_THAT(view.value(time_t {95us} - epsilon), true);
    ASSERT_THAT(view.value(time_t {95us}), false);

    ASSERT_THAT(view.value(time_t {100us}), false);
}

TEST(SimulationTest, HistoryViewIteratorValues) {
    const auto time = time_t {100us};
    const auto history_length = delay_t {100us};
    const auto history = HistoryBuffer {time_t {90us}, time_t {95us}};
    const auto last_value = false;

    const auto view = HistoryView {history, time, last_value, history_length};

    {
        auto it = view.from(time_t {95us});
        const auto end = view.until(time_t {100us});

        ASSERT_THAT((*it).first_time, time_t {95us});
        ASSERT_THAT((*it).last_time, time_t {100us});
        ASSERT_THAT((*it).value, false);

        ASSERT_THAT(end - it, 1);
        it++;
        ASSERT_THAT(it == end, true);
    }

    {
        auto it = view.from(time_t {92us});
        const auto end = view.until(time_t {95us});

        ASSERT_THAT((*it).first_time, time_t {90us});
        ASSERT_THAT((*it).last_time, time_t {95us});
        ASSERT_THAT((*it).value, true);

        ASSERT_THAT(end - it, 1);
        it++;
        ASSERT_THAT(it == end, true);
    }
}

}  // namespace simulation
}  // namespace logicsim