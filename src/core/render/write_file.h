#ifndef LOGICSIM_CORE_RENDER_WRITE_FILE_H
#define LOGICSIM_CORE_RENDER_WRITE_FILE_H

#include <blend2d.h>

#include <filesystem>

namespace logicsim {

/**
 * @brief: Create folder and write image to the file.
 *
 * At the moment *.png and *.qoi are supported.
 *
 * Note, only works for valid utf8 paths on Windows.
 */
auto write_to_file(const BLImage &bl_image,
                   const std::filesystem::path &filename) -> void;

}  // namespace logicsim

#endif
