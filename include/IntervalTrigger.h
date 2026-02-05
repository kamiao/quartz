#ifndef _SIIT_QUARTZ_INTERVAL_TRIGGER_H_
#define _SIIT_QUARTZ_INTERVAL_TRIGGER_H_

#include "Quartz.h"
#include "Trigger.h"
#include "foundation/Timespan.h"

namespace siit
{
    namespace quartz
    {
        class QUARTZ_API IntervalTrigger : public Trigger
        {
        public:
            explicit IntervalTrigger(const Timespan& interval);

            // 指定重复次数的构造
            IntervalTrigger(const Timespan& interval, siit::Int64 repeatCount);

            // 指定开始时间和重复次数的构造
            IntervalTrigger(const DateTime& startTime, const Timespan& interval, siit::Int64 repeatCount = -1);

            // 完整的构造：开始时间、间隔、重复次数、结束时间
            IntervalTrigger(const DateTime& startTime, const Timespan& interval, siit::Int64 repeatCount, const DateTime& endTime);

            // 禁止拷贝
            IntervalTrigger(const IntervalTrigger&) = delete;
            IntervalTrigger& operator=(const IntervalTrigger&) = delete;

            // 移动构造
            IntervalTrigger(IntervalTrigger&& other) noexcept;

            // 移动赋值
            IntervalTrigger& operator=(IntervalTrigger&& other) noexcept;

            // 实现基类接口
            DateTime nextFireTime(const DateTime& after) override;

            void increaseFireCount() override;

            // 获取触发器类型
            std::string type() const override;

            // 获取表达式描述
            std::string expr() const override;

            // 获取间隔
            Timespan interval() const;

            // 获取重复次数
            Int64 repeatCount() const;

            // 获取已触发次数
            Int64 fireCount() const;

            // 获取开始时间
            DateTime startTime() const;

            // 获取结束时间
            DateTime endTime() const;

            // 是否有结束时间
            bool hasEndTime() const;

            // 是否还有下一次触发
            bool hasNext();

            // 设置间隔
            void setInterval(const Timespan& interval);

            // 设置重复次数
            void setRepeatCount(int count);

            // 设置开始时间
            void setStartTime(const DateTime& startTime);

            // 设置结束时间
            void setEndTime(const DateTime& endTime);

            // 清除结束时间
            void clearEndTime();

        private:
            // 验证参数
            void validate() const;

        private:
            siit::Timespan _interval;                      // 触发间隔
            siit::Int64 _repeatCount = { -1 };             // 重复次数，-1 表示无限重复
            siit::Int64 _fireCount = {0};                  // 已触发次数
            siit::DateTime _startTime;                     // 开始时间
            siit::DateTime _endTime;                       // 结束时间（可选）
            bool _hasEndTime;                              // 是否有结束时间
            bool _firstFire;                               // 是否是第一次触发
            FastMutex _mutex;                               // 互斥锁
        };

        //
        // 获取间隔
        inline Timespan IntervalTrigger::interval() const
        {
            return _interval;
        }

        // 获取重复次数
        inline Int64 IntervalTrigger::repeatCount() const
        {
            return _repeatCount;
        }

        // 获取已触发次数
        inline Int64 IntervalTrigger::fireCount() const
        {
            return _fireCount;
        }

        // 获取开始时间
        inline DateTime IntervalTrigger::startTime() const
        {
            return _startTime;
        }

        // 获取结束时间
        inline DateTime IntervalTrigger::endTime() const
        {
            return _endTime;
        }

        // 是否有结束时间
        inline bool IntervalTrigger::hasEndTime() const
        {
            return _hasEndTime;
        }
    }
}

#endif // !_SIIT_QUARTZ_INTERVAL_TRIGGER_H_
