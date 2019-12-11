/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "ParseEngine.h"

using namespace m8r;

ParseEngine::OperatorInfo RODATA_ATTR ParseEngine::_opInfos[ ] = {
    { Token::STO,         1,  OperatorInfo::RightAssoc, false, Op::MOVE },
    { Token::ADDSTO,      2,  OperatorInfo::RightAssoc, true,  Op::ADD  },
    { Token::SUBSTO,      2,  OperatorInfo::RightAssoc, true,  Op::SUB  },
    { Token::MULSTO,      3,  OperatorInfo::RightAssoc, true,  Op::MUL  },
    { Token::DIVSTO,      3,  OperatorInfo::RightAssoc, true,  Op::DIV  },
    { Token::MODSTO,      3,  OperatorInfo::RightAssoc, true,  Op::MOD  },
    { Token::SHLSTO,      4,  OperatorInfo::RightAssoc, true,  Op::SHL  },
    { Token::SHRSTO,      4,  OperatorInfo::RightAssoc, true,  Op::SHR  },
    { Token::SARSTO,      4,  OperatorInfo::RightAssoc, true,  Op::SAR  },
    { Token::ANDSTO,      5,  OperatorInfo::RightAssoc, true,  Op::AND  },
    { Token::ORSTO,       5,  OperatorInfo::RightAssoc, true,  Op::OR   },
    { Token::XORSTO,      5,  OperatorInfo::RightAssoc, true,  Op::XOR  },
    { Token::LOR,         6,  OperatorInfo::LeftAssoc,  false, Op::LOR  },
    { Token::LAND,        7,  OperatorInfo::LeftAssoc,  false, Op::LAND },
    { Token::OR,          8,  OperatorInfo::LeftAssoc,  false, Op::OR   },
    { Token::XOR,         9,  OperatorInfo::LeftAssoc,  false, Op::XOR  },
    { Token::EQ,          11, OperatorInfo::LeftAssoc,  false, Op::EQ   },
    { Token::NE,          11, OperatorInfo::LeftAssoc,  false, Op::NE   },
    { Token::LT,          12, OperatorInfo::LeftAssoc,  false, Op::LT   },
    { Token::GT,          12, OperatorInfo::LeftAssoc,  false, Op::GT   },
    { Token::GE,          12, OperatorInfo::LeftAssoc,  false, Op::GE   },
    { Token::LE,          12, OperatorInfo::LeftAssoc,  false, Op::LE   },
    { Token::SHL,         13, OperatorInfo::LeftAssoc,  false, Op::SHL  },
    { Token::SHR,         13, OperatorInfo::LeftAssoc,  false, Op::SHR  },
    { Token::SAR,         13, OperatorInfo::LeftAssoc,  false, Op::SAR  },
    { Token::Plus,        14, OperatorInfo::LeftAssoc,  false, Op::ADD  },
    { Token::Minus,       14, OperatorInfo::LeftAssoc,  false, Op::SUB  },
    { Token::Star,        15, OperatorInfo::LeftAssoc,  false, Op::MUL  },
    { Token::Slash,       15, OperatorInfo::LeftAssoc,  false, Op::DIV  },
    { Token::Percent,     15, OperatorInfo::LeftAssoc,  false, Op::MOD  },
};

ParseEngine::ParseEngine(Parser* parser)
    : _parser(parser)
{
}

bool ParseEngine::expect(Token token)
{
    if (getToken() != token) {
        _parser->expectedError(token);
        return false;
    }
    retireToken();
    return true;
}

bool ParseEngine::expect(Token token, bool expected, const char* s)
{
    if (!expected) {
        _parser->expectedError(token, s);
    }
    return expected;
}

bool ParseEngine::statement()
{
    while (1) {
        if (getToken() == Token::EndOfFile) {
            return false;
        }
        if (getToken() == Token::Semicolon) {
            retireToken();
            return true;
        }
        if (functionStatement()) {
            return true;
        }
        if (classStatement()) {
            return true;
        }
        if (compoundStatement() || selectionStatement() ||
            switchStatement() || iterationStatement() || jumpStatement()) {
            return true;
        }
        if (getToken() == Token::Var) {
            retireToken();
            expect(Token::MissingVarDecl, variableDeclarationList() > 0);
            expect(Token::Semicolon);
            return true;
        } else if (getToken() == Token::Delete) {
            retireToken();
            leftHandSideExpression();
            expect(Token::Semicolon);
            return true;
        } else if (expression()) {
            _parser->discardResult();
            expect(Token::Semicolon);
            return true;
        } else {
            return false;
        }
    }
}

