#ifndef LOGIKSIM_FILE_H
#define LOGIKSIM_FILE_H

namespace logicsim {

auto save_file(std::string filename, std::string binary) -> bool;
auto load_file(std::string filename) -> std::string;

}  // namespace logicsim

#endif
