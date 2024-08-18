#ifndef LOGIKSIM_FILE_H
#define LOGIKSIM_FILE_H

#include <filesystem>

namespace logicsim {

auto save_file(const std::filesystem::path& filename, const std::string& binary) -> bool;

auto load_file(const std::filesystem::path& filename) -> std::string;

}  // namespace logicsim

#endif
