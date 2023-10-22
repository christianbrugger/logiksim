#ifndef LOGICSIM_SCHEMATIC_VALIDATION_H
#define LOGICSIM_SCHEMATIC_VALIDATION_H

#include "schematic_old.h"  // TODO remove

namespace logicsim {

class Schematic;

namespace schematic {

auto validate_has_no_placeholders(const SchematicOld::ConstElement element) -> void;
auto validate_all_outputs_connected(const SchematicOld::ConstElement element) -> void;
auto validate_all_inputs_disconnected(const SchematicOld::ConstElement element) -> void;
auto validate_all_outputs_disconnected(const SchematicOld::ConstElement element) -> void;

struct ValidationSettings {
    bool require_all_outputs_connected {false};
    bool require_all_placeholders_connected {false};
};

constexpr static auto validate_basic = ValidationSettings {false, false};
constexpr static auto validate_all = ValidationSettings {true, true};
}  // namespace schematic

void validate(const SchematicOld &schematic,
              schematic::ValidationSettings settings = schematic::validate_basic);

// TODO how much of this do we need ???
void validate(const Schematic &schematic,
              schematic::ValidationSettings settings = schematic::validate_basic);

}  // namespace logicsim

#endif
