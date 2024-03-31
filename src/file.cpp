#include "file.h"

#include <filesystem>
#include <fstream>

namespace logicsim {

auto save_file(const std::filesystem::path &filename, const std::string &binary) -> bool {
    auto file =
        std::ofstream(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    file << binary;
    return file.good();
}

auto load_file(const std::filesystem::path &filename) -> std::string {
    auto file = std::ifstream {filename, std::ios::in | std::ios::binary};
    auto buffer = std::ostringstream {};
    buffer << file.rdbuf();

    if (!file.good()) {
        return "";
    }

    return buffer.str();
}

}  // namespace logicsim