#include "gzip.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <sstream>

namespace logicsim {

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

}  // namespace logicsim
