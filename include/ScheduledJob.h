#ifndef _SIIT_QUARTZ_SCHEDULED_JOB_H_
#define _SIIT_QUARTZ_SCHEDULED_JOB_H_

#include "Quartz.h"
#include "Trigger.h"
#include "Job.h"
#include "JobStore.h"
#include "foundation/SharedPtr.h"
#include "foundation/Runnable.h"

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

        // Job执行任务
        class JobTask : public Runnable
        {
        public:
            JobTask(Job::Ptr job, const std::string& jobKey)
                : _job(job)
                , _jobKey(jobKey)
            {
            }

            ~JobTask()
            {
            }

            void run() override
            {
                if (!_job) {
                    return;
                }

                try {
                    _job->execute();
                }
                catch (const Exception& exc)
                {
                    throw;
                }
                catch (const std::exception& exc)
                {
                    throw;
                }
                catch (...) {
                    throw;
                }
            }

        private:
            Job::Ptr _job;
            std::string _jobKey;
        };

        class ScheduledJob
        {
        public:
            using Ptr = siit::SharedPtr<ScheduledJob>;
            ScheduledJob(Job::Ptr job, const std::string& jobKey);

            std::string jobKey;
            Job::Ptr job;
            Trigger::Ptr trigger;
            MisfirePolicy misfire = MisfirePolicy::FIRE_NOW;

            DateTime lastFire;
            DateTime nextFire;
            bool hasLast = false;
            bool hasNext = false;
            bool paused = false;
            bool cancelled = false;
            JobTask _task;
            // 用于优先队列比较
            bool operator>(const ScheduledJob& other) const
            {
                return nextFire > other.nextFire;
            }
        };
    }
}
#endif // !_SIIT_QUARTZ_SCHEDULED_JOB_H_
