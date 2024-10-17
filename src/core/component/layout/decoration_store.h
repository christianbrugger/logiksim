#ifndef LOGICSIM_CORE_COMPONENT_LAYOUT_DECORATION_STORE_H
#define LOGICSIM_CORE_COMPONENT_LAYOUT_DECORATION_STORE_H

#include "core/vocabulary/decoration_definition.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/decoration_type.h"
#include "core/vocabulary/display_state.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/rect.h"
#include "core/vocabulary/size_2d.h"

#include <ankerl/unordered_dense.h>

#include <vector>

namespace logicsim {

struct decoration_layout_data_t;

namespace layout {

template <typename T>
using decoration_attr_t = ankerl::unordered_dense::map<decoration_id_t, T>;

/**
 * @brief: Stores decorations of the layout.
 *
 * Class invariants:
 *     + All stored decorations have valid definitions.
 *     + All data vectors have the same size.
 *     + All bounding rects are fully representable on the grid.
 */
class DecorationStore {
   public:
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    // add & delete
    auto add(const DecorationDefinition &definition, point_t position,
             display_state_t display_state) -> decoration_id_t;
    auto swap_and_delete(decoration_id_t decoration_id) -> decoration_id_t;
    auto swap_items(decoration_id_t decoration_id_1,
                    decoration_id_t decoration_id_2) -> void;

    /**
     * @brief: brings the store in canonical form,
     *         so that visual equivalent layouts compare equal
     */
    auto normalize() -> void;
    [[nodiscard]] auto operator==(const DecorationStore &) const -> bool = default;

    // getters
    [[nodiscard]] auto type(decoration_id_t decoration_id) const -> DecorationType;
    [[nodiscard]] auto size(decoration_id_t decoration_id) const -> size_2d_t;
    [[nodiscard]] auto position(decoration_id_t decoration_id) const -> point_t;
    [[nodiscard]] auto display_state(decoration_id_t decoration_id) const
        -> display_state_t;
    [[nodiscard]] auto bounding_rect(decoration_id_t decoration_id) const -> rect_t;
    [[nodiscard]] auto attrs_text_element(decoration_id_t decoration_id) const
        -> const attributes_text_element_t &;

    // setters
    auto set_position(decoration_id_t decoration_id, point_t position) -> void;
    auto set_display_state(decoration_id_t decoration_id,
                           display_state_t display_state) -> void;
    auto set_attributes(decoration_id_t decoration_id,
                        attributes_text_element_t attrs) -> void;

   private:
    auto delete_last() -> void;
    auto last_decoration_id() const -> decoration_id_t;

   private:
    std::vector<DecorationType> decoration_types_ {};
    std::vector<size_2d_t> sizes_ {};

    std::vector<point_t> positions_ {};
    std::vector<display_state_t> display_states_ {};
    std::vector<rect_t> bounding_rects_ {};

    layout::decoration_attr_t<attributes_text_element_t> map_text_element_ {};
};

//
// Free Functions
//

[[nodiscard]] auto to_decoration_layout_data(const DecorationStore &store,
                                             decoration_id_t decoration_id)
    -> decoration_layout_data_t;

[[nodiscard]] auto to_decoration_layout_data(
    const DecorationStore &store, decoration_id_t decoration_id,
    point_t position) -> decoration_layout_data_t;

[[nodiscard]] auto to_decoration_definition(
    const DecorationStore &store, decoration_id_t decoration_id) -> DecorationDefinition;

}  // namespace layout

}  // namespace logicsim

#endif
