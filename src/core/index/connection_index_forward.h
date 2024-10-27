#ifndef LOGICSIM_CORE_INDEX_CONNECTION_INDEX_FORWARD_H
#define LOGICSIM_CORE_INDEX_CONNECTION_INDEX_FORWARD_H

namespace logicsim {

namespace connection_index {

enum class ContentType { LogicItem, Wire };
enum class DirectionType { Input, Output };

}  // namespace connection_index

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
class ConnectionIndex;

// Definitions

using LogicItemInputIndex = ConnectionIndex<connection_index::ContentType::LogicItem,
                                            connection_index::DirectionType::Input>;
using LogicItemOutputIndex = ConnectionIndex<connection_index::ContentType::LogicItem,
                                             connection_index::DirectionType::Output>;

using WireInputIndex = ConnectionIndex<connection_index::ContentType::Wire,
                                       connection_index::DirectionType::Input>;
using WireOutputIndex = ConnectionIndex<connection_index::ContentType::Wire,
                                        connection_index::DirectionType::Output>;

}  // namespace logicsim

#endif
