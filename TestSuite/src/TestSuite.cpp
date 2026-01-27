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
    auto store = std::make_shared<siit::quartz::MemoryJobStore>();

    // Scheduler
    siit::quartz::Scheduler sched(store);

    // Trigger：每 5 秒一次
    auto cron = std::make_shared<siit::quartz::CronTrigger>("*/10 * * ? * *");

    // Trigger：每 3 秒一次
    auto interval = std::make_shared<siit::quartz::IntervalTrigger>(siit::Timespan(0, 0, 0, 3, 0));

    // 调度
    sched.schedule(std::make_shared<PrintJob>("1"), cron, siit::quartz::MisfirePolicy::CATCH_UP);
    sched.schedule(std::make_shared<PrintJob>("2"), cron, siit::quartz::MisfirePolicy::CATCH_UP);

    // 启动
    std::cout << "Scheduler started. Press Enter to quit..." << std::endl;
    sched.start();

    siit::Thread::sleep(30 * 1000);
    sched.schedule(std::make_shared<CrashJob>(), interval, siit::quartz::MisfirePolicy::FIRE_NOW);

    std::cin.get();

    sched.stop();
    return 0;
}