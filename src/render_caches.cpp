#include "render_caches.h"

#include "collision.h"
#include "editable_circuit/editable_circuit.h"
#include "geometry.h"

#include <numbers>

namespace logicsim {

auto _directed_input_marker(BLContext& ctx, point_t point, color_t color,
                            orientation_t orientation, double size,
                            const OldRenderSettings& settings) -> void {
    auto _ = ContextGuard {ctx};

    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(color);

    const auto [x, y] = to_context(point, settings.view_config);
    const auto d = to_context(size, settings.view_config);
    const auto angle = to_angle(orientation);

    ctx.translate(BLPoint {x, y});
    ctx.rotate(angle);

    const auto pi = std::numbers::pi;

    ctx.strokeArc(BLArc {0, 0, d, d, -pi / 2, pi});
    ctx.strokeLine(BLLine {-d, -d, 0, -d});
    ctx.strokeLine(BLLine {-d, +d, 0, +d});
}

auto _undirected_input_marker(BLContext& ctx, point_t point, color_t color,
                              orientation_t orientation, double size,
                              const OldRenderSettings& settings) -> void {
    auto _ = ContextGuard {ctx};

    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(color);

    const auto [x, y] = to_context(point, settings.view_config);
    const auto d = to_context(size, settings.view_config);
    const auto h = d / 2;

    ctx.translate(BLPoint {x + 0.5, y + 0.5});

    ctx.strokeLine(BLLine {-d, -d, -h, -d});
    ctx.strokeLine(BLLine {+h, -d, +d, -d});

    ctx.strokeLine(BLLine {-d, -d, -d, -h});
    ctx.strokeLine(BLLine {-d, +h, -d, +d});

    ctx.strokeLine(BLLine {+d, -d, +d, -h});
    ctx.strokeLine(BLLine {+d, +h, +d, +d});

    ctx.strokeLine(BLLine {-d, +d, -h, +d});
    ctx.strokeLine(BLLine {+h, +d, +d, +d});
}

auto render_input_marker(BLContext& ctx, point_t point, color_t color,
                         orientation_t orientation, double size,
                         const OldRenderSettings& settings) -> void {
    if (orientation == orientation_t::undirected) {
        _undirected_input_marker(ctx, point, color, orientation, size, settings);
    } else {
        _directed_input_marker(ctx, point, color, orientation, size, settings);
    }
}

auto render_undirected_output(BLContext& ctx, point_t position, double size,
                              const OldRenderSettings& settings) {
    draw_point(ctx, position, PointShape::cross, defaults::color_green, size / 4,
               settings);
    draw_point(ctx, position, PointShape::plus, defaults::color_green, size / 3,
               settings);
}

auto render_editable_circuit_connection_cache(BLContext& ctx,
                                              const EditableCircuit& editable_circuit,
                                              const OldRenderSettings& settings) -> void {
    const auto scene_rect = get_scene_rect(settings.view_config);
    const auto& caches = editable_circuit.caches();

    for (auto [position, orientation] : caches.input_positions_and_orientations()) {
        if (!is_colliding(position, scene_rect)) {
            continue;
        }

        const auto size = 1.0 / 3.0;
        render_input_marker(ctx, position, defaults::color_green, orientation, size,
                            settings);
    }

    for (auto [position, orientation] : caches.output_positions_and_orientations()) {
        if (!is_colliding(position, scene_rect)) {
            continue;
        }

        const auto size = 0.8;
        if (orientation == orientation_t::undirected) {
            render_undirected_output(ctx, position, size, settings);
        } else {
            draw_arrow(ctx, position, defaults::color_green, orientation, size, settings);
        }
    }
}

auto render_editable_circuit_collision_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const OldRenderSettings& settings) -> void {
    constexpr static auto color = defaults::color_orange;
    constexpr static auto size = 0.25;

    const auto scene_rect = get_scene_rect(settings.view_config);

    for (auto [point, state] : editable_circuit.caches().collision_states()) {
        if (!is_colliding(point, scene_rect)) {
            continue;
        }

        switch (state) {
            using enum collision_cache::CacheState;

            case element_body: {
                draw_point(ctx, point, PointShape::square, color, size, settings);
                break;
            }
            case element_connection: {
                draw_point(ctx, point, PointShape::circle, color, size, settings);
                break;
            }
            case wire_connection: {
                draw_point(ctx, point, PointShape::full_square, color, size * (2. / 3),
                           settings);
                break;
            }
            case wire_horizontal: {
                draw_point(ctx, point, PointShape::horizontal, color, size, settings);
                break;
            }
            case wire_vertical: {
                draw_point(ctx, point, PointShape::vertical, color, size, settings);
                break;
            }
            case wire_corner_point: {
                draw_point(ctx, point, PointShape::diamond, color, size, settings);
                break;
            }
            case wire_cross_point: {
                draw_point(ctx, point, PointShape::cross, color, size, settings);
                break;
            }
            case wire_crossing: {
                draw_point(ctx, point, PointShape::plus, color, size, settings);
                break;
            }
            case element_wire_connection: {
                draw_point(ctx, point, PointShape::full_circle, color, size, settings);
                break;
            }
            case invalid_state: {
                throw_exception("invalid state encountered");
                break;
            }
        }
    }
}

auto render_editable_circuit_selection_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const OldRenderSettings& settings) -> void {
    const auto scene_rect = get_scene_rect_fine(settings.view_config);
    for (const rect_fine_t& rect : editable_circuit.caches().selection_rects()) {
        // TODO introduce is_visible
        if (!is_colliding(rect, scene_rect)) {
            continue;
        }

        draw_rect(ctx, rect,
                  RectAttributes {
                      .draw_type = DrawType::stroke,
                      .stroke_width = 1,
                      .stroke_color = defaults::color_lime,
                  },
                  settings);
    }
}

}  // namespace logicsim