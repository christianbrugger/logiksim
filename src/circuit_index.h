#ifndef LOGIKSIM_CIRCUIT_INDEX_H
#define LOGIKSIM_CIRCUIT_INDEX_H

#include "circuit.h"
#include "circuit_description.h"
#include "layout.h"
#include "schematic.h"
#include "vocabulary.h"

#include <vector>

namespace logicsim {

class CircuitIndex {
   public:
    CircuitIndex() = default;

    [[nodiscard]] auto circuit_count() const -> std::size_t;

    [[nodiscard]] auto borrow_circuit(circuit_id_t circuit_id) -> Circuit;
    [[nodiscard]] auto borrow_schematic(circuit_id_t circuit_id) -> Schematic;
    [[nodiscard]] auto borrow_schematics() -> std::vector<Schematic>;
    [[nodiscard]] auto borrow_layout(circuit_id_t circuit_id) -> Layout;

    auto return_schematic(Schematic&& schematic) -> void;
    auto return_schematics(std::vector<Schematic>&& schematics) -> void;
    auto return_layout(Layout&& layout) -> void;

    [[nodiscard]] auto description(circuit_id_t circuit_id) -> const CircuitDescription&;
    [[nodiscard]] auto descriptions() -> const std::vector<CircuitDescription>&;

    auto check_is_complete() const -> void;
    auto check_are_schematics_complete() const -> void;
    auto check_are_layouts_complete() const -> void;
    auto check_are_descriptions_complete() const -> void;

   private:
    auto check_equal_size() const -> void;

    std::vector<Schematic> schematics_ {Schematic {}};
    std::vector<Layout> layouts_ {Layout {}};
    std::vector<CircuitDescription> descriptions_ {CircuitDescription {}};
};
}  // namespace logicsim

#endif
