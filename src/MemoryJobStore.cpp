#include "include/MemoryJobStore.h"
#include "foundation/Nullable.h"

namespace siit
{
    namespace quartz
    {
        MemoryJobStore::MemoryJobStore()
        {
        }

        void MemoryJobStore::saveTrigger(const PersistedTrigger& t)
        {
            FastMutex::ScopedLock lock(_mutex);
            PersistedTrigger::Ptr triggerPtr(new PersistedTrigger(t));
            _triggerPtrsMap[t.jobKey] = triggerPtr;
        }

        void MemoryJobStore::saveTriggerPtr(const PersistedTrigger::Ptr& trigger)
        {
            if (!trigger) return;

            FastMutex::ScopedLock lock(_mutex);
            _triggerPtrsMap[trigger->jobKey] = trigger;
        }

        std::vector<PersistedTrigger> MemoryJobStore::loadTriggers()
        {
            FastMutex::ScopedLock lock(_mutex);
            std::vector<PersistedTrigger> triggers;

            for (auto it = _triggerPtrsMap.begin(); it != _triggerPtrsMap.end(); ++it) {
                if (it->second) {
                    triggers.push_back(*(it->second));
                }
            }

            return triggers;
        }

        std::vector<PersistedTrigger::Ptr> MemoryJobStore::loadTriggerPtrs()
        {
            FastMutex::ScopedLock lock(_mutex);
            std::vector<PersistedTrigger::Ptr> triggers;

            for (auto it = _triggerPtrsMap.begin(); it != _triggerPtrsMap.end(); ++it) {
                if (it->second) {
                    triggers.push_back(it->second);
                }
            }

            return triggers;
        }

        void MemoryJobStore::updateFireTimes(const std::string& jobKey,
            const DateTime& last,
            const DateTime& next,
            bool hasLast,
            bool hasNext)
        {
            FastMutex::ScopedLock lock(_mutex);

            auto it = _triggerPtrsMap.find(jobKey);
            if (it != _triggerPtrsMap.end() && it->second) {
                it->second->lastFireTime = last;
                it->second->nextFireTime = next;
                it->second->hasLast = hasLast;
                it->second->hasNext = hasNext;
            }
        }

        void MemoryJobStore::removeTrigger(const std::string& jobKey)
        {
            FastMutex::ScopedLock lock(_mutex);
            _triggerPtrsMap.erase(jobKey);
        }

        PersistedTrigger::Ptr MemoryJobStore::getTrigger(const std::string& jobKey) const
        {
            FastMutex::ScopedLock lock(_mutex);

            auto it = _triggerPtrsMap.find(jobKey);
            if (it != _triggerPtrsMap.end()) {
                return it->second;
            }

            return nullptr;
        }
    }
}
