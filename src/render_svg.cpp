#include "render_svg.h"

#include "blend2d.h"
#include "file.h"
#include "format.h"
#include "render_helper.h"
#include "resource.h"
#include "vocabulary.h"

#include <svgshapes.h>

namespace logicsim {

auto render_icon_impl(BLContext &bl_ctx, icon_t icon, BLPoint position, color_t color)
    -> void {
    const auto binary = load_file(get_icon_path(icon));

    auto doc = svg2b2d::SVGDocument {};
    auto byte_span = svg2b2d::ByteSpan {binary.data(), binary.size()};
    doc.readFromData(byte_span);

    {
        auto _ [[maybe_unused]] = make_context_guard(bl_ctx);

        bl_ctx.translate(position);
        bl_ctx.scale(10.0);
        bl_ctx.setStrokeStyle(color);

        doc.draw(bl_ctx);
    }
}

auto render_icon(BLContext &bl_ctx) -> void {
    render_icon_impl(bl_ctx, icon_t::setting_handle, BLPoint {100, 100},
                     defaults::color_green);
    render_icon_impl(bl_ctx, icon_t::copy, BLPoint {400, 100},
                     defaults::color_light_blue);
    render_icon_impl(bl_ctx, icon_t::zoom_in, BLPoint {700, 100}, defaults::color_orange);
}

}  // namespace logicsim