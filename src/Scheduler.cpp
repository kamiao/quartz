#include "include/Scheduler.h"
#include "foundation/Timespan.h"
#include "foundation/Thread.h"
#include "foundation/GUIDGenerator.h"
#include "foundation/Format.h"
#include "foundation/Nullable.h"
#include "foundation/RunnableAdapter.h"
#include <iostream>

namespace siit
{
    namespace quartz
    {
        Scheduler::Scheduler(JobStore::Ptr store)
            : _store(store)
            , _threadPool(ThreadPool::defaultPool())
        {
        }

        Scheduler::Scheduler(JobStore::Ptr store, ThreadPool& pool)
            : _store(std::move(store))
            , _threadPool(pool)
        {
        }

        Scheduler::~Scheduler()
        {
            try {
                stop(true);
            }
            catch (...) {
            }
        }

        std::string Scheduler::schedule(Job::Ptr job, Trigger::Ptr trigger, MisfirePolicy misfire)
        {
            if (!job || !trigger)
            {
                throw InvalidArgumentException("Job and trigger cannot be null");
            }

            std::string key = generateJobId();
            schedule(key, job, std::move(trigger), misfire);
            
            return key;
        }

        void Scheduler::schedule(const std::string& key, Job::Ptr job, Trigger::Ptr trigger, MisfirePolicy misfire)
        {
            if (!job || !trigger)
            {
                throw InvalidArgumentException("Job and trigger cannot be null");
            }

            // 检查key是否有效
            if (key.empty())
            {
                throw InvalidArgumentException("Job key cannot be empty");
            }

            FastMutex::ScopedLock lock(_mutex);

            // 检查key是否已存在
            if (_jobs.find(key) != _jobs.end())
            {
                throw ExistsException(siit::format("Job key already exists: %s", key));
            }

            // 计算首次触发时间
            DateTime now;
            DateTime nextFire = trigger->nextFireTime(now);

            if (nextFire == DateTime(1, 1, 1))
            {
                throw InvalidArgumentException("Trigger returned null next fire time");
            }

            // 创建调度任务
            ScheduledJob::Ptr scheduledJob = new ScheduledJob(job, key);
            scheduledJob->trigger = trigger;
            scheduledJob->misfire = misfire;
            scheduledJob->nextFire = nextFire;
            scheduledJob->hasNext = true;

            // 存储任务
            _jobs[key] = job;
            _scheduledJobs[key] = scheduledJob;

            // 添加到队列
            addJobToQueue(scheduledJob);

            // 持久化
            if (_store)
            {
                try {
                    PersistedTrigger pt;
                    pt.jobKey = key;
                    pt.triggerType = trigger->type();
                    pt.triggerExpr = trigger->expr();
                    pt.misfirePolicy = misfirePolicyToString(misfire);
                    pt.hasNext = true;
                    pt.nextFireTime = nextFire;

                    _store->saveTrigger(pt);
                }
                catch (const std::exception& e) {
                }
            }

            // 通知调度线程
            _condition.signal();
        }

        bool Scheduler::cancel(const std::string& key)
        {
            FastMutex::ScopedLock lock(_mutex);

            auto jobIt = _scheduledJobs.find(key);
            if (jobIt == _scheduledJobs.end())
            {
                return false;
            }

            // 可能当前正在执行，设置取消标志
            jobIt->second->cancelled = true;

            // 通知调度线程
            _condition.signal();

            return true;
        }

        bool Scheduler::pause(const std::string& key)
        {
            FastMutex::ScopedLock lock(_mutex);

            auto jobIt = _scheduledJobs.find(key);
            if (jobIt == _scheduledJobs.end())
            {

                return false;
            }

            if (jobIt->second->paused)
            {
                return true; // 已经暂停
            }

            jobIt->second->paused = true;

            // 从队列中移除
            removeJobFromQueue(key);

            // 通知调度线程
            _condition.signal();

            return true;
        }

