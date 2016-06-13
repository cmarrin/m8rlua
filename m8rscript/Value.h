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

#include "Atom.h"
#include "Containers.h"
#include "Value.h"

namespace m8r {

class Object;
class Value;

typedef union {
    void* v;
    float f;
    int32_t i;
    Object* o;
    const char* s;
    Atom a;
} U;

class Value {
public:
    typedef m8r::Map<Atom, Value> Map;
    enum class Type { None, Object, Float, Integer, String, Id, Ref };

    Value() : _value(nullptr), _type(Type::None), _id(NoId) { }
    Value(const Value& other) : _value(other._value), _type(other._type), _id(other._id) { }
    
    Value(Object* obj) : _value(valueFromObj(obj)) , _type(Type::Object), _id(NoId) { }
    Value(float value) : _value(valueFromFloat(value)) , _type(Type::Float), _id(NoId) { }
    Value(int32_t value) : _value(valueFromInt(value)) , _type(Type::Integer), _id(NoId) { }
    Value(const char* value) : _value(valueFromStr(value)) , _type(Type::String), _id(NoId) { }
    Value(Atom value) : _value(nullptr), _type(Type::Id), _id(value.rawAtom()) { }
    Value(Object* obj, uint16_t index) : _value(valueFromObj(obj)), _type(Type::Ref), _id(index) { }
    
    ~Value();
    
    Type type() const { return _type; }
    Object* objectValue() const { return (_type == Type::Object) ? objFromValue() : nullptr; }
    bool boolValue() const;
    int32_t intValue() const { return (_type == Type::Integer) ? intFromValue() : 0; }
    float floatValue() const { return (_type == Type::Float) ? floatFromValue() : 0; }
    const char* stringValue() const { return (_type == Type::String) ? strFromValue() : nullptr; }
    Atom idValue() const { return (_type == Type::Id) ? Atom::atomFromRawAtom(_id) : Atom::emptyAtom(); }
    
    bool setValue(const Value&);
    
private:    
    inline void* valueFromFloat(float f) const { U u; u.f = f; return u.v; }
    inline void* valueFromInt(int32_t i) const { U u; u.i = i; return u.v; }
    inline void* valueFromObj(Object* o) const { U u; u.o = o; return u.v; }
    inline void* valueFromStr(const char* s) const { U u; u.s = s; return u.v; }

    inline float floatFromValue() const { U u; u.v = _value; return u.f; }
    inline int32_t intFromValue() const { U u; u.v = _value; return u.i; }
    inline Object* objFromValue() const { U u; u.v = _value; return u.o; }
    inline const char* strFromValue() const { U u; u.v = _value; return u.s; }

    static constexpr uint16_t NoId = std::numeric_limits<uint16_t>::max();
    void* _value;
    Type _type : 4;
    uint16_t _id;
};

}
