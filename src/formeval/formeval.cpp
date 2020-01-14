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

#include "formeval.h"
#include <complex>
#include <vector>
#include <string>
#include <map>
#include <iostream>

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

#ifndef INFINITY
#define INFINITY (1.0 / 0.0)
#endif

using namespace std;

namespace FormEval
{

map<string, Value> constants = {{"pi", 3.14159265358979323846},
                                {"e", 2.71828182845904523536},
                                {"i", {0, 1}}};

map<string, Variable> variables;

map<string, std::pair<int, Function>> functions = {
    // Operators
    {"+", {2, [](Parameters p) { return *p[0] + *p[1]; }}},
    {"-", {2, [](Parameters p) { return *p[0] - *p[1]; }}},
    {"*", {2, [](Parameters p) { return *p[0] * *p[1]; }}},
    {"/", {2, [](Parameters p) { return *p[0] / *p[1]; }}},
    {"^", {2, [](Parameters p) { return pow(*p[0], *p[1]); }}},
    {"negate", {1, [](Parameters p) { return *p[0] * Value(-1); }}},

    // Complex-specific functions
    {"re", {1, [](Parameters p) { return Value(real(*p[0]), 0); }}},
    {"real", {1, [](Parameters p) { return Value(real(*p[0]), 0); }}},
    {"im", {1, [](Parameters p) { return Value(0, imag(*p[0])); }}},
    {"imag", {1, [](Parameters p) { return Value(0, imag(*p[0])); }}},
    {"abs", {1, [](Parameters p) { return Value(abs(*p[0]), 0); }}},
    {"arg", {1, [](Parameters p) { return Value(arg(*p[0]), 0); }}},
    {"norm", {1, [](Parameters p) { return Value(norm(*p[0]), 0); }}},
    {"conj", {1, [](Parameters p) { return conj(*p[0]); }}},
    {"proj", {1, [](Parameters p) { return proj(*p[0]); }}},

    // Exponential functions
    {"exp", {1, [](Parameters p) { return exp(*p[0]); }}},
    {"log", {1, [](Parameters p) { return log(*p[0]); }}},
    {"log10", {1, [](Parameters p) { return log10(*p[0]); }}},

    // Power functions
    {"pow", {2, [](Parameters p) { return pow(*p[0], *p[1]); }}},
    {"sqrt", {1, [](Parameters p) { return sqrt(*p[0]); }}},

    // Trig Functions
    {"sin", {1, [](Parameters p) { return sin(*p[0]); }}},
    {"cos", {1, [](Parameters p) { return cos(*p[0]); }}},
    {"tan", {1, [](Parameters p) { return tan(*p[0]); }}},
    {"asin", {1, [](Parameters p) { return asin(*p[0]); }}},
    {"acos", {1, [](Parameters p) { return acos(*p[0]); }}},
    {"atan", {1, [](Parameters p) { return atan(*p[0]); }}},

    // Hyperbolic trig functions
    {"sinh", {1, [](Parameters p) { return sinh(*p[0]); }}},
    {"cosh", {1, [](Parameters p) { return cosh(*p[0]); }}},
    {"tanh", {1, [](Parameters p) { return cosh(*p[0]); }}},
    {"asinh", {1, [](Parameters p) { return asinh(*p[0]); }}},
    {"acosh", {1, [](Parameters p) { return acosh(*p[0]); }}},
    {"atanh", {1, [](Parameters p) { return atanh(*p[0]); }}}};

Node::Node(Value val)
{
    type = NodeType::Constant;
    value = val;
}

Node::Node(string identifier)
{
    name = identifier;
    auto c = constants.find(identifier);
    if (c != constants.end()) {
        type = NodeType::Constant;
        value = c->second;
    } else {
        auto v = variables.find(identifier);
        if (v != variables.end()) {
            type = NodeType::Variable;
            variable = v->second;
        } else {
            auto f = functions.find(identifier);
            if (f != functions.end()) {
                type = NodeType::Function;
                arity = f->second.first;
                function = f->second.second;
            }
        }
    }
}

Node::Node() { type = NodeType::Invalid; }

Node::~Node()
{
    if (parameters)
        delete parameters;
}

void Node::addChild(Node child) { children.push_back(child); }

Variable Node::compile()
{
    if (type == NodeType::Constant) {
        return &value;
    } else if (type == NodeType::Variable) {
        return variable;
    } else if (type == NodeType::Function) {
        if (parameters)
            delete parameters;
        parameters = new Variable[arity];
        for (int i = 0; i < arity; i++) {
            parameters[i] = children[i].compile();
        }
        return &value;
    }
    return nullptr;
}

Value Node::eval()
{
    if (type == NodeType::Variable) {
        value = *variable;
    } else if (type == NodeType::Function) {
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i].isFunction())
                children[i].eval();
        }
        value = function(parameters);
    }
    return value;
}

