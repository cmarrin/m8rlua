/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Error.h"
#include "Executable.h"
#include "ScriptingLanguage.h"

struct lua_State;

namespace m8r {
    class Stream;
}

namespace lua {

class LuaScriptingLanguage : public m8r::ScriptingLanguage
{
public:
    virtual const char* suffix() const override { return "lua"; }
    virtual m8r::SharedPtr<m8r::Executable> create() const override;
};


//////////////////////////////////////////////////////////////////////////////
//
//  Class: LuaEngine
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class LuaEngine : public m8r::Executable
{
public:
    LuaEngine() { }
    
    ~LuaEngine();
    
    uint32_t nerrors() const { return _nerrors; }

    virtual bool load(const m8r::Stream&) override;
    virtual m8r::CallReturnValue execute() override;

private:
    static const char* readStream(lua_State*, void* data, size_t* size);

    lua_State * _state = nullptr;
    uint32_t _nerrors = 0;
    m8r::Error _error = m8r::Error::Code::OK;
    const m8r::Stream* _stream;
    m8r::String _errorString;
    int _functionIndex = -1;
    char _buf;
};

}
