#include "include/CronField.h"
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <map>

namespace siit
{
    namespace quartz
    {
        CronField::CronField(int minV, int maxV) : _min(minV), _max(maxV)
        {
        }

        void CronField::parse(const std::string& expr, FieldType type)
        {
            reset();
            _type = type;

            if (expr == "?") {
                if (type != FieldType::DAY_OF_MONTH && type != FieldType::DAY_OF_WEEK) {
                    throw std::invalid_argument("'?' only allowed in DOM or DOW");
                }
                _noSpec = true;
                return;
            }

            // DOM specials
            if (type == FieldType::DAY_OF_MONTH) {
                if (expr == "L") { _domLast = true; return; }
                if (expr == "LW") { _domLastWeekday = true; return; }
                auto wpos = expr.find('W');
                if (wpos != std::string::npos) {
                    int day = std::stoi(expr.substr(0, wpos));
                    _domNearestWeekday = true;
                    _domWday = day;
                    return;
                }
                auto lpos = expr.find("L-");
                if (lpos == 0) {
                    _domLastOffset = std::stoi(expr.substr(2));
                    return;
                }
            }

            // DOW specials
            if (type == FieldType::DAY_OF_WEEK) {
                auto lpos = expr.find('L');
                if (lpos != std::string::npos && lpos > 0) {
                    int dow = parseDow(expr.substr(0, lpos));
                    _dowLast = true;
                    _dowLastVal = dow;
                    return;
                }
                auto npos = expr.find('#');
                if (npos != std::string::npos) {
                    int dow = parseDow(expr.substr(0, npos));
                    int nth = std::stoi(expr.substr(npos + 1));
                    _dowNth = true;
                    _dowNthVal = dow;
                    _dowNthIndex = nth;
                    return;
                }
            }

            if (expr == "*") {
                for (int i = _min; i <= _max; ++i) _values.insert(i);
                return;
            }

            std::stringstream ss(expr);
            std::string token;
            while (std::getline(ss, token, ',')) parseToken(token);
        }

        bool CronField::isNoSpec() const { return _noSpec; }
        bool CronField::match(int v) const { return !_noSpec && _values.count(v) > 0; }
        int  CronField::first() const { return (_noSpec || _values.empty()) ? -1 : *_values.begin(); }

        int CronField::nextGE(int cur) const {
            if (_noSpec || _values.empty()) return -1;
            auto it = _values.lower_bound(cur);
            return it == _values.end() ? -1 : *it;
        }

        bool CronField::domLast() const { return _domLast; }
        bool CronField::domLastWeekday() const { return _domLastWeekday; }
        bool CronField::domNearestWeekday() const { return _domNearestWeekday; }
        int  CronField::domWday() const { return _domWday; }
        int  CronField::domLastOffset() const { return _domLastOffset; }

        bool CronField::dowLast() const { return _dowLast; }
        int  CronField::dowLastVal() const { return _dowLastVal; }
        bool CronField::dowNth() const { return _dowNth; }
        int  CronField::dowNthVal() const { return _dowNthVal; }
        int  CronField::dowNthIndex() const { return _dowNthIndex; }

        void CronField::reset() {
            _values.clear();
            _noSpec = false;
            _domLast = _domLastWeekday = _domNearestWeekday = false;
            _domWday = -1; _domLastOffset = 0;
            _dowLast = _dowNth = false;
            _dowLastVal = _dowNthVal = -1;
            _dowNthIndex = 0;
        }

        void CronField::parseToken(const std::string& token) {
            std::string base = token;
            int step = 1;
            auto slashPos = token.find('/');
            if (slashPos != std::string::npos) {
                base = token.substr(0, slashPos);
                step = std::stoi(token.substr(slashPos + 1));
                if (step <= 0) throw std::invalid_argument("step must be > 0");
            }

            if (base == "*") {
                for (int i = _min; i <= _max; i += step) _values.insert(i);
                return;
            }

            auto dashPos = base.find('-');
            if (dashPos != std::string::npos) {
                int start = parseValue(base.substr(0, dashPos));
                int end = parseValue(base.substr(dashPos + 1));
                if (start < _min) start = _min;
                if (end > _max) end = _max;
                for (int i = start; i <= end; i += step) _values.insert(i);
                return;
            }

            int v = parseValue(base);
            if (v >= _min && v <= _max) _values.insert(v);
        }

        int CronField::parseValue(const std::string& s) const {
            if (_type == FieldType::MONTH) {
                static std::map<std::string, int> M = {
                    {"JAN",1},{"FEB",2},{"MAR",3},{"APR",4},{"MAY",5},{"JUN",6},
                    {"JUL",7},{"AUG",8},{"SEP",9},{"OCT",10},{"NOV",11},{"DEC",12}
                };
                auto it = M.find(upper(s));
                if (it != M.end()) return it->second;
            }
            if (_type == FieldType::DAY_OF_WEEK) {
                return parseDow(s);
            }
            return std::stoi(s);
        }

        int CronField::parseDow(const std::string& s) const {
            static std::map<std::string, int> D = {
                {"SUN",1},{"MON",2},{"TUE",3},{"WED",4},{"THU",5},{"FRI",6},{"SAT",7}
            };
            auto it = D.find(upper(s));
            if (it != D.end()) return it->second;
            int v = std::stoi(s);
            if (v == 0) return 1;
            return v;
        }

        std::string CronField::upper(std::string s) {
            for (auto& c : s) c = (char)std::toupper(c);
            return s;
        }
    }
}