#include "render/write_file.h"

#include "algorithm/u8_conversion.h"

namespace logicsim {

auto write_to_file(const BLImage &bl_image,
                   const std::filesystem::path &filename) -> void {
    std::filesystem::create_directories(filename.parent_path());
    bl_image.writeToFile(to_string(filename.u8string()).c_str());
}

}  // namespace logicsim
