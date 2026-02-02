#ifndef _SIIT_QUARTZ_JOB_STORE_H_
#define _SIIT_QUARTZ_JOB_STORE_H_

#include "Quartz.h"
#include "foundation/DateTime.h"
#include "foundation/SharedPtr.h"
#include <vector>
#include <string>

namespace siit
{
    namespace quartz
    {
        class PersistedTrigger
        {
        public:
            using Ptr = SharedPtr<PersistedTrigger>;
            std::string jobKey;
            std::string triggerType;
            std::string triggerExpr;
            std::string misfirePolicy;
            DateTime lastFireTime;
            DateTime nextFireTime;
            bool hasLast = false;
            bool hasNext = false;
            bool paused = false;
        };

        class  JobStore
        {
        public:
            using Ptr = SharedPtr<JobStore>;
            virtual ~JobStore() = default;
            virtual void saveTrigger(const PersistedTrigger & t) = 0;
            virtual std::vector<PersistedTrigger> loadTriggers() = 0;
            virtual void updateFireTimes(const std::string & jobKey, const DateTime & last, const DateTime & next, bool hasLast, bool hasNext) = 0;
            virtual void removeTrigger(const std::string& jobKey) = 0;
        };
    }
}
#endif // !_SIIT_QUARTZ_JOB_STORE_H_
