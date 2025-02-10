#include "core_export/logicsim_core_export.h"

#include "core/algorithm/to_enum.h"
#include "core/circuit_ui_model.h"

#include <blend2d.h>
#include <gsl/gsl>

//
// C Interface
//

struct ls_circuit_t {
    logicsim::CircuitUIModel model {};
};

namespace {

template <typename Func>
[[nodiscard]] auto ls_translate_exception(Func&& func) noexcept {
    try {
        return std::invoke(func);
    } catch (const std::exception& exc) {
        // for now just terminate, later we forward them
        static_cast<void>(exc);
        std::terminate();
    } catch (...) {
        // for now just terminate, later we forward them
        std::terminate();
    }
}

}  // namespace

namespace logicsim {
namespace {

[[nodiscard]] auto to_c(circuit_ui_model::UIStatus status) -> ls_ui_status_t {
    return ls_ui_status_t {
        .repaint_required = status.repaint_required,
        .config_changed = status.config_changed,
        .history_changed = status.history_changed,
        .dialogs_changed = status.dialogs_changed,
    };
}

[[nodiscard]] auto to_c(const DefaultMouseAction action)
    -> exporting::DefaultMouseAction {
    switch (action) {
        using enum DefaultMouseAction;

        case selection:
            return exporting::DefaultMouseAction::selection;
        case insert_wire:
            return exporting::DefaultMouseAction::insert_wire;

        case insert_button:
            return exporting::DefaultMouseAction::insert_button;
        case insert_led:
            return exporting::DefaultMouseAction::insert_led;
        case insert_display_number:
            return exporting::DefaultMouseAction::insert_display_number;
        case insert_display_ascii:
            return exporting::DefaultMouseAction::insert_display_ascii;

        case insert_and_element:
            return exporting::DefaultMouseAction::insert_and_element;
        case insert_or_element:
            return exporting::DefaultMouseAction::insert_or_element;
        case insert_xor_element:
            return exporting::DefaultMouseAction::insert_xor_element;
        case insert_nand_element:
            return exporting::DefaultMouseAction::insert_nand_element;
        case insert_nor_element:
            return exporting::DefaultMouseAction::insert_nor_element;

        case insert_buffer_element:
            return exporting::DefaultMouseAction::insert_buffer_element;
        case insert_inverter_element:
            return exporting::DefaultMouseAction::insert_inverter_element;
        case insert_flipflop_jk:
            return exporting::DefaultMouseAction::insert_flipflop_jk;
        case insert_latch_d:
            return exporting::DefaultMouseAction::insert_latch_d;
        case insert_flipflop_d:
            return exporting::DefaultMouseAction::insert_flipflop_d;
        case insert_flipflop_ms_d:
            return exporting::DefaultMouseAction::insert_flipflop_ms_d;

        case insert_clock_generator:
            return exporting::DefaultMouseAction::insert_clock_generator;
        case insert_shift_register:
            return exporting::DefaultMouseAction::insert_shift_register;

        case insert_decoration_text_element:
            return exporting::DefaultMouseAction::insert_decoration_text_element;
    };
    std::terminate();
}

[[nodiscard]] auto to_c(const CircuitWidgetState& state) -> ls_circuit_state_t {
    if (std::holds_alternative<NonInteractiveState>(state)) {
        return ls_circuit_state_t {
            .type_enum = to_underlying(exporting::CircuitStateType::NonInteractive),
            .editing_default_mouse_action_enum = 0,
        };
    }

    if (std::holds_alternative<SimulationState>(state)) {
        return ls_circuit_state_t {
            .type_enum = to_underlying(exporting::CircuitStateType::Simulation),
            .editing_default_mouse_action_enum = 0,
        };
    }

    if (const auto editing = std::get_if<EditingState>(&state)) {
        return ls_circuit_state_t {
            .type_enum = to_underlying(exporting::CircuitStateType::Editing),
            .editing_default_mouse_action_enum =
                to_underlying(to_c(editing->default_mouse_action)),
        };
    }
    std::terminate();
}

[[nodiscard]] auto to_c(const CircuitUIConfig& config) -> ls_ui_config_t {
    return ls_ui_config_t {
        .simulation =
            ls_simulation_config_t {
                .simulation_time_rate_ns =
                    config.simulation.simulation_time_rate.rate_per_second.count_ns(),
                .use_wire_delay = config.simulation.use_wire_delay,
            },
        .render =
            ls_render_config_t {
                .thread_count_enum = to_underlying(config.render.thread_count),
                .wire_render_style_enum = to_underlying(config.render.wire_render_style),

                .do_benchmark = config.render.do_benchmark,
                .show_circuit = config.render.show_circuit,
                .show_collision_index = config.render.show_collision_index,
                .show_connection_index = config.render.show_connection_index,
                .show_selection_index = config.render.show_selection_index,

                .show_render_borders = config.render.show_render_borders,
                .show_mouse_position = config.render.show_mouse_position,
                .direct_rendering = config.render.direct_rendering,
                .jit_rendering = config.render.jit_rendering,
            },
        .state = to_c(config.state),
    };
};

[[nodiscard]] auto to_thread_count(const uint8_t count_enum) -> ThreadCount {
    const auto count = to_enum<exporting::ThreadCount>(count_enum);

    switch (count) {
        using enum exporting::ThreadCount;

        case synchronous:
            return ThreadCount::synchronous;
        case two:
            return ThreadCount::two;
        case four:
            return ThreadCount::four;
        case eight:
            return ThreadCount::eight;
    };
    std::terminate();
}

[[nodiscard]] auto to_wire_render_style(const uint8_t style_enum) -> WireRenderStyle {
    const auto style = to_enum<exporting::WireRenderStyle>(style_enum);

    switch (style) {
        using enum exporting::WireRenderStyle;

        case red:
            return WireRenderStyle::red;
        case bold:
            return WireRenderStyle::bold;
        case bold_red:
            return WireRenderStyle::bold_red;
    };
    std::terminate();
}

[[nodiscard]] auto to_default_mouse_action(const uint8_t action_enum)
    -> DefaultMouseAction {
    const auto action = to_enum<exporting::DefaultMouseAction>(action_enum);

    switch (action) {
        using enum exporting::DefaultMouseAction;

        case selection:
            return DefaultMouseAction::selection;
        case insert_wire:
            return DefaultMouseAction::insert_wire;

        case insert_button:
            return DefaultMouseAction::insert_button;
        case insert_led:
            return DefaultMouseAction::insert_led;
        case insert_display_number:
            return DefaultMouseAction::insert_display_number;
        case insert_display_ascii:
            return DefaultMouseAction::insert_display_ascii;

        case insert_and_element:
            return DefaultMouseAction::insert_and_element;
        case insert_or_element:
            return DefaultMouseAction::insert_or_element;
        case insert_xor_element:
            return DefaultMouseAction::insert_xor_element;
        case insert_nand_element:
            return DefaultMouseAction::insert_nand_element;
        case insert_nor_element:
            return DefaultMouseAction::insert_nor_element;

        case insert_buffer_element:
            return DefaultMouseAction::insert_buffer_element;
        case insert_inverter_element:
            return DefaultMouseAction::insert_inverter_element;
        case insert_flipflop_jk:
            return DefaultMouseAction::insert_flipflop_jk;
        case insert_latch_d:
            return DefaultMouseAction::insert_latch_d;
        case insert_flipflop_d:
            return DefaultMouseAction::insert_flipflop_d;
        case insert_flipflop_ms_d:
            return DefaultMouseAction::insert_flipflop_ms_d;

        case insert_clock_generator:
            return DefaultMouseAction::insert_clock_generator;
        case insert_shift_register:
            return DefaultMouseAction::insert_shift_register;

        case insert_decoration_text_element:
            return DefaultMouseAction::insert_decoration_text_element;
    };
    std::terminate();
}

[[nodiscard]] auto from_c(const ls_circuit_state_t& state) -> CircuitWidgetState {
    const auto type = to_enum<exporting::CircuitStateType>(state.type_enum);

    switch (type) {
        using enum exporting::CircuitStateType;

        case NonInteractive:
            return NonInteractiveState {};
        case Simulation:
            return SimulationState {};
        case Editing:
            return EditingState {.default_mouse_action = to_default_mouse_action(
                                     state.editing_default_mouse_action_enum)};
    };
    std::terminate();
}

[[nodiscard]] auto from_c(const ls_ui_config_t& config) -> CircuitUIConfig {
    return CircuitUIConfig {
        .simulation =
            SimulationConfig {
                .simulation_time_rate =
                    time_rate_t {std::chrono::duration<int64_t, std::nano> {
                        config.simulation.simulation_time_rate_ns}},
                .use_wire_delay = config.simulation.use_wire_delay,
            },
        .render =
            WidgetRenderConfig {
                .thread_count = to_thread_count(config.render.thread_count_enum),
                .wire_render_style =
                    to_wire_render_style(config.render.wire_render_style_enum),

                .do_benchmark = config.render.do_benchmark,
                .show_circuit = config.render.show_circuit,
                .show_collision_index = config.render.show_collision_index,
                .show_connection_index = config.render.show_connection_index,
                .show_selection_index = config.render.show_selection_index,

                .show_render_borders = config.render.show_render_borders,
                .show_mouse_position = config.render.show_mouse_position,
                .direct_rendering = config.render.direct_rendering,
                .jit_rendering = config.render.jit_rendering,
            },
        .state = from_c(config.state),
    };
};

[[nodiscard]] auto to_user_action(const uint8_t action_enum)
    -> circuit_ui_model::UserAction {
    const auto style = to_enum<exporting::UserAction>(action_enum);

    switch (style) {
        using enum exporting::UserAction;

        case clear_circuit:
            return circuit_ui_model::UserAction::clear_circuit;
        case reload_circuit:
            return circuit_ui_model::UserAction::reload_circuit;

        case undo:
            return circuit_ui_model::UserAction::undo;
        case redo:
            return circuit_ui_model::UserAction::redo;
        case select_all:
            return circuit_ui_model::UserAction::select_all;
        case copy_selected:
            return circuit_ui_model::UserAction::copy_selected;
        case paste_from_clipboard:
            return circuit_ui_model::UserAction::paste_from_clipboard;
        case cut_selected:
            return circuit_ui_model::UserAction::cut_selected;
        case delete_selected:
            return circuit_ui_model::UserAction::delete_selected;

        case zoom_in:
            return circuit_ui_model::UserAction::zoom_in;
        case zoom_out:
            return circuit_ui_model::UserAction::zoom_out;
        case reset_view:
            return circuit_ui_model::UserAction::reset_view;
    };
    std::terminate();
}

[[nodiscard]] auto to_c(const point_device_fine_t& point) -> ls_point_device_fine_t {
    return ls_point_device_fine_t {
        .x = point.x,
        .y = point.y,
    };
}

[[nodiscard]] auto to_c(const angle_delta_t& delta) -> ls_angle_delta_t {
    return ls_angle_delta_t {
        .horizontal_notches = delta.horizontal_notches,
        .vertical_notches = delta.vertical_notches,
    };
}

[[nodiscard]] auto to_c(KeyboardModifier modifier) -> exporting::KeyboardModifier {
    switch (modifier) {
        using enum KeyboardModifier;

        case Shift:
            return exporting::KeyboardModifier::Shift;
        case Control:
            return exporting::KeyboardModifier::Control;
        case Alt:
            return exporting::KeyboardModifier::Alt;
    };
    std::terminate();
}

[[nodiscard]] auto to_c(const KeyboardModifiers& modifiers) -> uint32_t {
    auto modifiers_result = exporting::KeyboardModifiers {};

    for (const auto modifier : all_keyboard_modifiers) {
        if (modifiers.is_set(modifier)) {
            modifiers_result.set(to_c(modifier));
        }
    }

    return modifiers_result.value();
}

[[nodiscard]] auto to_c(const MouseWheelEvent& event) -> ls_mouse_wheel_event_t {
    return ls_mouse_wheel_event_t {
        .position = to_c(event.position),
        .angle_delta = to_c(event.angle_delta),
        .keyboard_modifiers_bitset = to_c(event.modifiers),
    };
}

[[nodiscard]] auto to_c(const std::optional<double>& value) -> ls_optional_double_t {
    if (value) {
        return ls_optional_double_t {.value = *value, .is_valid = true};
    }
    return ls_optional_double_t {.value = 0, .is_valid = false};
}

[[nodiscard]] auto to_c(const circuit_ui_model::Statistics& statistics)
    -> ls_ui_statistics_t {
    return ls_ui_statistics_t {
        .simulation_events_per_second = to_c(statistics.simulation_events_per_second),
        .frames_per_second = statistics.frames_per_second,
        .pixel_scale = statistics.pixel_scale,
        .image_width_px = gsl::narrow<int32_t>(statistics.image_size.w),
        .image_height_px = gsl::narrow<int32_t>(statistics.image_size.h),
    };
}

[[nodiscard]] auto to_c(const HistoryStatus& status) -> ls_history_status_t {
    return ls_history_status_t {
        .undo_available = status.undo_available,
        .redo_available = status.redo_available,
    };
};

}  // namespace
}  // namespace logicsim

