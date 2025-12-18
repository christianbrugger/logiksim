#include "core/geometry/layout_calculation.h"

#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/rect.h"
#include "core/vocabulary/rect_fine.h"

#include <blend2d/blend2d.h>

#include <exception>

namespace logicsim {

auto connector_point(point_t position, orientation_t orientation, grid_fine_t offset)
    -> point_fine_t {
    const auto p0 = point_fine_t {position};

    switch (orientation) {
        using enum orientation_t;

        case right: {
            return point_fine_t {p0.x + offset, p0.y};
        }
        case left: {
            return point_fine_t {p0.x - offset, p0.y};
        }
        case up: {
            return point_fine_t {p0.x, p0.y - offset};
        }
        case down: {
            return point_fine_t {p0.x, p0.y + offset};
        }

        case undirected: {
            return p0;
        }
    };
    std::terminate();
}

auto connector_point(BLPoint position, orientation_t orientation, double offset)
    -> BLPoint {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return BLPoint {position.x + offset, position.y};
        }
        case left: {
            return BLPoint {position.x - offset, position.y};
        }
        case up: {
            return BLPoint {position.x, position.y - offset};
        }
        case down: {
            return BLPoint {position.x, position.y + offset};
        }

        case undirected: {
            return position;
        }
    };
    std::terminate();
}

auto transform(point_t element_position, orientation_t orientation, point_t offset)
    -> point_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return element_position + offset;
        }
        case left: {
            throw std::runtime_error("Please implement.");
        }
        case up: {
            throw std::runtime_error("Please implement.");
        }
        case down: {
            throw std::runtime_error("Please implement.");
        }
        case undirected: {
            return element_position + offset;
        }
    }
    std::terminate();
}

auto transform(point_t element_position, orientation_t orientation, point_fine_t offset)
    -> point_fine_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return point_fine_t {element_position} + offset;
        }
        case left: {
            throw std::runtime_error("Please implement.");
        }
        case up: {
            throw std::runtime_error("Please implement.");
        }
        case down: {
            throw std::runtime_error("Please implement.");
        }
        case undirected: {
            return point_fine_t {element_position} + offset;
        }
    }
    std::terminate();
}

auto transform(orientation_t element_orientation, orientation_t connector)
    -> orientation_t {
    switch (element_orientation) {
        using enum orientation_t;

        case right: {
            return connector;
        }
        case left: {
            throw std::runtime_error("Please implement.");
        }
        case up: {
            throw std::runtime_error("Please implement.");
        }
        case down: {
            throw std::runtime_error("Please implement.");
        }
        case undirected: {
            return connector;
        }
    }
    std::terminate();
}

auto transform(point_t position, orientation_t orientation, rect_t rect) -> rect_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return rect_t {position + rect.p0, position + rect.p1};
        }
        case left: {
            throw std::runtime_error("Please implement.");
        }
        case up: {
            throw std::runtime_error("Please implement.");
        }
        case down: {
            throw std::runtime_error("Please implement.");
        }
        case undirected: {
            return rect_t {position + rect.p0, position + rect.p1};
        }
    }
    std::terminate();
}

auto transform(point_t position, orientation_t orientation, rect_fine_t rect)
    -> rect_fine_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return rect_fine_t {point_fine_t {position} + rect.p0,
                                point_fine_t {position} + rect.p1};
        }
        case left: {
            throw std::runtime_error("Please implement.");
        }
        case up: {
            throw std::runtime_error("Please implement.");
        }
        case down: {
            throw std::runtime_error("Please implement.");
        }
        case undirected: {
            return rect_fine_t {point_fine_t {position} + rect.p0,
                                point_fine_t {position} + rect.p1};
        }
    }
    std::terminate();
}

}  // namespace logicsim
