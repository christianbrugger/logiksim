#include "core/render/write_file.h"

#include "core/algorithm/u8_conversion.h"

namespace logicsim {

auto write_to_file(const BLImage &bl_image,
                   const std::filesystem::path &filename) -> void {
    if (filename.has_parent_path()) {
        std::filesystem::create_directories(filename.parent_path());
    }
    // Note, throws for invalid utf8 filepaths on windows
    bl_image.writeToFile(to_string(filename.u8string()).c_str());
}

}  // namespace logicsim
