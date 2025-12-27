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

[[nodiscard]] auto test_layout(point_t point = point_t {1, 1}) -> Layout {
    auto layout = Layout {};
    layout.logicitems().add(default_element_definition(LogicItemType::button), point,
                            display_state_t::normal);
    return layout;
}

auto save_test_file(const std::filesystem::path& filename = TEST_FILE_OPEN,
                    const Layout& layout = test_layout()) -> void {
    const auto default_view_point = ViewConfig {}.view_point();
    const auto default_simulation_config = SimulationConfig {};

    ASSERT_TRUE(save_circuit_to_file(layout, filename, default_view_point,
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

//
// From new file
//

TEST(CircuitUIModelModal, NewFileFromEmpty) {
    auto model = CircuitUIModel {};

    const auto result = model.file_action(FileAction::new_file);
    ASSERT_TRUE(!result.next_step);
    ASSERT_TRUE(model.layout().empty());
}

TEST(CircuitUIModelModal, ExampleFromEmpty) {
    auto model = CircuitUIModel {};

    const auto result = model.file_action(FileAction::load_example_simple);
    ASSERT_TRUE(!result.next_step);
    ASSERT_TRUE(!model.layout().empty());
}

TEST(CircuitUIModelModal, OpenFromEmptyOpen) {
    auto model = CircuitUIModel {};

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }
}

TEST(CircuitUIModelModal, OpenFromEmptyCancel) {
    auto model = CircuitUIModel {};

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(OpenFileCancel {});
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), Layout {});
    }
}

TEST(CircuitUIModelModal, OpenFromEmptyError) {
    auto model = CircuitUIModel {};

    const auto file =
        std::filesystem::path {"example_circuits/errors/error_version_unknown.ls2"};

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(OpenFileOpen {file});
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<OpenFileError>(error);
        ASSERT_EQ(open_error.filename, file);
    }
}

TEST(CircuitUIModelModal, SaveFromEmptySave) {
    auto model = CircuitUIModel {};

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_SAVE});
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    }

    std::filesystem::remove(TEST_FILE_SAVE);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    }
}

TEST(CircuitUIModelModal, SaveFromEmptySaveCancel) {
    auto model = CircuitUIModel {};

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileCancel {});
        ASSERT_TRUE(!result.next_step);
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, SaveFromEmptySaveError) {
    auto model = CircuitUIModel {};

    std::filesystem::create_directory(TEST_FILE_FOLDER);
    ASSERT_TRUE(std::filesystem::is_directory(TEST_FILE_FOLDER));

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_FOLDER});
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<SaveFileError>(error);
        ASSERT_EQ(open_error.filename, TEST_FILE_FOLDER);
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, SaveAsFromEmptySave) {
    auto model = CircuitUIModel {};

    std::filesystem::create_directory(TEST_FILE_FOLDER);
    ASSERT_TRUE(std::filesystem::is_directory(TEST_FILE_FOLDER));

    {
        const auto result = model.file_action(FileAction::save_as_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_SAVE});
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    }

    std::filesystem::remove(TEST_FILE_SAVE);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    }

    {
        const auto result = model.file_action(FileAction::save_as_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, SaveAsFromEmptySaveCancel) {
    auto model = CircuitUIModel {};

    {
        const auto result = model.file_action(FileAction::save_as_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileCancel {});
        ASSERT_TRUE(!result.next_step);
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, SaveAsFromEmptySaveError) {
    auto model = CircuitUIModel {};

    std::filesystem::create_directory(TEST_FILE_FOLDER);
    ASSERT_TRUE(std::filesystem::is_directory(TEST_FILE_FOLDER));

    {
        const auto result = model.file_action(FileAction::save_as_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_FOLDER});
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<SaveFileError>(error);
        ASSERT_EQ(open_error.filename, TEST_FILE_FOLDER);
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

//
// From open file
//

TEST(CircuitUIModelModal, NewFileFromOpen) {
    auto model = CircuitUIModel {};

    // open initial file
    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    // new file

    const auto result = model.file_action(FileAction::new_file);
    ASSERT_TRUE(!result.next_step);
    ASSERT_TRUE(model.layout().empty());

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, ExampleFromOpen) {
    auto model = CircuitUIModel {};

    // open initial file
    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    // example circuit
    const auto result = model.file_action(FileAction::load_example_simple);
    ASSERT_TRUE(!result.next_step);
    ASSERT_TRUE(!model.layout().empty());
    ASSERT_TRUE(model.layout() != test_layout());

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, OpenFromOpenError) {
    auto model = CircuitUIModel {};

    // open initial file
    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    // open second file which fails

    const auto file =
        std::filesystem::path {"example_circuits/errors/error_version_unknown.ls2"};

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen {file});
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<OpenFileError>(error);
        ASSERT_EQ(open_error.filename, file);
    }

    // validate old layout
    ASSERT_EQ(model.layout(), test_layout());

    // saves to original file

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_OPEN));
    }
}

