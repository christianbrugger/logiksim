
#include "render_scene.h"

#include "format.h"
#include "range.h"

namespace logicsim {

SimulationScene::SimulationScene(const Simulation& simulation) noexcept
    : simulation_ {&simulation},
      draw_data_vector_(simulation.circuit().element_count()) {}

auto SimulationScene::set_position(Circuit::ConstElement element, point2d_t position)
    -> void {
    get_data(element).position = position;
}

auto SimulationScene::set_wire_tree(Circuit::ConstElement element,
                                    std::vector<point2d_t> points,
                                    std::vector<wire_index_t> indices) -> void {
    // this method is potentially very slow
    get_data(element).points.assign(points.begin(), points.end());
    get_data(element).indices.assign(indices.begin(), indices.end());
}

auto SimulationScene::get_data(Circuit::ConstElement element) -> DrawData& {
    return draw_data_vector_.at(element.element_id());
}

auto SimulationScene::get_data(Circuit::ConstElement element) const -> const DrawData& {
    return draw_data_vector_.at(element.element_id());
}

auto SimulationScene::draw_background(BLContext& ctx) const -> void {
    ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
    ctx.fillAll();
}

auto distance_1d(point2d_t p0, point2d_t p1) {
    auto dx = p1.x - p0.x;
    auto dy = p1.y - p0.y;
    assert(dx == 0 || dy == 0);  // distance_1d requires horizontal or vertical line
    return std::abs((dx == 0) ? dy : dx);
}

auto interpolate_1d(point_t v0, point_t v1, double ratio) -> double {
    return v0 + (v1 - v0) * ratio;
}

auto interpolate_line_1d(point2d_t p0, point2d_t p1, time_t t_start, time_t t_end,
                         time_t t_select) -> point2d_fine_t {
    fmt::print("interpolate_line_1d t_start = {} t_end = {} t_select = {}\n", t_start,
               t_end, t_select);
    assert(t_start <= t_end);

    if (t_select <= t_start) {
        return static_cast<point2d_fine_t>(p0);
    }
    if (t_select >= t_end) {
        return static_cast<point2d_fine_t>(p1);
    }
    // TODO check numerics
    const double ratio
        = static_cast<double>((t_select - t_start).count()) / (t_end - t_start).count();
    if (p0.x == p1.x) {
        return point2d_fine_t {static_cast<double>(p0.x),
                               interpolate_1d(p0.y, p1.y, ratio)};
    }
    assert(p0.y == p1.y);
    return point2d_fine_t {interpolate_1d(p0.x, p1.x, ratio), static_cast<double>(p0.y)};
}

template <typename PointType>
auto draw_line_segment(BLContext& ctx, PointType p0, PointType p1, bool wire_enabled)
    -> void {
    const uint32_t color = wire_enabled ? 0xFFFF0000u : 0xFF000000u;
    ctx.setStrokeStyle(BLRgba32(color));
    constexpr static double s = 12;
    ctx.strokeLine(BLLine(p0.x * s, p0.y * s, p1.x * s, p1.y * s));
}

auto draw_line_segment(BLContext& ctx, point2d_t p0, point2d_t p1, time_t time_start,
                       time_t time_end, const Simulation::history_vector_t& history,
                       bool wire_enabled) -> void {
    fmt::print("times = {} - {}\n", time_start, time_end);
    fmt::print("history = {}\n", history);

    if (history.size() == 0 || time_end >= history.at(0)) {
        draw_line_segment(ctx, p0, p1, wire_enabled);
        return;
    }

    const auto it_start = std::lower_bound(history.rbegin(), history.rend(), time_start);
    const auto it_end = std::lower_bound(history.rbegin(), history.rend(), time_end);

    const auto idx_start = history.rend() - it_start;
    const auto idx_end = history.rend() - it_end;

    fmt::print("index start = {} end = {}\n", idx_start, idx_end);

    if (idx_start == idx_end) {
        const bool value = static_cast<bool>(idx_start % 2) ^ wire_enabled;
        draw_line_segment(ctx, p0, p1, value);
        return;
    }

    auto p_pivot = static_cast<point2d_fine_t>(p0);

    for (auto index : range(idx_start, idx_end + 1)) {
        point2d_fine_t p_end;
        if (index >= std::ssize(history) || time_end >= history.at(index)) {
            p_end = static_cast<point2d_fine_t>(p1);
        } else {
            auto t_end = history.at(index);
            // TODO fix flipping of time_end and time_start
            p_end = interpolate_line_1d(p1, p0, time_end, time_start, t_end);
        }

        const bool value = static_cast<bool>(index % 2) ^ wire_enabled;
        draw_line_segment(ctx, p_pivot, p_end, value);

        p_pivot = p_end;
    }
}

auto SimulationScene::draw_wire(BLContext& ctx, Circuit::ConstElement element) const
    -> void {
    constexpr static double s = 12;
    ctx.setStrokeWidth(1);

    const auto& data = get_data(element);

    if (data.points.size() < 2) {
        return;
    }

    const auto& history = simulation_->get_input_history(element);
    const bool wire_enabled = simulation_->input_value(element.input(0));

    auto d0 = delay_t {0us};
    auto p0 = data.points.at(0);
    auto p1 = data.points.at(1);
    size_t i = 2;

    // TODO remove allocation
    folly::small_vector<delay_t, 128> delay_index {};
    delay_index.push_back(d0);

    while (true) {
        auto d1 = delay_t::runtime(
            d0.value + distance_1d(p0, p1) * Simulation::wire_delay_per_distance.value);
        delay_index.push_back(d1);

        const auto time_start = time_t {simulation_->time() - d0.value};
        const auto time_end = time_t {simulation_->time() - d1.value};
        draw_line_segment(ctx, p0, p1, time_start, time_end, history, wire_enabled);

        // uint32_t color
        //     = simulation_->input_value(element.input(0)) ? 0xFFFF0000u : 0xFF000000u;
        // ctx.setStrokeStyle(BLRgba32(color));
        // ctx.strokeLine(BLLine(p0.x * s, p0.y * s, p1.x * s, p1.y * s));

        if (i >= data.points.size()) {
            break;
        }
        auto p0_index = data.indices.at(i - 2);
        d0 = delay_index.at(p0_index);
        p0 = data.points.at(p0_index);
        p1 = data.points.at(i);
        ++i;
    }
}

auto SimulationScene::draw_standard_element(BLContext& ctx,
                                            Circuit::ConstElement element) const -> void {
    constexpr static double s = 12;
    ctx.setStrokeWidth(1);

    const auto& data = get_data(element);
    auto input_values = simulation_->input_values(element);
    auto output_values = simulation_->output_values(element);

    // draw rect
    double x = data.position.x * s;
    double y = data.position.y * s;
    auto height = std::max(std::ssize(input_values), std::ssize(output_values));
    BLPath path;
    path.addRect(x, y + -0.5 * s, 2 * s, height * s);

    // draw inputs & outputs
    // auto input_offset = (height - 1) / 2;  // ranges::size(input_values)) / 2;
    auto input_offset = (height - std::ssize(input_values)) / 2;
    for (int i = 0; const auto value : input_values) {
        double y_pin = y + (input_offset + i) * s;
        uint32_t color = value ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(x, y_pin, x - 0.75 * s, y_pin));
        ++i;
    }

    auto output_offset = (height - std::ssize(output_values)) / 2;
    for (int i = 0; const auto value : output_values) {
        double y_pin = y + (output_offset + i) * s;
        uint32_t color = value ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(x + 2 * s, y_pin, x + 2.75 * s, y_pin));
        ++i;
    }

    ctx.setFillStyle(BLRgba32(0xFFFFFF00u));
    ctx.setStrokeStyle(BLRgba32(0xFF000000u));
    ctx.fillPath(path);
    ctx.strokePath(path);
}

auto SimulationScene::render_scene(BLContext& ctx) const -> void {
    ctx.postTranslate(BLPoint(0.5, 0.5));
    ctx.postScale(1);

    draw_background(ctx);

    for (auto element : simulation_->circuit().elements()) {
        if (element.element_type() == ElementType::wire) {
            draw_wire(ctx, element);
        } else {
            draw_standard_element(ctx, element);
        }
    }
}

}  // namespace logicsim
