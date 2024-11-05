// The data stored is zipped json data.
//
// Load clipboard data in python with:
//
//     import json; import gzip; import base64;
//
//     json.loads(gzip.decompress(base64.b64decode(s)))
//
// Load savefiles in python with:
//
//     import json; import gzip;
//
//     json.loads(gzip.decompress(open("circuit.ls2", 'rb').read()))
//

#include "core/serialize.h"

#include "core/algorithm/trim_whitespace.h"
#include "core/base64.h"
#include "core/editable_circuit.h"
#include "core/geometry/line.h"
#include "core/geometry/point.h"
#include "core/gzip.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/selection.h"
#include "core/serialize_detail.h"
#include "core/validate_definition_decoration.h"
#include "core/validate_definition_logicitem.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/move_delta.h"
#include "core/vocabulary/placed_decoration.h"
#include "core/vocabulary/placed_logicitem.h"
#include "core/vocabulary/save_format.h"

#include <fmt/core.h>

namespace logicsim {

namespace serialize {

namespace {

[[nodiscard]] auto to_line(const SerializedLine& obj,
                           move_delta_t delta = {}) -> std::optional<line_t> {
    if (!is_orthogonal_line(obj.p0, obj.p1)) [[unlikely]] {
        return std::nullopt;
    }

    if (!is_representable(line_t {obj.p0, obj.p1}, delta.x, delta.y)) {
        return std::nullopt;
    }

    return add_unchecked(line_t {obj.p0, obj.p1}, delta.x, delta.y);
}

[[nodiscard]] auto parse_attr_clock_generator(
    const std::optional<SerializedAttributesClockGenerator>& obj)
    -> std::optional<attributes_clock_generator_t> {
    if (obj.has_value()) {
        static_assert(std::is_same_v<delay_t::period, std::nano>);
        static_assert(std::is_same_v<delay_t::rep, decltype(obj->time_symmetric_ns)>);
        static_assert(std::is_same_v<delay_t::rep, decltype(obj->time_on_ns)>);
        static_assert(std::is_same_v<delay_t::rep, decltype(obj->time_off_ns)>);

        auto limited_name = obj->name;
        if (limited_name.size() > clock_generator_name_max_size) {
            limited_name.resize(clock_generator_name_max_size);
        }

        return attributes_clock_generator_t {
            .name = limited_name,

            .time_symmetric = delay_t {obj->time_symmetric_ns * 1ns},
            .time_on = delay_t {obj->time_on_ns * 1ns},
            .time_off = delay_t {obj->time_off_ns * 1ns},

            .is_symmetric = obj->is_symmetric,
            .show_simulation_controls = obj->show_simulation_controls,
        };
    }

    return std::nullopt;
}

[[nodiscard]] auto to_connection_count(connection_count_t::value_type_rep value)
    -> std::optional<connection_count_t> {
    if (connection_count_t::min().count() <= value &&
        value <= connection_count_t::max().count()) {
        return connection_count_t {value};
    }
    return std::nullopt;
}

[[nodiscard]] auto to_placed_logicitem(const SerializedLogicItem& obj,
                                       move_delta_t delta = {})
    -> std::optional<PlacedLogicItem> {
    // definition
    const auto input_count = to_connection_count(obj.input_count);
    const auto output_count = to_connection_count(obj.output_count);

    if (!input_count || !output_count) {
        return std::nullopt;
    }

    const auto definition = LogicItemDefinition {
        .logicitem_type = obj.logicitem_type,
        .input_count = input_count.value(),
        .output_count = output_count.value(),
        .orientation = obj.orientation,
        .input_inverters = obj.input_inverters,
        .output_inverters = obj.output_inverters,

        .attrs_clock_generator =
            parse_attr_clock_generator(obj.attributes_clock_generator),
    };
    if (!is_valid(definition)) {
        return std::nullopt;
    }

    // position
    if (!is_representable(obj.position, delta.x, delta.y)) {
        return std::nullopt;
    }
    const auto moved_position = add_unchecked(obj.position, delta.x, delta.y);

    // layout
    const auto data = to_layout_calculation_data(definition, moved_position);
    if (!is_representable(data)) {
        return std::nullopt;
    }

    return PlacedLogicItem {
        .definition = definition,
        .position = moved_position,
    };
}

[[nodiscard]] auto parse_rgb_color(const SerializedRgbColor& color) -> color_t {
    return color_t {color.red, color.green, color.blue};
}

[[nodiscard]] auto parse_attr_text_element(
    const std::optional<SerializedAttributesTextElement>& obj)
    -> std::optional<attributes_text_element_t> {
    if (!obj.has_value()) {
        return std::nullopt;
    }

    auto limited_text = obj->text;
    if (limited_text.size() > text_element_text_max_size) {
        limited_text.resize(text_element_text_max_size);
    }

    return attributes_text_element_t {
        .text = limited_text,

        .horizontal_alignment = obj->horizontal_alignment,
        .font_style = obj->font_style,
        .text_color = parse_rgb_color(obj->text_color),
    };
}

[[nodiscard]] auto to_placed_decoration(const SerializedDecoration& obj,
                                        move_delta_t delta = {})
    -> std::optional<PlacedDecoration> {
    // definition
    auto definition = DecorationDefinition {
        .decoration_type = obj.decoration_type,
        .size = obj.size,

        .attrs_text_element = parse_attr_text_element(obj.attributes_text_element),
    };
    if (!is_valid(definition)) {
        return std::nullopt;
    }

    // position
    if (!is_representable(obj.position, delta.x, delta.y)) {
        return std::nullopt;
    }
    const auto moved_position = add_unchecked(obj.position, delta.x, delta.y);

    // layout
    const auto data = to_decoration_layout_data(definition, moved_position);
    if (!is_representable(data)) {
        return std::nullopt;
    }

    return PlacedDecoration {
        .definition = std::move(definition),
        .position = moved_position,
    };
}

[[nodiscard]] auto serialize_attr_clock_generator(const Layout& layout,
                                                  logicitem_id_t logicitem_id)
    -> std::optional<SerializedAttributesClockGenerator> {
    if (layout.logicitems().type(logicitem_id) == LogicItemType::clock_generator) {
        const auto& attr = layout.logicitems().attrs_clock_generator(logicitem_id);

        static_assert(std::is_same_v<delay_t::period, std::nano>);

        return SerializedAttributesClockGenerator {
            .name = attr.name,

            .time_symmetric_ns = attr.time_symmetric.count_ns(),
            .time_on_ns = attr.time_on.count_ns(),
            .time_off_ns = attr.time_off.count_ns(),

            .is_symmetric = attr.is_symmetric,
            .show_simulation_controls = attr.show_simulation_controls,
        };
    }

    return std::nullopt;
}

auto add_element(SerializedLayout& data, const Layout& layout,
                 logicitem_id_t logicitem_id) -> void {
    data.logicitems.push_back(SerializedLogicItem {
        .logicitem_type = layout.logicitems().type(logicitem_id),
        .input_count = layout.logicitems().input_count(logicitem_id).count(),
        .output_count = layout.logicitems().output_count(logicitem_id).count(),
        .input_inverters = layout.logicitems().input_inverters(logicitem_id),
        .output_inverters = layout.logicitems().output_inverters(logicitem_id),
        .position = layout.logicitems().position(logicitem_id),
        .orientation = layout.logicitems().orientation(logicitem_id),

        .attributes_clock_generator =
            serialize_attr_clock_generator(layout, logicitem_id),
    });
}

[[nodiscard]] auto serialized_rgb_color(color_t color) -> SerializedRgbColor {
    if (!is_rgb(color)) [[unlikely]] {
        throw std::runtime_error("Cannot serialize color with alpha channel as rgb.");
    }

    return SerializedRgbColor {
        .red = gsl::narrow<uint8_t>(color.r()),
        .green = gsl::narrow<uint8_t>(color.g()),
        .blue = gsl::narrow<uint8_t>(color.b()),
    };
}

[[nodiscard]] auto serialize_attr_text_element(const Layout& layout,
                                               decoration_id_t decoration_id)
    -> std::optional<SerializedAttributesTextElement> {
    if (layout.decorations().type(decoration_id) == DecorationType::text_element) {
        const auto& attr = layout.decorations().attrs_text_element(decoration_id);

        return SerializedAttributesTextElement {
            .text = attr.text,

            .horizontal_alignment = attr.horizontal_alignment,
            .font_style = attr.font_style,
            .text_color = serialized_rgb_color(attr.text_color),
        };
    }

    return std::nullopt;
}

auto add_element(SerializedLayout& data, const Layout& layout,
                 decoration_id_t decoration_id) -> void {
    data.decorations.push_back(SerializedDecoration {
        .decoration_type = layout.decorations().type(decoration_id),
        .position = layout.decorations().position(decoration_id),
        .size = layout.decorations().size(decoration_id),

        .attributes_text_element = serialize_attr_text_element(layout, decoration_id),
    });
}

auto add_element(SerializedLayout& data, const Layout& layout,
                 wire_id_t wire_id) -> void {
    for (const auto& info : layout.wires().segment_tree(wire_id)) {
        data.wire_segments.push_back(SerializedLine {info.line.p0, info.line.p1});
    }
}

[[nodiscard]] auto serialize_view_point(const ViewPoint& view_point)
    -> SerializedViewPoint {
    return SerializedViewPoint {
        .device_scale = view_point.device_scale,
        .grid_offset_x = view_point.offset.x,
        .grid_offset_y = view_point.offset.y,
    };
}

[[nodiscard]] auto parse_view_point(const SerializedViewPoint& serialized) -> ViewPoint {
    return ViewPoint {
        .offset = point_fine_t {serialized.grid_offset_x, serialized.grid_offset_y},
        .device_scale = serialized.device_scale > 0 ? serialized.device_scale
                                                    : ViewConfig().device_scale(),
    };
}

[[nodiscard]] auto serialize_simulation_config(const SimulationConfig config)
    -> SerializedSimulationConfig {
    const auto rate = config.simulation_time_rate.rate_per_second;

    return SerializedSimulationConfig {
        .simulation_time_rate_ns = rate.count_ns(),
        .use_wire_delay = config.use_wire_delay,
    };
};

[[nodiscard]] auto parse_simulation_config(const SerializedSimulationConfig& config)
    -> SimulationConfig {
    using namespace std::chrono_literals;

    const auto rate_stored = time_rate_t {config.simulation_time_rate_ns * 1ns};

    static_assert(std::is_same_v<decltype(rate_stored.rate_per_second)::rep,
                                 decltype(config.simulation_time_rate_ns)>);

    // const auto rate = std::clamp(rate_stored, time_rate_t {1us}, time_rate_t
    // {10s});

    return SimulationConfig {
        .simulation_time_rate = rate_stored,
        .use_wire_delay = config.use_wire_delay,
    };
}

[[nodiscard]] auto serialize_to_format(const SerializedLayout& data,
                                       SaveFormat format) -> std::string {
    switch (format) {
        using enum SaveFormat;

        case base64_gzip:
            return base64_encode(gzip_compress(json_dumps(data)));
        case gzip:
            return gzip_compress(json_dumps(data));
        case json:
            return json_dumps(data);
    }
    std::terminate();
}

[[nodiscard]] auto get_serialized_layout(const SerializeConfig& config)
    -> SerializedLayout {
    auto data = serialize::SerializedLayout {};

    if (config.view_point) {
        data.view_point = serialize::serialize_view_point(*config.view_point);
    }
    if (config.simulation_config) {
        data.simulation_config =
            serialize::serialize_simulation_config(*config.simulation_config);
    }
    if (config.save_position) {
        data.save_position = *config.save_position;
    }

    return data;
}

}  // namespace

}  // namespace serialize

[[nodiscard]] auto SerializeConfig::format() const -> std::string {
    return fmt::format(
        "SerializationConfig(\n"      //
        "  save_format = {}\n"        //
        "  view_config = {}\n"        //
        "  simulation_config = {}\n"  //
        "  save_position = {}\n"      //
        ")",                          //
        save_format, view_point, simulation_config, save_position);
}

[[nodiscard]] auto serialize_all(const Layout& layout,
                                 const SerializeConfig& config) -> std::string {
    if (!all_normal_display_state(layout)) {
        throw std::runtime_error("all items must have display state normal");
    }

    auto data = serialize::get_serialized_layout(config);

    for (const auto logicitem_id : logicitem_ids(layout)) {
        serialize::add_element(data, layout, logicitem_id);
    }
    for (const auto decoration_id : decoration_ids(layout)) {
        serialize::add_element(data, layout, decoration_id);
    }
    for (const auto wire_id : inserted_wire_ids(layout)) {
        serialize::add_element(data, layout, wire_id);
    }

    return serialize::serialize_to_format(data, config.save_format);
}

[[nodiscard]] auto serialize_selected(const Layout& layout, const Selection& selection,
                                      const SerializeConfig& config) -> std::string {
    if (!all_normal_display_state(selection, layout)) {
        throw std::runtime_error("all selected items must have display state normal");
    }

    auto data = serialize::get_serialized_layout(config);

    for (const auto logicitem_id : selection.selected_logicitems()) {
        serialize::add_element(data, layout, logicitem_id);
    }
    for (const auto decoration_id : selection.selected_decorations()) {
        serialize::add_element(data, layout, decoration_id);
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(layout, segment);

        for (const auto& part : parts) {
            const auto line = to_line(full_line, part);
            data.wire_segments.push_back(serialize::SerializedLine {line.p0, line.p1});
        }
    }

    return serialize::serialize_to_format(data, config.save_format);
}

namespace serialize {

namespace {

auto unserialize_base64_gzip_json(const std::string& binary)
    -> tl::expected<SerializedLayout, LoadError> {
    if (const auto format = guess_save_format(binary); format.has_value()) {
        switch (*format) {
            using enum SaveFormat;

            case base64_gzip:
                return base64_decode(trim(binary))
                    .and_then(gzip_decompress)
                    .and_then(json_loads);
            case gzip:
                return gzip_decompress(binary).and_then(json_loads);
            case json:
                return json_loads(binary);
        }
        std::terminate();
    }
    return tl::unexpected<LoadError> {
        LoadErrorType::unknown_file_format_error,
        "Unknown file format.",
    };
}

auto calculate_move_delta(point_t save_position,
                          std::optional<point_t> load_position) -> move_delta_t {
    if (!load_position) {
        return move_delta_t {0, 0};
    }
    return {int {load_position->x} - int {save_position.x},
            int {load_position->y} - int {save_position.y}};
}

}  // namespace

LoadLayoutResult::LoadLayoutResult(SerializedLayout&& serialize_layout)
    : data_ {std::make_shared<SerializedLayout>(std::move(serialize_layout))} {
    Ensures(data_ != nullptr);
}

auto LoadLayoutResult::add_to(EditableCircuit& editable_circuit,
                              AddParameters parameters) const -> void {
    Expects(data_ != nullptr);
    const auto delta =
        calculate_move_delta(data_->save_position, parameters.load_position);

    // logic items
    for (const auto& item : data_->logicitems) {
        if (const auto data = to_placed_logicitem(item, delta)) {
            editable_circuit.add_logicitem(data->definition, data->position,
                                           parameters.insertion_mode,
                                           parameters.selection_id);
        }
    }

    // decorations
    for (const auto& item : data_->decorations) {
        if (auto data = to_placed_decoration(item, delta)) {
            editable_circuit.add_decoration(std::move(data->definition), data->position,
                                            parameters.insertion_mode,
                                            parameters.selection_id);
        }
    }

    // wire segments
    for (const auto& entry : data_->wire_segments) {
        if (const auto line = to_line(entry, delta)) {
            editable_circuit.add_wire_segment(ordered_line_t {*line},
                                              parameters.insertion_mode,
                                              parameters.selection_id);
        }
    }
}

auto LoadLayoutResult::view_point() const -> ViewPoint {
    Expects(data_ != nullptr);
    return parse_view_point(data_->view_point);
}

auto LoadLayoutResult::simulation_config() const -> SimulationConfig {
    Expects(data_ != nullptr);
    return parse_simulation_config(data_->simulation_config);
}

auto LoadLayoutResult::save_position() const -> point_t {
    Expects(data_ != nullptr);
    return data_->save_position;
}

}  // namespace serialize

auto load_layout(const std::string& binary)
    -> tl::expected<serialize::LoadLayoutResult, LoadError> {
    return serialize::unserialize_base64_gzip_json(binary).transform(
        [](serialize::SerializedLayout&& data) {
            return serialize::LoadLayoutResult {std::move(data)};
        });
}

}  // namespace logicsim
