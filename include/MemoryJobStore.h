#ifndef _SIIT_QUARTZ_MEMORY_JOB_STORE_H_
#define _SIIT_QUARTZ_MEMORY_JOB_STORE_H_

#include "Quartz.h"
#include "JobStore.h"
#include <map>

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API MemoryJobStore : public JobStore
        {
        public:
            void saveTrigger(const PersistedTrigger& t) override;
            std::vector<PersistedTrigger> loadTriggers() override;
            void updateFireTimes(const std::string& jobKey, const DateTime& last, const DateTime& next, bool hasLast, bool hasNext) override;

        private:
            std::map<std::string, PersistedTrigger> _map;
        };
    }
}
#endif // !_SIIT_QUARTZ_MEMORY_JOB_STORE_H_
