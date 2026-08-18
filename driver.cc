#include "driver.hh"
#include "parser.tab.hh"

driver::driver()
    : trace_scanning (false), trace_parsing (false)
{
} // initialise the driver

driver::~driver()
{
}

int driver::parse (const string& f){
    file = f;
    scan_begin ();
    yy::parser parser (*this);
    parser.set_debug_level (trace_parsing);
    int res = parser.parse ();
    scan_end ();
    return res;
} 

void driver::error (const yy::location& l, const string& m){
    cerr << l << ": " << m << endl;
}

void driver::error (const string& m){
    cerr << m << endl;
} // code copied from c++ bison demo, used to manage the scanner and parser