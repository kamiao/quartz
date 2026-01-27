#ifndef _SIIT_QUARTZ_JOB_FACTORY_H_
#define _SIIT_QUARTZ_JOB_FACTORY_H_

#include "Quartz.h"
#include "Job.h"
#include <unordered_map>
#include <functional>

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API JobFactory
        {
        public:
            virtual ~JobFactory() = default;
            virtual std::shared_ptr<Job> create(const std::string& jobKey) = 0;
        };
    }
}


#endif // !_SIIT_QUARTZ_JOB_FACTORY_H_
