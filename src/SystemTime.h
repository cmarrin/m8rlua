/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
      
    - Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
      
    - Neither the name of the <ORGANIZATION> nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#pragma once

#include "SystemInterface.h"

#include <cstdint>
#include <limits>

namespace m8r {

// A Duration is a 32 bit number representing time. The upper 30 bits
// is an integer value. The lower 2 bits represents the units of the value:
//
//      0 - microseconds (range is 8.9 minutes)
//      1 - milliseconds (range is 6.2 days)
//      2 - seconds      (range is 139 years)
//      3 - reserved

class Duration
{
public:
    friend class Time;
    
    static constexpr uint32_t Shift = 2;
    static constexpr uint32_t UnitsMask = (1 << Shift) - 1;
    static constexpr uint32_t MaxValue = (1 << (sizeof(int32_t) - Shift)) - 1;
    
    enum class Units { us = 0, ms = 1, sec = 2, none = 3 };

    Duration() { }
    Duration(uint64_t value, Units units = Units::none)
    {
        if (units == Units::none) {
            // Select the best units
            bool sign = value < 0;
            if (sign) {
                value = -value;
            }
            if (value < MaxValue) {
                units = Units::us;
            } else {
                value /= 1000;
                if (value < MaxValue) {
                    units = Units::ms;
                } else {
                    value /= 1000;
                    units = Units::sec;
                }
            }
            if (sign) {
                value = -value;
            }
        }
        
        int32_t v = (value > MaxValue) ? MaxValue : static_cast<int32_t>(value);
        _value = (v << Shift);
        setUnits(units);
    }
        
    Duration operator - ()
    {
        Units u = units();
        _value = -(_value & ~UnitsMask);
        setUnits(u);
        return *this;
    }
    
    Duration operator+(const Duration& other) const { return Duration(us() + other.us()); }
    Duration operator-(const Duration& other) const { return Duration(us() - other.us()); }

    Duration operator+=(const Duration& other) { *this = *this + other; return *this; }
    Duration operator-=(const Duration& other) { *this = *this - other; return *this; }

    bool operator==(const Duration& other) const { return us() == other.us(); }
    bool operator!=(const Duration& other) const { return us() != other.us(); }
    bool operator<(const Duration& other)  const { return us() < other.us(); }
    bool operator<=(const Duration& other) const { return us() <= other.us(); }
    bool operator>(const Duration& other)  const { return us() > other.us(); }
    bool operator>=(const Duration& other) const { return us() >= other.us(); }

    int64_t us() const
    {
        int64_t v = _value >> Shift;
        switch (units()) {
            case Units::ms: v *= 1000; break;
            case Units::sec: v *= 1000000; break;
            default: break;
        }
        return v;
    }

    int32_t ms() const
    {
        int32_t v = _value >> Shift;
        switch (units()) {
            case Units::us: v /= 1000; break;
            case Units::sec: v *= 1000; break;
            default: break;
        }
        return v;
    }

private:
    Units units() const { return static_cast<Units>(_value & UnitsMask); }
    void setUnits(Units u) { _value = (_value & ~UnitsMask) | static_cast<int32_t>(u); }
    
    int32_t _value = 0;
};

static inline Duration operator ""  _ms(uint64_t v) { return Duration(v, Duration::Units::ms); }
static inline Duration operator ""  _us(uint64_t v) { return Duration(v, Duration::Units::us); }
static inline Duration operator "" _sec(uint64_t v) { return Duration(v, Duration::Units::sec); }

// A Time value is an absolute time starting when the system started.

class Time
{
public:
    Time() { }
    Time(const Time& other) : _value(other) { }

    static Time now() { return Time(SystemInterface::currentMicroseconds()); }
    static Time longestTime() { return Time(std::numeric_limits<uint64_t>::max()); }
    
    friend Time operator+(const Time& t, const Duration& d) { return Time(t._value + d.us()); }
    friend Time operator+(const Duration& d, const Time& t) { return Time(t._value + d.us()); }
    friend Time operator-(const Time& t, const Duration& d) { return Time(t._value - d.us()); }
    friend Duration operator-(const Time& t1, const Time& t2) { return Duration(t1._value - t2._value, Duration::Units::us); }

    Time operator+=(const Duration& other) { *this = *this + other; return *this; }
    Time operator-=(const Duration& other) { *this = *this - other; return *this; }

    bool operator==(const Time& other) const { return _value == other._value; }
    bool operator!=(const Time& other) const { return _value != other._value; }
    bool operator<(const Time& other)  const { return _value < other._value; }
    bool operator<=(const Time& other) const { return _value <= other._value; }
    bool operator>(const Time& other)  const { return _value > other._value; }
    bool operator>=(const Time& other) const { return _value >= other._value; }
    
    operator uint64_t () const { return _value; }

private:
    Time(uint64_t t) : _value(t) { }
    
    uint64_t _value = 0;
};

}