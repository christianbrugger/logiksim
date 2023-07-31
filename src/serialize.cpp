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
#include "layout.h"
#include "timer.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <glaze/glaze.hpp>

#include <fstream>
#include <iostream>

namespace logicsim {

constexpr static inline auto CURRENT_VERSION = 100;

auto gzip_compress(const std::string& input) -> std::string {
    auto output = std::ostringstream {};
    {
        auto filter_stream = boost::iostreams::filtering_ostream {};
        filter_stream.push(boost::iostreams::gzip_compressor());
        filter_stream.push(output);
        filter_stream.write(input.data(), input.size());
        filter_stream.flush();
    }
    return output.str();
}

auto gzip_decompress(const std::string& input) -> std::string {
    auto output = std::ostringstream {};
    {
        auto filter_stream = boost::iostreams::filtering_ostream {};
        filter_stream.push(boost::iostreams::gzip_decompressor());
        filter_stream.push(output);
        filter_stream.write(input.data(), input.size());
        filter_stream.flush();
    }
    return output.str();
}

auto base64_encode(const std::string& data) -> std::string {
    return cppcodec::base64_rfc4648::encode(data);
}

auto base64_decode(const std::string& data) -> std::string {
    try {
        return cppcodec::base64_rfc4648::decode<std::string>(data);
    } catch (cppcodec::parse_error&) {
    }
    return "";
}

}  // namespace logicsim

template <>
struct glz::meta<logicsim::ElementType> {
    using enum logicsim::ElementType;

    static constexpr auto value = enumerate(  //
        "unused", unused,                     //
        "placeholder", placeholder,           //
        "wire", wire,                         //

        "buffer_element", buffer_element,  //
        "and_element", and_element,        //
        "or_element", or_element,          //
        "xor_element", xor_element,        //

        "button", button,                    //
        "clock_generator", clock_generator,  //
        "flipflop_jk", flipflop_jk,          //
        "shift_register", shift_register,    //

        "sub_circuit", sub_circuit  //
    );
};

template <>
struct glz::meta<logicsim::orientation_t> {
    using enum logicsim::orientation_t;

    static constexpr auto value = enumerate(  //
        "right", right,                       //
        "left", left,                         //
        "up", up,                             //
        "down", down,                         //
        "undirected", undirected              //
    );
};

template <>
struct glz::meta<logicsim::grid_t> {
    using T = logicsim::grid_t;

    static constexpr auto value = &T::value;
};

template <>
struct glz::meta<logicsim::point_t> {
    using T = logicsim::point_t;

    static constexpr auto value = array(&T::x, &T::y);
};

namespace logicsim {

struct SerializedLine {
    point_t p0;
    point_t p1;

    struct glaze {
        using T = SerializedLine;
        static constexpr auto value = glz::array(&T::p0, &T::p1);
    };

    auto to_line() const -> std::optional<line_t> {
        if (is_orthogonal(p0, p1)) [[likely]] {
            return line_t {p0, p1};
        }
        return std::nullopt;
    }
};

struct SerializedLogicItem {
    ElementType element_type;
    std::size_t input_count;
    std::size_t output_count;
    logic_small_vector_t input_inverters;
    logic_small_vector_t output_inverters;
    point_t position;
    orientation_t orientation;

    struct glaze {
        using T = SerializedLogicItem;

        static constexpr auto value = glz::object(     //
            "element_type", &T::element_type,          //
            "input_count", &T::input_count,            //
            "output_count", &T::output_count,          //
            "input_inverters", &T::input_inverters,    //
            "output_inverters", &T::output_inverters,  //
            "position", &T::position,                  //
            "orientation", &T::orientation);
    };