        bool Scheduler::resume(const std::string& key)
        {
            FastMutex::ScopedLock lock(_mutex);

            auto jobIt = _scheduledJobs.find(key);
            if (jobIt == _scheduledJobs.end())
            {
                return false;
            }

            auto& job = jobIt->second;
            if (!job->paused) {
                return true; // 未暂停
            }

            job->paused = false;

            // 重新添加到队列
            if (job->hasNext && !job->cancelled)
            {
                addJobToQueue(job);
            }

            // 通知调度线程
            _condition.signal();

            return true;
        }

        bool Scheduler::deleteJob(const std::string& key)
        {
            return cancel(key);
        }

        void Scheduler::start()
        {
            if (_running.exchange(true))
            {
                throw IllegalStateException("Scheduler is already running");
            }

            _shutdown.store(false);

            // 启动调度线程
            _schedulerThread.start(*this);
        }

        void Scheduler::stop(bool waitForJobsToComplete)
        {
            if (!_running.exchange(false))
            {
                return; // 已经停止
            }

            _shutdown.store(true);

            // 通知所有等待的线程
            _condition.broadcast();

            // 等待调度线程结束
            if (_schedulerThread.isRunning())
            {
                _schedulerThread.join();
            }

            // 如果需要等待任务完成
            if (waitForJobsToComplete)
            {
                _threadPool.joinAll();
            }
        }

        void Scheduler::run()
        {
            while (_running.load() && !_shutdown.load())
            {
                ScheduledJob::Ptr nextJob;
                DateTime nextFireTime;

                // 阶段1：获取下一个任务
                {
                    // 使用 FastMutex 的 lock() 而不是 ScopedLock
                    _mutex.lock();

                    // 清理无效的队列条目
                    while (!_jobQueue.empty())
                    {
                        const auto& entry = _jobQueue.top();

                        if (!entry.valid)
                        {
                            _jobQueue.pop();
                            continue;
                        }

                        // 检查任务是否仍然有效
                        auto jobIt = _scheduledJobs.find(entry.job->jobKey);
                        if (jobIt == _scheduledJobs.end() ||  jobIt->second->cancelled || jobIt->second->paused)
                        {
                            _jobQueue.pop();
                            continue;
                        }

                        // 找到有效任务
                        nextJob = entry.job;
                        nextFireTime = entry.nextFire;
                        _jobQueue.pop();
                        break;
                    }

                    // 如果没有任务，等待通知
                    if (!nextJob)
                    {
                        // 使用 Condition 的 wait，它会自动解锁 _mutex 并等待
                        _condition.wait(_mutex);  // 注意：这里传递的是 _mutex 而不是 ScopedLock
                        _mutex.unlock();  // wait 返回后 _mutex 是锁定的，需要解锁

                        continue;
                    }

                    _mutex.unlock();  // 找到任务后解锁
                }

                // 阶段2：等待执行时间
                DateTime now = DateTime();
                if (now < nextFireTime)
                {
                    Timespan waitTime = nextFireTime - now;

                    if (waitTime.totalMilliseconds() > 0)
                    {
                        _mutex.lock();

                        // 等待指定时间，如果被提前唤醒则重新开始循环
                        if (_condition.tryWait(_mutex, static_cast<long>(waitTime.totalMilliseconds())))
                        {
                            // 被唤醒，将任务重新放回队列
                            // 这里需要重新入队，或者在下一次循环中重新获取
                            auto jobIt = _scheduledJobs.find(nextJob->jobKey);
                            if (jobIt != _scheduledJobs.end() && !jobIt->second->cancelled && !jobIt->second->paused)
                            {
                                // 任务仍然有效，重新入队
                                addJobToQueue(nextJob);
                            }

                            _mutex.unlock();
                            continue;
                        }

                        _mutex.unlock();
                        now = DateTime();
                    }
                }

                // 阶段3：执行任务
                try
                {
                    handleMisfire(nextJob, now);
                }
                catch (...) {
                    // 异常处理
                    Thread::sleep(100);
                }
            }
        }

