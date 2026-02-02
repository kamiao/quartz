#ifndef _SIIT_QUARTZ_MEMORY_JOB_STORE_H_
#define _SIIT_QUARTZ_MEMORY_JOB_STORE_H_

#include "Quartz.h"
#include "JobStore.h"
#include "foundation/HashMap.h"
#include "foundation/Mutex.h"
#include "foundation/SharedPtr.h"
#include <string>
#include <vector>

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API MemoryJobStore : public JobStore
        {
        public:
            using Ptr = SharedPtr<MemoryJobStore>;

            MemoryJobStore();

            void saveTrigger(const PersistedTrigger& t) override;
            void saveTriggerPtr(const PersistedTrigger::Ptr& trigger);

            std::vector<PersistedTrigger> loadTriggers() override;
            std::vector<PersistedTrigger::Ptr> loadTriggerPtrs();

            void updateFireTimes(const std::string& jobKey, const DateTime& last, const DateTime& next, bool hasLast, bool hasNext) override;

            void removeTrigger(const std::string& jobKey) override;

            PersistedTrigger::Ptr getTrigger(const std::string& jobKey) const;

        private:
            HashMap<std::string, PersistedTrigger::Ptr> _triggerPtrsMap;
            mutable FastMutex _mutex;
        };
    }
}
#endif // !_SIIT_QUARTZ_MEMORY_JOB_STORE_H_
