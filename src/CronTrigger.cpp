#include "include/CronTrigger.h"
#include "include/CronField.h"
#include "foundation/Timespan.h"
#include "foundation/LocalDateTime.h"
#include "foundation/Timezone.h"
#include <stdexcept>
#include <sstream>

namespace siit
{
    namespace quartz
    {
        CronTrigger::CronTrigger(const std::string& expr)
            : _sec(0, 59), _min(0, 59), _hour(0, 23),
            _day(1, 31), _month(1, 12), _week(1, 7), _year(1970, 2199)
        {
            parse(expr);
            validateQuartzDomDow();
        }

        void CronTrigger::increaseFireCount()
        {

        }

        void CronTrigger::parse(const std::string& expr)
        {
            _expr = expr;
            std::stringstream ss(expr);
            std::vector<std::string> fields;
            std::string token;
            while (ss >> token) fields.push_back(token);

            if (fields.size() != 6 && fields.size() != 7) {
                throw std::runtime_error("Cron must have 6 or 7 fields");
            }

            _sec.parse(fields[0], FieldType::SECOND);
            _min.parse(fields[1], FieldType::MINUTE);
            _hour.parse(fields[2], FieldType::HOUR);
            _day.parse(fields[3], FieldType::DAY_OF_MONTH);
            _month.parse(fields[4], FieldType::MONTH);
            _week.parse(fields[5], FieldType::DAY_OF_WEEK);

            if (fields.size() == 7)
            {
                _year.parse(fields[6], FieldType::YEAR);
            }
            else
            {
                _year.parse("*", FieldType::YEAR);
            }
        }

        void CronTrigger::validateQuartzDomDow() const
        {
            bool domNo = _day.isNoSpec();
            bool dowNo = _week.isNoSpec();
            if (domNo == dowNo)
            {
                throw std::invalid_argument("Quartz requires one of day-of-month or day-of-week to be '?'");
            }
        }

        int CronTrigger::quartzDOW(const DateTime& t) const
        {
            int w = t.dayOfWeek(); // 0=SUN..6=SAT
            return (w == 0) ? 1 : (w + 1);
        }

        bool CronTrigger::dayMatchQuartz(const DateTime& t) const
        {
            if (_day.isNoSpec())
            {
                return matchDOW(t);
            }

            return matchDOM(t);
        }

        bool CronTrigger::matchDOM(const DateTime& t) const
        {
            int y = t.year(), m = t.month(), d = t.day();
            int last = DateTime::daysOfMonth(y, m);

            if (_day.domLast()) return d == last;
            if (_day.domLastOffset() > 0) return d == (last - _day.domLastOffset());
            if (_day.domLastWeekday()) return d == lastWeekdayOfMonth(y, m);
            if (_day.domNearestWeekday()) return d == nearestWeekday(y, m, _day.domWday());
            return _day.match(d);
        }

        bool CronTrigger::matchDOW(const DateTime& t) const
        {
            int y = t.year(), m = t.month(), d = t.day();
            int dow = quartzDOW(t);

            if (_week.dowLast()) return d == lastDowInMonth(y, m, _week.dowLastVal());
            if (_week.dowNth()) return d == nthDowInMonth(y, m, _week.dowNthVal(), _week.dowNthIndex());
            
            return _week.match(dow);
        }

