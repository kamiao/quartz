#ifndef _SIIT_QUARTZ_CRON_FIELD_H_
#define _SIIT_QUARTZ_CRON_FIELD_H_

#include "Quartz.h"
#include <string>
#include <set>
#include <map>
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

        class CronField
        {
        public:
            // 构造函数，指定字段的取值范围
            CronField(int minValue, int maxValue);

            // 解析 cron 表达式字段
            void parse(const std::string& expr, FieldType type);

            // 检查是否是通配符或特殊符号
            bool isNoSpec() const;

            // 检查值是否匹配
            bool match(int value) const;

            // 获取第一个有效值
            int first() const;

            // 获取大于等于当前值的下一个有效值
            int nextGE(int current) const;

            // DOM（月份中的日）特殊语义方法
            bool domLast() const;              // 是否表示最后一天（L）
            bool domLastWeekday() const;       // 是否表示最后一个工作日（LW）
            bool domNearestWeekday() const;    // 是否表示最近的工作日（15W）
            int  domWday() const;              // 工作日值
            int  domLastOffset() const;        // 最后几天的偏移（L-3）

            // DOW（星期几）特殊语义方法
            bool dowLast() const;              // 是否表示最后一个星期几（6L）
            int  dowLastVal() const;           // 最后一个星期几的值
            bool dowNth() const;               // 是否表示第几个星期几（2#3）
            int  dowNthVal() const;            // 星期几的值
            int  dowNthIndex() const;          // 第几个

            // 获取字段类型
            FieldType type() const { return _type; }

            // 获取字段的最小值
            int minValue() const { return _min; }

            // 获取字段的最大值
            int maxValue() const { return _max; }

        private:
            // 字段的取值范围
            int _min;
            int _max;

            // 字段类型
            FieldType _type;

            // 存储所有有效值的集合
            std::set<int> _values;

            // 是否是通配符
            bool _noSpec = false;

            // DOM（月份中的日）特殊语义
            bool _domLast = false;             // L
            bool _domLastWeekday = false;      // LW
            bool _domNearestWeekday = false;   // 15W
            int  _domWday = -1;                // 工作日值
            int  _domLastOffset = 0;           // L-3

            // DOW（星期几）特殊语义
            bool _dowLast = false;             // 6L
            int  _dowLastVal = -1;             // 最后一个星期几的值
            bool _dowNth = false;              // 2#3
            int  _dowNthVal = -1;              // 星期几的值
            int  _dowNthIndex = 0;             // 第几个

            // 重置所有状态
            void reset();

            // 解析令牌
            void parseToken(const std::string& token);

            // 解析数值
            int parseValue(const std::string& str) const;

            // 解析星期几
            int parseDayOfWeek(const std::string& str) const;

            // 字符串转大写
            static std::string toUpper(const std::string& str);

            // 月份名称到数字的映射
            static const std::map<std::string, int> MONTH_MAP;

            // 星期名称到数字的映射
            static const std::map<std::string, int> DAY_OF_WEEK_MAP;
        };
    }
}
#endif // !_SIIT_QUARTZ_CRON_FIELD_H_
