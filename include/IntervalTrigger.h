#ifndef _SIIT_QUARTZ_INTERVAL_TRIGGER_H_
#define _SIIT_QUARTZ_INTERVAL_TRIGGER_H_

#include "Quartz.h"
#include "Trigger.h"
#include "foundation/Timespan.h"

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API IntervalTrigger : public Trigger
        {
        public:
            explicit IntervalTrigger(Timespan interval);

            DateTime nextFireTime(const DateTime& after) override;
            std::string type() const override;
            std::string expr() const override;

            Timespan interval() const;

        private:
            Timespan _interval;
        };
    }
}

#endif // !_SIIT_QUARTZ_INTERVAL_TRIGGER_H_