        DateTime CronTrigger::nextFireTime(const DateTime& after)
        {
            // 确保输入是 UTC 时间
            DateTime t = after;
            t.makeLocal(Timezone::tzd());
            // 从 after 的下一秒开始检查
            t += Timespan(0, 0, 0, 1, 0); // 加 1 秒

            constexpr int MAX_ITERATIONS = 200000; // 防止无限循环

            for (int guard = 0; guard < MAX_ITERATIONS; ++guard)
            {
                // 1. 检查年份
                int ny = _year.nextGE(t.year());
                if (ny < 0) {
                    // 没有符合条件的年份，返回当前时间（通常表示没有下一次）
                    return t;
                }
                if (ny != t.year()) {
                    // 年份不匹配，跳到下一年的 1月1日 00:00:00
                    t = DateTime(ny, 1, 1, 0, 0, 0, 0, 0);
                    continue;
                }

                // 2. 检查月份
                int nm = _month.nextGE(t.month());
                if (nm < 0) {
                    // 当前年份没有符合条件的月份，跳到下一年的 1月1日
                    t = DateTime(t.year() + 1, 1, 1, 0, 0, 0, 0, 0);
                    continue;
                }
                if (nm != t.month()) {
                    // 月份不匹配，跳到该年目标月份的第1天
                    t = DateTime(t.year(), nm, 1, 0, 0, 0, 0, 0);
                    continue;
                }

                // 3. 检查日期（日和周几）
                if (!dayMatchQuartz(t)) {
                    t = nextValidDayInMonthOrNext(t);
                    continue;
                }

                // 4. 检查小时
                int nh = _hour.nextGE(t.hour());
                if (nh < 0) {
                    // 当天没有合适的小时，跳到下一天的 00:00:00
                    t = DateTime(t.year(), t.month(), t.day(), 0, 0, 0, 0, 0);
                    t += Timespan(1, 0, 0, 0, 0); // 加 1 天
                    continue;
                }
                if (nh != t.hour()) {
                    // 小时不匹配，跳到该小时的第一分钟第一秒
                    t = DateTime(t.year(), t.month(), t.day(),
                        nh, _min.first(), _sec.first(), 0, 0);
                    continue;
                }

                // 5. 检查分钟
                int nmin = _min.nextGE(t.minute());
                if (nmin < 0) {
                    // 当前小时没有合适的分钟，跳到下一小时的 00:00
                    t = DateTime(t.year(), t.month(), t.day(), t.hour(), 0, 0, 0, 0);
                    t += Timespan(0, 1, 0, 0, 0); // 加 1 小时
                    continue;
                }
                if (nmin != t.minute()) {
                    // 分钟不匹配，跳到该分钟的第一秒
                    t = DateTime(t.year(), t.month(), t.day(), t.hour(), nmin, _sec.first(), 0, 0);
                    continue;
                }

                // 6. 检查秒
                int ns = _sec.nextGE(t.second());
                if (ns < 0) {
                    // 当前分钟没有合适的秒，跳到下一分钟的 00
                    t = DateTime(t.year(), t.month(), t.day(), t.hour(), t.minute(), 0, 0, 0);
                    t += Timespan(0, 0, 1, 0, 0); // 加 1 分钟
                    continue;
                }
                if (ns != t.second()) {
                    // 秒不匹配，跳到目标秒
                    t = DateTime(t.year(), t.month(), t.day(), t.hour(), t.minute(), ns, 0, 0);
                    continue;
                }

                // 所有字段都匹配，找到下一个触发时间
                t.makeUTC(Timezone::tzd());

                return t;
            }

            // 超过最大迭代次数，返回当前时间
            throw std::runtime_error("CronTrigger: Exceeded maximum iterations");
        }

        DateTime CronTrigger::nextValidDayInMonthOrNext(const DateTime& t) const
        {
            int y = t.year(), m = t.month();
            int last = DateTime::daysOfMonth(y, m);

            for (int d = t.day(); d <= last; ++d)
            {
                DateTime cand(y, m, d, 0, 0, 0);
                if (dayMatchQuartz(cand)) return cand;
            }

            DateTime nm(y, m, 1, 0, 0, 0);
            nm += Timespan(31, 0, 0, 0, 0);
            nm = DateTime(nm.year(), nm.month(), 1, 0, 0, 0);
            
            return nm;
        }

        int CronTrigger::lastWeekdayOfMonth(int y, int m)
        {
            int last = DateTime::daysOfMonth(y, m);
            for (int d = last; d >= last - 6; --d) {
                DateTime t(y, m, d, 0, 0, 0);
                int wd = t.dayOfWeek(); // 0=SUN..6=SAT
                if (wd != 0 && wd != 6) return d;
            }
            return last;
        }

        int CronTrigger::nearestWeekday(int y, int m, int day)
        {
            int last = DateTime::daysOfMonth(y, m);
            if (day < 1) day = 1;
            if (day > last) day = last;

            DateTime t(y, m, day, 0, 0, 0);
            int wd = t.dayOfWeek();
            if (wd >= 1 && wd <= 5) return day;

            if (wd == 0) return (day + 1 <= last) ? day + 1 : day - 1;
            return (day - 1 >= 1) ? day - 1 : day + 1;
        }

        int CronTrigger::lastDowInMonth(int y, int m, int dowQuartz)
        {
            int last = DateTime::daysOfMonth(y, m);
            for (int d = last; d >= 1; --d) {
                DateTime t(y, m, d, 0, 0, 0);
                int q = (t.dayOfWeek() == 0) ? 1 : t.dayOfWeek() + 1;
                if (q == dowQuartz) return d;
            }
            return -1;
        }

        int CronTrigger::nthDowInMonth(int y, int m, int dowQuartz, int nth)
        {
            int count = 0;
            int last = DateTime::daysOfMonth(y, m);
            for (int d = 1; d <= last; ++d) {
                DateTime t(y, m, d, 0, 0, 0);
                int q = (t.dayOfWeek() == 0) ? 1 : t.dayOfWeek() + 1;
                if (q == dowQuartz) {
                    ++count;
                    if (count == nth) return d;
                }
            }
            return -1;
        }
    }
}

