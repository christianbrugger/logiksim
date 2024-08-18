#ifndef LOGICSIM_VOCABULARY_LOGIC_VECTOR_H
#define LOGICSIM_VOCABULARY_LOGIC_VECTOR_H

#include <boost/container/vector.hpp>

namespace logicsim {

/**
 * @brief: Vector of logic values
 *
 * Note this is a replacement for std::vector<bool>
 */
using logic_vector_t = boost::container::vector<bool>;

}  // namespace logicsim

#endif