        void Scheduler::handleMisfire(ScheduledJob::Ptr job, const DateTime& now)
        {
            if (job->cancelled || job->paused || !job->hasNext)
            {
                return;
            }

            // 计算错过的时间
            Timespan misfireThreshold(1, 0); // 1秒阈值
            Timespan diff = now - job->nextFire;

            // 检查是否错过
            bool isMisfire = (diff > misfireThreshold);

            // 根据策略处理
            switch (job->misfire)
            {
            case MisfirePolicy::FIRE_NOW:
            {
                // 立即触发
                fireJob(job, isMisfire ? now : job->nextFire);
                break;
            }

            case MisfirePolicy::SKIP:
            {
                if (!isMisfire)
                {
                    fireJob(job, job->nextFire);
                }
                else
                {
                    // 跳过这次触发
                    job->lastFire = job->nextFire;
                    job->hasLast = true;

                    // 计算下一次触发
                    DateTime nextFire = job->trigger->nextFireTime(now);
                    if (nextFire != DateTime(1, 1, 1))
                    {
                        job->nextFire = nextFire;

                        // 重新添加到队列
                        FastMutex::ScopedLock lock(_mutex);
                        addJobToQueue(_scheduledJobs[job->jobKey]);
                    }
                    else
                    {
                        job->hasNext = false;
                    }

                    // 更新存储
                    if (_store)
                    {
                        try {
                            _store->updateFireTimes(job->jobKey, job->lastFire, job->nextFire, job->hasLast, job->hasNext);
                        }
                        catch (const std::exception& e) {
                        }
                    }
                }
                break;
            }
            case MisfirePolicy::CATCH_UP:
            {
                if (!isMisfire)
                {
                    fireJob(job, job->nextFire);
                }
                else
                {
                    // 追赶执行
                    if (auto intervalTrigger = dynamic_cast<IntervalTrigger*>(job->trigger.get()))
                    {
                        // 间隔触发器：执行所有错过的触发
                        Timespan interval = intervalTrigger->interval();
                        long long missedCount = diff.totalMilliseconds() / interval.totalMilliseconds();

                        if (missedCount > 0)
                        {
                            // 执行所有错过的触发
                            for (long long i = 0; i < missedCount; ++i)
                            {
                                DateTime fireTime = job->nextFire + interval.seconds() * i;
                                fireJob(job, fireTime);
                            }

                            job->lastFire = job->nextFire + interval.seconds() * (missedCount - 1);
                            job->hasLast = true;
                            job->nextFire = job->lastFire + interval;
                        }
                    }
                    else {
                        // 非间隔触发器：执行一次
                        fireJob(job, job->nextFire);
                    }
                }
                break;
            }
            }
        }

        void Scheduler::fireJob(ScheduledJob::Ptr job, const DateTime& scheduledTime)
        {
            if (job->cancelled || !job->job)
            {
                return;
            }

            // 更新执行记录
            job->lastFire = scheduledTime;
            job->hasLast = true;

            // 执行任务
            try
            {
                // 更新执行次数
                Trigger::Ptr trigger = job->trigger;
                trigger->increaseFireCount();

                // 计算下一次触发
                DateTime now;
                DateTime nextFire = job->trigger->nextFireTime(now);
                job->nextFire = nextFire;
                job->hasNext = nextFire != DateTime(1, 1, 1);

                _threadPool.start(job->_task);
            }
            catch (const siit::Exception& e)
            {
            }

            // 重新调度任务
            if (job->hasNext)
            {
                if (!job->cancelled)
                {
                    FastMutex::ScopedLock lock(_mutex);
                    addJobToQueue(_scheduledJobs[job->jobKey]);
                }
                else
                {
                    FastMutex::ScopedLock lock(_mutex);

                    auto jobIt = _scheduledJobs.find(job->jobKey);
                    if (jobIt == _scheduledJobs.end())
                    {
                        return;
                    }

                    // 从存储中移除
                    _jobs.erase(job->jobKey);
                    _scheduledJobs.erase(jobIt);

                    // 从队列中移除
                    removeJobFromQueue(job->jobKey);

                    // 从持久化存储中移除
                    if (_store)
                    {
                        try {
                            _store->removeTrigger(job->jobKey);
                        }
                        catch (const std::exception& e)
                        {
                        }
                    }
                }

                // 通知调度线程
                _condition.signal();
            }

            // 更新存储
            if (_store)
            {
                try {
                    _store->updateFireTimes(job->jobKey, job->lastFire, job->nextFire, true, job->hasNext);
                }
                catch (const std::exception& e) {
                }
            }
        }

