#include "file.h"

#include "format/std_type.h"
#include "timer.h"

#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <filesystem>
#include <fstream>

namespace logicsim {

auto save_file(const std::filesystem::path &filename, const std::string &binary) -> bool {
    auto file =
        std::ofstream(filename, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    return bool {file << binary};
}

auto load_file(const std::filesystem::path &filename)
    -> tl::expected<std::string, LoadError> {
    const auto t = Timer {fmt::format("{}", filename)};
    const auto path = boost::filesystem::path {filename.native()};

    auto map = boost::iostreams::mapped_file_source {};

    try {
        map.open(path);
    } catch (const std::ios::failure &exc) {
        return tl::unexpected<LoadError> {
            LoadErrorType::file_open_error,
            fmt::format("Unable to open file: {}", exc.what()),
        };
    }

    return std::string {map.begin(), map.end()};
}

}  // namespace logicsim
