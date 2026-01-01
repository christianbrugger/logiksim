#include "core/circuit_ui_model.h"

#include "core/algorithm/overload.h"
#include "core/circuit_example.h"
#include "core/component/circuit_ui_model/mouse_logic/mouse_logic_status.h"
#include "core/component/circuit_ui_model/mouse_logic/mouse_wheel_logic.h"
#include "core/geometry/rect.h"
#include "core/geometry/scene.h"
#include "core/load_save_file.h"
#include "core/vocabulary/allocation_info.h"
#include "core/vocabulary/mouse_event.h"

namespace logicsim {

namespace {

// constexpr auto inline simulation_interval = 20ms;

[[nodiscard]] auto serialize(const circuit_ui_model::CircuitStore& circuit_store,
                             const CircuitUIConfig& config) -> std::string {
    return serialize_circuit(circuit_store.layout(), config.simulation);
}

}  // namespace

namespace circuit_ui_model {

auto Statistics::format() const -> std::string {
    return fmt::format(
        "Statistics{{\n"
        "  simulation_events_per_second = {},\n"
        "  frames_per_second = {},\n"
        "  pixel_scale = {},\n"
        "  image_size = {}x{}px\n"
        "}}",
        simulation_events_per_second, frames_per_second, pixel_scale, image_size.w,
        image_size.h);
}

auto UnsavedName::format() const -> std::string {
    return fmt::format("UnsavedName{{{}}}", name);
}

auto SavedPath::format() const -> std::string {
    return fmt::format("SavedPath{{{}}}", path);
}

auto get_filename_no_extension(const NameOrPath& name_or_path) -> std::filesystem::path {
    return std::visit(overload([&](const UnsavedName& arg) { return arg.name; },
                               [&](const SavedPath& arg) { return arg.path.filename(); }),
                      name_or_path);
}

auto get_filename_with_extension(const NameOrPath& name_or_path)
    -> std::filesystem::path {
    return std::visit(overload(
                          [&](const UnsavedName& arg) {
                              auto res = arg.name;
                              res.replace_extension(".ls2");
                              return res;
                          },
                          [&](const SavedPath& arg) { return arg.path.filename(); }),
                      name_or_path);
}

auto SaveInformation::format() const -> std::string {
    return fmt::format(
        "SaveInformation{{\n"
        "  name_or_path = {},\n"
        "  serialized_circuit = {},\n"
        "}}",
        name_or_path,
        serialized.transform([](const std::string& v) { return v.size(); }));
}

auto SaveCurrentModal::format() const -> std::string {
    return fmt::format("SaveCurrentModal{{{}}}", filename);
}

auto OpenFileModal::format() -> std::string {
    return "OpenFileModal{}";
}

auto SaveFileModal::format() const -> std::string {
    return fmt::format("SaveFileModal{{{}}}", filename);
}

auto SaveCurrentYes::format() -> std::string {
    return "SaveCurrentYes{}";
}

auto SaveCurrentNo::format() -> std::string {
    return "SaveCurrentNo{}";
}

auto SaveCurrentCancel::format() -> std::string {
    return "SaveCurrentCancel{}";
}

auto OpenFileOpen::format() const -> std::string {
    return fmt::format("OpenFileOpen{{{}}}", filename);
}

auto OpenFileCancel::format() -> std::string {
    return "OpenFileCancel{}";
}

auto SaveFileSave::format() const -> std::string {
    return fmt::format("SaveFileSave{{{}}}", filename);
}

auto SaveFileCancel::format() -> std::string {
    return "SaveFileCancel{}";
}

auto SaveFileError::format() const -> std::string {
    return fmt::format("SaveFileError{{{}}}", filename);
}

auto OpenFileError::format() const -> std::string {
    return fmt::format(
        "OpenFileError{{\n"
        "  filename = {},\n"
        "  message = {},\n"
        "}}",
        filename, message);
}

auto ExitApplication::format() -> std::string {
    return "ExitApplication{}";
}

auto CircuitAction::format() const -> std::string {
    return fmt::format(
        "CircuitAction{{\n"
        "  action = {},\n"
        "  filename = {},\n"
        "}}",
        action, filename);
}

ModalState::ModalState(ModalRequest request_, FileAction action_,
                       const circuit_ui_model::CircuitStore& circuit_store
                       [[maybe_unused]],
                       const CircuitUIConfig& config [[maybe_unused]])
    : request {std::move(request_)},
      action {action_}
#ifdef _DEBUG
      ,
      serialized_ {serialize(circuit_store, config)}
#endif
{
}

auto ModalState::format() const -> std::string {
    return fmt::format(
        "ModalState{{\n"
        "  request = {},\n"
        "  action = {},\n"
        "}}",
        request, action);
}

}  // namespace circuit_ui_model

template <>
auto format(circuit_ui_model::UserAction action) -> std::string {
    switch (action) {
        using enum circuit_ui_model::UserAction;

        case reload_circuit:
            return "reload_circuit";

        case undo:
            return "undo";
        case redo:
            return "redo";
        case select_all:
            return "select_all";
        case copy_selected:
            return "copy_selected";
        case paste_from_clipboard:
            return "paste_from_clipboard";
        case cut_selected:
            return "cut_selected";
        case delete_selected:
            return "delete_selected";

        case zoom_in:
            return "zoom_in";
        case zoom_out:
            return "zoom_out";
        case reset_view:
            return "reset_view";
    };

    std::terminate();
}

template <>
auto format(circuit_ui_model::FileAction action) -> std::string {
    switch (action) {
        using enum circuit_ui_model::FileAction;

        case new_file:
            return "new_file";
        case open_file:
            return "open_file";
        case save_file:
            return "save_file";
        case save_as_file:
            return "save_as_file";
        case exit_application:
            return "exit_application";
        case load_example_simple:
            return "load_example_simple";
        case load_example_elements_and_wires:
            return "load_example_elements_and_wires";
        case load_example_elements:
            return "load_example_elements";
        case load_example_wires:
            return "load_example_wires";
    };
    std::terminate();
}

CircuitUIModel::CircuitUIModel() {
    // initial configs
    config_.state = EditingState {DefaultMouseAction::selection};
    static_cast<void>(editing_logic_manager_.set_circuit_state(
        config_.state, editable_circuit_pointer(circuit_store_)));
    circuit_store_.set_circuit_state(config_.state);
    circuit_store_.set_simulation_config(config_.simulation);
    circuit_renderer_.set_render_config(config_.render);
    // initial circuit
    save_information_ = SaveInformation {
        .name_or_path = UnsavedName {"Circuit"},
        .serialized = serialize(circuit_store_, config_),
    };

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitUIModel::set_config(const CircuitUIConfig& new_config) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    if (config_.state != new_config.state) {
        // close dialogs
        if (!is_editing_state(new_config.state)) {
            status |= close_all_setting_dialogs();
        }

        // finalizes editing if needed
        status |= editing_logic_manager_.set_circuit_state(
            new_config.state, editable_circuit_pointer(circuit_store_));

        // clear visible selection
        if (is_selection_state(config_.state)) {
            circuit_store_.editable_circuit().clear_visible_selection();
            circuit_store_.editable_circuit().finish_undo_group();
        }

        // circuit store
        circuit_store_.set_circuit_state(new_config.state);

        // simulation
        // if (is_simulation(new_config.circuit_state)) {
        //     timer_run_simulation_.setInterval(0);
        //     timer_run_simulation_.start();
        // } else {
        //     timer_run_simulation_.stop();
        // }

        // update & notify
        config_.state = new_config.state;
        status.config_changed = true;
        status.require_repaint = true;
    }

    if (config_.render != new_config.render) {
        circuit_renderer_.set_render_config(new_config.render);

        // if (new_config.do_benchmark) {
        //     timer_benchmark_render_.start();
        // } else {
        //     timer_benchmark_render_.stop();
        // }

        config_.render = new_config.render;
        status.config_changed = true;
        status.require_repaint = true;
    }

    if (config_.simulation != new_config.simulation) {
        circuit_store_.set_simulation_config(new_config.simulation);

        config_.simulation = new_config.simulation;
        status.config_changed = true;
        status.require_repaint = true;
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::config() const -> const CircuitUIConfig& {
    Expects(class_invariant_holds());
    return config_;
}

auto CircuitUIModel::view_config() const -> const ViewConfig& {
    Expects(class_invariant_holds());
    return circuit_renderer_.view_config();
}

auto CircuitUIModel::history_status() const -> HistoryStatus {
    Expects(class_invariant_holds());

    if (is_editing_state(config_.state)) {
        const auto& editable_circuit = circuit_store_.editable_circuit();
        return HistoryStatus {
            .undo_available = has_undo(editable_circuit) &&
                              undo_groups_count(editable_circuit) > std::size_t {0},
            .redo_available = has_redo(editable_circuit),
        };
    }
    return HistoryStatus {
        .undo_available = false,
        .redo_available = false,
    };
}

auto CircuitUIModel::allocation_info() const -> CircuitWidgetAllocInfo {
    Expects(class_invariant_holds());

    const auto t = Timer {};

    auto result = CircuitWidgetAllocInfo {
        .circuit_store = circuit_store_.allocation_info(),
        .circuit_renderer = circuit_renderer_.allocation_info(),
    };

    result.collection_time = t.delta();
    return result;
}

auto CircuitUIModel::statistics() const -> Statistics {
    Expects(class_invariant_holds());

    const auto surface_statistics = circuit_renderer_.statistics();
    const auto result = Statistics {
        .simulation_events_per_second = circuit_store_.simulation_events_per_second(),
        .frames_per_second = surface_statistics.frames_per_second,
        .pixel_scale = surface_statistics.pixel_scale,
        .image_size = surface_statistics.image_size,
    };

    return result;
}

auto CircuitUIModel::layout() const -> const Layout& {
    Expects(class_invariant_holds());
    return circuit_store_.layout();
}

auto CircuitUIModel::display_filename() const -> std::filesystem::path {
    Expects(class_invariant_holds());

    return get_filename_no_extension(save_information_.name_or_path);
}

auto CircuitUIModel::calculate_is_modified() const -> bool {
    Expects(class_invariant_holds());

    return serialize(circuit_store_, config_) != save_information_.serialized;
}

auto CircuitUIModel::do_action(UserAction action,
                               std::optional<point_device_fine_t> position) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    switch (action) {
        using enum UserAction;

        case reload_circuit: {
            status |= finalize_editing();
            const auto _ = Timer {"Reload Circuit"};
            auto layout = Layout {circuit_store_.layout()};
            // clear circuit to free memory
            status |= set_editable_circuit(EditableCircuit {});
            status |= set_editable_circuit(EditableCircuit {std::move(layout)});
            break;
        }

        case undo: {
            // this->undo();
            break;
        }
        case redo: {
            // this->redo();
            break;
        }
        case select_all: {
            // this->select_all();
            break;
        }
        case copy_selected: {
            // this->copy_selected();
            break;
        }
        case paste_from_clipboard: {
            // this->paste_clipboard();
            break;
        }
        case cut_selected: {
            // this->copy_selected();
            // this->delete_selected();
            break;
        }
        case delete_selected: {
            // this->delete_selected();
            break;
        }

        case zoom_in: {
            status |= this->zoom(+1, position);
            break;
        }
        case zoom_out: {
            status |= this->zoom(-1, position);
            break;
        }
        case reset_view: {
            circuit_renderer_.set_view_point(ViewConfig {}.view_point());
            status.require_repaint = true;
            break;
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

namespace circuit_ui_model {

namespace {

[[nodiscard]] auto requires_save_current_prompt(FileAction request) -> bool {
    switch (request) {
        using enum FileAction;

        case new_file:
        case open_file:
        case exit_application:
        case load_example_simple:
        case load_example_elements_and_wires:
        case load_example_elements:
        case load_example_wires:
            return true;

        case save_file:
        case save_as_file:
            return false;
    };
    std::terminate();
}

[[nodiscard]] auto is_modal_request(const std::optional<NextActionStep>& step) -> bool {
    return step && std::holds_alternative<ModalRequest>(step.value());
}

[[nodiscard]] auto get_if_modal_request(const std::optional<NextActionStep>& step)
    -> const ModalRequest* {
    if (step.has_value()) {
        if (const auto* request = std::get_if<ModalRequest>(&step.value())) {
            return request;
        }
    }
    return nullptr;
}

}  // namespace

}  // namespace circuit_ui_model

auto CircuitUIModel::file_action(FileAction action,
                                 std::optional<NextActionStep>& next_step_) -> UIStatus {
    print("file_action", action);
    Expects(class_invariant_holds());
    using namespace circuit_ui_model;
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    status |= finalize_editing();

    auto next_step = std::optional<NextActionStep> {};
    auto current_action = std::optional<CircuitAction> {};

    if (requires_save_current_prompt(action)) {
        const auto is_modifed = calculate_is_modified();

        if (is_modifed) {
            next_step = SaveCurrentModal {
                .filename = get_filename_with_extension(save_information_.name_or_path),
            };
        }
    }

    next_modal_action(action, next_step, current_action);
    if (const auto request = get_if_modal_request(next_step)) {
        modal_ = ModalState {*request, action, circuit_store_, config_};
    }
    status |= do_modal_action(current_action, next_step);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    Ensures(modal_.has_value() == is_modal_request(next_step));
    next_step_ = next_step;
    return status;
}

auto CircuitUIModel::submit_modal_result(const ModalResult& result,
                                         std::optional<NextActionStep>& next_step_)
    -> UIStatus {
    print("submit_modal_result", result);
    Expects(class_invariant_holds());
    using namespace circuit_ui_model;
    auto status = UIStatus {};

    if (!modal_) {
        throw std::runtime_error {"Cannot submit result without request"};
    }

    const auto action = modal_.value().action;
    const auto last_request = modal_.value().request;
    auto next_step = std::optional<NextActionStep> {};
    auto current_action = std::optional<CircuitAction> {};

    if (std::holds_alternative<SaveCurrentModal>(last_request)) {
        if (std::holds_alternative<SaveCurrentYes>(result)) {
            next_modal_action(FileAction::save_file, next_step, current_action);
            status |= do_modal_action(current_action, next_step);
            next_modal_action(action, next_step, current_action);
        } else if (std::holds_alternative<SaveCurrentNo>(result)) {
            next_modal_action(action, next_step, current_action);
        } else if (std::holds_alternative<SaveCurrentCancel>(result)) {
            // nothing
        } else {
            throw UnexpectedModalResultException {last_request, result};
        }
    } else if (std::holds_alternative<OpenFileModal>(last_request)) {
        if (const auto data = std::get_if<OpenFileOpen>(&result)) {
            current_action = CircuitAction {
                .action = action,
                .filename = data->filename,
            };
        } else if (std::holds_alternative<OpenFileCancel>(result)) {
            //  nothing
        } else {
            throw UnexpectedModalResultException {last_request, result};
        }
    } else if (std::holds_alternative<SaveFileModal>(last_request)) {
        if (const auto data = std::get_if<SaveFileSave>(&result)) {
            current_action = CircuitAction {
                .action = FileAction::save_file,
                .filename = data->filename,
            };
            if (requires_save_current_prompt(action)) {
                status |= do_modal_action(current_action, next_step);
                next_modal_action(action, next_step, current_action);
            }
        } else if (std::holds_alternative<SaveFileCancel>(result)) {
            // nothing
        } else {
            throw UnexpectedModalResultException {last_request, result};
        }
    } else {
        std::terminate();
    }

    if (const auto request = get_if_modal_request(next_step)) {
        modal_.value().request = *request;
    } else {
        modal_ = std::nullopt;
    }
    status |= do_modal_action(current_action, next_step);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    Ensures(modal_.has_value() == is_modal_request(next_step));
    next_step_ = next_step;
    return status;
}

auto CircuitUIModel::next_modal_action(
    circuit_ui_model::FileAction action,
    std::optional<circuit_ui_model::NextActionStep>& next_step,
    std::optional<circuit_ui_model::CircuitAction>& current_action) -> void {
    Expects(class_invariant_holds());
    Expects(!current_action);
    using namespace circuit_ui_model;

    [&] {
        if (next_step) {
            return;
        }

        switch (action) {
            case FileAction::open_file: {
                next_step = OpenFileModal {};
                return;
            }
            case FileAction::save_file: {
                if (const auto p =
                        std::get_if<SavedPath>(&save_information_.name_or_path)) {
                    current_action = CircuitAction {
                        .action = action,
                        .filename = p->path,
                    };
                } else {
                    next_step = SaveFileModal {
                        .filename =
                            get_filename_with_extension(save_information_.name_or_path),
                    };
                }
                return;
            }
            case FileAction::save_as_file: {
                next_step = SaveFileModal {
                    .filename =
                        get_filename_with_extension(save_information_.name_or_path),
                };
                return;
            }
            case FileAction::exit_application: {
                next_step = ExitApplication {};
                return;
            }

            case FileAction::new_file:
            case FileAction::load_example_simple:
            case FileAction::load_example_elements_and_wires:
            case FileAction::load_example_elements:
            case FileAction::load_example_wires: {
                current_action = CircuitAction {
                    .action = action,
                    .filename = std::nullopt,
                };
                return;
            }
        };
        std::terminate();
    }();

    Ensures(bool {next_step} ^ bool {current_action});
    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

namespace {

[[nodiscard]] auto is_error_message(
    const std::optional<circuit_ui_model::NextActionStep>& value) -> bool {
    return value && std::holds_alternative<circuit_ui_model::ErrorMessage>(*value);
}

[[nodiscard]] auto load_error_to_message(const LoadError& error,
                                         const std::filesystem::path& filename)
    -> std::string {
    // log full error
    print("WARNING: Failed to open:", filename);
    print("         Load error type:", error.type());
    print("         Message:", error.format());
    print();

    // Version Errors ask the users to update LogikSim to a specific version.
    // Those are the only ones a user can act upon. Log the rest.
    if (error.type() == LoadErrorType::json_version_error) {
        return error.format();
    }
    return "This is not a valid circuit file, or its format is not currently supported.";
}

}  // namespace

auto CircuitUIModel::do_modal_action(
    std::optional<circuit_ui_model::CircuitAction>& current_action,
    std::optional<circuit_ui_model::NextActionStep>& next_step) -> UIStatus {
    using namespace circuit_ui_model;
    Expects(class_invariant_holds());
    Expects(!current_action || !next_step || is_error_message(next_step));
    auto status = UIStatus {};

    [&] {
        if (!current_action) {
            return;
        }
        if (is_error_message(next_step)) {
            return;
        }
        Expects(!next_step);

        switch (current_action.value().action) {
            using enum FileAction;

            case new_file:
                status |= load_new_circuit();
                return;

            case open_file: {
                const auto& filename = current_action.value().filename.value();
                auto load_error = std::optional<LoadError> {};
                status |= this->open_from_file(filename, load_error);
                if (load_error) {
                    next_step = OpenFileError {
                        .filename = filename,
                        .message = load_error_to_message(load_error.value(), filename),
                    };
                }
                return;
            }

            case save_file:
            case save_as_file: {
                const auto& filename = current_action.value().filename.value();
                auto success = bool {};
                status |= this->save_to_file(filename, success);
                if (!success) {
                    next_step = SaveFileError {.filename = filename};
                }
                return;
            }

            case exit_application:
                return;

            case load_example_simple:
                status |= load_circuit_example(1);
                return;
            case load_example_elements_and_wires:
                status |= load_circuit_example(2);
                return;
            case load_example_elements:
                status |= load_circuit_example(3);
                return;
            case load_example_wires:
                status |= load_circuit_example(4);
                return;
        };
        std::terminate();
    }();

    current_action = std::nullopt;

    Ensures(!current_action);
    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::load_new_circuit() -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    const auto default_view_point = ViewConfig {}.view_point();
    const auto default_simulation_config = SimulationConfig {};

    status |= set_circuit_state(*this, defaults::selection_state);
    status |= set_editable_circuit(EditableCircuit {}, default_view_point,
                                   default_simulation_config);
    status |= set_save_information(SaveInformation {
        .name_or_path = UnsavedName {"Circuit"},
        .serialized = serialize(circuit_store_, config_),
    });

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::load_circuit_example(int number) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    const auto default_view_point = ViewConfig {}.view_point();
    const auto default_simulation_config = SimulationConfig {};

    // clear circuit to free memory first
    status |= set_editable_circuit(EditableCircuit {});
    status |= set_editable_circuit(load_example_with_logging(number), default_view_point,
                                   default_simulation_config);
    status |= set_save_information(SaveInformation {
        .name_or_path = UnsavedName {std::format("Example {}", number)},
        .serialized = serialize(circuit_store_, config_),
    });

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::save_to_file(const std::filesystem::path& filename, bool& success)
    -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    status |= finalize_editing();
    const auto result = save_circuit_to_file(circuit_store_.layout(), filename,
                                             circuit_renderer_.view_config().view_point(),
                                             config_.simulation);
    if (result) {
        status |= set_save_information(SaveInformation {
            .name_or_path = SavedPath {filename},
            .serialized = serialize(circuit_store_, config_),
        });
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    success = result;
    return status;
}

auto CircuitUIModel::open_from_file(const std::filesystem::path& filename,
                                    std::optional<LoadError>& load_error) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    // store original circuit in case load fails
    status |= finalize_editing();
    auto orig_layout = Layout {circuit_store_.layout()};
    // clear circuit to free memory
    status |= set_editable_circuit(EditableCircuit {});

    auto load_result = load_circuit_from_file(filename);
    if (load_result.has_value()) {
        status |=
            set_editable_circuit(std::move(load_result->editable_circuit),
                                 load_result->view_point, load_result->simulation_config);
        status |= set_save_information(SaveInformation {
            .name_or_path = SavedPath {filename},
            .serialized = serialize(circuit_store_, config_),
        });
    } else {
        status |= set_editable_circuit(EditableCircuit {std::move(orig_layout)});
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    if (load_result) {
        load_error = std::nullopt;
    } else {
        load_error = std::move(load_result).error();
    }
    return status;
}

auto CircuitUIModel::render(BLImage& bl_image, device_pixel_ratio_t device_pixel_ratio)
    -> void {
    Expects(class_invariant_holds());

    // TODO use more device_pixel_ratio_t
    circuit_renderer_.set_device_pixel_ratio(double {device_pixel_ratio});

    if (std::holds_alternative<NonInteractiveState>(config_.state)) {
        circuit_renderer_.render_layout(bl_image, circuit_store_.layout());
    }

    else if (std::holds_alternative<EditingState>(config_.state)) {
        const auto show_size_handles = !editing_logic_manager_.is_area_selection_active();
        circuit_renderer_.render_editable_circuit(
            bl_image, circuit_store_.editable_circuit(), show_size_handles);
    }

    else if (std::holds_alternative<SimulationState>(config_.state)) {
        circuit_renderer_.render_simulation(
            bl_image, circuit_store_.interactive_simulation().spatial_simulation());
    }

    else {
        std::terminate();
    }

    // simulation_image_update_pending_ = false;
    // update_history_status();

    Ensures(class_invariant_holds());
}

auto CircuitUIModel::mouse_press(const MousePressEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    status |= log_mouse_position("mousePressEvent", event.position);

    if (event.button == MouseButton::Middle) {
        mouse_drag_logic_.mouse_press(event.position);
    }

    if (event.button == MouseButton::Left) {
        const auto position_fine =
            to_grid_fine(event.position, circuit_renderer_.view_config());

        status |= editing_logic_manager_.mouse_press(
            position_fine, circuit_renderer_.view_config(), event.modifiers,
            event.double_click, editable_circuit_pointer(circuit_store_));
    }

    if (event.button == MouseButton::Left && is_simulation(config_.state)) {
        if (const auto point = to_grid(event.position, circuit_renderer_.view_config())) {
            // TODO: status
            circuit_store_.interactive_simulation().mouse_press(*point);
            status.require_repaint = true;
        }
    }

    if (event.button == MouseButton::Right) {
        status |= abort_current_action();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::mouse_move(const MouseMoveEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    status |= log_mouse_position("mouseMoveEvent", event.position);

    if (event.buttons.is_set(MouseButton::Middle)) {
        set_view_config_offset(circuit_renderer_,
                               mouse_drag_logic_.mouse_move(
                                   event.position, circuit_renderer_.view_config()));
        status.require_repaint = true;
    }

    if (event.buttons.is_set(MouseButton::Left)) {
        const auto position_fine =
            to_grid_fine(event.position, circuit_renderer_.view_config());

        status |= editing_logic_manager_.mouse_move(
            position_fine, editable_circuit_pointer(circuit_store_));
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::mouse_release(const MouseReleaseEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    status |= log_mouse_position("mouseReleaseEvent", event.position);

    if (event.button == MouseButton::Middle) {
        set_view_config_offset(circuit_renderer_,
                               mouse_drag_logic_.mouse_release(
                                   event.position, circuit_renderer_.view_config()));
        status.require_repaint = true;
    }

    if (event.button == MouseButton::Left) {
        const auto show_setting_dialog =
            [&](EditableCircuit& editable_circuit,
                std::variant<logicitem_id_t, decoration_id_t> element_id) {
                // TODO:
                // dialog_manager_.show_setting_dialog(editable_circuit, element_id);
                static_cast<void>(editable_circuit);
                static_cast<void>(element_id);
            };

        const auto position_fine =
            to_grid_fine(event.position, circuit_renderer_.view_config());

        status |= editing_logic_manager_.mouse_release(
            position_fine, editable_circuit_pointer(circuit_store_), show_setting_dialog);
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::mouse_wheel(const MouseWheelEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    status |= log_mouse_position("wheelEvent", event.position);

    if (const auto view_point =
            circuit_ui_model::wheel_scroll_zoom(event, circuit_renderer_.view_config())) {
        circuit_renderer_.set_view_point(view_point.value());
        status.require_repaint = true;
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::key_press(VirtualKey key) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (modal_) {
        throw std::runtime_error {"Model is modal and cannot be modified."};
    }

    // Escape
    if (key == VirtualKey::Escape) {
        status |= abort_current_action();
    }

    // Enter
    if (key == VirtualKey::Enter) {
        status |= editing_logic_manager_.confirm_editing(
            editable_circuit_pointer(circuit_store_));

        // TODO: use status
        // some elements might have been deleted (e.g. move-selection confirmation)
        // status |= on_setting_dialog_cleanup_request();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::set_editable_circuit(
    EditableCircuit&& editable_circuit, std::optional<ViewPoint> view_point,
    std::optional<SimulationConfig> simulation_config) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    status |= finalize_editing();
    status |= close_all_setting_dialogs();
    circuit_renderer_.reset();

    // disable simulation
    const auto was_simulation = is_simulation(config_.state);
    if (was_simulation) {
        status |= set_circuit_state(*this, NonInteractiveState {});
    }

    // set new circuit
    circuit_store_.set_editable_circuit(std::move(editable_circuit));
    if (view_point) {
        circuit_renderer_.set_view_point(view_point.value());
    }
    if (simulation_config) {
        status |= set_simulation_config(*this, simulation_config.value());
    }

    // re-enable simulation
    if (was_simulation) {
        status |= set_circuit_state(*this, SimulationState {});
    }

    status.require_repaint = true;

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::set_save_information(SaveInformation&& save_information)
    -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (save_information_.name_or_path != save_information.name_or_path) {
        status.filename_changed = true;
    }
    save_information_ = std::move(save_information);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::abort_current_action() -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (is_editing_state(config_.state)) {
        // 1) cancel current editing
        if (editing_logic_manager_.is_editing_active()) {
            status |= finalize_editing();
        }

        else {
            // 2) cancel active selection
            if (is_selection_state(config_.state)) {
                circuit_store_.editable_circuit().clear_visible_selection();
                circuit_store_.editable_circuit().finish_undo_group();
                status.require_repaint = true;
            }

            // 3) switch to selection editing mode
            if (is_inserting_state(config_.state)) {
                status |= set_circuit_state(*this, defaults::selection_state);
            }
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::finalize_editing() -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    status |=
        editing_logic_manager_.finalize_editing(editable_circuit_pointer(circuit_store_));

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::close_all_setting_dialogs() -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (is_editing_state(config_.state)) {
        status.dialogs_changed = !dialog_manager_.empty();
        dialog_manager_.close_all(circuit_store_.editable_circuit());
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

namespace {

auto to_position_inside_renderer(const circuit_ui_model::CircuitRenderer& renderer,
                                 std::optional<point_device_fine_t> point_device)
    -> point_device_fine_t {
    const auto& config = renderer.view_config();

    if (point_device) {
        const auto rect = get_scene_rect_fine(config);
        const auto point = to_grid_fine(point_device.value(), config);

        if (is_colliding(point, rect)) {
            return point_device.value();
        }
    }

    const auto rect = get_scene_rect_fine(config);
    const auto center = get_center(rect);

    return to_device_fine(center, config);
}

}  // namespace

auto CircuitUIModel::zoom(double steps, std::optional<point_device_fine_t> position)
    -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (steps != 0) {
        const auto center = to_position_inside_renderer(circuit_renderer_, position);
        status |= log_mouse_position("zoom", center);

        circuit_renderer_.set_view_point(
            zoomed_config(circuit_renderer_.view_config(), steps, center));
        status.require_repaint = true;
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::log_mouse_position(std::string_view source,
                                        point_device_fine_t position) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (circuit_renderer_.render_config().show_mouse_position) {
        circuit_renderer_.set_mouse_position_info(MousePositionInfo {
            .position = position,
            .labels = {std::string(source),
                       mouse_position_label("device", "point_device_fine_t", position)},
        });
        status.require_repaint = true;
    } else if (circuit_renderer_.has_mouse_position_info()) {
        circuit_renderer_.set_mouse_position_info(std::nullopt);
        status.require_repaint = true;
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::class_invariant_holds() const -> bool {
    // Configs
    Expects(circuit_renderer_.render_config() == config_.render);
    Expects(circuit_store_.simulation_config() == config_.simulation);
    Expects(circuit_store_.circuit_state() == config_.state);
    Expects(editing_logic_manager_.circuit_state() == config_.state);

    // Setting Dialogs
    Expects(is_editing_state(config_.state) || dialog_manager_.empty());

    if (is_editing_state(config_.state) && !editing_logic_manager_.is_editing_active()) {
        // Operation count
        Expects(circuit_store_.editable_circuit().visible_selection_operation_count() ==
                0);

        // History Group
        Expects(!has_ungrouped_undo_entries(circuit_store_.editable_circuit()));

        // History Enabled
        Expects(is_history_enabled(circuit_store_.editable_circuit()));
    }

    return true;
}

auto CircuitUIModel::expensive_invariant_holds() const -> bool {
    // insertion state (expensive so only assert)
    assert(editing_logic_manager_.is_editing_active() ||
           all_normal_display_state(circuit_store_.layout()));

    // editable circuit (expensive so only assert)
    assert(!is_editing_state(config_.state) ||
           is_valid(circuit_store_.editable_circuit()));

    // modal immutability (expensive so only assert)
#ifdef _DEBUG
    assert(!modal_ || modal_->serialized_ == serialize(circuit_store_, config_));
#endif

    return true;
}

//
// Free Functions
//

auto set_circuit_state(CircuitUIModel& model, CircuitWidgetState value) -> UIStatus {
    auto config = model.config();
    config.state = value;
    return model.set_config(config);
}

auto set_render_config(CircuitUIModel& model, WidgetRenderConfig value) -> UIStatus {
    auto config = model.config();
    config.render = value;
    return model.set_config(config);
}

auto set_simulation_config(CircuitUIModel& model, SimulationConfig value) -> UIStatus {
    auto config = model.config();
    config.simulation = value;
    return model.set_config(config);
}

}  // namespace logicsim