auto ls_circuit_construct() noexcept -> ls_circuit_t* {
    return ls_translate_exception([]() {
        return new ls_circuit_t;  // NOLINT(bugprone-unhandled-exception-at-new)
    });
}

auto ls_circuit_destruct(ls_circuit_t* obj) noexcept -> void {
    ls_translate_exception([&]() { delete obj; });
}

auto ls_circuit_load(ls_circuit_t* obj,
                     uint8_t example_circuit_enum) noexcept -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);

        const auto number = int {example_circuit_enum};
        return to_c(obj->model.load_circuit_example(number));
    });
}

namespace {

auto create_bl_image(int w, int h, void* pixel_data, intptr_t stride) -> BLImage {
    if (w == 0 || h == 0) {
        return BLImage {};
    }

    auto bl_image = BLImage {};
    if (bl_image.createFromData(w, h, BL_FORMAT_PRGB32, pixel_data, stride) !=
        BL_SUCCESS) {
        throw std::runtime_error("Unable to create BLImage");
    }

    return bl_image;
}

auto render_layout_impl(logicsim::CircuitUIModel& model, int32_t width, int32_t height,
                        double pixel_ratio, void* pixel_data, intptr_t stride) -> void {
    Expects(width >= 0);
    Expects(height >= 0);

    const auto w = gsl::narrow<int>(width);
    const auto h = gsl::narrow<int>(height);

    auto bl_image = create_bl_image(w, h, pixel_data, stride);
    model.render(bl_image, logicsim::device_pixel_ratio_t {pixel_ratio});
}
}  // namespace

