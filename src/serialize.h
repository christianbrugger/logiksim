#ifndef LOGIKSIM_SERIALIZE_H
#define LOGIKSIM_SERIALIZE_H

#include "vocabulary.h"

#include <gsl/gsl>

#include <memory>
#include <string>
#include <vector>

namespace logicsim {

class Layout;
class Selection;
class EditableCircuit;
class selection_handle_t;
class ViewConfig;
struct SimulationSettings;

using binary_t = std::vector<uint8_t>;

[[nodiscard]] auto serialize_inserted(
    const Layout& layout, const ViewConfig* view_config = nullptr,
    const SimulationSettings* simulation_settings = nullptr) -> std::string;

[[nodiscard]] auto serialize_selected(const Layout& layout, const Selection& selection,
                                      point_t save_position = {0, 0}) -> std::string;

[[nodiscard]] auto base64_encode(const std::string& data) -> std::string;
[[nodiscard]] auto base64_decode(const std::string& data) -> std::string;

namespace serialize {
struct SerializedLayout;

class LoadLayoutResult {
   public:
    LoadLayoutResult(SerializedLayout&& layout);

    LoadLayoutResult(LoadLayoutResult&&);
    LoadLayoutResult(const LoadLayoutResult&) = delete;
    auto operator=(LoadLayoutResult&&) -> LoadLayoutResult&;
    auto operator=(const LoadLayoutResult&) -> LoadLayoutResult& = delete;
    ~LoadLayoutResult();

   public:
    auto add(EditableCircuit& editable_circuit, InsertionMode insertion_mode,
             std::optional<point_t> load_position = {}) const -> selection_handle_t;

    auto apply(ViewConfig& view_config) const -> void;

   private:
    std::unique_ptr<SerializedLayout> data_;
};
}  // namespace serialize

auto load_layout(const std::string& binary) -> std::optional<serialize::LoadLayoutResult>;

}  // namespace logicsim

#endif
