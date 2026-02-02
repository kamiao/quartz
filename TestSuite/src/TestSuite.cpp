#include "include/Scheduler.h"
#include "include/CronTrigger.h"
#include "include/IntervalTrigger.h"
#include "include/JobStore.h"
#include "include/MemoryJobStore.h"
#include "foundation/ThreadPool.h"
#include <iostream>


class PrintJob : public siit::quartz::Job {
public:
    PrintJob(const std::string& name) : _name(name)
    {

    }
    void execute() override {
        siit::Thread::sleep(5000);
        std::cout << "[PrintJob] fired" << _name << std::endl;
    }
private:
    std::string _name;
};

class CrashJob : public siit::quartz::Job {
public:
    void execute() override {
        std::cout << "[CrashJob] fired" << std::endl;
    }
};

int main() {
    // 内存 JobStore
    siit::quartz::MemoryJobStore::Ptr store = new siit::quartz::MemoryJobStore();

    // Scheduler
    siit::quartz::Scheduler sched(store);

    // Trigger：每 5 秒一次
    siit::quartz::CronTrigger::Ptr cron = new siit::quartz::CronTrigger("*/10 * * ? * *");

    // Trigger：每 3 秒一次
    siit::quartz::IntervalTrigger::Ptr interval = new siit::quartz::IntervalTrigger(siit::Timespan(0, 0, 0, 3, 0));

    // 调度
    siit::SharedPtr<PrintJob> job1 = new PrintJob("1");
    siit::SharedPtr<PrintJob> job2 = new PrintJob("2");
    sched.schedule(job1, cron, siit::quartz::MisfirePolicy::CATCH_UP);
   std::string id = sched.schedule(job2, cron, siit::quartz::MisfirePolicy::CATCH_UP);

    // 启动
    std::cout << "Scheduler started. Press Enter to quit..." << std::endl;
    sched.start();

    siit::Thread::sleep(30 * 1000);
    siit::SharedPtr<CrashJob> job3 = new CrashJob();
    sched.schedule(job3, interval, siit::quartz::MisfirePolicy::FIRE_NOW);

    siit::Thread::sleep(30 *1000);
    //sched.cancel(id);

    std::cin.get();

    sched.stop();
    return 0;
}