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

#include "Array.h"
#include "Base64.h"
#include "GPIO.h"
#include "Iterator.h"
#include "Object.h"
#include "TCP.h"

namespace m8r {

class Function;

class Global : public ObjectFactory {
public:
    Global(Program*);
    virtual ~Global();

    static ObjectId addObject(Object* obj, bool collectable);
    static void removeObject(ObjectId objectId);
    
    static StringId createString(const char* s, int32_t length = -1);
    static StringId createString(const String& s);

    static bool isValid(const ObjectId& id) { return _objectStore.isValid(id); }
    static bool isValid(const StringId& id) { return _stringStore.isValid(id); }
    
    static Object* obj(const Value& value)
    {
        ObjectId id = value.asObjectIdValue();
        return id ? obj(id) : nullptr;
    }
    static Object* obj(const ObjectId& id) { return _objectStore.ptr(id); }
    
    static String& str(const Value& value)
    {
        return str(value.asStringIdValue());
    }
    
    static String& str(const StringId& id)
    {
        // _strings[0] contains an error entry for when invalid ids are passed
        String* s = _stringStore.ptr(id);
        return s ? *s : *_stringStore.ptr(StringId(0));
    }
    
    static void gc(ExecutionUnit*);
    static void gcMark(ExecutionUnit*, const Value& value);
    static void gcMark(ExecutionUnit*, const ObjectId& objectId);
    
private:        
    Array _array;
    Base64 _base64;
    GPIO _gpio;
    Iterator _iterator;
    TCPProto _tcp;
    UDPProto _udp;
    IPAddrProto _ipAddr;

    static CallReturnValue currentTime(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue delay(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue print(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue printf(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue println(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toFloat(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toUInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue arguments(ExecutionUnit*, Value thisValue, uint32_t nparams);

    NativeFunction _currentTime;
    NativeFunction _delay;
    NativeFunction _print;
    NativeFunction _printf;
    NativeFunction _println;
    NativeFunction _toFloat;
    NativeFunction _toInt;
    NativeFunction _toUInt;
    NativeFunction _arguments;

    template<typename IdType, typename ValueType> class IdStore {
    public:
        IdType add(ValueType*);
        void remove(IdType, bool del);
        bool isValid(const IdType& id) const { return id.raw() < _values.size(); }
        bool empty() const { return _values.empty(); }
        ValueType* ptr(const IdType& id) const { return isValid(id) ? _values[id.raw()] : nullptr; }
        
        void gcClear() { _valueMarked.clear(); _valueMarked.resize(_values.size()); }
        void gcMark(IdType id) { _valueMarked[id.raw()] = true; }
        
        bool isGCMarked(IdType id) { return _valueMarked[id.raw()]; }

        void gcSweep()
        {
            for (uint16_t i = 0; i < _values.size(); ++i) {
                if (_values[i] && !_valueMarked[i]) {
                    remove(IdType(i), true);
                }
            }
        }
        
    private:
        std::vector<ValueType*> _values;
        std::vector<bool> _valueMarked;
        uint32_t _freeValueIdCount = 0;
    };

    static IdStore<StringId, String> _stringStore;
    static IdStore<ObjectId, Object> _objectStore;
};
    
template<typename IdType, typename ValueType>
IdType Global::IdStore<IdType, ValueType>::add(ValueType* value)
{
    if (_freeValueIdCount) {
        for (uint32_t i = 0; i < _values.size(); ++i) {
            if (!_values[i]) {
                _values[i] = value;
                _freeValueIdCount--;
                return IdType(i);
            }
        }
        assert(false);
        return IdType();
    }
    
    IdType id(_values.size());
    _values.push_back(value);
    _valueMarked.resize(_values.size());
    _valueMarked[id.raw()] = true;
    return id;
}

template<typename IdType, typename ValueType>
void Global::IdStore<IdType, ValueType>::remove(IdType id, bool del)
{
    assert(id && id.raw() < _values.size() && _values[id.raw()]);

    if (del) {
        delete _values[id.raw()];
    }
    _values[id.raw()] = nullptr;
    _freeValueIdCount++;
}

template<>
inline void Global::IdStore<ObjectId, Object>::gcSweep()
{
    for (uint16_t i = 0; i < _values.size(); ++i) {
        if (_values[i] && !_valueMarked[i] && _values[i]->collectable()) {
            remove(ObjectId(i), true);
        }
    }
}

}
