#ifndef LOGICSIM_CORE_EXCEPTION_LOAD_ERROR_H
#define LOGICSIM_CORE_EXCEPTION_LOAD_ERROR_H

#include "format/struct.h"

#include <string>

namespace logicsim {

/**
 * @brief: Error while loading file or deserializing data.
 */
class LoadError {
   public:
    explicit LoadError(std::string message);

    [[nodiscard]] auto format() const -> const std::string&;

   private:
    std::string message_ {};
};

}  // namespace logicsim

#endif