bool ParseEngine::classContentsStatement()
{
    if (getToken() == Token::EndOfFile) {
        return false;
    }
    if (getToken() == Token::Function) {
        retireToken();
        Atom name = _parser->atomizeString(getTokenValue().str);
        expect(Token::Identifier);
        Mad<Function> f = functionExpression(false);
        _parser->currentClass()->setProperty(name, Value(f));
        return true;
    }
    if (getToken() == Token::Constructor) {
        retireToken();
        Mad<Function> f = functionExpression(true);
        if (!f.valid()) {
            return false;
        }
        _parser->currentClass()->setProperty(Atom(SA::constructor), Value(f));
        return true;
    }
    if (getToken() == Token::Var) {
        retireToken();

        if (getToken() != Token::Identifier) {
            return false;
        }
        Atom name = _parser->atomizeString(getTokenValue().str);
        retireToken();
        Value v = Value::NullValue();
        if (getToken() == Token::STO) {
            retireToken();
            
            switch(getToken()) {
                case Token::Float: v = Value(Float(getTokenValue().number)); retireToken(); break;
                case Token::Integer: v = Value(getTokenValue().integer); retireToken(); break;
                case Token::String: v = Value(_parser->program()->addStringLiteral(getTokenValue().str)); retireToken(); break;
                case Token::True: v = Value(true); retireToken(); break;
                case Token::False: v = Value(false); retireToken(); break;
                case Token::Null: v = Value::NullValue(); retireToken(); break;
                case Token::Undefined: v = Value(); retireToken(); break;
                default: expect(Token::ConstantValueRequired);
            }
        }
        _parser->currentClass()->setProperty(name, v);
        expect(Token::Semicolon);
        return true;
    }
    return false;
}

bool ParseEngine::functionStatement()
{
    if (getToken() != Token::Function) {
        return false;
    }
    retireToken();
    Atom name = _parser->atomizeString(getTokenValue().str);
    expect(Token::Identifier);
    Mad<Function> f = functionExpression(false);
    _parser->addNamedFunction(f, name);
    return true;
}

bool ParseEngine::classStatement()
{
    if (getToken() != Token::Class) {
        return false;
    }
    retireToken();
    Atom name = _parser->atomizeString(getTokenValue().str);
    _parser->addVar(name);
    _parser->emitId(name, Parser::IdType::MustBeLocal);

    expect(Token::Identifier);
    
    if (!expect(Token::Expr, classExpression(), "class")) {
        return false;
    }
    _parser->emitMove();
    _parser->discardResult();
    return true;
}

bool ParseEngine::compoundStatement()
{
    if (getToken() != Token::LBrace) {
        return false;
    }
    retireToken();
    while (statement()) ;
    expect(Token::RBrace);
    return true;
}

bool ParseEngine::selectionStatement()
{
    if (getToken() != Token::If) {
        return false;
    }
    retireToken();
    expect(Token::LParen);
    expression();
    
    Label ifLabel = _parser->label();
    Label elseLabel = _parser->label();
    _parser->addMatchedJump(m8r::Op::JF, elseLabel);

    expect(Token::RParen);
    statement();

    if (getToken() == Token::Else) {
        retireToken();
        _parser->addMatchedJump(m8r::Op::JMP, ifLabel);
        _parser->matchJump(elseLabel);
        statement();
        _parser->matchJump(ifLabel);
    } else {
        _parser->matchJump(elseLabel);
    }
    return true;
}

