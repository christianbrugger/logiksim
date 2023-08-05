#ifndef LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_FORWARD_H
#define LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_FORWARD_H

#include <variant>

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
struct SegmentInsertedstruct;
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

}  // namespace editable_circuit
}  // namespace logicsim

#endif