        void Scheduler::addJobToQueue(const ScheduledJob::Ptr& job)
        {
            if (!job || job->paused || job->cancelled || !job->hasNext)
            {
                return;
            }

            QueueEntry entry;
            entry.job = job;
            entry.nextFire = job->nextFire;
            entry.valid = true;
            _jobQueue.push(entry);
        }

        void Scheduler::removeJobFromQueue(const std::string& jobKey)
        {
            // 重建队列（标准库priority_queue不支持删除）
            rebuildQueue();
        }

        void Scheduler::rebuildQueue()
        {
            JobQueue newQueue;

            while (!_jobQueue.empty())
            {
                auto entry = _jobQueue.top();
                _jobQueue.pop();

                if (entry.valid)
                {
                    // 检查任务是否仍然有效
                    auto jobIt = _scheduledJobs.find(entry.job->jobKey);
                    if (jobIt != _scheduledJobs.end() && !jobIt->second->cancelled && !jobIt->second->paused && jobIt->second->hasNext)
                    {
                        newQueue.push(entry);
                    }
                }
            }

            _jobQueue = std::move(newQueue);
        }

        std::string Scheduler::generateJobId()
        {
            static std::atomic<uint64_t> counter{ 1 };
            uint64_t id = counter.fetch_add(1, std::memory_order_relaxed);

            // 生成UUID作为后缀
            try {
                std::string uuid = GUIDGenerator::create();
                return "job-" + std::to_string(id) + "-" + uuid.substr(0, 8);
            }
            catch (...) {
                // 如果UUID生成失败，使用简单ID
                return "job-" + std::to_string(id);
            }
        }

        size_t Scheduler::getJobCount() const
        {
            FastMutex::ScopedLock lock(_mutex);
            return _scheduledJobs.size();
        }

        bool Scheduler::jobExists(const std::string& key) const
        {
            FastMutex::ScopedLock lock(_mutex);
            return _scheduledJobs.find(key) != _scheduledJobs.end();
        }

        DateTime Scheduler::getNextFireTime(const std::string& key) const
        {
            FastMutex::ScopedLock lock(_mutex);

            auto it = _scheduledJobs.find(key);
            if (it == _scheduledJobs.end() || it->second->paused || it->second->cancelled || !it->second->hasNext)
            {
                return DateTime();
            }

            return it->second->nextFire;
        }

        std::vector<std::string> Scheduler::getJobKeys() const
        {
            FastMutex::ScopedLock lock(_mutex);

            std::vector<std::string> keys;
            keys.reserve(_scheduledJobs.size());

            for (const auto& pair : _scheduledJobs)
            {
                keys.push_back(pair.first);
            }

            return keys;
        }

        std::string Scheduler::misfirePolicyToString(MisfirePolicy policy)
        {
            switch (policy)
            {
            case MisfirePolicy::FIRE_NOW: return "FIRE_NOW";
            case MisfirePolicy::CATCH_UP: return "CATCH_UP";
            case MisfirePolicy::SKIP: return "SKIP";
            default: return "UNKNOWN";
            }
        }

        MisfirePolicy Scheduler::stringToMisfirePolicy(const std::string& str)
        {
            if (str == "FIRE_NOW") return MisfirePolicy::FIRE_NOW;
            if (str == "CATCH_UP") return MisfirePolicy::CATCH_UP;
            if (str == "SKIP") return MisfirePolicy::SKIP;
            return MisfirePolicy::FIRE_NOW;
        }
    }
}