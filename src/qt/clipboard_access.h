#ifndef LOGICSIM_QT_CLIPBOARD_ACCESS_H
#define LOGICSIM_QT_CLIPBOARD_ACCESS_H

#include <string>
#include <string_view>

namespace logicsim {

[[nodiscard]] auto get_clipboard_text() -> std::string;

auto set_clipboard_text(const std::string& text) -> void;

}  // namespace logicsim

#endif
