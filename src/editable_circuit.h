#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "layout.h"
#include "schematic.h"

#include <ankerl/unordered_dense.h>

#include <vector>

namespace logicsim {

class EditableCircuit {
   public:
    [[nodiscard]] EditableCircuit(Schematic &&schematic, Layout &&layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> const Schematic &;
    [[nodiscard]] auto layout() const noexcept -> const Layout &;

    [[nodiscard]] auto copy_input_positions() -> std::vector<point_t>;
    [[nodiscard]] auto copy_output_positions() -> std::vector<point_t>;

    auto add_standard_element(ElementType type, std::size_t input_count, point_t position,
                              DisplayOrientation orientation
                              = DisplayOrientation::default_right) -> void;

    auto add_wire(LineTree &&line_tree) -> void;

    // todo: extract_schematic, extract_layout

   private:
    auto connect_new_element(element_id_t element) -> void;

    using connection_map_t = ankerl::unordered_dense::map<point_t, connection_t>;
    connection_map_t input_connections_;
    connection_map_t output_connections_;

    Schematic schematic_;
    Layout layout_;
};

}  // namespace logicsim

#endif
