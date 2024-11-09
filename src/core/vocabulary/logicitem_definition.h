#ifndef LOGICSIM_VOCABULARY_LOGIK_ITEM_DEFINITION_H
#define LOGICSIM_VOCABULARY_LOGIK_ITEM_DEFINITION_H

#include "core/format/struct.h"
#include "core/logging.h"
#include "core/vocabulary/circuit_id.h"
#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/delay.h"
#include "core/vocabulary/logic_small_vector.h"
#include "core/vocabulary/logicitem_type.h"
#include "core/vocabulary/orientation.h"

#include <compare>
#include <optional>
#include <string>
#include <type_traits>

namespace logicsim {

class CopyTest {
   public:
    explicit CopyTest() {
        print("CopyTest: default construct");
    };

    explicit CopyTest(const CopyTest&) {
        print("CopyTest: COPY construct  !!!");
    };

    explicit CopyTest(CopyTest&&) noexcept {
        print("CopyTest: move construct");
    };

    auto operator=(const CopyTest&) -> CopyTest& {
        print("CopyTest: COPY assignment  !!!");
        return *this;
    };

    auto operator=(CopyTest&&) noexcept -> CopyTest& {
        print("CopyTest: move assignment");
        return *this;
    };

    ~CopyTest() {
        print("CopyTest: destroy");
    };

    [[nodiscard]] auto format() const -> std::string {
        return "";
    }

    [[nodiscard]] auto allocated_size() const -> std::size_t {
        return 0;
    };

    [[nodiscard]] auto operator==(const CopyTest& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const CopyTest&) const = default;
};

/**
 * @brief: Clock generator specific attributes.
 */
struct attributes_clock_generator_t {
    std::string name {"clock"};

    // all times are for half the clock period
    delay_t time_symmetric {500us};
    delay_t time_on {500us};
    delay_t time_off {500us};

    bool is_symmetric {true};
    bool show_simulation_controls {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_period() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto operator==(const attributes_clock_generator_t& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const attributes_clock_generator_t&) const = default;
};

static_assert(std::is_aggregate_v<attributes_clock_generator_t>);

/**
 * @brief: Defines all attributes of a circuit element.
 */
struct LogicItemDefinition {
    LogicItemType logicitem_type {LogicItemType::sub_circuit};
    connection_count_t input_count {0};
    connection_count_t output_count {0};
    orientation_t orientation {orientation_t::undirected};

    circuit_id_t sub_circuit_id {null_circuit};
    logic_small_vector_t input_inverters {};
    logic_small_vector_t output_inverters {};

    std::optional<attributes_clock_generator_t> attrs_clock_generator {};

    CopyTest test {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto operator==(const LogicItemDefinition& other) const -> bool =
                                                                                 default;
    [[nodiscard]] auto operator<=>(const LogicItemDefinition& other) const = default;
};

static_assert(std::is_aggregate_v<LogicItemDefinition>);

}  // namespace logicsim

#endif
