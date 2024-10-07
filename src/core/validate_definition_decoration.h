#ifndef LOGICSIM_CORE_VALIDATE_DEFINITION_DECORATION_H
#define LOGICSIM_CORE_VALIDATE_DEFINITION_DECORATION_H

namespace logicsim {

struct attributes_text_element_t;
struct DecorationDefinition;

[[nodiscard]] auto is_valid(const attributes_text_element_t& attrs) -> bool;

[[nodiscard]] auto is_valid(const DecorationDefinition& definition) -> bool;

}  // namespace logicsim

#endif
