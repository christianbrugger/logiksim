#include "core_export/logicsim_core_export.h"

#include "core/circuit_ui_model.h"

#include <blend2d.h>
#include <gsl/gsl>

#include <type_traits>

//
// C Interface
//

struct ls_circuit {
    logicsim::CircuitUiModel model {};
};

template <typename Func>
auto ls_translate_exception(Func&& func) {
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

auto ls_circuit_construct() -> ls_circuit_t {
    return ls_translate_exception([]() { return new ls_circuit; });
}

auto ls_circuit_destruct(ls_circuit_t obj) -> void {
    ls_translate_exception([&]() { delete obj; });
}

auto ls_circuit_load(ls_circuit_t obj, int32_t example_circuit) -> void {
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

auto ls_circuit_render_layout(ls_circuit_t obj, int32_t width, int32_t height,
                              double pixel_ratio, void* pixel_data,
                              intptr_t stride) -> void {
    ls_translate_exception([&]() {
        Expects(obj);
        render_layout_impl(obj->model, width, height, pixel_ratio, pixel_data, stride);
    });
}

namespace {

[[nodiscard]] auto to_mouse_event_type(int32_t type) -> logicsim::MouseEventType {
    using namespace logicsim;

    using T = std::underlying_type_t<exporting::MouseEventType>;
    const auto type_enum = static_cast<exporting::MouseEventType>(gsl::narrow<T>(type));

    switch (type_enum) {
        using enum exporting::MouseEventType;

        case Press:
            return MouseEventType::Press;
        case Move:
            return MouseEventType::Move;
        case Release:
            return MouseEventType::Release;
    };
    std::terminate();
}

}  // namespace

auto ls_circuit_mouse_event(ls_circuit_t obj, ls_point_device_fine_t position,
                            int32_t mouse_event_type) -> void {
    ls_translate_exception([&]() {
        using namespace logicsim;
        Expects(obj);

        obj->model.mouse_event(MouseEvent {
            .position = point_device_fine_t {position.x, position.y},
            .button = MouseButtonType::MiddleButton,
            .type = to_mouse_event_type(mouse_event_type),
            .modifier = KeyboardModifierType::NoModifier,
        });
    });
}
