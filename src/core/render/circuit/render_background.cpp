#include "core/render/circuit/render_background.h"

#include "core/algorithm/round.h"
#include "core/geometry/grid.h"
#include "core/geometry/scene.h"
#include "core/render/context.h"
#include "core/render/primitive/line.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/rect_fine.h"

namespace logicsim {

namespace {

auto draw_grid_space_limit(Context& ctx) {
    constexpr auto stroke_color = defaults::color_gray;
    constexpr auto stroke_width = grid_fine_t {5.0};

    const auto stroke_width_px = std::max(5.0, to_context(stroke_width, ctx));

    const auto p0 = to_context(point_t {grid_t::min(), grid_t::min()}, ctx);
    const auto p1 = to_context(point_t {grid_t::max(), grid_t::max()}, ctx);

    ctx.bl_ctx.setStrokeWidth(stroke_width_px);
    ctx.bl_ctx.strokeRect(BLRect {p0.x + 0.5, p0.y + 0.5, p1.x - p0.x, p1.y - p0.y},
                          stroke_color);
}

auto draw_background_pattern_checker(Context& ctx, rect_fine_t scene_rect, int delta,
                                     color_t color, int width) {
    const auto g0 = point_t {
        to_floored(floor(scene_rect.p0.x / delta) * delta),
        to_floored(floor(scene_rect.p0.y / delta) * delta),
    };
    const auto g1 = point_t {
        to_ceiled(ceil(scene_rect.p1.x / delta) * delta),
        to_ceiled(ceil(scene_rect.p1.y / delta) * delta),
    };

    /*
    for (int x = int {g0.x}; x <= int {g1.x}; x += delta) {
        const auto x_grid = grid_t {x};
        draw_line(ctx, line_t {{x_grid, g0.y}, {x_grid, g1.y}}, {color, width}, config);
    }
    for (int y = int {g0.y}; y <= int {g1.y}; y += delta) {
        const auto y_grid = grid_t {y};
        draw_line(ctx, line_t {{g0.x, y_grid}, {g1.x, y_grid}}, {color, width}, config);
    }
    */

    // this version is a bit faster
    const auto p0 = to_context(g0, ctx);
    const auto p1 = to_context(g1, ctx);

    const auto offset = ctx.view_config().offset();
    const auto scale = ctx.view_config().pixel_scale();

    // vertical
    for (int x = int {g0.x}; x <= int {g1.x}; x += delta) {
        const auto cx = round_fast(double {(grid_fine_t {x} + offset.x) * scale});
        draw_orthogonal_line(ctx, BLLine {cx, p0.y, cx, p1.y}, {color, width});
    }
    // horizontal
    for (int y = int {g0.y}; y <= int {g1.y}; y += delta) {
        const auto cy = round_fast(double {(grid_fine_t {y} + offset.y) * scale});
        draw_orthogonal_line(ctx, BLLine {p0.x, cy, p1.x, cy}, {color, width});
    }
}

auto draw_background_patterns(Context& ctx) {
    auto scene_rect = get_scene_rect_fine(ctx.view_config());

    constexpr static auto grid_definition = {
        std::tuple {1, monochrome(0xF0), 1},    //
        std::tuple {8, monochrome(0xE4), 1},    //
        std::tuple {64, monochrome(0xE4), 2},   //
        std::tuple {512, monochrome(0xD8), 2},  //
        std::tuple {4096, monochrome(0xC0), 2},
    };

    for (auto&& [delta, color, width] : grid_definition) {
        if (delta * ctx.view_config().device_scale() >=
            ctx.settings.background_grid_min_distance_device) {
            const auto draw_width_f = width * ctx.view_config().device_pixel_ratio();
            // we substract a little, as we want 150% scaling to round down
            const auto epsilon = 0.01;
            const auto draw_width = std::max(1, round_to<int>(draw_width_f - epsilon));
            draw_background_pattern_checker(ctx, scene_rect, delta, color, draw_width);
        }
    }
}
}  // namespace

auto render_background(Context& ctx) -> void {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.bl_ctx.fillAll(defaults::color_white);

    draw_background_patterns(ctx);
    draw_grid_space_limit(ctx);
}

}  // namespace logicsim
