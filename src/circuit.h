#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include "layout.h"
#include "layout_calculations.h"
#include "schematic.h"

#include <optional>

namespace logicsim {

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

   private:
    std::optional<Schematic> schematic_ {};
    std::optional<Layout> layout_ {};
};

// TODO remove this, as we now have the same interface

// [](element_id_t element_id, layout_calculation_data_t data){..}
// [](element_id_t element_id, segment_info_t segment, segment_index_t segment_index){..}
template <typename ElementCallback, typename SegmentCallback>
auto iter_inserted_circuit_items(const Circuit& circuit, ElementCallback element_callback,
                                 SegmentCallback segment_callback) -> void {
    const auto& schematic = circuit.schematic();
    const auto& layout = circuit.layout();

    for (const auto element : schematic.elements()) {
        const auto element_id = element.element_id();
        if (is_inserted(layout.display_state(element_id))) {
            if (element.is_element()) {
                const auto data = to_layout_calculation_data(circuit, element_id);
                element_callback(element_id, data);
            }
            if (element.is_wire()) {
                const auto& segment_tree = layout.segment_tree(element_id);
                for (const auto segment_index : segment_tree.indices()) {
                    const auto segment = segment_tree.segment(segment_index);
                    segment_callback(element_id, segment, segment_index);
                }
            }
        }
    }
}

}  // namespace logicsim

#endif
