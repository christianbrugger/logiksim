
#include "core/component/simulation/simulation_event.h"

#include <gtest/gtest.h>

namespace logicsim {

namespace simulation {

TEST(SimulationEventTest, EqualOperatorTest) {
    {
        simulation_event_t event1 {time_t {123us}, element_id_t {1}, connection_id_t {2},
                                   true};
        simulation_event_t event2 {time_t {123us}, element_id_t {1}, connection_id_t {2},
                                   true};
        // event1 == event2
        EXPECT_EQ(greater_time_element_id {}(event1, event2), false);
        EXPECT_EQ(greater_time_element_id {}(event2, event1), false);
    }
    {
        simulation_event_t event3 {time_t {123us}, element_id_t {1}, connection_id_t {3},
                                   true};
        simulation_event_t event4 {time_t {123us}, element_id_t {1}, connection_id_t {2},
                                   false};

        // event3 == event4
        EXPECT_EQ(greater_time_element_id {}(event3, event4), false);
        EXPECT_EQ(greater_time_element_id {}(event4, event3), false);
    }
}

TEST(SimulationEventTest, LessThanOperatorTest) {
    {
        simulation_event_t event1 {time_t {123us}, element_id_t {1}, connection_id_t {2},
                                   true};
        simulation_event_t event2 {time_t {789us}, element_id_t {3}, connection_id_t {4},
                                   false};

        // event1 < event2
        EXPECT_EQ(greater_time_element_id {}(event1, event2), false);
        EXPECT_EQ(greater_time_element_id {}(event2, event1), true);
    }
    {
        // event1 < event2
        simulation_event_t event3 {time_t {123us}, element_id_t {1}, connection_id_t {4},
                                   true};
        simulation_event_t event4 {time_t {123us}, element_id_t {3}, connection_id_t {2},
                                   false};

        // event3 < event4
        EXPECT_EQ(greater_time_element_id {}(event3, event4), false);
        EXPECT_EQ(greater_time_element_id {}(event4, event3), true);
    }
}

}  // namespace simulation

}  // namespace logicsim