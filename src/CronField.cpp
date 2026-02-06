#include "include/CronField.h"
#include "foundation/Exception.h"
#include "foundation/String.h"
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <map>

namespace siit
{
    namespace quartz
    {
        // 初始化静态映射
        const std::map<std::string, int> CronField::MONTH_MAP = {
            {"JAN", 1}, {"FEB", 2}, {"MAR", 3}, {"APR", 4}, {"MAY", 5}, {"JUN", 6},
            {"JUL", 7}, {"AUG", 8}, {"SEP", 9}, {"OCT", 10}, {"NOV", 11}, {"DEC", 12}
        };

        const std::map<std::string, int> CronField::DAY_OF_WEEK_MAP = {
            {"SUN", 1}, {"MON", 2}, {"TUE", 3}, {"WED", 4},
            {"THU", 5}, {"FRI", 6}, {"SAT", 7}
        };

        // 构造函数
        CronField::CronField(int minValue, int maxValue)
            : _min(minValue), _max(maxValue), _type(FieldType::SECOND)
        {
            if (minValue > maxValue) {
                throw InvalidArgumentException("minValue must be <= maxValue");
            }
        }

        // 解析 cron 表达式字段
        void CronField::parse(const std::string& expr, FieldType type)
        {
            reset();
            _type = type;

            // 处理 "?" 特殊符号（只在 DOM 或 DOW 中使用）
            if (expr == "?")
            {
                if (type != FieldType::DAY_OF_MONTH && type != FieldType::DAY_OF_WEEK)
                {
                    throw InvalidArgumentException("'?' only allowed in DAY_OF_MONTH or DAY_OF_WEEK field");
                }
                _noSpec = true;
                return;
            }

            // 处理 DOM（月份中的日）特殊语义
            if (type == FieldType::DAY_OF_MONTH)
            {
                if (expr == "L") {
                    _domLast = true;
                    return;
                }

                if (expr == "LW") {
                    _domLastWeekday = true;
                    return;
                }

                // 处理工作日（15W）
                auto wpos = expr.find('W');
                if (wpos != std::string::npos) {
                    try {
                        int day = std::stoi(expr.substr(0, wpos));
                        _domNearestWeekday = true;
                        _domWday = day;
                        return;
                    }
                    catch (const std::exception&) {
                        throw InvalidArgumentException("Invalid day in weekday expression: " + expr);
                    }
                }

                // 处理最后几天的偏移（L-3）
                auto lpos = expr.find("L-");
                if (lpos == 0) {
                    try {
                        _domLastOffset = std::stoi(expr.substr(2));
                        return;
                    }
                    catch (const std::exception&) {
                        throw InvalidArgumentException("Invalid offset in L- expression: " + expr);
                    }
                }
            }

            // 处理 DOW（星期几）特殊语义
            if (type == FieldType::DAY_OF_WEEK)
            {
                // 处理最后一个星期几（6L）
                auto lpos = expr.find('L');
                if (lpos != std::string::npos && lpos > 0) {
                    try {
                        int dow = parseDayOfWeek(expr.substr(0, lpos));
                        _dowLast = true;
                        _dowLastVal = dow;
                        return;
                    }
                    catch (const std::exception&) {
                        throw InvalidArgumentException("Invalid day of week in last expression: " + expr);
                    }
                }

                // 处理第几个星期几（2#3）
                auto npos = expr.find('#');
                if (npos != std::string::npos) {
                    try {
                        int dow = parseDayOfWeek(expr.substr(0, npos));
                        int nth = std::stoi(expr.substr(npos + 1));
                        if (nth < 1 || nth > 5) {
                            throw InvalidArgumentException("Nth value must be between 1 and 5: " + expr);
                        }
                        _dowNth = true;
                        _dowNthVal = dow;
                        _dowNthIndex = nth;
                        return;
                    }
                    catch (const std::exception&) {
                        throw InvalidArgumentException("Invalid nth day of week expression: " + expr);
                    }
                }
            }

            // 处理通配符 "*"
            if (expr == "*")
            {
                for (int i = _min; i <= _max; ++i) {
                    _values.insert(i);
                }
                return;
            }

            // 解析逗号分隔的多个表达式
            std::stringstream ss(expr);
            std::string token;
            while (std::getline(ss, token, ',')) {
                parseToken(token);
            }
        }

        // 检查是否是通配符或特殊符号
        bool CronField::isNoSpec() const
        {
            return _noSpec;
        }

        // 检查值是否匹配
        bool CronField::match(int value) const
        {
            if (_noSpec) {
                return true;
            }

            // 检查 DOM 特殊语义
            if (_type == FieldType::DAY_OF_MONTH) {
                // 特殊语义会在调用上下文中处理
                return _values.find(value) != _values.end();
            }

            // 检查 DOW 特殊语义
            if (_type == FieldType::DAY_OF_WEEK) {
                // 特殊语义会在调用上下文中处理
                return _values.find(value) != _values.end();
            }

            return _values.find(value) != _values.end();
        }

