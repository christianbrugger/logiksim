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

#include "editable_circuit\selection.h"
#include "layout.h"
#include "timer.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <glaze/glaze.hpp>

#include <fstream>
#include <iostream>

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

template <>
struct glz::meta<logicsim::ordered_line_t> {
    using T = logicsim::ordered_line_t;

    static constexpr auto value = array(&T::p0, &T::p1);
    // static constexpr auto value = object("p0", &T::p0, "p1", &T::p1);
};

namespace logicsim {

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
            "positions", &T::position,                 //
            "orientation", &T::orientation);
    };
};

struct SerializedLayout {
    int version {100};

    std::vector<SerializedLogicItem> logic_items;
    std::vector<ordered_line_t> wire_segments;

    struct glaze {
        using T = SerializedLayout;

        static constexpr auto value = glz::object(  //
            "version", &T::version,                 //
            "logic_items", &T::logic_items,         //
            "wire_segments", &T::wire_segments);
    };
};

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
    return cppcodec::base64_rfc4648::decode<std::string>(data);
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
            data.wire_segments.push_back(info.line);
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
            data.wire_segments.push_back(line);
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

}  // namespace logicsim
