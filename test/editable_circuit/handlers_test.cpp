
#include "editable_circuit/handlers.h"

#include "editable_circuit/caches.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

auto empty_circuit() -> Circuit {
    return Circuit {Schematic {}, Layout {}};
}

struct HandlerSetup {
    editable_circuit::RecordingReceiver receiver;
    CacheProvider cache;
    editable_circuit::State state;

    HandlerSetup(Circuit &circuit)
        : receiver {},
          cache {circuit},
          state {circuit, editable_circuit::MessageSender {receiver}, cache,
                 circuit.schematic(), circuit.layout()} {
        circuit.validate();
    }
};

TEST(EditableCircuitHandler, VerificationLogic) {
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

    circuit.validate();
    ASSERT_EQ(setup.state.layout.empty(), true);
    ASSERT_EQ(setup.state.schematic.empty(), true);
    ASSERT_EQ(setup.receiver.messages().empty(), true);
}

}  // namespace logicsim