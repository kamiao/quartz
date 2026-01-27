#include "include/MemoryJobStore.h"
#include "data/Statement.h"
#include "foundation/Nullable.h"

namespace siit
{
    namespace quartz
    {
        void MemoryJobStore::saveTrigger(const PersistedTrigger& t)
        {
            _map[t.jobKey] = t;
        }

        std::vector<PersistedTrigger> MemoryJobStore::loadTriggers()
        {
            std::vector<PersistedTrigger> v;
            for (auto& kv : _map)
            {
                v.push_back(kv.second);
            }

            return v;
        }

        void MemoryJobStore::updateFireTimes(const std::string& jobKey, const DateTime& last, const DateTime& next, bool hasLast, bool hasNext)
        {
            auto& t = _map[jobKey];
            t.lastFireTime = last;
            t.nextFireTime = next;
            t.hasLast = hasLast;
            t.hasNext = hasNext;
        }
    }
}
