#include "include/IntervalTrigger.h"

namespace siit
{
    namespace quartz
    {
        IntervalTrigger::IntervalTrigger(Timespan interval) : _interval(interval)
        {
        }

        DateTime IntervalTrigger::nextFireTime(const DateTime& after)
        {
            return after + _interval;
        }

        std::string IntervalTrigger::type() const
        {
            return "interval";
        }

        std::string IntervalTrigger::expr() const
        {
            return std::to_string(_interval.totalSeconds());
        }

        Timespan IntervalTrigger::interval() const
        {
            return _interval;
        }
    }
}
