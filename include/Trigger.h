#ifndef _SIIT_QUARTZ_TRIGGER_H_
#define _SIIT_QUARTZ_TRIGGER_H_

#include "Quartz.h"
#include "foundation/DateTime.h"
#include "foundation/SharedPtr.h"
#include <memory>
#include <string>

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API Trigger
        {
        public:
            using Ptr = SharedPtr<Trigger>;
            virtual ~Trigger() = default;
            virtual DateTime nextFireTime(const DateTime& after) = 0;
            virtual std::string type() const = 0;
            virtual std::string expr() const = 0;
        };
    }
}

#endif // !_SIIT_QUARTZ_TRIGGER_H_
