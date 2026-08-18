#ifndef DRIVER_HH
# define DRIVER_HH
# include <string>
# include <map>
# include "parser.tab.hh"
#include "exptree.hh"

using namespace std;

# define YY_DECL                                        \
       yy::parser::token_type                         \
       yylex (yy::parser::semantic_type* yylval,      \
              yy::parser::location_type* yylloc,      \
              driver& driver)
     // ... and declare it for the parser's sake.
     YY_DECL;

class driver // The driver of the parser/lexer - this is mostly inspired by the c++ Bison demo
    {
     public:
       driver ();
       virtual ~driver ();
     
     
       Ast* result; // the result of the parse - how the parser interfaces with the main program

       void scan_begin ();
       void scan_end ();
       bool trace_scanning;

       int parse(const string& f);
       string file;
       bool trace_parsing;

       void error (const yy::location& l, const string& m);
       void error (const string& m);
    };
    #endif // ! DRIVER_HH