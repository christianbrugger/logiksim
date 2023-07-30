#ifndef LOGIKSIM_SERIALIZE_H
#define LOGIKSIM_SERIALIZE_H

namespace logicsim {

class Layout;
class Selection;

using binary_t = std::vector<uint8_t>;

[[nodiscard]] auto serialize_inserted(const Layout& layout) -> std::string;
[[nodiscard]] auto serialize_selected(const Layout& layout, const Selection& selection)
    -> std::string;

[[nodiscard]] auto base64_encode(const std::string& data) -> std::string;
[[nodiscard]] auto base64_decode(const std::string& data) -> std::string;

auto save_layout(const Layout& layout, std::string filename = "data.json.gz") -> void;

}  // namespace logicsim

#endif
