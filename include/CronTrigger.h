#ifndef _SIIT_QUARTZ_CRON_TRIGGER_H_
#define _SIIT_QUARTZ_CRON_TRIGGER_H_

#include "Quartz.h"
#include "Trigger.h"
#include "CronField.h"
#include "foundation/Timespan.h"
#include <set>
#include <string>
#include <vector>


namespace siit
{
    namespace quartz
    {
        class QUARTZ_API CronTrigger : public Trigger
        {
        public:
            explicit CronTrigger(const std::string& expr);

            DateTime nextFireTime(const DateTime& after) override;
            std::string type() const override;
            std::string expr() const override;
            void increaseFireCount() override;
        private:
            CronField _sec, _min, _hour, _day, _month, _week, _year;
            std::string _expr;

            void parse(const std::string& expr);
            void validateQuartzDomDow() const;
            int  quartzDOW(const DateTime& t) const;

            bool dayMatchQuartz(const DateTime& t) const;
            bool matchDOM(const DateTime& t) const;
            bool matchDOW(const DateTime& t) const;

            DateTime nextValidDayInMonthOrNext(const DateTime& t) const;

            static int lastWeekdayOfMonth(int y, int m);
            static int nearestWeekday(int y, int m, int day);
            static int lastDowInMonth(int y, int m, int dowQuartz);
            static int nthDowInMonth(int y, int m, int dowQuartz, int nth);
        };

        // inline
        inline std::string CronTrigger::CronTrigger::type() const
        {
            return "cron";
        }

        inline std::string CronTrigger::CronTrigger::expr() const
        {
            return _expr;
        }
    }
}
#endif //_SIIT_QUARTZ_CRON_TRIGGER_H_