#include "render_caches.h"

#include "editable_circuit/editable_circuit.h"
#include "exception.h"
#include "geometry/orientation.h"
#include "geometry/rect.h"
#include "geometry/scene.h"

#include <numbers>

namespace logicsim {

auto _directed_input_marker(Context& ctx, point_t point, color_t color,
                            orientation_t orientation, grid_fine_t size) -> void {
    auto _ [[maybe_unused]] = make_context_guard(ctx);

    const auto [x, y] = to_context(point, ctx);
    const auto d = to_context(size, ctx);
    const auto angle = to_angle(orientation);

    ctx.bl_ctx.translate(BLPoint {x, y});
    ctx.bl_ctx.rotate(angle);

    const auto pi = std::numbers::pi;

    ctx.bl_ctx.setStrokeWidth(1);
    ctx.bl_ctx.strokeArc(BLArc {0, 0, d, d, -pi / 2, pi}, color);
    ctx.bl_ctx.strokeLine(BLLine {-d, -d, 0, -d}, color);
    ctx.bl_ctx.strokeLine(BLLine {-d, +d, 0, +d}, color);
}

auto _undirected_input_marker(Context& ctx, point_t point, color_t color,
                              grid_fine_t size) -> void {
    auto _ [[maybe_unused]] = make_context_guard(ctx);

    ctx.bl_ctx.setStrokeWidth(1);
    ctx.bl_ctx.setStrokeStyle(color);

    const auto [x, y] = to_context(point, ctx);
    const auto d = to_context(size, ctx);
    const auto h = d / 2;

    ctx.bl_ctx.translate(BLPoint {x + 0.5, y + 0.5});

    ctx.bl_ctx.strokeLine(BLLine {-d, -d, -h, -d});
    ctx.bl_ctx.strokeLine(BLLine {+h, -d, +d, -d});

    ctx.bl_ctx.strokeLine(BLLine {-d, -d, -d, -h});
    ctx.bl_ctx.strokeLine(BLLine {-d, +h, -d, +d});

    ctx.bl_ctx.strokeLine(BLLine {+d, -d, +d, -h});
    ctx.bl_ctx.strokeLine(BLLine {+d, +h, +d, +d});

    ctx.bl_ctx.strokeLine(BLLine {-d, +d, -h, +d});
    ctx.bl_ctx.strokeLine(BLLine {+h, +d, +d, +d});
}

auto render_input_marker(Context& ctx, point_t point, color_t color,
                         orientation_t orientation, grid_fine_t size) -> void {
    if (orientation == orientation_t::undirected) {
        _undirected_input_marker(ctx, point, color, size);
    } else {
        _directed_input_marker(ctx, point, color, orientation, size);
    }
}

auto render_undirected_output(Context& ctx, point_t position, grid_fine_t size) {
    draw_point(ctx, position, PointShape::cross, defaults::color_green, size / 4);
    draw_point(ctx, position, PointShape::plus, defaults::color_green, size / 3);
}

auto render_editable_circuit_connection_cache(Context& ctx,
                                              const EditableCircuit& editable_circuit)
    -> void {
    const auto scene_rect = get_scene_rect(ctx.settings.view_config);
    const auto& caches = editable_circuit.caches();

    for (auto [position, orientation] : caches.input_positions_and_orientations()) {
        if (!is_colliding(position, scene_rect)) {
            continue;
        }

        const auto size = grid_fine_t {1.0 / 3.0};
        render_input_marker(ctx, position, defaults::color_green, orientation, size);
    }

    for (auto [position, orientation] : caches.output_positions_and_orientations()) {
        if (!is_colliding(position, scene_rect)) {
            continue;
        }

        const auto size = grid_fine_t {0.8};
        if (orientation == orientation_t::undirected) {
            render_undirected_output(ctx, position, size);
        } else {
            draw_arrow(ctx, position, defaults::color_green, orientation, size);
        }
    }
}

auto render_editable_circuit_collision_cache(Context& ctx,
                                             const EditableCircuit& editable_circuit)
    -> void {
    constexpr static auto color = defaults::color_orange;
    constexpr static auto size = grid_fine_t {0.25};

    const auto scene_rect = get_scene_rect(ctx.settings.view_config);

    for (auto [point, state] : editable_circuit.caches().collision_states()) {
        if (!is_colliding(point, scene_rect)) {
            continue;
        }

        switch (state) {
            using enum collision_cache::CacheState;

            case element_body: {
                draw_point(ctx, point, PointShape::square, color, size);
                break;
            }
            case element_connection: {
                draw_point(ctx, point, PointShape::circle, color, size);
                break;
            }
            case wire_connection: {
                draw_point(ctx, point, PointShape::full_square, color, size * (2. / 3));
                break;
            }
            case wire_horizontal: {
                draw_point(ctx, point, PointShape::horizontal, color, size);
                break;
            }
            case wire_vertical: {
                draw_point(ctx, point, PointShape::vertical, color, size);
                break;
            }
            case wire_corner_point: {
                draw_point(ctx, point, PointShape::diamond, color, size);
                break;
            }
            case wire_cross_point: {
                draw_point(ctx, point, PointShape::cross, color, size);
                break;
            }
            case wire_crossing: {
                draw_point(ctx, point, PointShape::plus, color, size);
                break;
            }
            case element_wire_connection: {
                draw_point(ctx, point, PointShape::full_circle, color, size);
                break;
            }
            case invalid_state: {
                throw_exception("invalid state encountered");
                break;
            }
        }
    }
}

auto render_editable_circuit_selection_cache(Context& ctx,
                                             const EditableCircuit& editable_circuit)
    -> void {
    const auto scene_rect = get_scene_rect_fine(ctx.settings.view_config);
    for (const rect_fine_t& rect : editable_circuit.caches().selection_rects()) {
        // TODO introduce is_visible
        if (!is_colliding(rect, scene_rect)) {
            continue;
        }

        draw_rect(ctx, rect,
                  RectAttributes {
                      .draw_type = ShapeDrawType::stroke,
                      .stroke_width = 1,
                      .stroke_color = defaults::color_lime,
                  });
    }
}

}  // namespace logicsim