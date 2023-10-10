#ifndef LOGICSIM_RENDER_BL_ERROR_CHECK_H
#define LOGICSIM_RENDER_BL_ERROR_CHECK_H

class BLContext;

namespace logicsim {

/**
 * @brief: Check the error flags of the context.
 *
 * Note that blend2d doesn't raise errors immediately. Only a flag
 * is set within the context. Use this method checks those flags.
 *
 * Raises a std::runtime_error on errror.
 */
auto check_errors(const BLContext& ctx) -> void;

/**
 * @brief: Sync and check for errors flags.
 *
 * Note that blend2d doesn't raise errors immediately. Only a flag
 * is set within the context. Use this method checks those flags when syncing.
 *
 * Raises a std::runtime_error on errror.
 */
auto checked_sync(BLContext& ctx) -> void;

}  // namespace logicsim

#endif
