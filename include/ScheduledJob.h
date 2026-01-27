#ifndef _SIIT_QUARTZ_SCHEDULED_JOB_H_
#define _SIIT_QUARTZ_SCHEDULED_JOB_H_

#include "Quartz.h"
#include "Trigger.h"
#include "Job.h"
#include "JobStore.h"
#include <memory>

namespace siit
{
    namespace quartz
    {
        enum class MisfirePolicy 
        {
            FIRE_NOW,                       /// Immediately fire the misfired job
            CATCH_UP,                       /// Catch up all missed executions
            SKIP                            /// Skip missed executions
        };

        class QUARTZ_API ScheduledJob
        {
        public:
            std::string jobKey;
            std::shared_ptr<Job> job;
            std::shared_ptr<Trigger> trigger;
            MisfirePolicy misfire = MisfirePolicy::FIRE_NOW;

            DateTime lastFire;
            DateTime nextFire;
            bool hasLast = false;
            bool hasNext = false;
            bool paused = false;
        };
    }
}
#endif // !_SIIT_QUARTZ_SCHEDULED_JOB_H_