TEST(CircuitUIModelModal, SaveFromOpen) {
    auto model = CircuitUIModel {};

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_OPEN));
    }
}

TEST(CircuitUIModelModal, SaveFromOpenError) {
    auto model = CircuitUIModel {};

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));
    std::filesystem::create_directory(TEST_FILE_OPEN);
    const auto _ = gsl::final_action([] { std::filesystem::remove(TEST_FILE_OPEN); });
    ASSERT_TRUE(std::filesystem::is_directory(TEST_FILE_OPEN));

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<SaveFileError>(error);
        ASSERT_EQ(open_error.filename, TEST_FILE_OPEN);
    }
}

TEST(CircuitUIModelModal, SaveAsFromOpenSave) {
    auto model = CircuitUIModel {};

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));

    {
        const auto result = model.file_action(FileAction::save_as_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_SAVE});
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    }

    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));
}

TEST(CircuitUIModelModal, SaveAsFromOpenCancel) {
    auto model = CircuitUIModel {};

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));
    std::filesystem::remove(TEST_FILE_SAVE);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.file_action(FileAction::save_as_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileCancel {});
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_OPEN));
    }
}

TEST(CircuitUIModelModal, SaveAsFromOpenSaveError) {
    auto model = CircuitUIModel {};

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::create_directory(TEST_FILE_FOLDER);
    ASSERT_TRUE(std::filesystem::is_directory(TEST_FILE_FOLDER));

    {
        const auto result = model.file_action(FileAction::save_as_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_FOLDER});
        const auto error = std::get<ErrorMessage>(result.next_step.value());
        const auto open_error = std::get<SaveFileError>(error);
        ASSERT_EQ(open_error.filename, TEST_FILE_FOLDER);
    }

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_OPEN));
    }
}

//
// From modified new
//

TEST(CircuitUIModelModal, OpenFromModifiedNewYes) {
    auto model = CircuitUIModel {};

    auto status = UIStatus {};
    status |= insert_button(model, point_t {5, 5});

    const auto layout_0 = model.layout();

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        const auto modal = std::get<SaveCurrentModal>(request);
        ASSERT_TRUE(modal.filename == "Circuit");
    }

    {
        const auto result = model.submit_modal_result(SaveCurrentYes {});
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    std::filesystem::remove(TEST_FILE_SAVE);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_SAVE});
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
        ASSERT_TRUE(layout_0 == load_layout_file(TEST_FILE_SAVE));
        ASSERT_TRUE(layout_0 == model.layout());
    }

    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_OPEN));
    }
}

