#ifndef LOGICSIM_VOCABULARY_MOUSE_POSTION_INFO_H
#define LOGICSIM_VOCABULARY_MOUSE_POSTION_INFO_H

#include "format/struct.h"
#include "vocabulary/point_device_fine.h"

#include <concepts>
#include <vector>

namespace logicsim {

struct position_source_point_t {
    double x {};
    double y {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const position_source_point_t&) const -> bool = default;
};

static_assert(std::regular<position_source_point_t>);

struct position_sources_entry_t {
    std::string title;
    position_source_point_t position;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const position_sources_entry_t&) const
        -> bool = default;
};

static_assert(std::regular<position_sources_entry_t>);

struct MousePositionInfo {
    point_device_fine_t final_position {};
    std::vector<position_sources_entry_t> sources {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const MousePositionInfo&) const -> bool = default;
};

static_assert(std::regular<MousePositionInfo>);

}  // namespace logicsim

#endif
