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
#include <queue>

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API Scheduler : public Runnable
        {
        public:
            using Ptr = siit::SharedPtr<Scheduler>;

            // 构造函数
            explicit Scheduler(JobStore::Ptr store);
            Scheduler(JobStore::Ptr store, ThreadPool& pool);

            // 禁止拷贝和移动
            Scheduler(const Scheduler&) = delete;
            Scheduler& operator=(const Scheduler&) = delete;
            Scheduler(Scheduler&&) = delete;
            Scheduler& operator=(Scheduler&&) = delete;

            // 析构函数
            ~Scheduler();

            // 任务调度接口
            std::string schedule(Job::Ptr job, Trigger::Ptr trigger, MisfirePolicy misfire = MisfirePolicy::FIRE_NOW);

            void schedule(const std::string& key, Job::Ptr job, Trigger::Ptr trigger, MisfirePolicy misfire = MisfirePolicy::FIRE_NOW);

            // 任务管理接口
            bool cancel(const std::string& key);
            bool pause(const std::string& key);
            bool resume(const std::string& key);
            bool deleteJob(const std::string& key);

            // 调度器控制
            void start();
            void stop(bool waitForJobsToComplete = true);

            // 状态查询
            bool isRunning() const;

            bool isShutdown() const;

            size_t getJobCount() const;
            bool jobExists(const std::string& key) const;
            DateTime getNextFireTime(const std::string& key) const;
            std::vector<std::string> getJobKeys() const;

            // Runnable接口
            void run() override;
        private:
            struct QueueEntry
            {
                ScheduledJob::Ptr job;
                DateTime nextFire;
                bool valid = true;

                bool operator>(const QueueEntry& other) const
                {
                    return nextFire > other.nextFire;
                }
            };

            // 优先队列类型
            using JobQueue = std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>>;

            // 私有方法
            void addJobToQueue(const ScheduledJob::Ptr& job);
            void removeJobFromQueue(const std::string& jobKey);
            void rebuildQueue();

            void handleMisfire(ScheduledJob::Ptr job, const DateTime& now);
            void fireJob(ScheduledJob::Ptr job, const DateTime& scheduledTime);

            std::string generateJobId();

            static std::string misfirePolicyToString(MisfirePolicy policy);
            static MisfirePolicy stringToMisfirePolicy(const std::string& str);
        private:
            // 存储
            JobStore::Ptr _store;
            ThreadPool& _threadPool;

            // 任务存储
            std::map<std::string, Job::Ptr> _jobs;
            std::map<std::string, ScheduledJob::Ptr> _scheduledJobs;

            // 任务队列
            JobQueue _jobQueue;

            // 线程和同步
            Thread _schedulerThread;
            mutable FastMutex _mutex;
            Condition _condition;

            // 状态标志
            std::atomic<bool> _running{ false };
            std::atomic<bool> _shutdown{ false };
            std::atomic<bool> _paused{ false };
        };

        //inline
        inline bool Scheduler::isRunning() const
        {
            return _running.load();
        }

        inline bool Scheduler::isShutdown() const
        {
            return _shutdown.load();
        }
    }
}
#endif // !_SIIT_QUARTZ_SCHEDULER_H_
