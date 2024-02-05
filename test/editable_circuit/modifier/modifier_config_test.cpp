
#include "./test_helpers.h"
#include "component/editable_circuit/modifier.h"
#include "editable_circuit.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// Test Construction
//

TEST(ECModifierConfig, DefaultConstruction) {
    auto modifier = Modifier {};

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
}

TEST(ECModifierConfig, ConstructionWithLayout) {
    auto layout = Layout {};
    add_and_element(layout, display_state_t::normal);

    auto modifier = Modifier {Layout {layout}};

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

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().store_messages, true);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);

    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::insert_or_discard);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), false);
}

TEST(ECModifierConfig, VerifyLogging2) {
    auto modifier = Modifier {
        ModifierConfig {.store_messages = true},
    };

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().store_messages, true);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);

    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::insert_or_discard);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), false);
}

TEST(ECModifierConfig, VerifyNoLogging1) {
    auto modifier = Modifier {};

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().store_messages, false);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);

    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::insert_or_discard);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);
}

TEST(ECModifierConfig, VerifyNoLogging2) {
    auto modifier = Modifier {Layout {}};

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().store_messages, false);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);

    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::insert_or_discard);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);
}

//
// Message Verification Defaults
//

TEST(ECModifierConfig, VerifyModiferNDEBUG1) {
    const auto modifier = Modifier {};
    const auto &circuit = modifier.circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
}

TEST(ECModifierConfig, VerifyModiferNDEBUG2) {
    const auto modifier = Modifier {Layout {}};
    const auto &circuit = modifier.circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
}

TEST(ECModifierConfig, VerifyEditableCircuitNDEBUG1) {
    const auto editable_circuit = EditableCircuit {};
    const auto &circuit = editable_circuit.modifier().circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
}

TEST(ECModifierConfig, VerifyEditableCircuitNDEBUG2) {
    const auto editable_circuit = EditableCircuit {Layout {}};
    const auto &circuit = editable_circuit.modifier().circuit_data();

#ifdef NDEBUG
    ASSERT_EQ(circuit.message_validator.has_value(), false);
#else
    ASSERT_EQ(circuit.message_validator.has_value(), true);
#endif
}

//
// Message Verification Config
//

TEST(ECModifierConfig, VerifierOverwriteDefault1) {
    const auto modifier = Modifier {
        Layout {},
        ModifierConfig {.validate_messages = true},
    };
    const auto &circuit = modifier.circuit_data();

    ASSERT_EQ(circuit.message_validator.has_value(), true);
}

TEST(ECModifierConfig, VerifierOverwriteDefault2) {
    const auto modifier = Modifier {
        ModifierConfig {.validate_messages = true},
    };
    const auto &circuit = modifier.circuit_data();

    ASSERT_EQ(circuit.message_validator.has_value(), true);
}

TEST(ECModifierConfig, VerifierOverwriteDefaultEC1) {
    const auto editable_circuit = EditableCircuit {
        Layout {},
        ModifierConfig {.validate_messages = true},
    };
    const auto &circuit = editable_circuit.modifier().circuit_data();

    ASSERT_EQ(circuit.message_validator.has_value(), true);
}

TEST(ECModifierConfig, VerifierOverwriteDefaultEC2) {
    const auto editable_circuit = EditableCircuit {
        ModifierConfig {.validate_messages = true},
    };
    const auto &circuit = editable_circuit.modifier().circuit_data();

    ASSERT_EQ(circuit.message_validator.has_value(), true);
}

//
// Test Helper Config
//

TEST(ECModifierConfig, TestHelperModifierConfig) {
    {
        const auto modifier = get_modifier();
        ASSERT_EQ(modifier.circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(modifier.circuit_data().store_messages, false);
    }
    {
        const auto modifier = get_modifier(Layout {});
        ASSERT_EQ(modifier.circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(modifier.circuit_data().store_messages, false);
    }
    {
        const auto modifier = get_logging_modifier();
        ASSERT_EQ(modifier.circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(modifier.circuit_data().store_messages, true);
    }
    {
        const auto modifier = get_logging_modifier(Layout {});
        ASSERT_EQ(modifier.circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(modifier.circuit_data().store_messages, true);
    }
}

TEST(ECModifierConfig, TestHelperECConfig) {
    {
        const auto ec = get_editable_circuit();
        ASSERT_EQ(ec.modifier().circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(ec.modifier().circuit_data().store_messages, false);
    }
    {
        const auto ec = get_editable_circuit(Layout {});
        ASSERT_EQ(ec.modifier().circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(ec.modifier().circuit_data().store_messages, false);
    }
    {
        const auto ec = get_logging_editable_circuit();
        ASSERT_EQ(ec.modifier().circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(ec.modifier().circuit_data().store_messages, true);
    }
    {
        const auto ec = get_logging_editable_circuit(Layout {});
        ASSERT_EQ(ec.modifier().circuit_data().message_validator.has_value(), true);
        ASSERT_EQ(ec.modifier().circuit_data().store_messages, true);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
