#include "component/editable_circuit/modifier.h"
#include "editable_circuit.h"
#include "test_core/editable_circuit/modifier/test_helpers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// Test Construction
//

TEST(ECModifierConfig, DefaultConstruction) {
    auto modifier = Modifier {};
    Expects(is_valid(modifier));

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
}

TEST(ECModifierConfig, ConstructionWithLayout) {
    auto layout = Layout {};
    add_and_element(layout, display_state_t::normal);

    auto modifier = Modifier {Layout {layout}};
    Expects(is_valid(modifier));

    ASSERT_EQ(modifier.circuit_data().layout.empty(), false);
    ASSERT_EQ(modifier.circuit_data().layout, layout);
}

//
// Test Logging
//

TEST(ECModifierConfig, VerifyLogging1) {
    auto modifier = Modifier {
        Layout {},
        ModifierConfig {.store_messages = true},
    };
    Expects(is_valid(modifier));

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().messages.has_value(), true);
    ASSERT_EQ(modifier.circuit_data().messages.value().empty(), true);

    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::insert_or_discard);
    ASSERT_EQ(modifier.circuit_data().messages.value().empty(), false);
}

//
// Message Verification Defaults
//

TEST(ECModifierConfig, VerifyModiferNDEBUG1) {
    const auto modifier = Modifier {};
    Expects(is_valid(modifier));
    const auto &circuit = modifier.circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
    ASSERT_EQ(circuit.messages.has_value(), false);
}

TEST(ECModifierConfig, VerifyModiferNDEBUG2) {
    const auto modifier = Modifier {Layout {}};
    Expects(is_valid(modifier));
    const auto &circuit = modifier.circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
    ASSERT_EQ(circuit.messages.has_value(), false);
}

TEST(ECModifierConfig, VerifyEditableCircuitNDEBUG1) {
    const auto editable_circuit = EditableCircuit {};
    Expects(is_valid(editable_circuit));
    const auto &circuit = editable_circuit.modifier().circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
    ASSERT_EQ(circuit.messages.has_value(), false);
}

TEST(ECModifierConfig, VerifyEditableCircuitNDEBUG2) {
    const auto editable_circuit = EditableCircuit {};
    Expects(is_valid(editable_circuit));
    const auto &circuit = editable_circuit.modifier().circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
    ASSERT_EQ(circuit.messages.has_value(), false);
}

}  // namespace editable_circuit

}  // namespace logicsim
