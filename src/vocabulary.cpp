#include "vocabulary.h"

#include "format.h"

#include <blend2d/rgba.h>

namespace logicsim {

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