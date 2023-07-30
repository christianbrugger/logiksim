#include "serialize.h"

#include "timer.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
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
struct glz::meta<logicsim::Layout> {
    using T = logicsim::Layout;

    static constexpr auto value = object(          //
        "element_types", &T::element_types_,       //
        "input_counts", &T::input_counts_,         //
        "output_counts", &T::output_counts_,       //
        "input_inverters", &T::input_inverters_,   //
        "input_inverters", &T::output_inverters_,  //
        "positions", &T::positions_,               //
        "orientation", &T::orientation_);
};

namespace logicsim {

auto compress(const std::string& input) -> std::string {
    std::stringstream output;
    {
        boost::iostreams::filtering_ostream filter_stream;
        filter_stream.push(boost::iostreams::gzip_compressor());
        filter_stream.push(output);
        filter_stream.write(input.c_str(), input.size());
    }
    return output.str();
}

auto decompress(const std::string& input) -> std::string {
    std::stringstream output;
    {
        boost::iostreams::filtering_ostream filter_stream;
        filter_stream.push(boost::iostreams::gzip_decompressor());
        filter_stream.push(output);
        filter_stream.write(input.c_str(), input.size());
    }
    return output.str();
}

auto serialize_json(const Layout& layout) -> void {
    auto buffer = std::string {};
    auto compressed = std::string {};

    {
        const auto t = Timer("Convert to arrays", Timer::Unit::ms, 3);
        glz::write_json(layout, buffer);
    }
    {
        const auto t = Timer("Compress", Timer::Unit::ms, 3);
        compressed = compress(buffer);
    }

    {
        auto file = std::ofstream("data.json.gz");
        file.write(compressed.c_str(), compressed.size());
        file.close();
    }

    print("Compressed size", compressed.size());
}

}  // namespace logicsim
