#ifndef LOGICSIM_VALIDATE_DEFINITION_H
#define LOGICSIM_VALIDATE_DEFINITION_H

namespace logicsim {

struct attributes_clock_generator_t;
struct ElementDefinition;

[[nodiscard]] auto is_valid(const attributes_clock_generator_t& attrs) -> bool;

[[nodiscard]] auto is_valid(const ElementDefinition& definition) -> bool;

}  // namespace logicsim

#endif
