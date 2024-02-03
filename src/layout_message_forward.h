#ifndef LOGIKSIM_LAYOUT_MESSAGE_FORWARD_H
#define LOGIKSIM_LAYOUT_MESSAGE_FORWARD_H

#include <variant>
#include <vector>

namespace logicsim {
namespace editable_circuit {

namespace info_message {

struct LogicItemCreated;
struct LogicItemIdUpdated;
struct LogicItemDeleted;

struct LogicItemInserted;
struct InsertedLogicItemIdUpdated;
struct LogicItemUninserted;

struct SegmentCreated;
struct SegmentIdUpdated;
struct SegmentPartMoved;
struct SegmentPartDeleted;

struct SegmentInserted;
struct InsertedSegmentIdUpdated;
struct InsertedEndPointsUpdated;
struct SegmentUninserted;

using Message = std::variant<                                                //
    LogicItemCreated, LogicItemDeleted, LogicItemIdUpdated,                  //
    LogicItemInserted, LogicItemUninserted, InsertedLogicItemIdUpdated,      //
    SegmentCreated, SegmentIdUpdated, SegmentPartMoved, SegmentPartDeleted,  //
    SegmentInserted, InsertedSegmentIdUpdated, InsertedEndPointsUpdated,
    SegmentUninserted>;

}  // namespace info_message

using InfoMessage = info_message::Message;

using message_vector_t = std::vector<editable_circuit::InfoMessage>;

}  // namespace editable_circuit
}  // namespace logicsim

#endif