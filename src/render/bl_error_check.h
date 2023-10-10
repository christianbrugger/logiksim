#ifndef LOGICSIM_RENDER_BL_ERROR_CHECK_H
#define LOGICSIM_RENDER_BL_ERROR_CHECK_H

class BLContext;

namespace logicsim {

/**
 * @brief: Check the error flags of the context.
 *
 * Raises a std::runtime_error on errror.
 */
auto check_errors(const BLContext& ctx) -> void;

/**
 * @brief: Sync and check for errors flags.
 *
 * Raises a std::runtime_error on errror.
 */
auto checked_sync(BLContext& ctx) -> void;

}  // namespace logicsim

#endif
