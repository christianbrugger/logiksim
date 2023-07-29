#ifndef LOGIKSIM_SERIALIZE_H
#define LOGIKSIM_SERIALIZE_H

#include "layout.h"

namespace logicsim {

auto serialize_json(const Layout &layout) -> void;

}

#endif
