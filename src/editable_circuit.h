#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "layout.h"
#include "schematic.h"

#include <ankerl/unordered_dense.h>

#include <vector>

namespace logicsim {

template <bool IsInput>
class ConnectionCache {
   public:
    using map_type = ankerl::unordered_dense::map<point_t, connection_t>;

   public:
    using connection_proxy
        = std::conditional_t<IsInput, Schematic::Input, Schematic::Output>;
    using const_connection_proxy
        = std::conditional_t<IsInput, Schematic::ConstInput, Schematic::ConstOutput>;

    auto insert(element_id_t element_id, const Schematic &schematic, const Layout &layout)
        -> void;
    auto remove(element_id_t element_id, const Schematic &schematic, const Layout &layout)
        -> void;
    auto update(element_id_t new_element_id, element_id_t old_element_id,
                const Schematic &schematic, const Layout &layout) -> void;

    [[nodiscard]] auto find(point_t position) const -> std::optional<connection_t>;
    [[nodiscard]] auto find(point_t position, Schematic &schematic) const
        -> std::optional<connection_proxy>;
    [[nodiscard]] auto find(point_t position, const Schematic &schematic) const
        -> std::optional<const_connection_proxy>;

    [[nodiscard]] auto positions() const {
        return std::ranges::views::keys(connections_);
    }

   private:
    map_type connections_;
};

class EditableCircuit {
   public:
    // Make private and move to connection cache class
    using connection_map_t = ankerl::unordered_dense::map<point_t, connection_t>;

   public:
    [[nodiscard]] EditableCircuit(Schematic &&schematic, Layout &&layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> const Schematic &;
    [[nodiscard]] auto layout() const noexcept -> const Layout &;

    [[nodiscard]] auto input_positions() const {
        return input_connections_.positions();
    };

    [[nodiscard]] auto output_positions() const {
        return output_connections_.positions();
    };

    auto add_inverter_element(point_t position, DisplayOrientation orientation
                                                = DisplayOrientation::default_right)
        -> void;
    auto add_standard_element(ElementType type, std::size_t input_count, point_t position,
                              DisplayOrientation orientation
                              = DisplayOrientation::default_right) -> void;

    auto add_wire(LineTree &&line_tree) -> void;

    // swaps the element with last one and deletes it
    auto swap_and_delete_element(element_id_t element_id) -> void;

    // todo: extract_schematic, extract_layout

   private:
    auto add_placeholder_element() -> element_id_t;
    auto add_missing_placeholders(element_id_t element_id) -> void;

    // invalidates the element_id, as element output placeholders might be deleted
    auto connect_new_element(element_id_t &element_id) -> void;

    auto swap_and_delete_single_element(element_id_t element_id) -> void;
    auto swap_and_delete_multiple_elements(std::span<const element_id_t> element_ids)
        -> void;

    auto cache_insert(element_id_t element_id) -> void;
    auto cache_remove(element_id_t element_id) -> void;
    auto cache_update(element_id_t new_element_id, element_id_t old_element_id) -> void;

    ConnectionCache<true> input_connections_;
    ConnectionCache<false> output_connections_;

    Schematic schematic_;
    Layout layout_;
};

}  // namespace logicsim

#endif
