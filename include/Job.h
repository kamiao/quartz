#ifndef _SIIT_QUARTZ_JOB_H_
#define _SIIT_QUARTZ_JOB_H_

#include "Quartz.h"
#include "foundation/SharedPtr.h"
#include <memory>
#include <string>

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API Job
        {
        public:
            virtual ~Job() = default;
            virtual void execute() = 0;
        };
    }
}

#endif // !_SIIT_QUARTZ_JOB_H_
