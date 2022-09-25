
#include "simulation.h"


#include <format>

namespace logicsim {

    std::string LogicEvent::format() const
    {
        auto element_id_str = (element == null_element) ? "NULL" : std::format("{}", element);
        return std::format("<LogicEvent_{}: at {}s set Element_{}[{}] = {}>",
            event_id, time, element_id_str, output, (value ? "true" : "false"));
    }


    bool LogicEvent::operator==(const LogicEvent& other) const
    {
        return this->event_id == other.event_id;
    }
    bool LogicEvent::operator<(const LogicEvent& other) const
    {
        if (this->time == other.time)
            return this->event_id < other.event_id;
        else
            return this->time < other.time;
    }


    bool LogicEvent::operator!=(const LogicEvent& other) const
    {
        return !(this->operator==(other));
    }
    bool LogicEvent::operator>(const LogicEvent& other) const
    {
        return other.operator<(*this);
    }
    bool LogicEvent::operator<=(const LogicEvent& other) const
    {
        return !(this->operator>(other));
    }
    bool LogicEvent::operator>=(const LogicEvent& other) const
    {
        return !(this->operator<(other));
    }

}

