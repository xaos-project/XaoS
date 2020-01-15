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

/**
 * @brief constants Table of symbolic constants accessible from within the
 * expression. See Parser::addConstant for details.
 */
map<string, Value> constants = {{"pi", 3.14159265358979323846},
                                {"e", 2.71828182845904523536},
                                {"i", {0, 1}}};

/**
 * @brief variables Table of variables accessible from within the expression.
 * See Parser::addVariable for details.
 */
map<string, Variable> variables;

/**
 * @brief functions Table of functions accessible from with the
 * expression. See Parser::addFunction for details.
 */
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

/**
 * @brief Node::Node Constructs a node for a numeric constant.
 * @param val The value of the numeric constant the node is to contain.
 */
Node::Node(Value val)
{
    type = NodeType::Constant;
    value = val;
}

/**
 * @brief Node::Node Constructs a node for a symbolic constant, variable, or
 * function. Initialized with the address or value from the appropriate symbol
 * table.
 * @param identifier The string identifier of the constant, variable or
 * function.
 */
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

/**
 * @brief Node::~Node Destructor frees any parameter pointers that have been
 * allocated.
 */
Node::~Node()
{
    if (parameters)
        delete parameters;
}

/**
 * @brief Node::addChild Adds a child (parameter) Node to a function node). Only
 * function nodes have children.
 * @param child The parameter/operand to add to this function.
 */
void Node::addChild(Node child) { children.push_back(child); }

/**
 * @brief Node::compile Converts the parsed AST into a stack of
 * operations to evaluate iteratively using depth first search. Stack notation
 * allows functions to be called iteratively removing the overhead of recursive
 * function calls.  Called recursively from the root node.  For each function
 * node, allocates and initializes an array pointers within directly to the
 * return value and any child parameters.  Should be called on the root node
 * before evaluation.
 * @param stack The vector of Node pointers onto which the nodes should be
 * pushed.
 * @return The pointer to the return value of the root node.
 */
Variable Node::compile(vector<Node *> &stack)
{
    if (type == NodeType::Constant) {
        return &value;
    } else if (type == NodeType::Variable) {
        return variable;
    } else if (type == NodeType::Function) {
        if (parameters)
            delete parameters;
        parameters = new Variable[arity + 1];
        parameters[0] = &value;
        for (int i = 0; i < arity; i++) {
            parameters[i + 1] = children[i].compile(stack);
        }
        stack.push_back(this);
        return &value;
    }
    return nullptr;
}

/**
 * @brief Parser::addConstant Registers a symbolic constant to be accessible
 * within an expression.
 * @param name The name of the constant.
 * @param value The value of the constant of type Value (usually complex<long
 * double>)
 */
void Parser::addConstant(string name, Value value) { constants[name] = value; }

/**
 * @brief Parser::addVariable registers a variable to be accessible within
 * an expression.
 * @param name The name of the variable accessible within the expression.
 * @param address  The address of the variable taken using &.
 */
void Parser::addVariable(string name, Variable address)
{
    variables[name] = address;
}

/**
 * @brief Parser::addFunction registers a function to be used within an
 * expression.  The function returns void and takes a single Parameters
 * argument, which is array of pointers with the address of return
 * value and the addresses of any parameters.  The function will most likely
 * be a generalized wrapper for another function, which passes paremeters to
 * the main function and assigns the return value as follows:
 * *p[0] = op(*p[1], *p[2]);
 * @param name Name of the function to be used within an expression
 * @param address Pointer of type Function to a compatible wrapper function.
 * @param arity Number of parameters the function takes.
 */
void Parser::addFunction(string name, Function address, int arity)
{
    functions[name] = pair<int, Function>(arity, address);
}

/**
 * @brief Parser::nextToken lexes the next available token (number, identifier,
 * or operator).  Whitespace between tokens is skipped.
 */
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

