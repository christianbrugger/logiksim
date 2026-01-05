#ifndef LOGIKSIM_SERIALIZE_H
#define LOGIKSIM_SERIALIZE_H

#include "core/format/struct.h"
#include "core/vocabulary/insertion_mode.h"
#include "core/vocabulary/load_error.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/save_format.h"
#include "core/vocabulary/selection_id.h"
#include "core/vocabulary/simulation_config.h"
#include "core/vocabulary/view_config.h"

#include <gsl/gsl>
#include <tl/expected.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace logicsim {

class Layout;
class Selection;
class EditableCircuit;

struct SerializeConfig {
    SaveFormat save_format;

    std::optional<ViewPoint> view_point = {};
    std::optional<SimulationConfig> simulation_config {};
    // Save position is used for copy & paste to store the mouse position.
    std::optional<point_t> save_position {};

    [[nodiscard]] auto operator==(const SerializeConfig& config) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

/**
 * @brief: Serialize the given layout, view_point and simulation_config.
 *
 * Throws an exception if any element is not display state normal.
 */
[[nodiscard]] auto serialize_all(const Layout& layout, const SerializeConfig& config)
    -> std::string;

/**
 * @brief: Serialize the selected elements.
 *
 *
 * Throws an exception if a selected element does not have display state normal.
 */
[[nodiscard]] auto serialize_selected(const Layout& layout, const Selection& selection,
                                      const SerializeConfig& config) -> std::string;

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
    auto add_to(EditableCircuit& editable_circuit, AddParameters parameters) const
        -> void;
    [[nodiscard]] auto view_point() const -> ViewPoint;
    [[nodiscard]] auto simulation_config() const -> SimulationConfig;
    [[nodiscard]] auto save_position() const -> point_t;

   private:
    // read-only, preserving whole parts relationship
    std::shared_ptr<const SerializedLayout> data_;
};

static_assert(std::copyable<LoadLayoutResult>);

}  // namespace serialize

/**
 * @brief: Load layout form json data that is optionall gzipped and base64 encoded.
 */
[[nodiscard]] auto load_layout(std::string_view binary)
    -> tl::expected<serialize::LoadLayoutResult, LoadError>;

}  // namespace logicsim

#endif
