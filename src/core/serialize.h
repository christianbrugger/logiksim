#ifndef LOGIKSIM_SERIALIZE_H
#define LOGIKSIM_SERIALIZE_H

#include "vocabulary/insertion_mode.h"
#include "vocabulary/point.h"
#include "vocabulary/save_format.h"
#include "vocabulary/selection_id.h"

#include <gsl/gsl>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace logicsim {

class Layout;
class Selection;
class EditableCircuit;
struct ViewPoint;
struct SimulationConfig;

/**
 * @brief: Serialize the given layout, view_point and simulation_config.
 *
 * Throws an exception if any element is not display state normal.
 */
[[nodiscard]] auto serialize_all(const Layout& layout,
                                 std::optional<ViewPoint> view_point,
                                 std::optional<SimulationConfig> simulation_config,
                                 SaveFormat format) -> std::string;

/**
 * @brief: Serialize the selected elements.
 *
 * Save position is used for copy & paste to offset the saved positions.
 *
 * Throws an exception if a selected element does not have display state normal.
 */
[[nodiscard]] auto serialize_selected(const Layout& layout, const Selection& selection,
                                      std::optional<point_t> save_position,
                                      SaveFormat format) -> std::string;

namespace serialize {
struct SerializedLayout;

struct AddParameters {
    InsertionMode insertion_mode {InsertionMode::insert_or_discard};
    selection_id_t selection_id {null_selection_id};
    std::optional<point_t> load_position {};
};

class LoadLayoutResult {
   public:
    explicit LoadLayoutResult(SerializedLayout&& layout);

   public:
    auto add_to(EditableCircuit& editable_circuit,
                AddParameters parameters) const -> void;
    [[nodiscard]] auto view_point() const -> ViewPoint;
    [[nodiscard]] auto simulation_config() const -> SimulationConfig;

   private:
    // read-only, preserving whole parts relationship
    std::shared_ptr<const SerializedLayout> data_;
};

static_assert(std::copyable<LoadLayoutResult>);

}  // namespace serialize

/**
 * @brief: Load layout form json data that is optionall gzipped and base64 encoded.
 */
[[nodiscard]] auto load_layout(const std::string& binary)
    -> std::optional<serialize::LoadLayoutResult>;

}  // namespace logicsim

#endif
