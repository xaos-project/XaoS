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

namespace FormEval
{

using namespace std;

/**
 * @brief number_t The base type for real numbers.
 */
typedef long double number_t;

/**
 * @brief Value a complex number value, used for constants, variables, and all
 * function parameters and return values.
 */
typedef complex<number_t> Value;

/**
 * @brief Variable Pointer to a value, used for variables within the expression,
 * as well as function return values and parameters.
 */
typedef Value *Variable;

/**
 * @brief Parameters Array of pointers used to store function return and
 * parameter locations.
 */
typedef Value **Parameters;

/**
 * @brief Function pointer to wrapper functions accessible from within the
 * expression.  Must return void and take a single Parameters argument.  Does
 * not return a value; instead assigns the return value to the zeroth element of
 * the Parameters array.  Arguments start at element 1.
 */
typedef void (*Function)(Parameters);

/**
 * @brief The NodeType enum identifies the different types of AST node.
 */
enum class NodeType { Invalid, Constant, Variable, Function };

/**
 * @brief The Node class represents a node in the abstract syntax tree built by
 * the parser.  Transforms into a stack of function calls to be executed during
 * expression evaluation.
 */
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
    inline void eval()
    {
        function(parameters);
        asm("");
    }
};

/**
 * @brief The Error enum identifies different error states encountered by the
 * lexer or parser.
 */
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
    UnexpectedEnd,
    UnexpectedToken
};

/**
 * @brief The Token enum identifies the different token types that have been
 * lexed for the parser to consume.
 */
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

/**
 * @brief The Parser class contains state during a parsing operation and holds
 * the AST built from the expression, as well as the stack of function calls
 * that the AST is transformed into.  Not re-entrant; the Parser and any
 * variables registered to it must be thread_local.
 */
class Parser
{
  private:
    Token token = Token::Whitespace;
    Error error = Error::None;
    number_t number = 0;
    string name;

    string expression;
    const char *first = nullptr;
    const char *start = nullptr;
    const char *next = nullptr;
    pair<int, int> errorloc;

    Node root;
    vector<Node *> stack;
    Variable result = nullptr;

    void setError(Error err);
    void nextToken();
    Node function(Node function);
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
    inline Value eval()
    {
        for (Node *node : stack)
            node->eval();
        return *result;
    }
};

} // namespace FormEval

#endif /*__FORMEVAL_H__*/
