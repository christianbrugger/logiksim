#ifndef LOGIKSIM_FILE_H
#define LOGIKSIM_FILE_H

#include "vocabulary/load_error.h"

#include <tl/expected.hpp>

#include <filesystem>

namespace logicsim {

auto save_file(const std::filesystem::path& filename, const std::string& binary) -> bool;

auto load_file(const std::filesystem::path& filename)
    -> tl::expected<std::string, LoadError>;

}  // namespace logicsim

#endif
