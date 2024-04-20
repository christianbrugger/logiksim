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
//     json.loads(gzip.decompress(open("data.json.gz", 'rb').read()))
//

#include "serialize.h"

#include "editable_circuit.h"
#include "file.h"
#include "geometry/line.h"
#include "geometry/point.h"
#include "gzip.h"
#include "layout.h"
#include "layout_info.h"
#include "selection.h"
#include "serialize_detail.h"
#include "validate_definition.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/logicitem_definition.h"
#include "vocabulary/placed_element.h"
#include "vocabulary/simulation_config.h"
#include "vocabulary/view_config.h"

#include <optional>

namespace logicsim {

namespace serialize {
struct move_delta_t {
    int x;
    int y;
};

auto to_line(const SerializedLine& obj, move_delta_t delta = {})
    -> std::optional<line_t> {
    if (!is_orthogonal_line(obj.p0, obj.p1)) [[unlikely]] {
        return std::nullopt;
    }

    if (!is_representable(line_t {obj.p0, obj.p1}, delta.x, delta.y)) {
        return std::nullopt;
    }

    return add_unchecked(line_t {obj.p0, obj.p1}, delta.x, delta.y);
}

auto parse_attr_clock_generator(
    const std::optional<SerializedAttributesClockGenerator>& obj)
    -> std::optional<attributes_clock_generator_t> {
    if (obj.has_value()) {
        static_assert(std::is_same_v<delay_t::period, std::nano>);
        static_assert(std::is_same_v<delay_t::rep, decltype(obj->time_symmetric_ns)>);
        static_assert(std::is_same_v<delay_t::rep, decltype(obj->time_on_ns)>);
        static_assert(std::is_same_v<delay_t::rep, decltype(obj->time_off_ns)>);

        auto limited_name = obj->name;
        if (limited_name.size() > name_max_size) {
            limited_name.resize(name_max_size);
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

namespace {
auto to_connection_count(connection_count_t::value_type_rep value)
    -> std::optional<connection_count_t> {
    if (connection_count_t::min().count() <= value &&
        value <= connection_count_t::max().count()) {
        return connection_count_t {value};
    }
    return std::nullopt;
}
}  // namespace

auto to_placed_element(const SerializedLogicItem& obj, move_delta_t delta = {})
    -> std::optional<PlacedElement> {
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

    return PlacedElement {
        .definition = definition,
        .position = moved_position,
    };
}

auto serialize_attr_clock_generator(const Layout& layout, logicitem_id_t logicitem_id)
    -> std::optional<SerializedAttributesClockGenerator> {
    if (layout.logic_items().type(logicitem_id) == LogicItemType::clock_generator) {
        const auto& attr = layout.logic_items().attrs_clock_generator(logicitem_id);

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
    data.logic_items.push_back(SerializedLogicItem {
        .logicitem_type = layout.logic_items().type(logicitem_id),
        .input_count = layout.logic_items().input_count(logicitem_id).count(),
        .output_count = layout.logic_items().output_count(logicitem_id).count(),
        .input_inverters = layout.logic_items().input_inverters(logicitem_id),
        .output_inverters = layout.logic_items().output_inverters(logicitem_id),
        .position = layout.logic_items().position(logicitem_id),
        .orientation = layout.logic_items().orientation(logicitem_id),

        .attributes_clock_generator =
            serialize_attr_clock_generator(layout, logicitem_id),
    });
}

auto add_element(SerializedLayout& data, const Layout& layout, wire_id_t wire_id)
    -> void {
    for (const auto& info : layout.wires().segment_tree(wire_id)) {
        data.wire_segments.push_back(SerializedLine {info.line.p0, info.line.p1});
    }
}

auto serialize_view_point(const ViewPoint& view_point) -> SerializedViewPoint {
    return SerializedViewPoint {
        .device_scale = view_point.device_scale,
        .grid_offset_x = view_point.offset.x,
        .grid_offset_y = view_point.offset.y,
    };
}

auto parse_view_point(const SerializedViewPoint& serialized) -> ViewPoint {
    return ViewPoint {
        .offset = point_fine_t {serialized.grid_offset_x, serialized.grid_offset_y},
        .device_scale = serialized.device_scale > 0 ? serialized.device_scale
                                                    : ViewConfig().device_scale(),
    };
}

auto serialize_simulation_config(const SimulationConfig config)
    -> SerializedSimulationConfig {
    const auto rate = config.simulation_time_rate.rate_per_second;

    return SerializedSimulationConfig {
        .simulation_time_rate_ns = rate.count_ns(),
        .use_wire_delay = config.use_wire_delay,
    };
};

auto parse_simulation_config(const SerializedSimulationConfig& config)
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

}  // namespace serialize

auto serialize_all(const Layout& layout, std::optional<ViewPoint> view_point,
                   std::optional<SimulationConfig> simulation_config) -> std::string {
    if (!all_normal_display_state(layout)) {
        throw std::runtime_error("all items must have display state normal");
    }

    auto data = serialize::SerializedLayout {};

    if (view_point) {
        data.view_point = serialize::serialize_view_point(*view_point);
    }
    if (simulation_config) {
        data.simulation_config =
            serialize::serialize_simulation_config(*simulation_config);
    }

    for (const auto logicitem_id : logicitem_ids(layout)) {
        serialize::add_element(data, layout, logicitem_id);
    }
    for (const auto wire_id : inserted_wire_ids(layout)) {
        serialize::add_element(data, layout, wire_id);
    }

    return gzip_compress(json_dumps(data));
}

auto serialize_selected(const Layout& layout, const Selection& selection,
                        point_t save_position) -> std::string {
    if (!all_normal_display_state(selection, layout)) {
        throw std::runtime_error("all selected items must have display state normal");
    }

    auto data = serialize::SerializedLayout {
        .save_position = save_position,
    };

    for (const auto logicitem_id : selection.selected_logic_items()) {
        serialize::add_element(data, layout, logicitem_id);
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(layout, segment);

        for (const auto& part : parts) {
            const auto line = to_line(full_line, part);
            data.wire_segments.push_back(serialize::SerializedLine {line.p0, line.p1});
        }
    }

    return gzip_compress(json_dumps(data));
}

namespace serialize {

auto unserialize_data(const std::string& binary) -> std::optional<SerializedLayout> {
    // unip
    const auto json_text = gzip_decompress(binary);
    if (json_text.empty()) {
        return std::nullopt;
    }

    return json_loads(json_text);
}

auto calculate_move_delta(point_t save_position, std::optional<point_t> load_position)
    -> move_delta_t {
    if (!load_position) {
        return move_delta_t {0, 0};
    }
    return {int {load_position->x} - int {save_position.x},
            int {load_position->y} - int {save_position.y}};
}

LoadLayoutResult::LoadLayoutResult(SerializedLayout&& serialize_layout)
    : data_ {std::make_shared<SerializedLayout>(std::move(serialize_layout))} {
    Ensures(data_ != nullptr);
}

auto LoadLayoutResult::add(EditableCircuit& editable_circuit,
                           AddParameters parameters) const -> void {
    Expects(data_ != nullptr);
    const auto delta =
        calculate_move_delta(data_->save_position, parameters.load_position);

    // logic items
    for (const auto& item : data_->logic_items) {
        if (const auto data = to_placed_element(item, delta)) {
            editable_circuit.add_logicitem(data->definition, data->position,
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

}  // namespace serialize

auto load_layout(const std::string& binary)
    -> std::optional<serialize::LoadLayoutResult> {
    auto data = serialize::unserialize_data(binary);
    if (!data) {
        return std::nullopt;
    }
    return std::optional<serialize::LoadLayoutResult> {std::move(data.value())};
}

}  // namespace logicsim