        // 获取第一个有效值
        int CronField::first() const
        {
            if (_noSpec || _values.empty()) {
                return -1;
            }
            return *_values.begin();
        }

        // 获取大于等于当前值的下一个有效值
        int CronField::nextGE(int current) const
        {
            if (_noSpec || _values.empty()) {
                return -1;
            }

            auto it = _values.lower_bound(current);
            return it == _values.end() ? -1 : *it;
        }

        // DOM 特殊语义方法
        bool CronField::domLast() const { return _domLast; }
        bool CronField::domLastWeekday() const { return _domLastWeekday; }
        bool CronField::domNearestWeekday() const { return _domNearestWeekday; }
        int CronField::domWday() const { return _domWday; }
        int CronField::domLastOffset() const { return _domLastOffset; }

        // DOW 特殊语义方法
        bool CronField::dowLast() const { return _dowLast; }
        int CronField::dowLastVal() const { return _dowLastVal; }
        bool CronField::dowNth() const { return _dowNth; }
        int CronField::dowNthVal() const { return _dowNthVal; }
        int CronField::dowNthIndex() const { return _dowNthIndex; }

        // 重置所有状态
        void CronField::reset()
        {
            _values.clear();
            _noSpec = false;

            // 重置 DOM 特殊语义
            _domLast = false;
            _domLastWeekday = false;
            _domNearestWeekday = false;
            _domWday = -1;
            _domLastOffset = 0;

            // 重置 DOW 特殊语义
            _dowLast = false;
            _dowLastVal = -1;
            _dowNth = false;
            _dowNthVal = -1;
            _dowNthIndex = 0;
        }

        // 解析令牌
        void CronField::parseToken(const std::string& token)
        {
            std::string base = token;
            int step = 1;

            // 处理步长表达式（*/5 或 0-10/2）
            auto slashPos = token.find('/');
            if (slashPos != std::string::npos) {
                base = token.substr(0, slashPos);
                try {
                    step = std::stoi(token.substr(slashPos + 1));
                    if (step <= 0) {
                        throw InvalidArgumentException("Step value must be greater than 0: " + token);
                    }
                }
                catch (const std::exception&) {
                    throw InvalidArgumentException("Invalid step value in expression: " + token);
                }
            }

            // 处理通配符 "*"
            if (base == "*") {
                for (int i = _min; i <= _max; i += step) {
                    _values.insert(i);
                }
                return;
            }

            // 处理范围表达式（1-10）
            auto dashPos = base.find('-');
            if (dashPos != std::string::npos) {
                try {
                    int start = parseValue(base.substr(0, dashPos));
                    int end = parseValue(base.substr(dashPos + 1));

                    // 确保范围在有效区间内
                    if (start < _min) start = _min;
                    if (end > _max) end = _max;

                    for (int i = start; i <= end; i += step) {
                        _values.insert(i);
                    }
                    return;
                }
                catch (const std::exception& e) {
                    throw InvalidArgumentException("Invalid range expression: " + base + ", error: " + e.what());
                }
            }

            // 处理单个值
            try {
                int value = parseValue(base);
                if (value < _min || value > _max) {
                    throw InvalidArgumentException(
                        "Value " + std::to_string(value) +
                        " out of range [" + std::to_string(_min) +
                        "-" + std::to_string(_max) + "]");
                }
                _values.insert(value);
            }
            catch (const std::exception& e) {
                throw InvalidArgumentException("Invalid value expression: " + base + ", error: " + e.what());
            }
        }

        // 解析数值
        int CronField::parseValue(const std::string& str) const
        {
            // 月份字段的特殊处理
            if (_type == FieldType::MONTH) {
                std::string upperStr = toUpper(str);
                auto it = MONTH_MAP.find(upperStr);
                if (it != MONTH_MAP.end()) {
                    return it->second;
                }
            }

            // 星期几字段的特殊处理
            if (_type == FieldType::DAY_OF_WEEK) {
                return parseDayOfWeek(str);
            }

            // 普通数值处理
            try {
                return std::stoi(str);
            }
            catch (const std::exception&) {
                throw InvalidArgumentException("Invalid numeric value: " + str);
            }
        }

        // 解析星期几
        int CronField::parseDayOfWeek(const std::string& str) const
        {
            std::string upperStr = toUpper(str);
            auto it = DAY_OF_WEEK_MAP.find(upperStr);
            if (it != DAY_OF_WEEK_MAP.end()) {
                return it->second;
            }

            try {
                int value = std::stoi(str);
                // Quartz 中星期天可以是 1 或 7，但通常用 1 表示
                if (value == 0) return 1;  // 0 也表示星期天
                if (value < 1 || value > 7) {
                    throw InvalidArgumentException("Day of week must be between 1-7 or 0: " + str);
                }
                return value;
            }
            catch (const std::exception&) {
                throw InvalidArgumentException("Invalid day of week value: " + str);
            }
        }

        // 字符串转大写
        std::string CronField::toUpper(const std::string& str)
        {
            std::string result = siit::toUpper(str);

            return result;
        }
    }
}