void Parser::addConstant(string name, Value value) { constants[name] = value; }

void Parser::addVariable(string name, Variable address)
{
    variables[name] = address;
}

void Parser::addFunction(string name, Function address, int arity)
{
    functions[name] = pair<int, Function>(arity, address);
}

void Parser::nextToken()
{
    if (token == Token::Error)
        return;
    token = Token::Whitespace;
    start = next;
    while (token == Token::Whitespace) {
        if (!*next) {
            token = Token::End;
        } else if (isdigit(*next) || *next == '.') {
            // Numbers
            token = Token::Number;
            number = strtold(next, (char **)&next);
            name = string(start, next - start);
        } else if (isalpha(*next)) {
            // Identifiers
            while (isalnum(*next) || *next == '_')
                next++;
            token = Token::Identifier;
            name = string(start, next - start);
        } else {
            // Single character tokens and whitespace
            name = string(next, 1);
            switch (*next) {
                case '+':
                    token = Token::Plus;
                    break;
                case '-':
                    token = Token::Minus;
                    break;
                case '*':
                    token = Token::Times;
                    break;
                case '/':
                    token = Token::Divide;
                    break;
                case '^':
                    token = Token::Power;
                    break;
                case '(':
                    token = Token::OpenParen;
                    break;
                case ')':
                    token = Token::CloseParen;
                    break;
                case ',':
                    token = Token::Comma;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    // Ignore whitespace
                    break;
                default:
                    setError(Error::InvalidCharacter);
                    break;
            }
            next++;
        }
    }
}

void Parser::setError(Error err)
{
    if (token != Token::Error) {
        token = Token::Error;
        error = err;
    }
}

// <function> = <function-0> {"(" ")"} | <function-1> <power> | <function-X> "("
// <expr> {"," <expr>} ")"
Node Parser::function(Node ret)
{
    switch (ret.getArity()) {
        case 0:
            nextToken();
            if (token == Token::OpenParen) {
                nextToken();
                if (token != Token::CloseParen) {
                    setError(Error::MissingParen);
                } else {
                    nextToken();
                }
            }
            break;
        case 1:
            nextToken();
            ret.addChild(power());
            break;
        default:
            nextToken();
            if (token != Token::OpenParen) {
                setError(Error::MissingParen);
            } else {
                int i;
                for (i = 0; i < ret.getArity(); i++) {
                    nextToken();
                    ret.addChild(expr());
                    if (token != Token::Comma) {
                        break;
                    }
                }
                if (token != Token::CloseParen) {
                    if (token != Token::Comma && token != Token::End) {
                        setError(Error::MissingComma);
                    } else if (i > ret.getArity() - 1) {
                        setError(Error::TooManyParameters);
                    } else {
                        setError(Error::MissingParen);
                    }
                } else if (i < ret.getArity() - 1) {
                    setError(Error::TooFewParameters);
                } else {
                    nextToken();
                }
            }
    }
    return ret;
}