auto ls_circuit_render_layout(ls_circuit_t* obj, int32_t width, int32_t height,
                              double pixel_ratio, void* pixel_data,
                              intptr_t stride) noexcept -> void {
    ls_translate_exception([&]() {
        Expects(obj);
        render_layout_impl(obj->model, width, height, pixel_ratio, pixel_data, stride);
    });
}

namespace {

[[nodiscard]] auto to_virtual_key(logicsim::exporting::VirtualKey key)
    -> logicsim::VirtualKey {
    using namespace logicsim;

    switch (key) {
        using enum exporting::VirtualKey;

        case Enter:
            return VirtualKey::Enter;
        case Escape:
            return VirtualKey::Escape;
    };
    std::terminate();
}

[[nodiscard]] auto to_virtual_key(uint8_t key) -> logicsim::VirtualKey {
    using namespace logicsim;

    return to_virtual_key(to_enum<exporting::VirtualKey>(key));
}

[[nodiscard]] auto to_mouse_button(logicsim::exporting::MouseButton button)
    -> logicsim::MouseButton {
    using namespace logicsim;

    switch (button) {
        using enum exporting::MouseButton;

        case Left:
            return MouseButton::Left;
        case Right:
            return MouseButton::Right;
        case Middle:
            return MouseButton::Middle;
    };
    std::terminate();
}

[[nodiscard]] auto to_mouse_button(uint8_t button) -> logicsim::MouseButton {
    using namespace logicsim;

    return to_mouse_button(to_enum<exporting::MouseButton>(button));
}

// compiles to 2 instruction on clang: https://godbolt.org/z/KGKrMMqnM
[[nodiscard]] auto to_mouse_buttons(uint32_t buttons_value) -> logicsim::MouseButtons {
    using namespace logicsim;

    const auto buttons_export = exporting::MouseButtons {buttons_value};
    auto buttons_result = MouseButtons {};

    for (const auto button : exporting::all_mouse_buttons) {
        if (buttons_export.is_set(button)) {
            buttons_result.set(to_mouse_button(button));
        }
    }

    return buttons_result;
}

[[nodiscard]] auto to_keyboard_modifier(logicsim::exporting::KeyboardModifier modifier)
    -> logicsim::KeyboardModifier {
    using namespace logicsim;

    switch (modifier) {
        using enum exporting::KeyboardModifier;

        case Shift:
            return KeyboardModifier::Shift;
        case Control:
            return KeyboardModifier::Control;
        case Alt:
            return KeyboardModifier::Alt;
    };
    std::terminate();
}

[[nodiscard]] auto to_keyboard_modifiers(uint32_t modifiers_value)
    -> logicsim::KeyboardModifiers {
    using namespace logicsim;

    const auto modifiers_export = exporting::KeyboardModifiers {modifiers_value};
    auto modifiers_result = KeyboardModifiers {};

    for (const auto modifier : exporting::all_keyboard_modifiers) {
        if (modifiers_export.is_set(modifier)) {
            modifiers_result.set(to_keyboard_modifier(modifier));
        }
    }

    return modifiers_result;
}

[[nodiscard]] auto to_point_device_fine(const ls_point_device_fine_t& point)
    -> logicsim::point_device_fine_t {
    return logicsim::point_device_fine_t {point.x, point.y};
}

[[nodiscard]] auto to_angle_delta(const ls_angle_delta_t& angle_delta)
    -> logicsim::angle_delta_t {
    return logicsim::angle_delta_t {
        .horizontal_notches = angle_delta.horizontal_notches,
        .vertical_notches = angle_delta.vertical_notches,
    };
}

[[nodiscard]] auto to_mouse_wheel_event(const ls_mouse_wheel_event_t& event)
    -> logicsim::MouseWheelEvent {
    return logicsim::MouseWheelEvent {
        .position = to_point_device_fine(event.position),
        .angle_delta = to_angle_delta(event.angle_delta),
        .modifiers = to_keyboard_modifiers(event.keyboard_modifiers_bitset),
    };
}

}  // namespace

