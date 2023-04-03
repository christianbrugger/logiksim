
#include "editable_circuit/handlers.h"

#include "editable_circuit/caches.h"
#include "format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

auto empty_circuit() -> Circuit {
    return Circuit {Schematic {}, Layout {}};
}

struct HandlerSetup {
    Circuit &circuit;
    CacheProvider cache;
    editable_circuit::MessageRecorder recorder;
    editable_circuit::MessageSender sender;
    editable_circuit::State state;

    HandlerSetup(Circuit &circuit)
        : circuit {circuit},
          cache {circuit},
          recorder {},
          sender {editable_circuit::MessageSender {recorder}},
          state {circuit, sender, cache, circuit.schematic(), circuit.layout()} {
        validate();
    }

    auto validate() -> void {
        circuit.validate();
        cache.validate(circuit);
    }
};

auto add_temporary_and_element(Circuit &circuit) {
    circuit.schematic().add_element(Schematic::NewElementData {
        .element_type = ElementType::and_element,
        .input_count = 3,
        .output_count = 1,
    });
    circuit.layout().add_logic_element(point_t {0, 0}, orientation_t::right,
                                       display_state_t::new_temporary);
}

TEST(EditableCircuitHandler, VerificationSetup) {
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

    setup.validate();
    ASSERT_EQ(setup.state.layout.empty(), true);
    ASSERT_EQ(setup.state.schematic.empty(), true);
    ASSERT_EQ(setup.recorder.messages().empty(), true);
}

TEST(EditableCircuitHandler, DeleteTemporaryElement) {
    using namespace editable_circuit::info_message;
    auto circuit = empty_circuit();
    add_temporary_and_element(circuit);

    auto setup = HandlerSetup {circuit};
    auto element_id = element_id_t {0};
    auto preserved_id = element_id_t {0};
    swap_and_delete_single_element(circuit, setup.sender, element_id, &preserved_id);

    setup.validate();
    ASSERT_EQ(setup.state.layout.empty(), true);
    ASSERT_EQ(setup.state.schematic.empty(), true);
    ASSERT_EQ(element_id, null_element);
    ASSERT_EQ(preserved_id, null_element);
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0),
              Message {ElementDeleted {element_id_t {0}}});
}

}  // namespace logicsim