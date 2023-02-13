#include "circuit_index.h"

#include "exceptions.h"
#include "range.h"

#include <algorithm>

namespace logicsim {

auto CircuitIndex::circuit_count() const -> std::size_t {
    check_equal_size();
    return schematics_.size();
}

auto CircuitIndex::borrow_schematic(circuit_id_t circuit_id) -> Schematic {
    auto& source = schematics_.at(circuit_id.value);

    if (source.circuit_id() != circuit_id) [[unlikely]] {
        throw_exception("Cannot borrow missing schematics.");
    }
    auto result = Schematic {null_circuit};
    source.swap(result);
    return result;
}

auto CircuitIndex::borrow_schematics() -> std::vector<Schematic> {
    check_are_schematics_complete();

    auto result = std::vector<Schematic>(schematics_.size(), Schematic {null_circuit});
    schematics_.swap(result);
    return result;
}

auto CircuitIndex::borrow_layout(circuit_id_t circuit_id) -> Layout {
    auto& source = layouts_.at(circuit_id.value);

    if (source.circuit_id() != circuit_id) [[unlikely]] {
        throw_exception("Cannot borrow missing layout.");
    }
    auto result = Layout {null_circuit};
    source.swap(result);
    return result;
}

auto CircuitIndex::return_schematic(Schematic&& schematic) -> void {
    auto& source = schematics_.at(schematic.circuit_id().value);

    if (source.circuit_id() != null_circuit) [[unlikely]] {
        throw_exception("Cannot return ocupied schematic.");
    }

    source.swap(schematic);
}

auto CircuitIndex::return_schematics(std::vector<Schematic>&& schematics) -> void {
    if (schematics.size() != schematics_.size()) {
        throw_exception("Cannot return different number of schematics, than borrowed.");
    }
    if (!std::ranges::all_of(schematics_, [](const Schematic& schematic) {
            return schematic.circuit_id() == null_circuit;
        })) {
        throw_exception("Cannot return occupied schematic.");
    }
    if (!std::ranges::equal(
            schematics, range(schematics.size()), {},
            [](const Schematic& schematic) { return schematic.circuit_id().value; })) {
        throw_exception("Circuit-ids need to have correct order.");
    }

    schematics_.swap(schematics);
}

auto CircuitIndex::return_layout(Layout&& layout) -> void {
    auto& source = layouts_.at(layout.circuit_id().value);

    if (source.circuit_id() != null_circuit) [[unlikely]] {
        throw_exception("Cannot return ocupied layout.");
    }

    source.swap(layout);
}

auto CircuitIndex::description(circuit_id_t circuit_id) -> const CircuitDescription& {
    return descriptions_.at(circuit_id.value);
}

auto CircuitIndex::descriptions() -> const std::vector<CircuitDescription>& {
    return descriptions_;
}

auto CircuitIndex::check_is_complete() const -> void {
    check_equal_size();

    check_are_schematics_complete();
    check_are_layouts_complete();
    check_are_descriptions_complete();
}

auto CircuitIndex::check_are_schematics_complete() const -> void {
    if (!std::ranges::equal(
            schematics_, range(schematics_.size()), {},
            [](const Schematic& item) { return item.circuit_id().value; })) {
        throw_exception("Some schematics are missing.");
    }
}

auto CircuitIndex::check_are_layouts_complete() const -> void {
    if (!std::ranges::equal(layouts_, range(layouts_.size()), {},
                            [](const Layout& item) { return item.circuit_id().value; })) {
        throw_exception("Some layouts are missing.");
    }
}

auto CircuitIndex::check_are_descriptions_complete() const -> void {
    if (!std::ranges::equal(
            descriptions_, range(descriptions_.size()), {},
            [](const CircuitDescription& item) { return item.circuit_id.value; })) {
        throw_exception("Some descriptions are missing.");
    }
}

auto CircuitIndex::check_equal_size() const -> void {
    if (!(schematics_.size() == layouts_.size() == descriptions_.size())) {
        throw_exception("Schematics, layouts and descriptions have different sizes.");
    }
}

}  // namespace logicsim