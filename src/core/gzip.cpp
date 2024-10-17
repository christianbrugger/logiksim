#include "core/gzip.h"

#include "core/timer.h"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <fmt/core.h>
#include <gsl/gsl>
#include <zlib.h>

#include <iterator>
#include <sstream>

namespace logicsim {

namespace {

/**
 * @brief: Resource managed inflate z_stream.
 */
struct ZInflateStream {
    z_stream value {};

    constexpr explicit ZInflateStream() = default;

    ~ZInflateStream() {
        inflateEnd(&value);
    };

    ZInflateStream(ZInflateStream&&) = delete;
    ZInflateStream(const ZInflateStream&) = delete;
    auto operator=(ZInflateStream&&) = delete;
    auto operator=(const ZInflateStream&) = delete;
};

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
}

struct zlib_progress {
    uLong total_in;
    uLong total_out;
};

[[nodiscard]] auto get_progress(const z_stream& stream) -> zlib_progress {
    return zlib_progress {stream.total_in, stream.total_out};
}

[[nodiscard]] auto made_progress(const z_stream& stream,
                                 zlib_progress last_progress) -> bool {
    return stream.total_in > last_progress.total_in ||
           stream.total_out > last_progress.total_out;
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

/**
 * Note zlib is used directly here instead of boost, due to boost not giving
 * enough control over error handling. It always uses exceptions that don't
 * place nice with sanitizer the sanitizer builds.
 */
auto gzip_decompress(std::string_view input) -> tl::expected<std::string, LoadError> {
    const auto t = Timer {"gzip_decompress"};

    // initialize inflate
    auto stream = ZInflateStream {};
    constexpr auto window_max_size = 15;
    constexpr auto window_gzip_format = 16;
    if (const auto ret =
            inflateInit2(&stream.value, window_max_size + window_gzip_format);
        ret != Z_OK) {
        return tl::unexpected<LoadError> {
            LoadErrorType::gzip_decompress_error,
            fmt::format("Failed to initialize zlib for decompression {}.",
                        format_zlib_status_code(ret)),
        };
    }

    // input
    // this is defined behavior as long as zlib does not write to next_in
    static_assert(std::same_as<Bytef, unsigned char>);
    stream.value.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    stream.value.avail_in = gsl::narrow<uInt>(input.size());

    // buffer
    constexpr static auto chunk_size = uInt {16 * 1024};
    auto buffer = std::vector<Bytef> {};
    buffer.resize(chunk_size);

    // output
    auto output = std::string {};
    output.reserve(input.size() * 2);

    // decompression
    while (true) {
        const auto last_progress = get_progress(stream.value);
        stream.value.avail_out = gsl::narrow<uInt>(buffer.size());
        stream.value.next_out = buffer.data();

        const auto ret = inflate(&stream.value, Z_NO_FLUSH);

        if (ret != Z_OK && ret != Z_STREAM_END) {
            return tl::unexpected<LoadError> {
                LoadErrorType::gzip_decompress_error,
                fmt::format("Zlib decompression error {}.", format_zlib_status_code(ret)),
            };
        }

        // copy chunk
        static_assert(std::is_unsigned_v<decltype(stream.value.avail_out)>);
        Expects(stream.value.avail_out <= chunk_size);
        const auto offset = std::ptrdiff_t {chunk_size - stream.value.avail_out};
        output.append(buffer.begin(), std::next(buffer.begin(), offset));

        if (ret == Z_STREAM_END) {
            break;
        }
        if (!made_progress(stream.value, last_progress)) {
            return tl::unexpected<LoadError> {
                LoadErrorType::gzip_decompress_error,
                fmt::format("Zlib decompression did not make forward progress."),
            };
        }
    };

    return output;
}

}  // namespace logicsim
