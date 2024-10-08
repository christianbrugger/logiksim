#ifndef LOGICSIM_VALIDATE_DEFINITION_LOGIKITEM_H
#define LOGICSIM_VALIDATE_DEFINITION_LOGIKITEM_H

namespace logicsim {

struct attributes_clock_generator_t;
struct LogicItemDefinition;
struct delay_t;

[[nodiscard]] auto clock_generator_min_time() -> delay_t;
[[nodiscard]] auto clock_generator_max_time() -> delay_t;

[[nodiscard]] auto is_valid(const attributes_clock_generator_t& attrs) -> bool;

[[nodiscard]] auto is_valid(const LogicItemDefinition& definition) -> bool;

}  // namespace logicsim

#endif
