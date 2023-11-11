#ifndef LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H
#define LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H

#include "algorithm/fmt_join.h"
#include "editable_circuit/cache.h"
#include "editable_circuit/handler.h"
#include "editable_circuit/message.h"
#include "editable_circuit/message_sender.h"
#include "layout.h"
#include "logging.h"
#include "vocabulary/logicitem_definition.h"

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
                            point_t position = point_t {0, 0}) -> logicitem_id_t {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::and_element,

        .input_count = input_count,
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return layout.logic_items().add(definition, position, display_type);
}

inline auto assert_logicitem_count(const Layout &layout, std::size_t count) -> void {
    ASSERT_EQ(layout.logic_items().size(), count);
}

inline auto assert_logicitem_equal(
    const Layout &layout, logicitem_id_t logicitem_id,
    connection_count_t input_count = connection_count_t {3},
    point_t position = point_t {0, 0}) -> void {
    ASSERT_EQ(layout.logic_items().input_count(logicitem_id), input_count);
    ASSERT_EQ(layout.logic_items().position(logicitem_id), position);
}

}  // namespace logicsim

#endif