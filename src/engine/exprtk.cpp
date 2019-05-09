#include "aconfig.h"
#ifdef USE_EXPRTK
#include "exprtk.h"
#include "exprtk.hpp"
#include <complex>

typedef std::complex<double> complex_t;
typedef exprtk::symbol_table<complex_t> symbol_table_t;
typedef exprtk::expression<complex_t> expression_t;
typedef exprtk::parser<complex_t> parser_t;


bool operator<(const complex_t& a, const complex_t& b)
{
    return false;
}

bool operator<=(const complex_t& a, const complex_t& b)
{
    return false;
}

bool operator>(const complex_t& a, const complex_t& b)
{
    return false;
}

bool operator>=(const complex_t& a, const complex_t& b)
{
    return false;
}

static complex_t z;
static complex_t c;
static symbol_table_t symbol_table;
static expression_t expression;
static parser_t parser;

extern "C" {

void exprtk_setexpr(const char* expression_string)
{
    parser.compile(expression_string, expression);
}

void exprtk_init()
{

    symbol_table.add_variable("z", z);
    symbol_table.add_variable("c", c);
    symbol_table.add_constants();
    expression.register_symbol_table(symbol_table);
    exprtk_setexpr("z*z+c");
}

exprtk_cmplx exprtk_eval(double zre, double zim, double cre, double cim)
{
    z = complex_t(zre, zim);
    c = complex_t(cre, cim);
    z = expression.value();
    exprtk_cmplx ret;
    ret.re = z.real();
    ret.im = z.imag();
    return ret;
}

}
#endif