TEST(CircuitUIModelModal, NewFromModifiedNewYes) {
    auto model = CircuitUIModel {};

    auto status = UIStatus {};
    status |= insert_button(model, point_t {5, 5});
    const auto layout_0 = model.layout();

    {
        const auto result = model.file_action(FileAction::new_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        const auto modal = std::get<SaveCurrentModal>(request);
        ASSERT_TRUE(modal.filename == "Circuit");
    }

    {
        const auto result = model.submit_modal_result(SaveCurrentYes {});
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    std::filesystem::remove(TEST_FILE_SAVE);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_SAVE});
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(layout_0 == load_layout_file(TEST_FILE_SAVE));
        ASSERT_TRUE(model.layout().empty());
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, NewFromModifiedNewNo) {
    auto model = CircuitUIModel {};

    auto status = UIStatus {};
    status |= insert_button(model, point_t {5, 5});
    const auto layout_0 = model.layout();

    {
        const auto result = model.file_action(FileAction::new_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        const auto modal = std::get<SaveCurrentModal>(request);
        ASSERT_TRUE(modal.filename == "Circuit");
    }

    {
        const auto result = model.submit_modal_result(SaveCurrentNo {});
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout().empty());
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, NewFromModifiedNewCancel) {
    auto model = CircuitUIModel {};

    auto status = UIStatus {};
    status |= insert_button(model, point_t {5, 5});
    const auto layout_0 = model.layout();

    {
        const auto result = model.file_action(FileAction::new_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        const auto modal = std::get<SaveCurrentModal>(request);
        ASSERT_TRUE(modal.filename == "Circuit");
    }

    {
        const auto result = model.submit_modal_result(SaveCurrentCancel {});
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == layout_0);
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

TEST(CircuitUIModelModal, ExampleFromModifiedNewYes) {
    auto model = CircuitUIModel {};

    auto status = UIStatus {};
    status |= insert_button(model, point_t {5, 5});
    const auto layout_0 = model.layout();

    {
        const auto result = model.file_action(FileAction::load_example_simple);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        const auto modal = std::get<SaveCurrentModal>(request);
        ASSERT_TRUE(modal.filename == "Circuit");
    }

    {
        const auto result = model.submit_modal_result(SaveCurrentYes {});
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }

    std::filesystem::remove(TEST_FILE_SAVE);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.submit_modal_result(SaveFileSave {TEST_FILE_SAVE});
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(layout_0 == load_layout_file(TEST_FILE_SAVE));

        ASSERT_TRUE(!model.layout().empty());
        ASSERT_TRUE(model.layout() != layout_0);
    }

    {
        const auto result = model.file_action(FileAction::save_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<SaveFileModal>(request));
    }
}

//
// Modified open
//

TEST(CircuitUIModelModal, OpenFromModifiedOpenYes) {
    auto model = CircuitUIModel {};

    // initial open

    save_test_file();

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
    }
    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_OPEN));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::remove(TEST_FILE_OPEN);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_OPEN));

    // modify

    auto status = UIStatus {};
    status |= insert_button(model, point_t {5, 5});
    const auto layout_0 = model.layout();

    // open again

    save_test_file(TEST_FILE_SAVE);

    {
        const auto result = model.file_action(FileAction::open_file);
        const auto request = std::get<ModalRequest>(result.next_step.value());
        const auto modal = std::get<SaveCurrentModal>(request);
        ASSERT_TRUE(modal.filename == TEST_FILE_OPEN);
    }

    {
        const auto result = model.submit_modal_result(SaveCurrentYes {});
        const auto request = std::get<ModalRequest>(result.next_step.value());
        ASSERT_TRUE(std::holds_alternative<OpenFileModal>(request));
        ASSERT_TRUE(layout_0 == load_layout_file(TEST_FILE_OPEN));
        ASSERT_TRUE(layout_0 == model.layout());
    }

    {
        const auto result = model.submit_modal_result(OpenFileOpen(TEST_FILE_SAVE));
        ASSERT_TRUE(!result.next_step);
        ASSERT_EQ(model.layout(), test_layout());
    }

    std::filesystem::remove(TEST_FILE_SAVE);
    ASSERT_FALSE(std::filesystem::is_regular_file(TEST_FILE_SAVE));

    {
        const auto result = model.file_action(FileAction::save_file);
        ASSERT_TRUE(!result.next_step);
        ASSERT_TRUE(model.layout() == load_layout_file(TEST_FILE_SAVE));
    }
}

}  // namespace circuit_ui_model

}  // namespace logicsim