bool ParseEngine::switchStatement()
{
    if (getToken() != Token::Switch) {
        return false;
    }
    retireToken();
    expect(Token::LParen);
    expression();
    expect(Token::RParen);
    expect(Token::LBrace);

    typedef struct { Label toStatement; Label fromStatement; int32_t statementAddr; } CaseEntry;

    Vector<CaseEntry> cases;
    
    // This pushes a deferral block onto the deferred stack.
    // We use resumeDeferred()/endDeferred() for each statement block
    int32_t deferredStatementStart = _parser->startDeferred();
    _parser->endDeferred();
    
    int32_t defaultStatement = 0;
    Label defaultFromStatementLabel;
    bool haveDefault = false;
    
    while (true) {
        if (getToken() == Token::Case || getToken() == Token::Default) {
            bool isDefault = getToken() == Token::Default;
            retireToken();

            if (isDefault) {
                expect(Token::DuplicateDefault, !haveDefault);
                haveDefault = true;
            } else {
                expression();
                _parser->emitCaseTest();
            }
            
            expect(Token::Colon);
            
            if (isDefault) {
                defaultStatement = _parser->resumeDeferred();
                statement();
                defaultFromStatementLabel = _parser->label();
                _parser->addMatchedJump(Op::JMP, defaultFromStatementLabel);
                _parser->endDeferred();
            } else {
                CaseEntry entry;
                entry.toStatement = _parser->label();
                _parser->addMatchedJump(Op::JT, entry.toStatement);
                entry.statementAddr = _parser->resumeDeferred();
                if (statement()) {
                    entry.fromStatement = _parser->label();
                    _parser->addMatchedJump(Op::JMP, entry.fromStatement);
                } else {
                    entry.fromStatement.label = -1;
                }
                _parser->endDeferred();
                cases.push_back(entry);
            }
        } else {
            break;
        }
    }
    
    expect(Token::RBrace);
    
    // We need a JMP statement here. It will either jump after all the case
    // statements or to the default statement
    Label endJumpLabel = _parser->label();
    _parser->addMatchedJump(Op::JMP, endJumpLabel);
    
    int32_t statementStart = _parser->emitDeferred();
    Label afterStatementsLabel = _parser->label();
    
    if (haveDefault) {
        _parser->matchJump(endJumpLabel, defaultStatement - deferredStatementStart + statementStart);

        // Adjust the matchedAddr in the defaultFromStatementLabel into the code space it got copied to
        defaultFromStatementLabel.matchedAddr += statementStart - deferredStatementStart;
        _parser->matchJump(defaultFromStatementLabel, afterStatementsLabel);
    } else {
        _parser->matchJump(endJumpLabel, afterStatementsLabel);
    }

    for (auto it : cases) {
        _parser->matchJump(it.toStatement, it.statementAddr - deferredStatementStart + statementStart);
        
        if (it.fromStatement.label >= 0) {
            // Adjust the matchedAddr in the fromStatement into the code space it got copied to
            it.fromStatement.matchedAddr += statementStart - deferredStatementStart;
            _parser->matchJump(it.fromStatement, afterStatementsLabel);
        }
    }
    
    _parser->discardResult();
    return true;
}

void ParseEngine::forLoopCondAndIt()
{
    // On entry, we are at the semicolon before the cond expr
    expect(Token::Semicolon);
    Label label = _parser->label();
    expression(); // cond expr
    _parser->addMatchedJump(m8r::Op::JF, label);
    _parser->startDeferred();
    expect(Token::Semicolon);
    expression(); // iterator
    _parser->discardResult();
    _parser->endDeferred();
    expect(Token::RParen);
    statement();

    // resolve the continue statements
    for (auto it : _continueStack.back()) {
        _parser->matchJump(it);
    }

    _parser->emitDeferred();
    _parser->jumpToLabel(Op::JMP, label);
    _parser->matchJump(label);
}

void ParseEngine::forIteration(Atom iteratorName)
{
    // On entry we have the name of the iterator variable and the colon has been parsed.
    // We need to parse the obj expression and then generate the equivalent of the following:
    //
    //      for (var it = new obj.iterator(obj); !it.done; it.next()) ...
    //
    if (iteratorName) {
        _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    }
    leftHandSideExpression();
    expect(Token::RParen);

    _parser->emitDup();
    _parser->emitPush();
    _parser->emitId(Atom(SA::iterator), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::NEW, -1, 1);
    _parser->emitMove();
    _parser->discardResult();
    
    Label label = _parser->label();
    _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    _parser->emitId(Atom(SA::done), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::CALL, -1, 0);

    _parser->addMatchedJump(m8r::Op::JT, label);

    statement();

    // resolve the continue statements
    for (auto it : _continueStack.back()) {
        _parser->matchJump(it);
    }

    _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    _parser->emitId(Atom(SA::next), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::CALL, -1, 0);
    _parser->discardResult();

    _parser->jumpToLabel(Op::JMP, label);
    _parser->matchJump(label);
}

