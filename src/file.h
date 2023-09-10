#ifndef LOGIKSIM_FILE_H
#define LOGIKSIM_FILE_H

#include <QString>

#include <string>

namespace logicsim {

auto save_file(const QString& filename, const std::string& binary) -> bool;
auto save_file(const std::string& filename, const std::string& binary) -> bool;

auto load_file(const QString& filename) -> std::string;
auto load_file(const std::string& filename) -> std::string;

}  // namespace logicsim

#endif
