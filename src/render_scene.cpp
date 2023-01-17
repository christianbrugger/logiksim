
#include "render_scene.h"

#include <iostream>

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

auto SimulationScene::draw_wire(BLContext& ctx, Circuit::ConstElement element) const
    -> void {
    constexpr static double s = 12;
    ctx.setStrokeWidth(1);

    const auto& data = get_data(element);

    if (data.points.size() < 2) {
        return;
    }

    auto p0 = data.points.at(0);
    auto p1 = data.points.at(1);
    size_t i = 2;

    while (true) {
        uint32_t color
            = simulation_->input_value(element.input(0)) ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(p0.x * s, p0.y * s, p1.x * s, p1.y * s));

        if (i >= data.points.size()) {
            break;
        }
        p0 = data.points.at(data.indices.at(i - 2));
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
        std::cout << value;
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
