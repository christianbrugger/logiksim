
#include "core/load_save_file.h"

#include "core/file.h"
#include "core/spatial_simulation.h"
#include "core/vocabulary/save_format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// 2.1.0
//

TEST(LoadSaveFile, Load210Files16BitCounter) {
    const auto file = std::filesystem::path {"example_circuits/2.1.0/16_bit_counter.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    // view point
    const auto view_point_expected = ViewPoint {
        .offset = point_fine_t {-17.731137763641335, 21.414292348529337},
        .device_scale = 14.87603305785139,
    };
    EXPECT_EQ(result->view_point, view_point_expected);

    // simulation config
    const auto simulation_config_expected = SimulationConfig {
        .simulation_time_rate = time_rate_t {7544318ns},
        .use_wire_delay = false,
    };
    EXPECT_EQ(result->simulation_config.simulation_time_rate.rate_per_second.count_ns(),
              simulation_config_expected.simulation_time_rate.rate_per_second.count_ns());
    EXPECT_EQ(result->simulation_config, simulation_config_expected);

    // counts
    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 20);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              100);
}

TEST(LoadSaveFile, Load210FilesAllComponents) {
    const auto file = std::filesystem::path {"example_circuits/2.1.0/all_components.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 153);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              374);
}

TEST(LoadSaveFile, Load210FilesCounterDisplay1To4) {
    const auto file =
        std::filesystem::path {"example_circuits/2.1.0/counter_display_1_to_4.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 13);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              70);
}

TEST(LoadSaveFile, Load210FilesCounterStopsClock) {
    const auto file =
        std::filesystem::path {"example_circuits/2.1.0/counter_stops_clock.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 8);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              35);

    // Run Simulation - until steady state is reached
    auto simulation = SpatialSimulation {
        Layout {result->editable_circuit.layout()},
        result->simulation_config.wire_delay_per_distance(),
    };
    EXPECT_EQ(simulation.simulation().time(), time_t {0us});
    simulation.simulation().run(simulation::RunConfig {.max_events = 1000});
    EXPECT_EQ(simulation.simulation().time(), time_t {5006us});
}

TEST(LoadSaveFile, Load210FilesJkFlipFlop) {
    const auto file = std::filesystem::path {"example_circuits/2.1.0/jk-flip-flop.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 14);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              49);
}

//
// 2.2.0
//

TEST(LoadSaveFile, Load220FilesAllComponentsGzip) {
    const auto file = std::filesystem::path {"example_circuits/2.2.0/all_components.ls2"};
    ASSERT_EQ(guess_save_format(load_file(file).value()), SaveFormat::gzip);

    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 153);
    EXPECT_EQ(result->editable_circuit.layout().decorations().size(), 1);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              378);
}

TEST(LoadSaveFile, Load210FilesCounterDisplay1To4Gzip) {
    const auto file =
        std::filesystem::path {"example_circuits/2.2.0/counter_display_1_to_4_gzip.ls2"};
    ASSERT_EQ(guess_save_format(load_file(file).value()), SaveFormat::gzip);

    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 13);
    EXPECT_EQ(result->editable_circuit.layout().decorations().size(), 1);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              70);
}

TEST(LoadSaveFile, Load210FilesCounterDisplay1To4Json) {
    const auto file =
        std::filesystem::path {"example_circuits/2.2.0/counter_display_1_to_4_json.ls2"};
    ASSERT_EQ(guess_save_format(load_file(file).value()), SaveFormat::json);

    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 13);
    EXPECT_EQ(result->editable_circuit.layout().decorations().size(), 1);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              70);
}

TEST(LoadSaveFile, Load210FilesCounterDisplay1To4Base64) {
    const auto file = std::filesystem::path {
        "example_circuits/2.2.0/counter_display_1_to_4_base64.ls2"};
    ASSERT_EQ(guess_save_format(load_file(file).value()), SaveFormat::base64_gzip);

    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), true);

    EXPECT_EQ(result->editable_circuit.layout().logicitems().size(), 13);
    EXPECT_EQ(result->editable_circuit.layout().decorations().size(), 1);
    visible_selection_select_all(result->editable_circuit);
    EXPECT_EQ(result->editable_circuit.visible_selection().selected_segments().size(),
              70);
}

//
// Save and load
//

TEST(LoadSaveFile, SaveLoadExample1) {
    const auto file = std::filesystem::path {"test_example_1.ls2"};

    // generate
    auto rng = get_random_number_generator(4);
    auto editable_circuit = EditableCircuit {};
    add_example(rng, editable_circuit);
    EXPECT_GT(editable_circuit.layout().logicitems().size(), 0);
    EXPECT_GT(editable_circuit.layout().decorations().size(), 0);

    // save
    const auto success = save_circuit_to_file(editable_circuit.layout(), file, {}, {});
    ASSERT_EQ(success, true);

    // make sure it is gzip
    const auto binary = load_file(file);
    ASSERT_EQ(binary.has_value(), true);
    EXPECT_EQ(guess_save_format(binary.value()).value(), SaveFormat::gzip);

    // load
    auto load_result = load_circuit_from_file(file);
    ASSERT_EQ(load_result.has_value(), true);

    // compare
    EXPECT_EQ(are_normalized_equal(editable_circuit.extract_layout(),
                                   load_result->editable_circuit.extract_layout()),
              true);
}

//
// Error Handling
//

TEST(LoadSaveFile, ErrorMissingFile) {
    const auto file =
        std::filesystem::path {"example_circuits/errors/error_missing_file.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), false);
    EXPECT_EQ(result.error().type(), LoadErrorType::file_open_error);
}

TEST(LoadSaveFile, ErrorBase64Padding) {
    const auto file =
        std::filesystem::path {"example_circuits/errors/error_b64_padding.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), false);
    EXPECT_EQ(result.error().type(), LoadErrorType::base64_decode_error);
}

TEST(LoadSaveFile, ErrorBase64Symbol) {
    const auto file =
        std::filesystem::path {"example_circuits/errors/error_b64_symbol.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), false);
    EXPECT_EQ(result.error().type(), LoadErrorType::base64_decode_error);
}

TEST(LoadSaveFile, ErrorGZipCRC) {
    const auto file =
        std::filesystem::path {"example_circuits/errors/error_gzip_crc.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), false);
    EXPECT_EQ(result.error().type(), LoadErrorType::gzip_decompress_error);
}

TEST(LoadSaveFile, ErrorVersionMissing) {
    const auto file =
        std::filesystem::path {"example_circuits/errors/error_version_missing.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), false);
    EXPECT_EQ(result.error().type(), LoadErrorType::json_parse_error);
}

TEST(LoadSaveFile, ErrorVersion1020) {
    const auto file =
        std::filesystem::path {"example_circuits/errors/error_version_10.2.0.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), false);
    EXPECT_EQ(result.error().type(), LoadErrorType::json_version_error);

    const auto message = result.error().format();
    EXPECT_EQ(message.find("10.2.0") != std::string::npos, true);
    EXPECT_EQ(message.find("10.3.0") != std::string::npos, false);
}

TEST(LoadSaveFile, ErrorVersionUnknown) {
    const auto file =
        std::filesystem::path {"example_circuits/errors/error_version_unknown.ls2"};
    auto result = load_circuit_from_file(file);
    ASSERT_EQ(result.has_value(), false);
    EXPECT_EQ(result.error().type(), LoadErrorType::json_version_error);
}

}  // namespace logicsim
