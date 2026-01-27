#ifndef _SIIT_QUARTZ_SCHEDULER_H_
#define _SIIT_QUARTZ_SCHEDULER_H_

#include "Quartz.h"
#include "JobFactory.h"
#include "Trigger.h"
#include "IntervalTrigger.h"
#include "CronTrigger.h"
#include "JobStore.h"
#include "ScheduledJob.h"
#include "foundation/ThreadPool.h"
#include "foundation/Runnable.h"
#include "foundation/Mutex.h"
#include "foundation/Condition.h"
#include <map>
#include <atomic>
#include <thread>

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API Scheduler : public Runnable
        {
        public:
            explicit Scheduler(std::shared_ptr<JobStore> store);
            explicit Scheduler(std::shared_ptr<JobStore> store, ThreadPool& pool);

            std::string schedule(std::shared_ptr<Job> job, std::shared_ptr<Trigger> trigger, MisfirePolicy misfire = MisfirePolicy::FIRE_NOW);

            void schedule(const std::string& key, std::shared_ptr<Job> job, std::shared_ptr<Trigger> trigger, MisfirePolicy misfire);

            void start();
            void stop();

            void run() override;

        private:
            bool addJob(const std::string& key, std::shared_ptr<Job> job);
            std::string genJobId();
        private:
            std::map<std::string, std::shared_ptr<Job>> _jobs;
            std::map<std::string, ScheduledJob> _runtime;
            std::shared_ptr<JobStore> _store;

            Thread _thread;
            ThreadPool& _pool;
            Mutex _mtx;
            Condition _cv;
            std::atomic<bool> _running;

            void handleMisfire(ScheduledJob& rj, const DateTime& now);
            void fireOnce(ScheduledJob& rj, const DateTime& now);
            void fireAt(ScheduledJob& rj, const DateTime& fireTime);

            static std::string misfireToString(MisfirePolicy p);
        };
    }
}
#endif // !_SIIT_QUARTZ_SCHEDULER_H_