auto ls_circuit_config(const ls_circuit_t* obj) noexcept -> ls_ui_config_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);

        return to_c(obj->model.config());
    });
}

auto ls_circuit_set_config(ls_circuit_t* obj,
                           const ls_ui_config_t* config) noexcept -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(config);

        return to_c(obj->model.set_config(from_c(*config)));
    });
}

auto ls_circuit_statistics(const ls_circuit_t* obj) noexcept -> ls_ui_statistics_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);

        return to_c(obj->model.statistics());
    });
}

auto ls_circuit_history_status(const ls_circuit_t* obj) noexcept -> ls_history_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);

        return to_c(obj->model.history_status());
    });
}

auto ls_circuit_do_action(ls_circuit_t* obj,
                          uint8_t action_enum) noexcept -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);

        return to_c(obj->model.do_action(to_user_action(action_enum)));
    });
}

auto ls_circuit_mouse_press(
    ls_circuit_t* obj, const ls_mouse_press_event_t* event) noexcept -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(event);

        return to_c(obj->model.mouse_press(MousePressEvent {
            .position = to_point_device_fine(event->position),
            .modifiers = to_keyboard_modifiers(event->keyboard_modifiers_bitset),
            .button = to_mouse_button(event->button_enum),
            .double_click = event->double_click,
        }));
    });
}

