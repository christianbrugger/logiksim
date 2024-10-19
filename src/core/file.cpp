#include "core/file.h"

#include "core/format/std_type.h"
#include "core/macro/try_catch.h"

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

namespace {

[[nodiscard]] LS_TRY_CATCH_NON_EMPTY auto get_mapped_file_source(
    const boost::filesystem::path &path)
    -> tl::expected<boost::iostreams::mapped_file_source, LoadError> {
    try {
        return boost::iostreams::mapped_file_source(path);
    } catch (const std::ios::failure &exc) {
        return tl::unexpected<LoadError> {
            LoadErrorType::file_open_error,
            fmt::format("Unable to open file: {}", exc.what()),
        };
    }
}

[[nodiscard]] auto mmap_source_to_string(boost::iostreams::mapped_file_source &&map)
    -> std::string {
    return std::string {map.begin(), map.end()};
};

}  // namespace

auto load_file(const std::filesystem::path &filename)
    -> tl::expected<std::string, LoadError> {
    const auto path = boost::filesystem::path {filename.native()};

    // Note, memory mapping files for reading is much faster than ifstream.
    return get_mapped_file_source(path).transform(mmap_source_to_string);
}

}  // namespace logicsim
