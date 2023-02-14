#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "layout.h"
#include "schematic.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

// TODO consider this into vocabulary
struct ConnectionEntry {
    element_id_t element_id;
    connection_id_t connection_id;

    // TODO move to cpp
    auto format() const -> std::string {
        return fmt::format("<Con: {}-{}>", element_id, connection_id);
    }
};

class EditableCircuit {
   public:
    [[nodiscard]] EditableCircuit(Schematic &&schematic, Layout &&layout);

    [[nodiscard]] auto schematic() const noexcept -> const Schematic &;
    [[nodiscard]] auto layout() const noexcept -> const Layout &;

    auto add_standard_element(ElementType type, std::size_t input_count, point_t position,
                              DisplayOrientation orientation
                              = DisplayOrientation::default_right) -> void;

    auto add_wire(LineTree &&line_tree) -> void;

   private:
    auto connect_new_element(element_id_t element) -> void;

    using connection_map_t = ankerl::unordered_dense::map<point_t, ConnectionEntry>;
    connection_map_t input_connections_;
    connection_map_t output_connections_;

    Schematic schematic_;
    Layout layout_;
};

}  // namespace logicsim

#endif
