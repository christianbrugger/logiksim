#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include "layout.h"
#include "layout_calculation.h"
#include "schematic.h"

#include <optional>

namespace logicsim {

class Circuit;

[[nodiscard]] auto is_inserted(const Circuit& circuit, element_id_t element_id) -> bool;
[[nodiscard]] auto is_logic_item(const Circuit& circuit, element_id_t element_id) -> bool;
[[nodiscard]] auto is_wire(const Circuit& circuit, element_id_t element_id) -> bool;

[[nodiscard]] auto get_segment_info(const Circuit& circuit, segment_t segment)
    -> segment_info_t;

class Circuit {
   public:
    [[nodiscard]] Circuit() = default;
    [[nodiscard]] Circuit(Schematic&& schematic, Layout&& layout);

    [[nodiscard]] auto format() const -> std::string;
    auto validate() const -> void;

    [[nodiscard]] auto schematic() const -> const Schematic&;
    [[nodiscard]] auto layout() const -> const Layout&;

    [[nodiscard]] auto schematic() -> Schematic&;
    [[nodiscard]] auto layout() -> Layout&;

    [[nodiscard]] auto extract_schematic() -> Schematic;
    [[nodiscard]] auto extract_layout() -> Layout;

    auto swap_elements(element_id_t element_id_0, element_id_t element_id_1) -> void;

   private:
    std::optional<Schematic> schematic_ {};
    std::optional<Layout> layout_ {};
};

}  // namespace logicsim

#endif