// <base> = <constant> | <variable> | <function> | "(" <complex> ")"
Node Parser::base()
{
    Node ret;
    switch (token) {
        case Token::Number:
            ret = Node(number);
            nextToken();
            break;
        case Token::Identifier: {
            Node identifier = Node(name);
            if (!identifier.isValid()) {
                setError(Error::UnknownIdentifier);
            } else if (identifier.isFunction()) {
                ret = function(identifier);
            } else {
                // Variable or constant
                ret = identifier;
                nextToken();
            }
            break;
        }
        case Token::OpenParen:
            nextToken();
            ret = expr();
            if (token != Token::CloseParen) {
                setError(Error::MissingParen);
            } else {
                nextToken();
            }
            break;
        case Token::CloseParen:
            setError(Error::UnexpectedParen);
            break;
        case Token::Comma:
            setError(Error::UnexpectedComma);
            break;
        case Token::Plus:
        case Token::Minus:
        case Token::Times:
        case Token::Divide:
        case Token::Power:
            setError(Error::MissingOperand);
            break;
        default:
            setError(Error::UnexpectedToken);
            break;
    }
    return ret;
}

// <power> = {("-" | "+")} <base>
Node Parser::power()
{
    int sign = 1;
    while (token == Token::Plus || token == Token::Minus) {
        if (token == Token::Minus)
            sign = -sign;
        nextToken();
    }
    Node ret;
    if (sign == 1) {
        ret = base();
    } else {
        ret = Node("negate");
        ret.addChild(base());
    }
    return ret;
}

// <factor> = <power> {"^" <power>}
Node Parser::factor()
{
    Node ret = power();
    while (token == Token::Power) {
        Node op = Node(name);
        op.addChild(ret);
        nextToken();
        op.addChild(power());
        ret = op;
    }
    return ret;
}

// <term> = <factor> {("*" | "/" | "%") <factor>}
Node Parser::term()
{
    Node ret = factor();
    while (token == Token::Times || token == Token::Divide) {
        Node op = Node(name);
        op.addChild(ret);
        nextToken();
        op.addChild(factor());
        ret = op;
    }
    return ret;
}

// <expr> = <term> {("+" | "-") <term>}
Node Parser::expr()
{
    Node ret = term();
    while (token == Token::Plus || token == Token::Minus) {
        Node op = Node(name);
        op.addChild(ret);
        nextToken();
        op.addChild(term());
        ret = op;
    }
    return ret;
}

Error Parser::parse(string expression)
{
    error = Error::None;
    start = next = expression.c_str();
    nextToken();
    root = expr();
    if (token == Token::End) {
        root.compile();
    }
    return error;
}

Value Parser::eval()
{
    if (error != Error::None)
        return NAN;
    else
        return root.eval();
}

string Parser::errorMessage()
{
    static map<Error, string> errors = {
        {Error::None, "No error"},
        {Error::InvalidCharacter, "Invalid character"},
        {Error::UnknownIdentifier, "Unknown identifier"},
        {Error::MissingParen, "Missing parenthesis"},
        {Error::MissingComma, "Missing comma"},
        {Error::TooFewParameters, "Too few parameters"},
        {Error::TooManyParameters, "Too many parameters"},
        {Error::UnexpectedComma, "Unexpected comma"},
        {Error::UnexpectedParen, "Unexpected parenthesis"},
        {Error::MissingOperand, "Missing operand"},
        {Error::UnexpectedToken, "Unexpected token"}};
    return errors[error];
}

} // namespace FormEval

int main(int argc, char *argv[])
{
    if (argc < 2) {
        cout << "Usage: formeval expression" << std::endl;
        return -1;
    }
    FormEval::Parser p;
    FormEval::Error error = p.parse(argv[1]);
    if (error != FormEval::Error::None)
        cout << p.errorMessage() << std::endl;
    else
        cout << p.eval() << std::endl;
    return 0;
}
