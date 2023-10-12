#include "geometry/layout1.h"

#include "vocabulary/grid_fine.h"
#include "vocabulary/point.h"
#include "vocabulary/point_fine.h"
#include "vocabulary/rect.h"

#include <blend2d.h>

#include <exception>

namespace logicsim {

auto orientations_compatible(orientation_t a, orientation_t b) -> bool {
    using enum orientation_t;
    return (a == left && b == right) || (a == right && b == left) ||
           (a == up && b == down) || (a == down && b == up) || (a == undirected) ||
           (b == undirected);
}

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

auto transform(point_t position, orientation_t orientation, point_t p0, point_t p1)
    -> rect_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return rect_t {position + p0, position + p1};
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
            return rect_t {position + p0, position + p1};
        }
    }
    std::terminate();
}

}  // namespace logicsim
