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

#include "Object.h"
#include "UDP.h"

namespace m8r {

class UDPSocketProto : public ObjectFactory {
public:
    UDPSocketProto(Program*);

private:
    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue send(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue disconnect(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
    NativeFunction _constructor;
    NativeFunction _send;
    NativeFunction _disconnect;
};

class MyUDPDelegate : public NativeObject, public UDPDelegate {
public:
    MyUDPDelegate(IPAddr ip, uint16_t port, const Value& func, const Value& parent);
    virtual ~MyUDPDelegate()
    {
        if (_udp) {
            delete _udp;
        }
    }

    void send(int16_t connectionId, const char* data, uint16_t size)
    {
        if (!_udp) {
            return;
        }
        _udp->send(connectionId, data, size);
    }

    void disconnect(int16_t connectionId)
    {
        if (!_udp) {
            return;
        }
        _udp->disconnect(connectionId);
    }

    // UDPDelegate overrides
    virtual void UDPevent(UDP* udp, Event, int16_t connectionId, const char* data, uint16_t length) override;

private:
    UDP* _udp = nullptr;
    Value _func;
    Value _parent;
};
    

    
}