#ifndef _SIIT_QUARTZ_CRON_FIELD_H_
#define _SIIT_QUARTZ_CRON_FIELD_H_

#include "Quartz.h"
#include <string>
#include <set>
#include <sstream>

namespace siit
{
    namespace quartz
    {
        enum class FieldType
        {
            SECOND,
            MINUTE,
            HOUR,
            DAY_OF_MONTH,
            MONTH,
            DAY_OF_WEEK,
            YEAR
        };

        class QUARTZ_API CronField
        {
        public:
            CronField(int minV, int maxV);

            void parse(const std::string& expr, FieldType type);

            bool isNoSpec() const;
            bool match(int v) const;
            int  first() const;
            int  nextGE(int cur) const;

            // DOM / DOW Ãÿ ‚”Ô“Â
            bool domLast() const;
            bool domLastWeekday() const;
            bool domNearestWeekday() const;
            int  domWday() const;
            int  domLastOffset() const;

            bool dowLast() const;
            int  dowLastVal() const;
            bool dowNth() const;
            int  dowNthVal() const;
            int  dowNthIndex() const;

        private:
            int _min, _max;
            FieldType _type;
            std::set<int> _values;
            bool _noSpec = false;

            // DOM specials
            bool _domLast = false;
            bool _domLastWeekday = false;
            bool _domNearestWeekday = false;
            int  _domWday = -1;
            int  _domLastOffset = 0;

            // DOW specials
            bool _dowLast = false;
            int  _dowLastVal = -1;
            bool _dowNth = false;
            int  _dowNthVal = -1;
            int  _dowNthIndex = 0;

            void reset();
            void parseToken(const std::string& token);
            int  parseValue(const std::string& s) const;
            int  parseDow(const std::string& s) const;
            static std::string upper(std::string s);
        };
    }
}
#endif // !_SIIT_QUARTZ_CRON_FIELD_H_
