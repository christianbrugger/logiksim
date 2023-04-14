#include "./test_helpers.h"
#include "editable_circuit/handler_examples.h"
#include "editable_circuit/handlers.h"
#include "timer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

auto add_many_wires(Rng& rng, bool random_modes) {
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

    editable_circuit::examples::add_many_wires(rng, setup.state, random_modes);

    setup.validate();
}

TEST(HandlerWireFuzz, AddTempSegmentRandomModes) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        add_many_wires(rng, true);
    }
}

TEST(HandlerWireFuzz, AddTempSegmentInsertionModes) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        add_many_wires(rng, false);
    }
}

}  // namespace logicsim