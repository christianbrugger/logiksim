
#include "render_scene.h"

#include <iostream>

namespace logicsim {

void render_background(BLContext& ctx) {
    ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
    ctx.fillAll();
}

void draw_wire_element(BLContext& ctx, Circuit::ConstElement element,
                       const DrawAttribute& attributes, const Simulation& simulation) {
    constexpr static double s = 10;
    ctx.setStrokeWidth(1);

    if (attributes.points.size() < 2) [[unlikely]] {
        throw_exception("A line needs at least two points to be drawn.");
    }

    auto p0 = attributes.points.at(0);
    auto p1 = attributes.points.at(1);
    size_t i = 2;

    while (true) {
        uint32_t color
            = simulation.input_value(element.input(0)) ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(p0.x * s, p0.y * s, p1.x * s, p1.y * s));

        if (i >= attributes.points.size()) {
            break;
        }
        p0 = attributes.points.at(attributes.indices.at(i - 2));
        p1 = attributes.points.at(i);
        ++i;
    }
}

void draw_standard_element(BLContext& ctx, ElementType type,
                           const DrawAttribute& attributes,
                           const std::ranges::input_range auto input_values,
                           const std::ranges::input_range auto output_values) {
    constexpr static double s = 10;
    ctx.setStrokeWidth(1);

    // draw rect
    double x = attributes.points.at(0).x * s;
    double y = attributes.points.at(0).y * s;
    int height = std::max(
        2, 2);  // ranges::size(input_values), ranges::size(output_values));  TODO
    BLPath path;
    path.addRect(x, y + -0.5 * s, 2 * s, height * s);

    // draw inputs & outputs
    int input_offset = (height - 1) / 2;  // ranges::size(input_values)) / 2;  TODO
    for (int i = 0; const auto value : input_values) {
        double y_pin = y + (input_offset + i) * s;
        uint32_t color = value ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(x, y_pin, x - 0.75 * s, y_pin));
        ++i;
    }

    int output_offset = (height - 2) / 2;  // ranges::size(output_values)) / 2;  TODO
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

    std::cout << static_cast<int>(type);
}

void draw_element(BLContext& ctx, Circuit::ConstElement element,
                  const DrawAttribute& attributes, const Simulation& simulation) {
    switch (element.element_type()) {
        using enum ElementType;

        case wire:
            draw_wire_element(ctx, element, attributes, simulation);
            break;
        case inverter_element:
        case and_element:
        case or_element:
        case xor_element:
            draw_standard_element(ctx, element.element_type(), attributes,
                                  simulation.input_values(element),
                                  simulation.output_values(element));
        default:
            break;
    }
}

auto render_scene(BLContext& ctx, const Simulation& simulation,
                  const attribute_vector_t& attributes) -> void {
    ctx.postTranslate(BLPoint(0.5, 0.5));
    // ctx.postScale(2);

    render_background(ctx);

    for (auto element : simulation.circuit().elements()) {
        draw_element(ctx, element, attributes.at(element.element_id()), simulation);
    }

    // std::ranges::for_each(circuit.elements(), [&](auto element) {
    //     draw_element(
    //         ctx, element.element_type(), attributes.at(element.element_id()),
    //         ranges::views::transform(
    //             element.inputs(),
    //             [&](auto input) { return simulation.input_values.at(input.input_id());
    //             }),
    //         ranges::views::transform(element.outputs(), [&](auto output) {
    //             return simulation.output_values.at(output.output_id());
    //         }));
    // });
}

}  // namespace logicsim
