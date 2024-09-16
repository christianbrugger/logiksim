#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_CONNECTOR_LABEL_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_CONNECTOR_LABEL_H

#include "layout.h"
#include "layout_info.h"
#include "vocabulary/element_draw_state.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/orientation.h"
#include "vocabulary/text_alignment.h"

#include <gsl/gsl>

#include <array>
#include <string_view>

namespace logicsim {

struct color_t;
struct point_t;

struct Context;

auto get_connector_label_color(ElementDrawState state) -> color_t;

auto connector_horizontal_alignment(orientation_t orientation) -> HTextAlignment;

auto connector_vertical_alignment(orientation_t orientation) -> VTextAlignment;

// TODO: move to vocabulary or remove ?
template <std::size_t size>
using string_array = std::array<std::string_view, size>;

// TODO: format & equality
struct ConnectorLabels {
    // TODO: why GSL span ??
    gsl::span<const std::string_view> input_labels {};
    gsl::span<const std::string_view> output_labels {};
};

auto draw_connector_label(Context& ctx, point_t position, orientation_t orientation,
                          std::string_view label, ElementDrawState state) -> void;

auto draw_connector_labels(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, ConnectorLabels labels,
                           ElementDrawState state) -> void;

template <typename Func>
auto draw_input_connector_labels(Context& ctx, const Layout& layout,
                                 logicitem_id_t logicitem_id, ElementDrawState state,
                                 Func to_input_label) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);

    for (const auto&& info : input_locations_and_id(layout_data)) {
        draw_connector_label(ctx, info.position, info.orientation,
                             to_input_label(info.input_id), state);
    }
}

}  // namespace logicsim

#endif
