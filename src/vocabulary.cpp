#include "vocabulary.h"

#include <blend2d/rgba.h>

namespace logicsim {

template <>
auto format(ElementType type) -> std::string {
    switch (type) {
        using enum ElementType;

        case unused:
            return "Unused";
        case placeholder:
            return "Placeholder";
        case wire:
            return "Wire";

        case buffer_element:
            return "Buffer";
        case and_element:
            return "AndElement";
        case or_element:
            return "OrElement";
        case xor_element:
            return "XorElement";

        case button:
            return "Button";
        case led:
            return "LED";
        case display_number:
            return "display_number";
        case display_ascii:
            return "display_ascii";

        case clock_generator:
            return "ClockGenerator";
        case flipflop_jk:
            return "JK-FlipFlop";
        case shift_register:
            return "ShiftRegister";

        case latch_d:
            return "D-Latch";
        case flipflop_d:
            return "D-FlipFlop";
        case flipflop_ms_d:
            return "MS-D-FlipFlop";

        case sub_circuit:
            return "SubCircuit";
    }
    throw_exception("Don't know how to convert ElementType to string.");
}

auto is_logic_item(ElementType element_type) -> bool {
    using enum ElementType;
    return element_type != unused && element_type != placeholder && element_type != wire;
}

template <>
auto format(orientation_t orientation) -> std::string {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return "right";
        case left:
            return "left";
        case up:
            return "up";
        case down:
            return "down";

        case undirected:
            return "undirected";
    }
    throw_exception("Don't know how to convert orientation_t to string.");
}

template <>
auto format(display_state_t state) -> std::string {
    switch (state) {
        using enum display_state_t;

        case normal:
            return "normal";

        case valid:
            return "valid";
        case colliding:
            return "colliding";

        case temporary:
            return "temporary";
    }
    throw_exception("Don't know how to convert display_state_t to string.");
}

auto is_inserted(display_state_t display_state) -> bool {
    return display_state == display_state_t::normal ||
           display_state == display_state_t::valid;
}

template <>
auto format(InsertionMode mode) -> std::string {
    switch (mode) {
        using enum InsertionMode;

        case insert_or_discard:
            return "insert_or_discard";
        case collisions:
            return "collisions";
        case temporary:
            return "temporary";
    }
    throw_exception("Don't know how to convert insertion mode to string.");
}

auto to_insertion_mode(display_state_t display_state) -> InsertionMode {
    switch (display_state) {
        using enum display_state_t;

        case normal:
            return InsertionMode::insert_or_discard;
        case colliding:
            return InsertionMode::collisions;
        case valid:
            return InsertionMode::collisions;
        case temporary:
            return InsertionMode::temporary;
    };

    throw_exception("Unknown display state.");
};

auto circuit_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto element_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto connection_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto connection_t::format() const -> std::string {
    return fmt::format("<Element {}, Conection {}>", element_id, connection_id);
}

auto segment_index_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto segment_t::format() const -> std::string {
    if (!*this) {
        return fmt::format("<NullSegment>", segment_index, element_id);
    }
    return fmt::format("<Element {}, Segment {}>", element_id, segment_index);
}

auto part_t::format() const -> std::string {
    return fmt::format("<part {}-{}>", begin, end);
}

auto part_copy_definition_t::format() const -> std::string {
    return fmt::format("<part_copy_definition destination = {}, source = {}>",
                       destination, source);
}

auto segment_part_t::format() const -> std::string {
    return fmt::format("<Element {}, Segment {}, part {}-{}>", segment.element_id,
                       segment.segment_index, part.begin, part.end);
}

namespace {

template <typename T>
auto format_microsecond_time(T time_value) {
    if (-1us < time_value && time_value < 1us) {
        return fmt::format("{}ns", time_value.count());
    }
    auto time_us = std::chrono::duration<double, std::micro> {time_value};
    return fmt::format("{:L}us", time_us.count());
}

template <typename T>
auto format_time(T time_value) {
    using namespace std::chrono_literals;

    if (-1us < time_value && time_value < 1us) {
        auto time_ns = std::chrono::duration<double, std::nano> {time_value};
        return fmt::format("{:.3g}ns", time_ns.count());
    }

    if (-1ms < time_value && time_value < 1ms) {
        auto time_us = std::chrono::duration<double, std::micro> {time_value};
        return fmt::format("{:.3g}us", time_us.count());
    }

    if (-1s < time_value && time_value < 1s) {
        auto time_ms = std::chrono::duration<double, std::milli> {time_value};
        return fmt::format("{:.3g}ms", time_ms.count());
    }

    auto time_s = std::chrono::duration<double, std::ratio<1>> {time_value};
    return fmt::format("{:.2f}s", time_s.count());
}

}  // namespace

auto time_t::format() const -> std::string {
    return format_microsecond_time(value);
}

auto delay_t::format() const -> std::string {
    // return format_microsecond_time(value);
    return format_time(value);
}

auto time_rate_t::format() const -> std::string {
    return fmt::format("{}/s", format_time(rate_per_second.value));
}

auto color_t::format() const -> std::string {
    return fmt::format("{:X}", value);
}

color_t::operator BLRgba32() const noexcept {
    return BLRgba32 {value};
}

auto grid_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto point_fine_t::format() const -> std::string {
    return fmt::format("[{:.3f}, {:.3f}]", x, y);
}

auto point_t::format() const -> std::string {
    return fmt::format("[{}, {}]", x, y);
}

auto line_t::format() const -> std::string {
    return fmt::format("Line({}, {})", p0, p1);
}

auto ordered_line_t::format() const -> std::string {
    return fmt::format("OrderedLine({}, {})", p0, p1);
}

auto line_fine_t::format() const -> std::string {
    return fmt::format("LineFine({}, {})", p0, p1);
}

auto rect_t::format() const -> std::string {
    return fmt::format("Rect({}, {})", p0, p1);
}

auto rect_fine_t::format() const -> std::string {
    return fmt::format("RectFine({}, {})", p0, p1);
}

auto offset_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim