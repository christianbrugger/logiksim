#include "gzip.h"

#include "logging.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <fmt/core.h>
#include <gsl/gsl>

#include <sstream>

namespace logicsim {

auto gzip_compress(std::string_view input) -> std::string {
    auto output = std::ostringstream {};
    {
        auto filter_stream = boost::iostreams::filtering_ostream {};
        filter_stream.push(boost::iostreams::gzip_compressor());
        filter_stream.push(output);
        filter_stream.write(input.data(), gsl::narrow<std::streamsize>(input.size()));
        filter_stream.flush();
        filter_stream.reset();
    }
    return output.str();
}

auto gzip_decompress(std::string_view input) -> tl::expected<std::string, LoadError> {
    auto output = std::ostringstream {};

    try {
        auto params = boost::iostreams::gzip_params {};

        auto filter_stream = boost::iostreams::filtering_ostream {};
        filter_stream.push(boost::iostreams::gzip_decompressor());
        filter_stream.push(output);
        filter_stream.write(input.data(), gsl::narrow<std::streamsize>(input.size()));
        filter_stream.flush();
        filter_stream.reset();
    } catch (const boost::iostreams::gzip_error& error) {
        return tl::unexpected<LoadError> {
            LoadErrorType::gzip_decompress_error,
            fmt::format("{}. Gzip error code {}. Zlib error code {}.", error.what(),
                        error.error(), error.zlib_error_code()),
        };
    }

    return output.str();
}

}  // namespace logicsim
