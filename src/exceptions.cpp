#include "exceptions.h"

#include <boost/stacktrace.hpp>

#include <fmt/format.h>


namespace logicsim {

	[[noreturn]] void throw_exception(const char *msg) {
		const auto stacktrace = boost::stacktrace::to_string(boost::stacktrace::stacktrace());
		const auto full_msg = fmt::format("{}\nException: {}\n", stacktrace, msg);
		throw std::runtime_error(full_msg.c_str());
	}

}
