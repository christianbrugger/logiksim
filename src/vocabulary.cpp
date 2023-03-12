#include "vocabulary.h"

namespace logicsim {

auto format(ElementType type) -> std::string {
    switch (type) {
        using enum ElementType;

        case placeholder:
            return "Placeholder";
        case wire:
            return "Wire";
        case inverter_element:
            return "Inverter";
        case and_element:
            return "AndElement";
        case or_element:
            return "OrElement";
        case xor_element:
            return "XorElement";
        case clock_generator:
            return "ClockGenerator";
        case flipflop_jk:
            return "JK-FlipFlop";
        case shift_register:
            return "ShiftRegister";
    }
    throw_exception("Don't know how to convert ElementType to string.");
}

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

auto circuit_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto element_key_t::format() const -> std::string {
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

constexpr segment_t::operator bool() const noexcept {
    return element_id != null_element;
}

template <typename T>
auto format_microsecond_time(T time_value) {
    if (-1us < time_value && time_value < 1us) {
        return fmt::format("{}ns", time_value.count());
    }
    auto time_us = std::chrono::duration<double, std::micro> {time_value};
    return fmt::format("{:L}us", time_us.count());
}

auto time_t::format() const -> std::string {
    return format_microsecond_time(value);
}

auto delay_t::format() const -> std::string {
    return format_microsecond_time(value);
}

auto color_t::format() const -> std::string {
    return fmt::format("{:X}", value);
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

auto rect_t::format() const -> std::string {
    return fmt::format("Rect({}, {})", p0, p1);
}

auto rect_fine_t::format() const -> std::string {
    return fmt::format("RectFine({}, {})", p0, p1);
}

}  // namespace logicsim