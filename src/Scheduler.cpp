#include "include/Scheduler.h"
#include "foundation/Timespan.h"
#include "foundation/Thread.h"
#include <iostream>

namespace siit
{
    namespace quartz
    {
        Scheduler::Scheduler(std::shared_ptr<JobStore> store)
            : _store(std::move(store))
            , _pool(ThreadPool::defaultPool())
            , _running(false)
        {

        }

        Scheduler::Scheduler(std::shared_ptr<JobStore> store, ThreadPool& pool)
            : _store(std::move(store))
            , _pool(pool)
            , _running(false)
        {
        }

        std::string Scheduler::schedule(std::shared_ptr<Job> job, std::shared_ptr<Trigger> trigger, MisfirePolicy misfire)
        {
            if (!job || !trigger)
            {
                throw std::invalid_argument("job/trigger is null");
            }

            Mutex::ScopedLock lock(_mtx);

            std::string key = genJobId();
            while (!addJob(key, job))
            {
                key = genJobId(); // 极小概率冲突时重试
            }

            ScheduledJob rj;
            rj.jobKey = key;
            rj.job = _jobs.at(key);
            rj.trigger = trigger;
            rj.misfire = misfire;

            rj.nextFire = trigger->nextFireTime(DateTime());
            rj.hasNext = true;

            _runtime[key] = rj;

            PersistedTrigger pt;
            pt.jobKey = key;
            pt.triggerType = trigger->type();
            pt.triggerExpr = trigger->expr();
            pt.misfirePolicy = misfireToString(misfire);
            pt.hasNext = true;
            pt.nextFireTime = rj.nextFire;
            _store->saveTrigger(pt);

            _cv.signal();

            return key;
        }

        void Scheduler::schedule(const std::string& key, std::shared_ptr<Job> job, std::shared_ptr<Trigger> trigger, MisfirePolicy misfire)
        {
            if (!job || !trigger)
            {
                throw std::invalid_argument("job/trigger is null");
            }

            Mutex::ScopedLock lock(_mtx);

            if (!addJob(key, job))
            {
                throw std::invalid_argument("job key already exists: " + key);
            }

            ScheduledJob rj;
            rj.jobKey = key;
            rj.job = _jobs.at(key);
            rj.trigger = trigger;
            rj.misfire = misfire;
            rj.nextFire = trigger->nextFireTime(DateTime());
            rj.hasNext = true;

            _runtime[key] = rj;

            PersistedTrigger pt;
            pt.jobKey = key;
            pt.triggerType = trigger->type();
            pt.triggerExpr = trigger->expr();
            pt.misfirePolicy = misfireToString(misfire);
            pt.hasNext = true;
            pt.nextFireTime = rj.nextFire;
            _store->saveTrigger(pt);

            _cv.signal();
        }

        void Scheduler::start()
        {
            _running = true;
            _thread.start(*this);
        }

        void Scheduler::stop()
        {
            _running = false;
            _cv.signal();
            _thread.join();
        }

        void Scheduler::run()
        {
            try
            {
                while (_running)
                {
                    // 选出“下一个要触发”的任务
                    ScheduledJob* nextJob = nullptr;
                    DateTime now;
                    DateTime soonest;

                    {
                        Mutex::ScopedLock lock(_mtx);
                        now = DateTime();
                        for (auto& kv : _runtime)
                        {
                            auto& rj = kv.second;
                            if (rj.paused || !rj.hasNext)
                            {
                                continue;
                            }

                            // 遍历所有任务，挑出 nextFire 最早的那个
                            if (!nextJob || rj.nextFire < soonest)
                            {
                                soonest = rj.nextFire;
                                nextJob = &rj;
                            }
                        }
                    }

                    if (!nextJob)
                    {
                        Mutex::ScopedLock lock(_mtx);
                        _cv.wait(_mtx);
                        continue;
                    }

                    // 若还没到触发时间，则等待到那一刻
                    now = DateTime();
                    if (now < nextJob->nextFire)
                    {
                        Timespan wait = nextJob->nextFire - now;
                        Mutex::ScopedLock lock(_mtx);
                        _cv.tryWait(_mtx, wait.totalMilliseconds());
                        continue;
                    }

                    handleMisfire(*nextJob, now);
                }
            }
            catch (const std::exception&)
            {
                std::cout << "error" << std::endl;
            }
        }

