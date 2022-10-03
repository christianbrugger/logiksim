
#include "render_scene.h"

#include <boost/range.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/adaptors.hpp>
#include <range/v3/iterator/operations.hpp>

#include <ranges>
#include <iostream>


namespace logicsim {

	void render_background(BLContext& ctx) {
		ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
		ctx.fillAll();
	}


	void draw_wire_element(BLContext& ctx, const DrawAttributes& attributes,
		const std::ranges::input_range auto input_values)
	{
		constexpr static double s = 20;
		ctx.setStrokeWidth(2);

		uint32_t color = *std::begin(input_values) ? 0xFFFF0000u : 0xFF000000u;
		ctx.setStrokeStyle(BLRgba32(color));

		const auto& p0 = attributes.line_tree.at(0);
		const auto& p1 = attributes.line_tree.at(1);

		ctx.strokeLine(BLLine(p0[0] * s, p0[1] * s, p1[0] * s, p1[1] * s));
	}

	void draw_standard_element(BLContext& ctx, ElementType type, const DrawAttributes& attributes,
		const std::ranges::input_range auto input_values,
		const std::ranges::input_range auto output_values)
	{
		constexpr static double s = 20;
		ctx.setStrokeWidth(2);


		// draw rect
		double x = attributes.position[0] * s;
		double y = attributes.position[1] * s;
		int height = std::max(std::size(input_values), std::size(output_values));
		BLPath path;
		path.addRect(x, y + -0.5 * s, 2 * s, height * s);

		// draw inputs & outputs
		int input_offset = (height - std::size(input_values)) / 2;
		for (int i = 0;  const auto value : input_values) {
			double y_pin = y + (input_offset + i) * s;
			uint32_t color = value ? 0xFFFF0000u : 0xFF000000u;
			ctx.setStrokeStyle(BLRgba32(color));
			ctx.strokeLine(BLLine(x, y_pin, x - 0.75 * s, y_pin));
			++i;
		}

		int output_offset = (height - std::size(output_values)) / 2;
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


	void draw_element(BLContext& ctx, ElementType type, const DrawAttributes& attributes,
		const std::ranges::input_range auto input_values,
		const std::ranges::input_range auto output_values)
	{
		switch (type) {
		case ElementType::wire:
			draw_wire_element(ctx, attributes, input_values);
			break;
		case ElementType::inverter_element:
		case ElementType::and_element:
		case ElementType::or_element:
		case ElementType::xor_element:
			draw_standard_element(ctx, type, attributes, input_values, output_values);
		default:
			break;
		}
	}

	void render_scene(
		BLContext& ctx, 
		const CircuitGraph& graph, 
		const SimulationResult& simulation, 
		const attribute_vector_t& attributes
	)
	{
		render_background(ctx);

		std::ranges::for_each(graph.elements(), [&](auto element) {
			draw_element(
				ctx,
				graph.get_type(element),
				attributes.at(element),
				std::ranges::views::transform(graph.inputs(element),
					[&](auto input) { return simulation.input_values.at(input); }),
				std::ranges::views::transform(graph.outputs(element),
					[&](auto input) { return simulation.output_values.at(input); })
			);
		});

	}

}