bool ParseEngine::iterationStatement()
{
    Token type = getToken();
    if (type != Token::While && type != Token::Do && type != Token::For) {
        return false;
    }
    
    retireToken();
    
    _breakStack.emplace_back();
    _continueStack.emplace_back();
    if (type == Token::While) {
        expect(Token::LParen);
        Label label = _parser->label();
        expression();
        _parser->addMatchedJump(m8r::Op::JF, label);
        expect(Token::RParen);
        statement();
        
        // resolve the continue statements
        for (auto it : _continueStack.back()) {
            _parser->matchJump(it);
        }
        
        _parser->jumpToLabel(Op::JMP, label);
        _parser->matchJump(label);
    } else if (type == Token::Do) {
        Label label = _parser->label();
        statement();

        // resolve the continue statements
        for (auto it : _continueStack.back()) {
            _parser->matchJump(it);
        }

        expect(Token::While);
        expect(Token::LParen);
        expression();
        _parser->jumpToLabel(m8r::Op::JT, label);
        expect(Token::RParen);
        expect(Token::Semicolon);
    } else if (type == Token::For) {
        expect(Token::LParen);
        if (getToken() == Token::Var) {
            retireToken();
            
            // Hang onto the identifier. If this is a for..in we need to know it
            Atom name;
            if (getToken() == Token::Identifier) {
                name = _parser->atomizeString(getTokenValue().str);
            }
            
            uint32_t count = variableDeclarationList();
            expect(Token::MissingVarDecl, count  > 0);
            if (getToken() == Token::Colon) {
                // for-in case with var
                expect(Token::OneVarDeclAllowed, count == 1);
                retireToken();
                forIteration(name);
            } else {
                expect(Token::MissingVarDecl, count > 0);
                forLoopCondAndIt();
            }
        } else {
            if (expression()) {
                if (getToken() == Token::Colon) {
                    // for-in case with left hand expr
                    retireToken();
                    forIteration(Atom());
                } else {
                    forLoopCondAndIt();
                }
            }
        }
    }

    // resolve the break statements
    for (auto it : _breakStack.back()) {
        _parser->matchJump(it);
    }
    
    _breakStack.pop_back();
    _continueStack.pop_back();
    
    return true;
}

bool ParseEngine::jumpStatement()
{
    if (getToken() == Token::Break || getToken() == Token::Continue) {
        bool isBreak = getToken() == Token::Break;
        retireToken();
        expect(Token::Semicolon);
        
        // Add a JMP which will get resolved by the enclosing iteration statement
        Label label = _parser->label();
        _parser->addMatchedJump(Op::JMP, label);
        if (isBreak) {
            _breakStack.back().push_back(label);
        } else {
            _continueStack.back().push_back(label);
        }
        return true;
    }
    if (getToken() == Token::Return) {
        retireToken();
        uint32_t count = 0;
        if (expression()) {
            count = 1;
        }
        
        // If this is a ctor, we need to return this if we're not returning anything else
        if (!count && _parser->functionIsCtor()) {
            _parser->pushThis();
            count = 1;
        }
        
        _parser->emitCallRet(m8r::Op::RET, -1, count);
        expect(Token::Semicolon);
        return true;
    }
    return false;
}

uint32_t ParseEngine::variableDeclarationList()
{
    uint32_t count = 0;
    while (variableDeclaration()) {
        ++count;
        if (getToken() != Token::Comma) {
            break;
        }
        retireToken();
    }
    return count;
}

bool ParseEngine::variableDeclaration()
{
    if (getToken() != Token::Identifier) {
        return false;
    }
    Atom name = _parser->atomizeString(getTokenValue().str);
    _parser->addVar(name);
    retireToken();
    if (getToken() != Token::STO) {
        return true;
    }
    retireToken();
    _parser->emitId(name, Parser::IdType::MustBeLocal);

    if (!expect(Token::Expr, expression(), "variable")) {
        return false;
    }

    _parser->emitMove();
    _parser->discardResult();
    return true;
}

