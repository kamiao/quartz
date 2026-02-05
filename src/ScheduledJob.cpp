#include "include/ScheduledJob.h"
namespace siit
{
    namespace quartz
    {
        ScheduledJob::ScheduledJob(Job::Ptr job, const std::string& jobKey)
            : job(job)
            , jobKey(jobKey)
            , paused(false)
            , cancelled(false)
            , hasNext(false)
            , _task(job, jobKey)
        {

        }
    }
}
