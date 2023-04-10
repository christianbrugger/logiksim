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
    circuit.schematic().add_element(Schematic::NewElementData {
        .element_type = ElementType::and_element,
        .input_count = input_count,
        .output_count = 1,
    });
    return circuit.layout().add_logic_element(position, orientation_t::right,
                                              display_type);
}

inline auto add_placeholder(Circuit &circuit) -> Schematic::Element {
    const auto element = circuit.schematic().add_element(Schematic::NewElementData {
        .element_type = ElementType::placeholder,
        .input_count = 1,
        .output_count = 0,
    });
    circuit.layout().add_placeholder(display_state_t::normal);
    return element;
}

inline auto add_placeholder(Circuit &circuit, Schematic::Output output) -> element_id_t {
    const auto element = add_placeholder(circuit);
    element.input(connection_id_t {0}).connect(output);
    return element.element_id();
}

inline auto add_placeholders(Circuit &circuit, element_id_t element_id) -> void {
    for (auto output : circuit.schematic().element(element_id).outputs()) {
        add_placeholder(circuit, output);
    }
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

inline auto assert_is_placeholder(const Circuit &circuit, element_id_t element_id)
    -> void {
    ASSERT_EQ(circuit.schematic().element(element_id).is_placeholder(), true);
    ASSERT_EQ(circuit.layout().display_state(element_id), display_state_t::normal);
}

}  // namespace logicsim

#endif