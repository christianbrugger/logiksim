#include "core_export/logicsim_core_export.h"

#include "core/algorithm/to_enum.h"
#include "core/circuit_ui_model.h"

#include <blend2d.h>
#include <gsl/gsl>

//
// C Interface
//

struct ls_circuit_t {
    logicsim::CircuitUiModel model {};
};

template <typename Func>
auto ls_translate_exception(Func&& func) noexcept {
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

auto ls_circuit_construct() LS_NOEXCEPT -> ls_circuit_t* {
    return ls_translate_exception([]() {
        return new ls_circuit_t;  // NOLINT(bugprone-unhandled-exception-at-new)
    });
}

auto ls_circuit_destruct(ls_circuit_t* obj) LS_NOEXCEPT -> void {
    ls_translate_exception([&]() { delete obj; });
}

auto ls_circuit_load(ls_circuit_t* obj, int32_t example_circuit) LS_NOEXCEPT -> void {
    ls_translate_exception([&]() {
        Expects(obj);
        const auto number = gsl::narrow<int>(example_circuit);
        obj->model.load_circuit_example(number);
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

auto render_layout_impl(logicsim::CircuitUiModel& model, int32_t width, int32_t height,
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
                              intptr_t stride) LS_NOEXCEPT -> void {
    ls_translate_exception([&]() {
        Expects(obj);
        render_layout_impl(obj->model, width, height, pixel_ratio, pixel_data, stride);
    });
}

namespace {

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

[[nodiscard]] auto to_mouse_button(int32_t button) -> logicsim::MouseButton {
    using namespace logicsim;

    return to_mouse_button(to_enum<exporting::MouseButton>(button));
}

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

}  // namespace

auto ls_circuit_mouse_press(ls_circuit_t* obj,
                            const ls_mouse_press_event_t* event) LS_NOEXCEPT -> void {
    ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(event);

        obj->model.mouse_press(MousePressEvent {
            .position = to_point_device_fine(event->position),
            .modifiers = to_keyboard_modifiers(event->keyboard_modifiers),
            .button = to_mouse_button(event->button),
            .double_click = event->double_click != 0,
        });
    });
}

auto ls_circuit_mouse_move(ls_circuit_t* obj,
                           const ls_mouse_move_event_t* event) LS_NOEXCEPT -> void {
    ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(event);

        obj->model.mouse_move(MouseMoveEvent {
            .position = to_point_device_fine(event->position),
            .buttons = to_mouse_buttons(event->buttons),
        });
    });
}

auto ls_circuit_mouse_release(ls_circuit_t* obj,
                              const ls_mouse_release_event_t* event) LS_NOEXCEPT -> void {
    ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);
        Expects(event);

        obj->model.mouse_release(MouseReleaseEvent {
            .position = to_point_device_fine(event->position),
            .button = to_mouse_button(event->button),
        });
    });
}
