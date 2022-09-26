#include "algorithms.h"

namespace logicsim {

	[[noreturn]] void throw_exception(const char *msg) {
		throw std::runtime_error(msg);
	}

}
