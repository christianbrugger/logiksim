
#include "component/simulation/simulation_event.h"

#include <gtest/gtest.h>

namespace logicsim {

namespace simulation {

TEST(SimulationEventTest, EqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    EXPECT_TRUE(event1 == event2);

    SimulationEvent event3 {time_t {123us}, element_id_t {1}, connection_id_t {3}, true};
    SimulationEvent event4 {time_t {123us}, element_id_t {1}, connection_id_t {2}, false};
    EXPECT_TRUE(event3 == event4);
}

TEST(SimulationEventTest, LessThanOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event1 < event2);

    SimulationEvent event3 {time_t {123us}, element_id_t {1}, connection_id_t {4}, true};
    SimulationEvent event4 {time_t {123us}, element_id_t {3}, connection_id_t {2}, false};
    EXPECT_TRUE(event3 < event4);
}

TEST(SimulationEventTest, NotEqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event1 != event2);
}

TEST(SimulationEventTest, GreaterThanOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event2 > event1);
}

TEST(SimulationEventTest, LessThanOrEqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event1 <= event2);
}

TEST(SimulationEventTest, GreaterThanOrEqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event2 >= event1);
}

}  // namespace simulation

}  // namespace logicsim