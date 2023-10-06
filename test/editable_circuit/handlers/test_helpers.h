#ifndef LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H
#define LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H

#include "algorithm/fmt_join.h"
#include "editable_circuit/cache.h"
#include "editable_circuit/handler.h"
#include "logging.h"
#include "vocabulary/element_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

// forward message to cache and store them
class MessageRecorder {
   public:
    MessageRecorder() = default;

    MessageRecorder(CacheProvider &cache_provider) : cache_provider_ {&cache_provider} {}

    inline auto submit(const editable_circuit::InfoMessage &message) -> void {
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

inline auto make_sender(MessageRecorder &recorder) -> editable_circuit::MessageSender {
    const auto callback = [recorder = &recorder](const auto &message) {
        recorder->submit(message);
    };
    return editable_circuit::MessageSender {callback};
}

struct HandlerSetup {
    Layout &layout;
    CacheProvider cache;
    MessageRecorder recorder;
    editable_circuit::MessageSender sender;
    editable_circuit::State state;

    HandlerSetup(Layout &layout_)
        : layout {layout_},
          cache {layout_},
          recorder {cache},
          sender {make_sender(recorder)},
          state {layout_, sender, cache} {
        validate();
    }

    auto validate() -> void {
        layout.validate();
        cache.validate(layout);
    }
};

struct SenderSetup {
    MessageRecorder recorder;
    editable_circuit::MessageSender sender;

    SenderSetup() : recorder {}, sender {make_sender(recorder)} {}
};

inline auto add_and_element(Layout &layout, display_state_t display_type,
                            connection_count_t input_count = connection_count_t {3},
                            point_t position = point_t {0, 0}) -> element_id_t {
    const auto definition = ElementDefinition {
        .element_type = ElementType::and_element,

        .input_count = input_count,
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return layout.add_element(definition, position, display_type);
}

inline auto assert_element_count(const Layout &layout, std::size_t count) -> void {
    ASSERT_EQ(layout.element_count(), count);
}

inline auto assert_element_equal(const Layout &layout, element_id_t element_id,
                                 connection_count_t input_count = connection_count_t {3},
                                 point_t position = point_t {0, 0}) -> void {
    ASSERT_EQ(layout.element(element_id).input_count(), input_count);
    ASSERT_EQ(layout.position(element_id), position);
}

}  // namespace logicsim

#endif