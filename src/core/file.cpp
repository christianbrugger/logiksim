#include "file.h"

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
    auto file = std::ifstream {filename, std::ios::in | std::ios::binary};

    if (!file.is_open()) {
        return tl::unexpected<LoadError> {
            LoadErrorType::file_open_error,
            "Unable to open file.",
        };
    }

    auto buffer = std::ostringstream {};
    if (buffer << file.rdbuf()) {
        return buffer.str();
    }

    return tl::unexpected<LoadError> {
        LoadErrorType::file_read_error,
        "Unable to read file.",
    };
}

}  // namespace logicsim
