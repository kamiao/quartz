#include "include/IntervalTrigger.h"

namespace siit
{
    namespace quartz
    {
        IntervalTrigger::IntervalTrigger(const Timespan& interval)
            : _interval(interval),
            _repeatCount(-1),  // -1 表示无限重复
            _fireCount(0),
            _startTime(DateTime()),
            _endTime(DateTime()),  // 默认无结束时间
            _hasEndTime(false),
            _firstFire(true)
        {
            validate();
        }

        // 指定重复次数的构造
        IntervalTrigger::IntervalTrigger(const Timespan& interval, siit::Int64 repeatCount)
            : _interval(interval),
            _repeatCount(repeatCount),
            _fireCount(0),
            _startTime(DateTime()),
            _endTime(DateTime()),
            _hasEndTime(false),
            _firstFire(true)
        {
            validate();
            if (repeatCount < 0 && repeatCount != -1)
            {
                throw std::invalid_argument("repeatCount must be >= 0 or -1 for infinite");
            }
        }

        // 指定开始时间和重复次数的构造
        IntervalTrigger::IntervalTrigger(const DateTime& startTime, const Timespan& interval, siit::Int64 repeatCount)
            : _interval(interval),
            _repeatCount(repeatCount),
            _fireCount(0),
            _startTime(startTime),
            _endTime(DateTime()),
            _hasEndTime(false),
            _firstFire(true)
        {
            validate();
            if (repeatCount < 0 && repeatCount != -1)
            {
                throw std::invalid_argument("repeatCount must be >= 0 or -1 for infinite");
            }
        }

        // 完整的构造：开始时间、间隔、重复次数、结束时间
        IntervalTrigger::IntervalTrigger(const DateTime& startTime, const Timespan& interval, siit::Int64 repeatCount, const DateTime& endTime)
            : _interval(interval),
            _repeatCount(repeatCount),
            _fireCount(0),
            _startTime(startTime),
            _endTime(endTime),
            _hasEndTime(true),
            _firstFire(true)
        {
            validate();
            if (repeatCount < 0 && repeatCount != -1)
            {
                throw std::invalid_argument("repeatCount must be >= 0 or -1 for infinite");
            }
            if (endTime <= startTime)
            {
                throw std::invalid_argument("endTime must be after startTime");
            }
        }

        // 移动构造
        IntervalTrigger::IntervalTrigger(IntervalTrigger&& other) noexcept
            : _interval(other._interval),
            _repeatCount(other._repeatCount),
            _fireCount(other._fireCount),
            _startTime(other._startTime),
            _endTime(other._endTime),
            _hasEndTime(other._hasEndTime),
            _firstFire(other._firstFire)
        {
        }

        // 移动赋值
        IntervalTrigger& IntervalTrigger::operator=(IntervalTrigger&& other) noexcept
        {
            if (this != &other)
            {
                _interval = other._interval;
                _repeatCount = other._repeatCount;
                _fireCount = other._fireCount;
                _startTime = other._startTime;
                _endTime = other._endTime;
                _hasEndTime = other._hasEndTime;
                _firstFire = other._firstFire;
            }
            return *this;
        }

        // 实现基类接口
        DateTime IntervalTrigger::nextFireTime(const DateTime& after)
        {
            FastMutex::ScopedLock lock(_mutex);

            // 如果已经达到重复次数限制，返回无效时间
            if (_repeatCount >= 0 && _fireCount >= _repeatCount)
            {
                return DateTime(1, 1, 1);  // 返回默认构造的 DateTime 表示无效
            }

            DateTime result;

            if (_firstFire)
            {
                // 第一次触发，返回开始时间
                result = _startTime;
                _firstFire = false;
            }
            else
            {
                // 计算下一个触发时间
                DateTime lastFireTime = _startTime + siit::Timespan(_interval.totalMicroseconds() * _fireCount);

                // 计算在 after 时间之后的下一个触发时间
                if (after < lastFireTime)
                {
                    result = lastFireTime;
                }
                else
                {
                    // 计算经过多少个间隔后到达 after 之后的时间
                    Timespan diff = after - _startTime;
                    long long intervals = diff.totalMilliseconds() / _interval.totalMilliseconds();

                    // 如果 after 正好落在触发时间点上，需要加一个间隔
                    if (diff.totalMilliseconds() % _interval.totalMilliseconds() == 0)
                    {
                        intervals++;
                    }
                    else
                    {
                        intervals++;
                    }

                    result = _startTime + siit::Timespan(_interval.totalMicroseconds() * intervals);
                }
            }

            // 检查是否超过重复次数限制
            if (_fireCount >= _repeatCount)  // 一目了然
            {
                return DateTime(1, 1, 1);
            }

            // 检查是否超过结束时间
            if (_hasEndTime && result > _endTime)
            {
                return DateTime(1, 1, 1);  // 超过结束时间
            }

            return result;
        }

        void IntervalTrigger::increaseFireCount()
        {
            FastMutex::ScopedLock lock(_mutex);
            _fireCount++;
        }

        // 获取触发器类型
        std::string IntervalTrigger::type() const
        {
            return "IntervalTrigger";
        }

        // 获取表达式描述
        std::string IntervalTrigger::expr() const
        {
            std::string exprStr = "interval=" + std::to_string(_interval.totalMilliseconds()) + "ms";

            if (_repeatCount >= 0)
            {
                exprStr += ", repeat=" + std::to_string(_repeatCount);
            }
            else
            {
                exprStr += ", repeat=infinite";
            }

            if (_hasEndTime)
            {
                exprStr += ", endTime=" + std::to_string(_endTime.timestamp().epochTime());
            }

            return exprStr;
        }

        // 是否还有下一次触发
        bool IntervalTrigger::hasNext()
        {
            FastMutex::ScopedLock lock(_mutex);
            if (_repeatCount >= 0 && _fireCount >= _repeatCount)
            {
                return false;
            }

            if (_hasEndTime)
            {
                DateTime nextTime = _startTime + Timespan(_interval.totalMicroseconds() * _fireCount);
                return nextTime <= _endTime;
            }

            return true;
        }

        // 设置间隔
        void IntervalTrigger::setInterval(const Timespan& interval)
        {
            if (interval.totalMilliseconds() <= 0)
            {
                throw std::invalid_argument("interval must be positive");
            }
            _interval = interval;
        }

        // 设置重复次数
        void IntervalTrigger::setRepeatCount(int count)
        {
            if (count < 0 && count != -1)
            {
                throw std::invalid_argument("repeatCount must be >= 0 or -1 for infinite");
            }
            _repeatCount = count;
        }

        // 设置开始时间
        void IntervalTrigger::setStartTime(const DateTime& startTime)
        {
            _startTime = startTime;
            _firstFire = true;
        }

        // 设置结束时间
        void IntervalTrigger::setEndTime(const DateTime& endTime)
        {
            _endTime = endTime;
            _hasEndTime = true;
        }

        // 清除结束时间
        void IntervalTrigger::clearEndTime()
        {
            _hasEndTime = false;
        }

        // 验证参数
        void IntervalTrigger::validate() const
        {
            if (_interval.totalMilliseconds() <= 0)
            {
                throw std::invalid_argument("interval must be positive");
            }
        }
    }
}