bool ParseEngine::arithmeticPrimary()
{
    if (getToken() == Token::LParen) {
        retireToken();
        expression();
        expect(Token::RParen);
        return true;
    }
    
    Op op;
    switch(getToken()) {
        case Token::INC: op = Op::PREINC; break;
        case Token::DEC: op = Op::PREDEC; break;
        case Token::Minus: op = Op::UMINUS; break;
        case Token::Twiddle: op = Op::UNOT; break;
        case Token::Bang: op = Op::UNEG; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op != Op::UNKNOWN) {
        retireToken();
        arithmeticPrimary();
        _parser->emitUnOp(op);
        return true;
    }
    
    if (!leftHandSideExpression()) {
        return false;
    }
    switch(getToken()) {
        case Token::INC: op = Op::POSTINC; break;
        case Token::DEC: op = Op::POSTDEC; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op != Op::UNKNOWN) {
        retireToken();
        _parser->emitUnOp(op);
    }
    return true;
}

bool ParseEngine::expression(uint8_t minPrec)
{
    if (!arithmeticPrimary()) {
        return false;
    }
    
    if (getToken() == Token::Question) {
        // Test the value on TOS. If true leave the next value on the stack, otherwise leave the one after that
        retireToken();

        Label ifLabel = _parser->label();
        Label elseLabel = _parser->label();
        _parser->addMatchedJump(m8r::Op::JF, elseLabel);
        _parser->pushTmp();
        expression();
        _parser->emitMove();
        expect(Token::Colon);
        _parser->addMatchedJump(m8r::Op::JMP, ifLabel);
        _parser->matchJump(elseLabel);
        expression();
        _parser->emitMove();
        _parser->matchJump(ifLabel);
    }
    
    while(1) {
        OperatorInfo* endit = _opInfos + sizeof(_opInfos) / sizeof(OperatorInfo);
        OperatorInfo* it = std::find(_opInfos, endit, getToken());
        if (it == endit || it->prec < minPrec) {
            break;
        }
        uint8_t nextMinPrec = (it->assoc == OperatorInfo::LeftAssoc) ? (it->prec + 1) : it->prec;
        retireToken();
        if (it->sto) {
            _parser->emitDup();
        }
    
        // If the op is LAND or LOR we want to short circuit. Add logic
        // here to jump over the next expression if TOS is false in the
        // case of LAND or true in the case of LOR
        if (it->op == Op::LAND || it->op == Op::LOR) {
            _parser->emitDup();
            Label passLabel = _parser->label();
            Label skipLabel = _parser->label();
            bool skipResult = it->op != Op::LAND;
            _parser->addMatchedJump(skipResult ? Op::JT : Op::JF, skipLabel);
            
            if (!expect(Token::Expr, expression(nextMinPrec), "right-hand side")) {
                return false;
            }
            
            _parser->emitBinOp(it->op);
            _parser->addMatchedJump(Op::JMP, passLabel);
            _parser->matchJump(skipLabel);
            _parser->pushK(Value(skipResult));
            _parser->emitMove();
            _parser->matchJump(passLabel);
        } else {
            if (!expect(Token::Expr, expression(nextMinPrec), "right-hand side")) {
                return false;
            }
            _parser->emitBinOp(it->op);
        }
        
        if (it->sto) {
            _parser->emitMove();
        }
    }
    return true;
}

bool ParseEngine::leftHandSideExpression()
{
    if (!memberExpression()) {
        return false;
    }
    
    int32_t objectReg = -1;
    while(1) {
        if (getToken() == Token::LParen) {
            retireToken();
            uint32_t argCount = argumentList();
            expect(Token::RParen);
            _parser->emitCallRet(m8r::Op::CALL, objectReg, argCount);
            objectReg = -1;
        } else if (getToken() == Token::LBracket) {
            retireToken();
            expression();
            expect(Token::RBracket);
            objectReg = _parser->emitDeref(Parser::DerefType::Elt);
        } else if (getToken() == Token::Period) {
            retireToken();
            Atom name = _parser->atomizeString(getTokenValue().str);
            expect(Token::Identifier);
            _parser->emitId(name, Parser::IdType::NotLocal);
            objectReg = _parser->emitDeref(Parser::DerefType::Prop);
        } else {
            return true;
        }
    }
}

