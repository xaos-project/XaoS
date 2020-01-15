/*
 * XaoS Formula Evaluator
 * Copyright (c) 2020 J.B. Langston
 *
 * Lexing and parsing logic based on TINYEXPR
 * Copyright (c) 2015-2018 Lewis Van Winkle
 * http://CodePlea.com
 *
 * Bytecode idea (but no code) comes from SFFE
 * https://github.com/malczak/sffe
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgement in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __FORMEVAL_H__
#define __FORMEVAL_H__

#include <complex>
#include <vector>
#include <string>
#include <iostream>

namespace FormEval
{

using namespace std;

typedef long double number_t;
typedef complex<number_t> Value;
typedef Value *Variable;
typedef Value **Parameters;
typedef void (*Function)(Parameters);

enum class NodeType { Invalid, Constant, Variable, Function };

class Node
{
  private:
    string name;
    NodeType type = NodeType::Invalid;
    Value value = 0;
    Variable variable = nullptr;
    Function function = nullptr;
    int arity = 0;
    vector<Node> children;
    Parameters parameters = nullptr;

  public:
    Node(Value value);
    Node(string identifier);
    Node();
    ~Node();

    void addChild(Node child);
    bool isValid() { return type != NodeType::Invalid; }
    bool isFunction() { return type == NodeType::Function; }
    int getArity() { return arity; }
    string getName() { return name; }
    Value getValue() { return value; }
    Variable compile(vector<Node *> &stack);
    inline void eval() { function(parameters); asm (""); }
};

enum class Error {
    None,
    InvalidCharacter,
    UnknownIdentifier,
    MissingParen,
    MissingComma,
    TooFewParameters,
    TooManyParameters,
    UnexpectedComma,
    UnexpectedParen,
    MissingOperand,
    UnexpectedToken
};

enum class Token {
    Whitespace,
    Error,
    End,
    Comma,
    OpenParen,
    CloseParen,
    Number,
    Identifier,
    Plus,
    Minus,
    Times,
    Divide,
    Power
};

class Parser
{
  private:
    Token token = Token::Whitespace;
    Error error = Error::None;
    string expression;
    const char *first = nullptr;
    const char *start = nullptr;
    const char *next = nullptr;
    pair<int, int> errorloc;
    Node root;
    vector<Node *> stack;
    number_t number = 0;
    string name;
    Variable result = nullptr;

    void setError(Error err);
    void nextToken();
    Node function(Node function);
    //Node list();
    Node expr();
    Node power();
    Node base();
    Node factor();
    Node term();

  public:
    static void addConstant(string name, Value value);
    static void addVariable(string name, Variable address);
    static void addFunction(string name, Function address, int arity);

    string getExpression();
    string errorMessage();
    pair<int, int> errorLocation();
    Error parse(string expression);
    inline Value eval() {
        for (Node *node : stack) {
            node->eval();
        }
        return *result;
    }
};

} // namespace FormEval

#endif /*__FORMEVAL_H__*/
