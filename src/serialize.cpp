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
#include "geometry.h"
#include "layout.h"
#include "scene.h"
#include "serialize_detail.h"
#include "timer.h"

namespace logicsim::serialize {

struct move_delta_t {
    int x;
    int y;
};

auto to_line(const SerializedLine& obj, move_delta_t delta = {})
    -> std::optional<line_t> {
    if (!is_orthogonal(obj.p0, obj.p1)) [[unlikely]] {
        return std::nullopt;
    }

    if (!is_representable(line_t {obj.p0, obj.p1}, delta.x, delta.y)) {
        return std::nullopt;
    }

    return add_unchecked(line_t {obj.p0, obj.p1}, delta.x, delta.y);
}

auto to_definition(const SerializedLogicItem& obj, move_delta_t delta = {})
    -> std::optional<LogicItemData> {
    // element type
    if (!is_logic_item(obj.element_type)) {
        return std::nullopt;
    }
    // input & output counts
    if (!is_input_output_count_valid(obj.element_type, obj.input_count,
                                     obj.output_count)) {
        return std::nullopt;
    }

    // inverters
    if (obj.input_inverters.size() != obj.input_count) {
        return std::nullopt;
    }
    if (obj.output_inverters.size() != obj.output_count) {
        return std::nullopt;
    }

    // orientation
    if (!is_orientation_valid(obj.element_type, obj.orientation)) {
        return std::nullopt;
    }

    // position
    if (!is_representable(obj.position, delta.x, delta.y)) {
        return std::nullopt;
    }
    const auto moved_position = add_unchecked(obj.position, delta.x, delta.y);

    const auto data = layout_calculation_data_t {
        .input_count = obj.input_count,
        .output_count = obj.output_count,
        .position = moved_position,
        .orientation = obj.orientation,
        .element_type = obj.element_type,
    };
    if (!is_representable(data)) {
        return std::nullopt;
    }

    return LogicItemData {
            .definition = LogicItemDefinition {
                .element_type = obj.element_type,
                .input_count = obj.input_count,
                .output_count = obj.output_count,
                .orientation = obj.orientation,
                .input_inverters = obj.input_inverters,
                .output_inverters = obj.output_inverters,
            },
            .position = moved_position,
        };
}

auto add_element(SerializedLayout& data, const layout::ConstElement element) -> void {
    if (element.is_logic_item()) {
        data.logic_items.push_back(SerializedLogicItem {
            .element_type = element.element_type(),
            .input_count = element.input_count(),
            .output_count = element.output_count(),
            .input_inverters = element.input_inverters(),
            .output_inverters = element.output_inverters(),
            .position = element.position(),
            .orientation = element.orientation(),
        });
    }

    else if (element.is_wire()) {
        for (const auto& info : element.segment_tree().segment_infos()) {
            data.wire_segments.push_back(SerializedLine {info.line.p0, info.line.p1});
        }
    }
}

}  // namespace logicsim::serialize

namespace logicsim {

auto serialize_inserted(const Layout& layout) -> std::string {
    auto data = serialize::SerializedLayout {};

    for (const auto element : layout.elements()) {
        if (element.is_inserted()) {
            serialize::add_element(data, element);
        }
    }

    return gzip_compress(json_dumps(data));
}

auto serialize_selected(const Layout& layout, const Selection& selection,
                        point_t save_position) -> std::string {
    auto data = serialize::SerializedLayout {.save_position = save_position};

    for (const auto element_id : selection.selected_logic_items()) {
        serialize::add_element(data, layout.element(element_id));
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        for (const auto& part : parts) {
            const auto line
                = get_line(layout, segment_part_t {.segment = segment, .part = part});
            data.wire_segments.push_back(serialize::SerializedLine {line.p0, line.p1});
        }
    }

    return gzip_compress(json_dumps(data));
}

auto save_layout(const Layout& layout, std::string filename) -> void {
    const auto binary = serialize_inserted(layout);
    save_file(filename, binary);
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
    return {load_position->x.value - save_position.x.value,
            load_position->y.value - save_position.y.value};
}

}  // namespace serialize

auto add_layout(const std::string& binary, EditableCircuit& editable_circuit,
                InsertionMode insertion_mode, std::optional<point_t> load_position)
    -> selection_handle_t {
    const auto data = serialize::unserialize_data(binary);
    if (!data) {
        return selection_handle_t {};
    }

    auto handle = editable_circuit.create_selection();
    const auto delta
        = serialize::calculate_move_delta(data->save_position, load_position);

    // logic items
    for (const auto& item : data->logic_items) {
        if (const auto definition = to_definition(item, delta)) {
            editable_circuit.add_logic_item(definition->definition, definition->position,
                                            insertion_mode, handle);
        }
    }

    // wire segments
    for (const auto& entry : data->wire_segments) {
        if (const auto line = to_line(entry, delta)) {
            editable_circuit.add_line_segment(line.value(), insertion_mode, handle);
        }
    }

    return handle;
}

}  // namespace logicsim