auto ls_circuit_mouse_move(ls_circuit_t* obj, const ls_mouse_move_event_t* event) noexcept
    -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(event);

        return to_c(obj->model.mouse_move(MouseMoveEvent {
            .position = to_point_device_fine(event->position),
            .buttons = to_mouse_buttons(event->buttons_bitset),
        }));
    });
}

auto ls_circuit_mouse_release(
    ls_circuit_t* obj, const ls_mouse_release_event_t* event) noexcept -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(event);

        return to_c(obj->model.mouse_release(MouseReleaseEvent {
            .position = to_point_device_fine(event->position),
            .button = to_mouse_button(event->button_enum),
        }));
    });
}

auto ls_circuit_mouse_wheel(
    ls_circuit_t* obj, const ls_mouse_wheel_event_t* event) noexcept -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(event);

        return to_c(obj->model.mouse_wheel(to_mouse_wheel_event(*event)));
    });
}

auto ls_combine_wheel_event(const ls_mouse_wheel_event_t* first,
                            const ls_mouse_wheel_event_t* second) noexcept
    -> ls_combine_wheel_event_result_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(first);
        Expects(second);

        if (const auto result = combine_wheel_event(to_mouse_wheel_event(*first),
                                                    to_mouse_wheel_event(*second))) {
            return ls_combine_wheel_event_result_t {
                .value = to_c(*result),
                .is_valid = true,
            };
        }
        return ls_combine_wheel_event_result_t {.value = {}, .is_valid = false};
    });
}

auto ls_circuit_key_press(ls_circuit_t* obj, int32_t key) noexcept -> ls_ui_status_t {
    return ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);

        return to_c(obj->model.key_press(to_virtual_key(key)));
    });
}
