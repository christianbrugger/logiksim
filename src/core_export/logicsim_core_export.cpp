#include "core_export/logicsim_core_export.h"

#include "core/circuit_example.h"
#include "core/editable_circuit.h"
#include "core/render/circuit/render_background.h"
#include "core/render/circuit/render_circuit.h"
#include "core/render/context_cache.h"
#include "core/render/image_surface.h"
#include "core/vocabulary/context_render_settings.h"
#include "core/vocabulary/widget_render_config.h"

#include <blend2d.h>
#include <gsl/gsl>

namespace logicsim {

//
// Exported Circuit Impl
//

class ExportedCircuit_Impl {
   public:
    auto load_circuit(int number) -> void;
    auto render_layout(int32_t width, int32_t height, double pixel_ratio,
                       void* pixel_data, intptr_t stride) -> void;

   private:
    EditableCircuit editable_circuit_ {Layout {},
                                       EditableCircuit::Config {.enable_history = true}};

    WidgetRenderConfig render_config_ {};
    ContextRenderSettings context_settings_ {};
    // used for layered rendering
    ImageSurface context_surface_ {};
    // to cache SVG and Text
    ContextCache context_cache_ {cache_with_default_fonts()};
};

auto ExportedCircuit_Impl::load_circuit(int number) -> void {
    editable_circuit_ = load_example_with_logging(number);
}

auto render_to_context_2(Context& ctx, ImageSurface& surface,
                         const WidgetRenderConfig& render_config,
                         const Layout& layout) -> void {
    render_background(ctx);
    if (render_config.show_circuit) {
        render_layout(ctx, surface, layout);
    }
}

auto ExportedCircuit_Impl::render_layout(int32_t width, int32_t height,
                                         double pixel_ratio, void* pixel_data,
                                         intptr_t stride) -> void {
    const auto w = gsl::narrow<int>(width);
    const auto h = gsl::narrow<int>(height);

    context_settings_.view_config.set_size(BLSizeI {w, h});
    context_settings_.view_config.set_device_pixel_ratio(pixel_ratio);

    auto bl_image = BLImage {};
    if (bl_image.createFromData(w, h, BL_FORMAT_PRGB32, pixel_data, stride) !=
        BL_SUCCESS) {
        throw std::runtime_error("Unable to create BLImage");
    }

    render_to_image(bl_image, context_settings_, context_cache_, [&](Context& ctx) {
        render_to_context_2(ctx, context_surface_, render_config_,
                            editable_circuit_.layout());
    });
}

}  // namespace logicsim

//
// C Interface
//

struct ls_circuit {
    logicsim::ExportedCircuit_Impl data;
};

template <typename Func>
auto ls_translate_exception(Func&& func) {
    try {
        return std::invoke(func);
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
        obj->data.load_circuit(example_circuit);
    });
}

auto ls_circuit_render_layout(ls_circuit_t obj, int32_t width, int32_t height,
                              double pixel_ratio, void* pixel_data,
                              intptr_t stride) -> void {
    ls_translate_exception([&]() {
        Expects(obj);
        obj->data.render_layout(width, height, pixel_ratio, pixel_data, stride);
    });
}