    auto to_definition() const -> std::optional<LogicItemDefinition> {
        if (!is_logic_item(element_type)) {
            return std::nullopt;
        }
        if (!is_input_output_count_valid(element_type, input_count, output_count)) {
            return std::nullopt;
        }

        if (input_inverters.size() != input_count) {
            return std::nullopt;
        }
        if (output_inverters.size() != output_count) {
            return std::nullopt;
        }

        if (!is_orientation_valid(element_type, orientation)) {
            return std::nullopt;
        }

        const auto data = layout_calculation_data_t {
            .input_count = input_count,
            .output_count = output_count,
            .position = position,
            .orientation = orientation,
            .element_type = element_type,
        };
        if (!is_representable(data)) {
            return std::nullopt;
        }

        return LogicItemDefinition {
            .element_type = element_type,
            .input_count = input_count,
            .output_count = output_count,
            .orientation = orientation,
            .input_inverters = input_inverters,
            .output_inverters = output_inverters,
        };
    }
};

struct SerializedLayout {
    int version {CURRENT_VERSION};

    std::vector<SerializedLogicItem> logic_items;
    std::vector<SerializedLine> wire_segments;

    struct glaze {
        using T = SerializedLayout;

        static constexpr auto value = glz::object(  //
            "version", &T::version,                 //
            "logic_items", &T::logic_items,         //
            "wire_segments", &T::wire_segments);
    };
};

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

auto serialize_inserted(const Layout& layout) -> std::string {
    auto data = SerializedLayout {};

    for (const auto element : layout.elements()) {
        if (element.is_inserted()) {
            add_element(data, element);
        }
    }

    return gzip_compress(glz::write_json(data));
}

auto serialize_selected(const Layout& layout, const Selection& selection) -> std::string {
    auto data = SerializedLayout {};

    for (const auto element_id : selection.selected_logic_items()) {
        add_element(data, layout.element(element_id));
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        for (const auto& part : parts) {
            const auto line
                = get_line(layout, segment_part_t {.segment = segment, .part = part});
            data.wire_segments.push_back(SerializedLine {line.p0, line.p1});
        }
    }

    return gzip_compress(glz::write_json(data));
}

auto save_layout(const Layout& layout, std::string filename) -> void {
    const auto data = serialize_inserted(layout);

    auto file = std::ofstream(filename, std::ios::binary);
    file.write(data.data(), data.size());
    file.close();
}

auto unserialize_data(const std::string& binary) -> std::optional<SerializedLayout> {
    // unip
    const auto json_text = gzip_decompress(binary);
    if (json_text.empty()) {
        return std::nullopt;
    }

    // peak version
    auto version = glz::get_as_json<int, "/version">(json_text);
    if (!version.has_value()) {
        try {
            print(glz::format_error(version.error(), json_text));
        } catch (std::runtime_error&) {
            print("error parsing json");
        }
        return std::nullopt;
    }
    if (version.value() != CURRENT_VERSION) {
        print("Error wrong version. Expected", CURRENT_VERSION, "got", version.value());
        return std::nullopt;
    }

    // parse json
    auto result = std::optional<SerializedLayout> {SerializedLayout {}};
    const auto error = glz::read_json<SerializedLayout>(result.value(), json_text);
    if (error) {
        try {
            print(glz::format_error(error, json_text));
        } catch (std::runtime_error&) {
            print("error parsing json");
        }
        return std::nullopt;
    }

    return result;
}

auto add_layout(const std::string& binary, EditableCircuit& editable_circuit,
                InsertionMode insertion_mode) -> selection_handle_t {
    const auto data = unserialize_data(binary);
    if (!data) {
        return selection_handle_t {};
    }

    auto handle = editable_circuit.create_selection();

    // logic items
    for (const auto& item : data.value().logic_items) {
        if (const auto definition = item.to_definition()) {
            editable_circuit.add_logic_item(definition.value(), item.position,
                                            insertion_mode, handle);
        }
    }

    // wire segments
    for (const auto& entry : data.value().wire_segments) {
        if (const auto line = entry.to_line()) {
            editable_circuit.add_line_segment(line.value(), insertion_mode, handle);
        }
    }

    return handle;
}

}  // namespace logicsim
