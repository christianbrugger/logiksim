#include "gzip.h"

#include "logging.h"
#include "timer.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <fmt/core.h>
#include <gsl/gsl>
#include <zlib.h>

#include <sstream>

namespace logicsim {

namespace {
[[nodiscard]] auto format_zlib_status_code(int code) -> std::string {
    switch (code) {
        case Z_OK:
            return "Z_OK";
        case Z_STREAM_END:
            return "Z_STREAM_END";
        case Z_NEED_DICT:
            return "Z_NEED_DICT";
        case Z_ERRNO:
            return "Z_ERRNO";
        case Z_STREAM_ERROR:
            return "Z_STREAM_ERROR";
        case Z_DATA_ERROR:
            return "Z_DATA_ERROR";
        case Z_MEM_ERROR:
            return "Z_MEM_ERROR";
        case Z_BUF_ERROR:
            return "Z_BUF_ERROR";
        case Z_VERSION_ERROR:
            return "Z_VERSION_ERROR";
        default:
            return fmt::format("Unknown Zlib Status Code ({})", code);
    }
    std::terminate();
}
}  // namespace

auto gzip_compress(std::string_view input) -> std::string {
    const auto t = Timer {"gzip_compress"};

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

auto gzip_decompress(std::string_view data) -> tl::expected<std::string, LoadError> {
    const auto t = Timer {"gzip_decompress"};

    // TODO make sure inflateEnd is always called
    auto stream = z_stream {};

    constexpr auto window_max_size = 15;
    constexpr auto window_gzip_format = 16;

    if (const auto ret = inflateInit2(&stream, window_max_size + window_gzip_format);
        ret != Z_OK) {
        return tl::unexpected<LoadError> {
            LoadErrorType::gzip_decompress_error,
            fmt::format("Failed to initialize zlib for decompression {}.",
                        format_zlib_status_code(ret)),
        };
    }

    // Copy to not introduce UB
    auto input = std::vector<Bytef> {};
    input.reserve(data.size());
    std::ranges::copy(data, std::back_inserter(input));

    auto output = std::vector<Bytef> {};
    // TODO protect multiplication
    output.resize(input.size() * 2);

    stream.avail_in = gsl::narrow<uInt>(input.size());
    stream.next_in = input.data();

    while (true) {
        if (stream.total_out >= output.size()) {
            // TODO protect multiplication
            output.resize(output.size() * 3);
        }

        const auto total_out_start = stream.total_out;
        const auto total_out = gsl::narrow<std::size_t>(stream.total_out);
        Expects(stream.total_out < output.size());
        stream.avail_out = gsl::narrow<uInt>(output.size() - total_out);
        stream.next_out = &output[total_out];

        const auto ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_STREAM_END) {
            output.resize(stream.total_out);
            break;
        }
        if (ret != Z_OK || stream.avail_out != 0) {
            return tl::unexpected<LoadError> {
                LoadErrorType::gzip_decompress_error,
                fmt::format("Zlib decompression error {}.", format_zlib_status_code(ret)),
            };
        }
        if (stream.total_out <= total_out_start) {
            return tl::unexpected<LoadError> {
                LoadErrorType::gzip_decompress_error,
                fmt::format("Zlib decompression did not make forward progress."),
            };
        }
    };

    auto result = std::string {};
    result.reserve(output.size());
    std::ranges::copy(output, std::back_inserter(result));

    return result;
    /*
    print(format_zlib_status_code(10));

    auto output = std::ostringstream {};

    try {
        auto params = boost::iostreams::gzip_params {};

        auto filter_stream = boost::iostreams::filtering_ostream {};
        filter_stream.push(boost::iostreams::gzip_decompressor());
        filter_stream.push(output);
        filter_stream.write(data.data(), gsl::narrow<std::streamsize>(data.size()));
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
    */
}

}  // namespace logicsim
