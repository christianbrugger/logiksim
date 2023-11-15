#ifndef LOGIKSIM_SERIALIZE_H
#define LOGIKSIM_SERIALIZE_H

#include "vocabulary.h"
#include "vocabulary/selection_id.h"

#include <gsl/gsl>

#include <memory>
#include <string>
#include <vector>

namespace logicsim {

class Layout;
class Selection;
class EditableCircuit;
struct ViewPoint;
struct ViewConfig;
struct SimulationConfig;

using binary_t = std::vector<uint8_t>;

[[nodiscard]] auto serialize_inserted(const Layout& layout,
                                      const ViewConfig* view_config = nullptr,
                                      const SimulationConfig* simulation_config = nullptr)
    -> std::string;

[[nodiscard]] auto serialize_selected(const Layout& layout, const Selection& selection,
                                      point_t save_position = point_t {0, 0})
    -> std::string;

namespace serialize {
struct SerializedLayout;

struct AddParameters {
    InsertionMode insertion_mode {InsertionMode::insert_or_discard};
    selection_id_t selection_id {null_selection_id};
    std::optional<point_t> load_position {};
};

class LoadLayoutResult {
   public:
    LoadLayoutResult(SerializedLayout&& layout);

    LoadLayoutResult(LoadLayoutResult&&);
    LoadLayoutResult(const LoadLayoutResult&) = delete;
    auto operator=(LoadLayoutResult&&) -> LoadLayoutResult&;
    auto operator=(const LoadLayoutResult&) -> LoadLayoutResult& = delete;
    ~LoadLayoutResult();

   public:
    auto add(EditableCircuit& editable_circuit, AddParameters parameters) const -> void;
    [[nodiscard]] auto view_point() const -> ViewPoint;
    [[nodiscard]] auto simulation_config() const -> SimulationConfig;

   private:
    std::unique_ptr<SerializedLayout> data_;
};

}  // namespace serialize

auto load_layout(const std::string& binary) -> std::optional<serialize::LoadLayoutResult>;

}  // namespace logicsim

#endif
