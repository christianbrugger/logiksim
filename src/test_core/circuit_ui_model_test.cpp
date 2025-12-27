#include "core/circuit_ui_model.h"

#include "core/default_element_definition.h"
#include "core/geometry/scene.h"
#include "core/load_save_file.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

namespace logicsim {

namespace circuit_ui_model {

constexpr static auto TEST_FILE_OPEN = "circuit_ui_model_open.ls2";
constexpr static auto TEST_FILE_SAVE = "circuit_ui_model_save.ls2";
constexpr static auto TEST_FILE_FOLDER = "circuit_ui_model_folder";

[[nodiscard]] auto insert_button(CircuitUIModel& model, point_t position) {
    auto status = UIStatus {};

    const auto point_device = to_device_fine(position, model.view_config());

    status |= set_circuit_state(model, EditingState {DefaultMouseAction::insert_button});
    status |= model.mouse_press(MousePressEvent {
        .position = point_device,
        .modifiers = {},
        .button = MouseButton::Left,
        .double_click = false,
    });
    status |= model.mouse_release(MouseReleaseEvent {
        .position = point_device,
        .button = MouseButton::Left,
    });

    return status;
}

[[nodiscard]] auto test_layout() -> Layout {
    auto layout = Layout {};
    layout.logicitems().add(default_element_definition(LogicItemType::button),
                            point_t {1, 1}, display_state_t::normal);
    return layout;
}

auto save_test_file() -> void {
    const auto default_view_point = ViewConfig {}.view_point();
    const auto default_simulation_config = SimulationConfig {};

    EXPECT_TRUE(save_circuit_to_file(test_layout(), TEST_FILE_OPEN, default_view_point,
                                     default_simulation_config));
}

auto load_layout_file(const std::filesystem::path& filename) -> Layout {
    auto result = load_circuit_from_file(filename);
    return result.value().editable_circuit.extract_layout();
}

// TEST(CircuitUIModelModal, ModalDialog) {
//     auto model = CircuitUIModel {};
//     auto status = UIStatus {};
//
//     status |= insert_button(model, point_t {10, 10});
//     status |= insert_button(model, point_t {0, 0});
//     print(model.layout());
// }

TEST(CircuitUIModelModal, NewFileFromEmpty) {
    auto model = CircuitUIModel {};

    const auto result = model.file_action(FileAction::new_file);
    EXPECT_TRUE(!result.next_step);
    EXPECT_TRUE(model.layout().empty());
}

TEST(CircuitUIModelModal, ExampleFromEmpty) {
    auto model = CircuitUIModel {};

    const auto result = model.file_action(FileAction::load_example_simple);
    EXPECT_TRUE(!result.next_step);
    EXPECT_TRUE(!model.layout().empty());
}

TEST(CircuitUIModelModal, OpenFromEmptyOpen) {
    auto model = CircuitUIModel {};

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        EXPECT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        EXPECT_TRUE(!result.next_step);
        EXPECT_EQ(model.layout(), test_layout());
    }
}

TEST(CircuitUIModelModal, OpenFromEmptyCancel) {
    auto model = CircuitUIModel {};

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        EXPECT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(OpenFileCancel {});
        EXPECT_TRUE(!result.next_step);
        EXPECT_EQ(model.layout(), Layout {});
    }
}

TEST(CircuitUIModelModal, OpenFromEmptyError) {
    auto model = CircuitUIModel {};

    const auto file =
        std::filesystem::path {"example_circuits/errors/error_version_unknown.ls2"};

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        EXPECT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(OpenFileOpen {file});
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<OpenFileError>(error);
        EXPECT_EQ(open_error.filename, file);
    }
}

TEST(CircuitUIModelModal, SaveFromEmptySave) {
    auto model = CircuitUIModel {};

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        EXPECT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_SAVE});
        EXPECT_TRUE(!result.next_step);
        EXPECT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    }

    std::filesystem::remove(TEST_FILE_SAVE);
    EXPECT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    // {
    //     const auto result = model.file_action(FileAction::save_file);
    //     EXPECT_TRUE(!result.next_step);
    //     EXPECT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    // }
}

TEST(CircuitUIModelModal, SaveFromEmptySaveCancel) {
    auto model = CircuitUIModel {};

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        EXPECT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileCancel {});
        EXPECT_TRUE(!result.next_step);
    }
}

TEST(CircuitUIModelModal, SaveFromEmptySaveError) {
    auto model = CircuitUIModel {};

    std::filesystem::create_directory(TEST_FILE_FOLDER);
    EXPECT_TRUE(std::filesystem::is_directory(TEST_FILE_FOLDER));

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        EXPECT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_FOLDER});
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<SaveFileError>(error);
        EXPECT_EQ(open_error.filename, TEST_FILE_FOLDER);
    }
}

}  // namespace circuit_ui_model

}  // namespace logicsim
