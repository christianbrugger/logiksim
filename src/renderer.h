#ifndef LOGIKSIM_RENDER_H
#define LOGIKSIM_RENDER_H

#include "format.h"
#include "layout.h"
#include "render_generic.h"
#include "scene.h"
#include "schematic.h"
#include "simulation.h"

#include <blend2d.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <array>
#include <cstdint>
#include <variant>

namespace logicsim {

class Selection;

//
// primitives
//

auto stroke_offset(int stroke_width) -> double;
auto stroke_offset(const RenderSettings& settings) -> double;

auto render_input_marker(BLContext& ctx, point_t point, color_t color,
                         orientation_t orientation, double size,
                         const RenderSettings& settings) -> void;

//
// Temporary Circuit Items
//

// TODO remove selected
auto draw_logic_item(BLContext& ctx, layout::ConstElement element, bool selected,
                     const RenderSettings& settings) -> void;

auto draw_segment_tree(BLContext& ctx, layout::ConstElement element,
                       const RenderSettings& settings) -> void;

auto draw_element_shadow(BLContext& ctx, layout::ConstElement element, bool selected,
                         const RenderSettings& settings) -> void;

auto draw_wire_selected_parts_shadow(BLContext& ctx, ordered_line_t line,
                                     std::span<const part_t> parts,
                                     const RenderSettings& settings) -> void;

auto draw_wire_colliding_shadow(BLContext& ctx, const SegmentTree& segment_tree,
                                const RenderSettings& settings) -> void;

//
// scenes
//

class EditableCircuit;

auto render_background(BLContext& ctx, const RenderSettings& settings = {}) -> void;

using selection_mask_t = boost::container::vector<bool>;
using visibility_mask_t = boost::container::vector<bool>;

auto create_selection_mask(const Layout& layout, const Selection& selection)
    -> selection_mask_t;

// TODO better grouping, group RenderSettings and BLContext, Circuit struct
struct render_args_t {
    const Layout& layout;
    const Schematic* schematic {nullptr};
    const Simulation* simulation {nullptr};
    const Selection* selection {nullptr};
    const RenderSettings& settings {};
};

auto render_circuit(BLContext& ctx, render_args_t args) -> void;

auto render_circuit(render_args_t args, int width, int height, std::string filename)
    -> void;

}  // namespace logicsim

#endif
