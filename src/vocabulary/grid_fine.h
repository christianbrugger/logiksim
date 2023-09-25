#ifndef LOGICSIM_VOCABULARY_GRID_FINE_H
#define LOGICSIM_VOCABULARY_GRID_FINE_H

namespace logicsim {

/**
 * @brief: A continuous location on the grid in one dimension.
 * 
 * TODO make this a strong type
 */
using grid_fine_t = double;

// Would float enough for our fine grid?
// The highest representable float integer is 2**24 = 16'777'216.
// At 2**15 = 32'768 we have 9 fractional bits, a resolution of 2**-9 = 0.001953125.
// So yes, but we want to interoperate with blend2d which is double based.

}  // namespace logicsim

#endif