        void Scheduler::handleMisfire(ScheduledJob& rj, const DateTime& now)
        {
            if (!rj.hasNext) return;

            if (rj.misfire == MisfirePolicy::FIRE_NOW)
            {
                fireOnce(rj, now);
                
                return;
            }

            if (rj.misfire == MisfirePolicy::SKIP)
            {
                rj.lastFire = rj.nextFire;
                rj.hasLast = true;
                rj.nextFire = rj.trigger->nextFireTime(now);
                rj.hasNext = true;
                _store->updateFireTimes(rj.jobKey, rj.lastFire, rj.nextFire, true, true);
                
                return;
            }

            // CATCH_UP
            if (auto intervalTrigger = std::dynamic_pointer_cast<IntervalTrigger>(rj.trigger))
            {
                Timespan interval = intervalTrigger->interval();
                Timespan diff = now - rj.nextFire;
                long count = diff.totalSeconds() / interval.totalSeconds() + 1;
                for (long i = 0; i < count; ++i)
                {
                    fireAt(rj, rj.nextFire + interval.seconds() * i);
                }

                rj.lastFire = rj.nextFire + interval.seconds() * (count - 1);
                rj.hasLast = true;
                rj.nextFire = rj.lastFire + interval;
                rj.hasNext = true;
                _store->updateFireTimes(rj.jobKey, rj.lastFire, rj.nextFire, true, true);
                
                return;
            }
            else
            {
                DateTime t = rj.nextFire;
                while (t <= now)
                {
                    fireAt(rj, t);
                    rj.lastFire = t;
                    rj.hasLast = true;
                    t = rj.trigger->nextFireTime(t);
                }
                rj.nextFire = t;
                rj.hasNext = true;
                _store->updateFireTimes(rj.jobKey, rj.lastFire, rj.nextFire, rj.hasLast, rj.hasNext);
            }
        }

        void Scheduler::fireOnce(ScheduledJob& rj, const DateTime& now)
        {
            fireAt(rj, rj.nextFire);
            rj.lastFire = rj.nextFire;
            rj.hasLast = true;
            rj.nextFire = rj.trigger->nextFireTime(now);
            rj.hasNext = true;
            _store->updateFireTimes(rj.jobKey, rj.lastFire, rj.nextFire, true, true);
        }

        class JobTask : public Runnable
        {
        public:
            explicit JobTask(std::shared_ptr<Job> job) : _job(std::move(job))
            {
            }

            void run() override
            {
                _job->execute();
            }
        private:
            std::shared_ptr<Job> _job;
        };

        void Scheduler::fireAt(ScheduledJob& rj, const DateTime&)
        {
            _pool.start(*new JobTask(rj.job)); // 简化：不回收任务对象（演示）
        }

        std::string Scheduler::misfireToString(MisfirePolicy p)
        {
            switch (p) {
            case MisfirePolicy::FIRE_NOW: return "FIRE_NOW";
            case MisfirePolicy::CATCH_UP: return "CATCH_UP";
            case MisfirePolicy::SKIP: return "SKIP";
            }
            return "FIRE_NOW";
        }

        bool Scheduler::addJob(const std::string& key, std::shared_ptr<Job> job)
        {
            if (!job)
            {
                return false;
            }

            std::pair<std::map<std::string, std::shared_ptr<Job>>::iterator, bool> ret = _jobs.emplace(key, std::move(job));

            return ret.second; // false 表示 key 已存在
        }

        std::string Scheduler::genJobId()
        {
            static std::atomic<uint64_t> seq{ 1 };
            uint64_t id = seq.fetch_add(1, std::memory_order_relaxed);

            return "job-" + std::to_string(id);
        }
    }
}