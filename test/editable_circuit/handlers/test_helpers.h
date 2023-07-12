#ifndef LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H
#define LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H

#include "editable_circuit/caches.h"
#include "editable_circuit/handlers.h"
#include "format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

inline auto empty_circuit() -> Circuit {
    return Circuit {Schematic {}, Layout {}};
}

// forward message to cache and store them
class MessageRecorder : public editable_circuit::VirtualReceiver {
   public:
    MessageRecorder() = default;

    MessageRecorder(CacheProvider &cache_provider) : cache_provider_ {&cache_provider} {}

    inline auto submit(editable_circuit::InfoMessage message) -> void override {
        messages_.push_back(message);
        if (cache_provider_) {
            cache_provider_->submit(message);
        }
    }

    inline auto messages() -> const std::vector<editable_circuit::InfoMessage> & {
        return messages_;
    }

    inline auto print() -> void {
        logicsim::print();
        logicsim::print(fmt_join("{}", messages_, "\n"));
        logicsim::print();
    }

   private:
    CacheProvider *cache_provider_ {nullptr};
    std::vector<editable_circuit::InfoMessage> messages_ {};
};

struct HandlerSetup {
    Circuit &circuit;
    CacheProvider cache;
    MessageRecorder recorder;
    editable_circuit::MessageSender sender;
    editable_circuit::State state;

    HandlerSetup(Circuit &circuit_)
        : circuit {circuit_},
          cache {circuit_},
          recorder {cache},
          sender {editable_circuit::MessageSender {recorder}},
          state {circuit_, sender, cache, circuit.schematic(), circuit.layout()} {
        validate();
    }

    auto validate() -> void {
        circuit.validate();
        cache.validate(circuit);
    }
};

struct SenderSetup {
    MessageRecorder recorder;
    editable_circuit::MessageSender sender;

    SenderSetup() : recorder {}, sender {editable_circuit::MessageSender {recorder}} {}
};

inline auto add_and_element(Circuit &circuit, display_state_t display_type,
                            std::size_t input_count = 3,
                            point_t position = point_t {0, 0}) -> element_id_t {
    circuit.schematic().add_element(Schematic::ElementData {
        .element_type = ElementType::and_element,
        .input_count = input_count,
        .output_count = 1,
    });
    return circuit.layout().add_element({
        .display_state = display_type,
        .element_type = ElementType::and_element,

        .input_count = input_count,
        .output_count = 1,
        .position = position,
        .orientation = orientation_t::right,
    });
}

inline auto assert_element_count(const Circuit &circuit, std::size_t count) -> void {
    ASSERT_EQ(circuit.schematic().element_count(), count);
    ASSERT_EQ(circuit.layout().element_count(), count);
}

inline auto assert_element_equal(const Circuit &circuit, element_id_t element_id,
                                 std::size_t input_count = 3,
                                 point_t position = point_t {0, 0}) -> void {
    ASSERT_EQ(circuit.schematic().element(element_id).input_count(), input_count);
    ASSERT_EQ(circuit.layout().position(element_id), position);
}

}  // namespace logicsim

#endif