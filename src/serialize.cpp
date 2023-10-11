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

#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection.h"
#include "file.h"
#include "geometry/layout.h"
#include "geometry/point.h"
#include "gzip.h"
#include "layout.h"
#include "layout_calculation.h"
#include "serialize_detail.h"
#include "validate_definition.h"
#include "vocabulary/element_definition.h"
#include "vocabulary/placed_element.h"
#include "vocabulary/simulation_setting.h"
#include "vocabulary/view_config.h"

#include <optional>

namespace logicsim::serialize {

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
    // element type
    if (!is_logic_item(obj.element_type)) {
        return std::nullopt;
    }

    // definition
    const auto input_count = to_connection_count(obj.input_count);
    const auto output_count = to_connection_count(obj.output_count);

    if (!input_count || !output_count) {
        return std::nullopt;
    }

    const auto definition = ElementDefinition {
        .element_type = obj.element_type,
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

auto serialize_attr_clock_generator(const layout::ConstElement element)
    -> std::optional<SerializedAttributesClockGenerator> {
    if (element.element_type() == ElementType::clock_generator) {
        const auto& attr = element.attrs_clock_generator();

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

auto add_element(SerializedLayout& data, const layout::ConstElement element) -> void {
    if (element.is_logic_item()) {
        data.logic_items.push_back(SerializedLogicItem {
            .element_type = element.element_type(),
            .input_count = element.input_count().count(),
            .output_count = element.output_count().count(),
            .input_inverters = element.input_inverters(),
            .output_inverters = element.output_inverters(),
            .position = element.position(),
            .orientation = element.orientation(),

            .attributes_clock_generator = serialize_attr_clock_generator(element),
        });
    }

    else if (element.is_wire()) {
        for (const auto& info : element.segment_tree().segment_infos()) {
            data.wire_segments.push_back(SerializedLine {info.line.p0, info.line.p1});
        }
    }
}

auto serialize_view_config(const ViewConfig& view_config) -> SerializedViewConfig {
    return SerializedViewConfig {
        .device_scale = view_config.device_scale(),
        .grid_offset_x = view_config.offset().x,
        .grid_offset_y = view_config.offset().y,
    };
}

auto apply_view_config(const SerializedViewConfig& serialized, ViewConfig& view_config)
    -> void {
    // device scale
    view_config.set_device_scale(serialized.device_scale > 0
                                     ? serialized.device_scale
                                     : ViewConfig().device_scale());

    // offsets
    view_config.set_offset(point_fine_t {
        serialized.grid_offset_x,
        serialized.grid_offset_y,
    });
}

auto serialize_simulation_settings(const SimulationSettings settings)
    -> SerializedSimulationSettings {
    const auto rate = settings.simulation_time_rate.rate_per_second;

    return SerializedSimulationSettings {
        .simulation_time_rate_ns = rate.count_ns(),
        .use_wire_delay = settings.use_wire_delay,
    };
};

auto unserialize_simulation_settings(const SerializedSimulationSettings& settings)
    -> SimulationSettings {
    using namespace std::chrono_literals;

    const auto rate_stored = time_rate_t {settings.simulation_time_rate_ns * 1ns};

    static_assert(std::is_same_v<decltype(rate_stored.rate_per_second)::rep,
                                 decltype(settings.simulation_time_rate_ns)>);

    // const auto rate = std::clamp(rate_stored, time_rate_t {1us}, time_rate_t {10s});

    return SimulationSettings {
        .simulation_time_rate = rate_stored,
        .use_wire_delay = settings.use_wire_delay,
    };
}

}  // namespace logicsim::serialize

namespace logicsim {

auto serialize_inserted(const Layout& layout, const ViewConfig* view_config,
                        const SimulationSettings* simulation_settings) -> std::string {
    auto data = serialize::SerializedLayout {};

    if (view_config != nullptr) {
        data.view_config = serialize::serialize_view_config(*view_config);
    }
    if (simulation_settings != nullptr) {
        data.simulation_settings =
            serialize::serialize_simulation_settings(*simulation_settings);
    }

    for (const auto element : layout.elements()) {
        if (element.is_inserted()) {
            serialize::add_element(data, element);
        }
    }

    return gzip_compress(json_dumps(data));
}

auto serialize_selected(const Layout& layout, const Selection& selection,
                        point_t save_position) -> std::string {
    auto data = serialize::SerializedLayout {
        .save_position = save_position,
    };

    for (const auto element_id : selection.selected_logic_items()) {
        serialize::add_element(data, layout.element(element_id));
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        for (const auto& part : parts) {
            const auto line =
                get_line(layout, segment_part_t {.segment = segment, .part = part});
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

LoadLayoutResult::LoadLayoutResult(SerializedLayout&& layout)
    : data_ {std::make_unique<SerializedLayout>(std::move(layout))} {}

LoadLayoutResult::LoadLayoutResult(LoadLayoutResult&&) = default;

auto LoadLayoutResult::operator=(LoadLayoutResult&&) -> LoadLayoutResult& = default;

LoadLayoutResult::~LoadLayoutResult() = default;

auto LoadLayoutResult::add(EditableCircuit& editable_circuit,
                           InsertionMode insertion_mode,
                           std::optional<point_t> load_position) const
    -> selection_handle_t {
    if (!data_) {
        throw_exception("no layout data");
    }

    auto handle = editable_circuit.get_handle();
    const auto delta = calculate_move_delta(data_->save_position, load_position);

    // logic items
    for (const auto& item : data_->logic_items) {
        if (const auto data = to_placed_element(item, delta)) {
            editable_circuit.add_logic_item(data->definition, data->position,
                                            insertion_mode, handle);
        }
    }

    // wire segments
    for (const auto& entry : data_->wire_segments) {
        if (const auto line = to_line(entry, delta)) {
            editable_circuit.add_line_segment(line.value(), insertion_mode, handle);
        }
    }

    return handle;
}

auto LoadLayoutResult::apply(ViewConfig& view_config) const -> void {
    if (!data_) {
        throw_exception("no layout data");
    }

    apply_view_config(data_->view_config, view_config);
}

auto LoadLayoutResult::simulation_settings() const -> SimulationSettings {
    if (!data_) {
        throw_exception("no layout data");
    }
    return unserialize_simulation_settings(data_->simulation_settings);
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
