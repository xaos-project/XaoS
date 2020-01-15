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
#include <cctype>

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
    {"+", {2, [](Parameters p) { *p[0] = *p[1] + *p[2]; }}},
    {"-", {2, [](Parameters p) { *p[0] = *p[1] - *p[2]; }}},
    {"*", {2, [](Parameters p) { *p[0] = *p[1] * *p[2]; }}},
    {"/", {2, [](Parameters p) { *p[0] = *p[1] / *p[2]; }}},
    {"^", {2, [](Parameters p) { *p[0] = pow(*p[1], *p[2]); }}},
    {"negate", {1, [](Parameters p) { *p[0] = *p[1] * Value(-1); }}},

    // Complex-specific functions
    {"re", {1, [](Parameters p) { *p[0] = Value(real(*p[1]), 0); }}},
    {"real", {1, [](Parameters p) { *p[0] = Value(real(*p[1]), 0); }}},
    {"im", {1, [](Parameters p) { *p[0] = Value(0, imag(*p[1])); }}},
    {"imag", {1, [](Parameters p) { *p[0] = Value(0, imag(*p[1])); }}},
    {"abs", {1, [](Parameters p) { *p[0] = Value(abs(*p[1])); }}},
    {"arg", {1, [](Parameters p) { *p[0] = Value(arg(*p[1])); }}},
    {"norm", {1, [](Parameters p) { *p[0] = Value(norm(*p[1])); }}},
    {"conj", {1, [](Parameters p) { *p[0] = conj(*p[1]); }}},
    {"proj", {1, [](Parameters p) { *p[0] = proj(*p[1]); }}},

    // Exponential functions
    {"exp", {1, [](Parameters p) { *p[0] = exp(*p[1]); }}},
    {"log", {1, [](Parameters p) { *p[0] = log(*p[1]); }}},
    {"log10", {1, [](Parameters p) { *p[0] = log10(*p[1]); }}},

    // Power functions
    {"pow", {2, [](Parameters p) { *p[0] = pow(*p[1], *p[2]); }}},
    {"sqrt", {1, [](Parameters p) { *p[0] = sqrt(*p[1]); }}},

    // Trig Functions
    {"sin", {1, [](Parameters p) { *p[0] = sin(*p[1]); }}},
    {"cos", {1, [](Parameters p) { *p[0] = cos(*p[1]); }}},
    {"tan", {1, [](Parameters p) { *p[0] = tan(*p[1]); }}},
    {"asin", {1, [](Parameters p) { *p[0] = asin(*p[1]); }}},
    {"acos", {1, [](Parameters p) { *p[0] = acos(*p[1]); }}},
    {"atan", {1, [](Parameters p) { *p[0] = atan(*p[1]); }}},

    // Hyperbolic trig functions
    {"sinh", {1, [](Parameters p) { *p[0] = sinh(*p[1]); }}},
    {"cosh", {1, [](Parameters p) { *p[0] = cosh(*p[1]); }}},
    {"tanh", {1, [](Parameters p) { *p[0] = cosh(*p[1]); }}},
    {"asinh", {1, [](Parameters p) { *p[0] = asinh(*p[1]); }}},
    {"acosh", {1, [](Parameters p) { *p[0] = acosh(*p[1]); }}},
    {"atanh", {1, [](Parameters p) { *p[0] = atanh(*p[1]); }}}};


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

Variable Node::compile(vector<Node *> &stack)
{
    if (type == NodeType::Constant) {
        return &value;
    } else if (type == NodeType::Variable) {
        return variable;
    } else if (type == NodeType::Function) {
        if (parameters)
            delete parameters;
        parameters = new Variable[arity+1];
        parameters[0] = &value;
        for (int i = 0; i < arity; i++) {
            parameters[i+1] = children[i].compile(stack);
        }
        stack.push_back(this);
        return &value;
    }
    return nullptr;
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

Error Parser::parse(string exp)
{
    error = Error::None;
    expression = exp;
    first = start = next = expression.c_str();
    nextToken();
    root = expr();
    if (token == Token::End) {
        stack.clear();
        result = root.compile(stack);
    }
    return error;
}

void Parser::setError(Error err)
{
    if (token != Token::Error) {
        token = Token::Error;
        error = err;
        errorloc.first = start - first;
        errorloc.second = next - first;
    }
}

string Parser::getExpression()
{
    return expression;
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

pair<int, int> Parser::errorLocation()
{
    return errorloc;
}

} // namespace FormEval

using namespace FormEval;


int main(int argc, char *argv[])
{
    if (argc < 2) {
        cout << "Usage: formeval expression" << endl;
        return -1;
    }
    Parser p;
    Error error = p.parse(argv[1]);
    if (error != Error::None) {
        pair<int, int> loc = p.errorLocation();
        int len = loc.second - loc.first - 1;
        if (len < 0)
            len = 0;
        cout << p.errorMessage() << ':' << endl;
        cout << p.getExpression() << endl;
        cout << string(loc.first, ' ');
        cout << '^' << string(len, '~') << endl;
    } else {
        cout << p.eval() << endl;
    }
    return 0;
}

/*
int main()
{
    Parser p;
    Value c;
    Value z;
    Value t;

    Parser::addVariable("c", &c);
    Parser::addVariable("z", &c);
    p.parse("z^2+c");

    const int ixsize = 160;
    const int iysize = 50;
    const number_t cxmin = -1.5;
    const number_t cxmax = 2;
    const number_t cymin = -1;
    const number_t cymax = 1;
    const number_t maxit = 100;

    for (unsigned int iy = 0; iy < iysize; ++iy)
    {
        for (unsigned int ix = 0; ix < ixsize; ++ix) {
            c = {cxmin + ix/(ixsize-1.0)*(cxmax-cxmin), cymin + iy/(iysize-1.0)*(cymax-cymin)};
            z = {0,0};
            unsigned int iter;

            for (iter = 0; iter < maxit && abs(z) < 2.0L; ++iter) {
                z = p.eval();
                //z = z*z+c;
                cout << z << endl;
            }
            cout << endl;

            //if (iter == maxit)
            //    cout << ' ';
            //else
            //    cout << '*';

        }
        //cout << endl;
    }
}
*/
