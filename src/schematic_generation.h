#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

namespace logicsim {

struct delay_t;
struct logicitem_id_t;
struct wire_id_t;
class Layout;
struct element_id_t;
class Schematic;

auto add_missing_placeholders(Schematic& schematic) -> void;

[[nodiscard]] auto generate_schematic(const Layout& layout,
                                      delay_t wire_delay_per_distance) -> Schematic;

[[nodiscard]] auto to_element_id(logicitem_id_t logicitem_id)
    -> element_id_t;
[[nodiscard]] auto to_element_id(const Layout& layout, wire_id_t logicitem_id)
    -> element_id_t;

[[nodiscard]] auto to_logicitem_id(const Layout& layout, element_id_t element_id)
    -> logicitem_id_t;
[[nodiscard]] auto to_wire_id(const Layout& layout, element_id_t element_id) -> wire_id_t;

}  // namespace logicsim

#endif
