#include "render/circuit/render_connector_label.h"

#include "geometry/layout_calculation.h"
#include "render/circuit/alpha_values.h"
#include "render/context.h"
#include "render/primitive/text.h"

namespace logicsim {

namespace defaults {
namespace font {
constexpr static inline auto connector_label_color = defaults::color_black;
constexpr static inline auto connector_label_size = grid_fine_t {0.6};
constexpr static inline auto connector_label_margin = grid_fine_t {0.2};
}  // namespace font
}  // namespace defaults

//
auto get_connector_label_color(ElementDrawState state) -> color_t {
    return with_alpha_runtime(defaults::font::connector_label_color, state);
}

auto connector_horizontal_alignment(orientation_t orientation) -> HTextAlignment {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return HTextAlignment::right;
        case left:
            return HTextAlignment::left;
        case up:
            return HTextAlignment::center;
        case down:
            return HTextAlignment::center;

        case undirected:
            throw std::runtime_error("orientation has no horizontal alignment");
    };

    throw std::runtime_error("unknown orientation type");
}

auto connector_vertical_alignment(orientation_t orientation) -> VTextAlignment {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return VTextAlignment::center;
        case left:
            return VTextAlignment::center;
        case up:
            return VTextAlignment::top;
        case down:
            return VTextAlignment::baseline;

        case undirected:
            throw std::runtime_error("orienation has no vertical alignment");
    };

    throw std::runtime_error("unknown orientation type");
}

auto draw_connector_label(Context& ctx, point_t position, orientation_t orientation,
                          std::string_view label, ElementDrawState state) -> void {
    const auto point = !label.empty() && label.at(0) == '>'
                           ? point_fine_t {position}
                           : connector_point(position, orientation,
                                             -defaults::font::connector_label_margin);

    draw_text(ctx, point, label,
              TextAttributes {
                  .font_size = defaults::font::connector_label_size,
                  .color = get_connector_label_color(state),
                  .horizontal_alignment = connector_horizontal_alignment(orientation),
                  .vertical_alignment = connector_vertical_alignment(orientation),
              });
}

auto draw_connector_labels(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, ConnectorLabels labels,
                           ElementDrawState state) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);

    for (auto info : input_locations_and_id(layout_data)) {
        draw_connector_label(ctx, info.position, info.orientation,
                             labels.input_labels[std::size_t {info.input_id}], state);
    }

    for (auto info : output_locations_and_id(layout_data)) {
        draw_connector_label(ctx, info.position, info.orientation,
                             labels.output_labels[std::size_t {info.output_id}], state);
    }
}

}  // namespace logicsim