/**
 * @brief Parser::function parses a function call. Functions with zero or one
 * parameters can optionally take parentheses. Functions with two or more
 * parameters require them.  Each parameter to the function can be a complete
 * expression.
 * // <function> = <function-0> {"(" ")"} | <function-1> <power> |
 * <function-X> "(" <expr> {"," <expr>} ")"
 * @param ret The Node containing the function call, to which child parameters
 * are added.
 * @return The function node with the child parameters added.
 */
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

/**
 * @brief Parser::base parses a base token, which can be a real or complex
 * number, identifier, function, or expression surrounded by parentheses.
 * <base> = <constant> | <variable> | <function> | "(" <complex> ")"
 * @return The node consisting of the base token.
 */
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
        case Token::End:
            setError(Error::UnexpectedEnd);
            break;
        default:
            setError(Error::UnexpectedToken);
            break;
    }
    return ret;
}

/**
 * @brief Parser::power parses a unary negation operator, the operand
 * to an exponentiation.
 * <power> = {("-" | "+")} <base>
 * @return If there is an odd number of negation operators, returns a negation
 * operation with a child node.  If no negation or an even number of negations,
 * returns the child node by itself.
 */
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

/**
 * @brief Parser::factor Parses a factor (the operand to multiplcation or
 * division), consisting of a exponentiation.
 * @return The exponentiation operator with its two surrounding operands.
 */
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

/**
 * @brief Parser::term Parses a term (the operand to addition or subtraction,
 * consisting of a multiplication or division.
 * <term> = <factor> {("*" | "/") <factor>}
 * @return Node containing the multiplication or division operator with its
 * two surrounding factors.
 */
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

/**
 * @brief Parser::expr Parses an expression (the outermost production),
 * consisting of two terms to be added or subtracted.
 * <expr> = <term> {("+" | "-") <term>}
 * @return Node containing the addition or subtraction operator with its two
 * surrounding terms.
 */
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

/**
 * @brief Parser::parse Parses the provided expression and compiles it to
 * bytecode for evaluation.
 * @param exp Mathematical expression to parse
 * @return Error code for the first error encountered during parsing.
 * Error::None (0) is returned on success.
 */
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
/**
 * @brief Parser::setError Sets the error code and location only if no other
 * error has previously been encountered.
 * @param err The error to set.
 */
void Parser::setError(Error err)
{
    if (token != Token::Error) {
        token = Token::Error;
        error = err;
        errorloc.first = start - first;
        errorloc.second = next - first;
    }
}

/**
 * @brief Parser::getExpression returns the last expression passed to
 * Parser::parse.
 * @return String containing expression.
 */
string Parser::getExpression() { return expression; }
/**
 * @brief Parser::errorMessage Returns human-readable error message
 * corresponding to current error status.
 * @return String describing error corresponding to enum value.
 */
string Parser::errorMessage()
{
    static map<Error, string> errors = {
        {Error::None, "No error"},
        {Error::InvalidCharacter, "Invalid character"},
        {Error::UnknownIdentifier, "Unknown identifier"},
        {Error::MissingParen, "Missing parenthesis"},
        {Error::MissingComma, "Missing comma"},
        {Error::MissingOperand, "Missing operand"},
        {Error::TooFewParameters, "Too few parameters"},
        {Error::TooManyParameters, "Too many parameters"},
        {Error::UnexpectedComma, "Unexpected comma"},
        {Error::UnexpectedParen, "Unexpected parenthesis"},
        {Error::UnexpectedEnd, "Unexpected end of formula"},
        {Error::UnexpectedToken, "Unexpected token"}};
    return errors[error];
}

/**
 * @brief Parser::errorLocation Returns the position where the first error
 * occurred during the last call to Parser::parse.
 * @return Pair of ints with the position of the first character of the token
 * that failed to parse and the first character after the token that failed to
 * parse.
 */
pair<int, int> Parser::errorLocation() { return errorloc; }

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
            c = {cxmin + ix/(ixsize-1.0)*(cxmax-cxmin), cymin +
iy/(iysize-1.0)*(cymax-cymin)}; z = {0,0}; unsigned int iter;

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