bool ParseEngine::memberExpression()
{
    if (getToken() == Token::New) {
        retireToken();
        memberExpression();
        uint32_t argCount = 0;
        if (getToken() == Token::LParen) {
            retireToken();
            argCount = argumentList();
            expect(Token::RParen);
        }
        _parser->emitCallRet(m8r::Op::NEW, -1, argCount);
        return true;
    }
    
    if (getToken() == Token::Function) {
        retireToken();
        Mad<Function> f = functionExpression(false);
        if (!f.valid()) {
            return false;
        }
        _parser->pushK(Value(f));
        return true;
    }
    if (getToken() == Token::Class) {
        retireToken();
        classExpression();
        return true;
    }
    return primaryExpression();
}

uint32_t ParseEngine::argumentList()
{
    uint32_t i = 0;
    
    if (!expression()) {
        return i;
    }
    _parser->emitPush();
    ++i;
    while (getToken() == Token::Comma) {
        retireToken();
        expression();
        _parser->emitPush();
        ++i;
    }
    return i;
}

bool ParseEngine::primaryExpression()
{
    switch(getToken()) {
        case Token::Identifier: _parser->emitId(_parser->atomizeString(getTokenValue().str), Parser::IdType::MightBeLocal); retireToken(); break;
        case Token::This: _parser->pushThis(); retireToken(); break;
        case Token::Float: _parser->pushK(Value(Float(getTokenValue().number))); retireToken(); break;
        case Token::Integer: _parser->pushK(Value(getTokenValue().integer)); retireToken(); break;
        case Token::String: _parser->pushK(getTokenValue().str); retireToken(); break;
        case Token::True: _parser->pushK(Value(true)); retireToken(); break;
        case Token::False: _parser->pushK(Value(false)); retireToken(); break;
        case Token::Null: _parser->pushK(Value::NullValue()); retireToken(); break;
        case Token::Undefined: _parser->pushK(Value()); retireToken(); break;
        case Token::LBracket:
            retireToken();
            _parser->emitLoadLit(true);
            if (expression()) {
                _parser->emitAppendElt();
                while (getToken() == Token::Comma) {
                    retireToken();
                    if (!expect(Token::Expr, expression(), "array element")) {
                        break;
                    }
                    _parser->emitAppendElt();
                }
            }
            expect(Token::RBracket);
            break;
        case Token::LBrace:
            retireToken();
            _parser->emitLoadLit(false);
            if (propertyAssignment()) {
                _parser->emitAppendProp();
                while (getToken() == Token::Comma) {
                    retireToken();
                    if (!expect(Token::PropertyAssignment, propertyAssignment())) {
                        break;
                    }
                    _parser->emitAppendProp();
                }
            }
            expect(Token::RBrace);
            break;
            
            break;
        
        default: return false;
    }
    return true;
}

bool ParseEngine::propertyAssignment()
{
    if (!propertyName()) {
        return false;
    }
    return expect(Token::Colon) && expect(Token::Expr, expression());
}

bool ParseEngine::propertyName()
{
    switch(getToken()) {
        case Token::Identifier: _parser->emitId(_parser->atomizeString(getTokenValue().str), Parser::IdType::NotLocal); retireToken(); return true;
        case Token::String: _parser->pushK(getTokenValue().str); retireToken(); return true;
        case Token::Float: _parser->pushK(Value(Float(getTokenValue().number))); retireToken(); return true;
        case Token::Integer:
            _parser->pushK(Value(getTokenValue().integer));
            retireToken();
            return true;
        default: return false;
    }
}

Mad<Function> ParseEngine::functionExpression(bool ctor)
{
    expect(Token::LParen);
    _parser->functionStart(ctor);
    formalParameterList();
    _parser->functionParamsEnd();
    expect(Token::RParen);
    expect(Token::LBrace);
    while(statement()) { }
    expect(Token::RBrace);
    return _parser->functionEnd();
}

bool ParseEngine::classExpression()
{
    _parser->classStart();
    expect(Token::LBrace);
    while(classContentsStatement()) { }
    expect(Token::RBrace);
    _parser->classEnd();
    return true;
}

void ParseEngine::formalParameterList()
{
    if (getToken() != Token::Identifier) {
        return;
    }
    while (1) {
        _parser->functionAddParam(_parser->atomizeString(getTokenValue().str));
        retireToken();
        if (getToken() != Token::Comma) {
            return;
        }
        retireToken();
        if (getToken() != Token::Identifier) {
            _parser->expectedError(Token::Identifier);
            return;
        }
    }
}

