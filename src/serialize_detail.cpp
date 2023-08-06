#include "serialize_detail.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <glaze/glaze.hpp>

#include <fstream>
#include <ostream>

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

        "button", button,                          //
        "clock_generator", clock_generator,        //
        "flipflop_jk", flipflop_jk,                //
        "shift_register", shift_register,          //
        "latch_d", latch_d,                        //
        "flipflop_d", flipflop_d,                  //
        "flipflop_master_slave_d", flipflop_ms_d,  //

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

using logicsim::serialize::SerializedLine;

template <>
struct glz::meta<SerializedLine> {
    using T = SerializedLine;
    static constexpr auto value = glz::array(&T::p0, &T::p1);
};

using logicsim::serialize::SerializedLogicItem;

template <>
struct glz::meta<SerializedLogicItem> {
    using T = SerializedLogicItem;

    static constexpr auto value = glz::object(  //
        "element_type", &T::element_type,       //
        "input_count", &T::input_count,         //

        "output_count", &T::output_count,          //
        "input_inverters", &T::input_inverters,    //
        "output_inverters", &T::output_inverters,  //

        "position", &T::position,  //
        "orientation", &T::orientation);
};

using logicsim::serialize::SerializedLayout;

template <>
struct glz::meta<SerializedLayout> {
    using T = SerializedLayout;

    static constexpr auto value = glz::object(  //
        "version", &T::version,                 //
        "save_position", &T::save_position,     //
        "logic_items", &T::logic_items,         //
        "wire_segments", &T::wire_segments);
};

namespace logicsim {

auto json_dumps(const serialize::SerializedLayout& data) -> std::string {
    return glz::write_json(data);
}

auto json_loads(std::string text) -> std::optional<serialize::SerializedLayout> {
    auto version = glz::get_as_json<int, "/version">(text);
    if (!version.has_value()) {
        try {
            print(glz::format_error(version.error(), text));
        } catch (std::runtime_error&) {
            print("error parsing json");
        }
        return std::nullopt;
    }
    if (version.value() != serialize::CURRENT_VERSION) {
        print("Error wrong version. Expected", serialize::CURRENT_VERSION, "got",
              version.value());
        return std::nullopt;
    }

    auto result = std::optional<SerializedLayout> {SerializedLayout {}};
    const auto error = glz::read_json<SerializedLayout>(result.value(), text);
    if (error) {
        try {
            print(glz::format_error(error, text));
        } catch (std::runtime_error&) {
            print("error parsing json");
        }
        return std::nullopt;
    }

    return result;
}

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

auto save_file(std::string filename, std::string binary) -> void {
    auto file = std::ofstream(filename, std::ios::binary);
    file.write(binary.data(), binary.size());
    file.close();
}

}  // namespace logicsim