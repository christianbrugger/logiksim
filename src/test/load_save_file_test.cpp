
#include "load_save_file.h"

#include "algorithm/to_path.h"
#include "logging.h"
#include "spatial_simulation.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(LoadSaveFile, Load210Files16BitCounter) {
    const auto file = to_path("example_circuits/2.1.0/16_bit_counter.ls2");
    auto result = load_circuit_from_file(file);
    EXPECT_EQ(result.success, true);

    // view point
    const auto view_point_expected = ViewPoint {
        .offset = point_fine_t {-17.731137763641335, 21.414292348529337},
        .device_scale = 14.87603305785139,
    };
    EXPECT_EQ(result.view_point, view_point_expected);

    // simulation config
    const auto simulation_config_expected = SimulationConfig {
        .simulation_time_rate = time_rate_t {7544318ns},
        .use_wire_delay = false,
    };
    EXPECT_EQ(result.simulation_config.simulation_time_rate.rate_per_second.count_ns(),
              simulation_config_expected.simulation_time_rate.rate_per_second.count_ns());
    EXPECT_EQ(result.simulation_config, simulation_config_expected);

    // counts
    EXPECT_EQ(result.editable_circuit.layout().logicitems().size(), 20);
    visible_selection_select_all(result.editable_circuit);
    EXPECT_EQ(result.editable_circuit.visible_selection().selected_segments().size(),
              100);
}

TEST(LoadSaveFile, Load210FilesAllComponents) {
    const auto file = to_path("example_circuits/2.1.0/all_components.ls2");
    auto result = load_circuit_from_file(file);

    EXPECT_EQ(result.success, true);
    EXPECT_EQ(result.editable_circuit.layout().logicitems().size(), 153);

    visible_selection_select_all(result.editable_circuit);
    EXPECT_EQ(result.editable_circuit.visible_selection().selected_segments().size(),
              374);
}

TEST(LoadSaveFile, Load210FilesCounterDisplay1To4) {
    const auto file = to_path("example_circuits/2.1.0/counter_display_1_to_4.ls2");
    auto result = load_circuit_from_file(file);

    EXPECT_EQ(result.success, true);
    EXPECT_EQ(result.editable_circuit.layout().logicitems().size(), 13);

    visible_selection_select_all(result.editable_circuit);
    EXPECT_EQ(result.editable_circuit.visible_selection().selected_segments().size(), 70);
}

TEST(LoadSaveFile, Load210FilesCounterStopsClock) {
    const auto file = to_path("example_circuits/2.1.0/counter_stops_clock.ls2");
    auto result = load_circuit_from_file(file);

    EXPECT_EQ(result.success, true);
    EXPECT_EQ(result.editable_circuit.layout().logicitems().size(), 8);

    visible_selection_select_all(result.editable_circuit);
    EXPECT_EQ(result.editable_circuit.visible_selection().selected_segments().size(), 35);

    // Run Simulation - until steady state is reached
    auto simulation = SpatialSimulation {
        Layout {result.editable_circuit.layout()},
        result.simulation_config.wire_delay_per_distance(),
    };
    EXPECT_EQ(simulation.simulation().time(), time_t {0us});
    simulation.simulation().run(simulation::RunConfig {.max_events = 1000});
    EXPECT_EQ(simulation.simulation().time(), time_t {5006us});
}

TEST(LoadSaveFile, Load210FilesJkFlipFlop) {
    const auto file = to_path("example_circuits/2.1.0/jk-flip-flop.ls2");
    auto result = load_circuit_from_file(file);

    EXPECT_EQ(result.success, true);
    EXPECT_EQ(result.editable_circuit.layout().logicitems().size(), 14);

    visible_selection_select_all(result.editable_circuit);
    EXPECT_EQ(result.editable_circuit.visible_selection().selected_segments().size(), 49);
}

}  // namespace logicsim
