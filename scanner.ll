%{                                            /* -*- C++ -*- */
# include <stdlib.h>
# include <errno.h>
# include <limits.h>
# include <string>
# include "driver.hh"
# include "parser.tab.hh"

/* Work around an incompatibility in flex (at least versions
2.5.31 through 2.5.33): it generates code that does
not conform to C89.  See Debian bug 333231
<http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.  */
# undef yywrap
# define yywrap() 1

/* By default yylex returns int, we use token_type.
Unfortunately yyterminate by default returns 0, which is
not of token_type.  */
#define yyterminate() return token::END
%}

%option noyywrap nounput batch debug

id    [a-zA-Z][a-zA-Z_0-9]*
int   [0-9]+
blank [ \t]
comment "//".*

%{
# define YY_USER_ACTION  yylloc->columns (yyleng);
%}
%%

%{
    yylloc->step ();
%}
{blank}+   yylloc->step ();
{comment}+  yylloc->step ();
[\n]+      yylloc->lines (yyleng); yylloc->step ();

%{
typedef yy::parser::token token;
%}
        /* Convert ints to the actual type of tokens.  */
[-+*/();{}=!?:&|@,.#]     return yy::parser::token_type (yytext[0]);


":="       return token::ASSIGN;
">>"   return token::PRINT;
"<<"   return token::INPUT;
">="  return token::GE;
"<="  return token::LE;
"!=" return token::NE;
":+" return token::APPEND;
">"       return token::GT;
"<"       return token::LT;
"[" return yy::parser::token_type ('[');
"]" return yy::parser::token_type (']');
"=>" return token::DEF;

{int}      {
    long n = strtol (yytext, NULL, 10);
    yylval->ival = n;
    return token::NUMBER;
}
{id}       {
    yylval->sval = new std::string (yytext);
    return token::ID;
}
.          driver.error (*yylloc, "invalid character");
%%

void
driver::scan_begin ()
{
yy_flex_debug = trace_scanning;
if (file == "-")
    yyin = stdin;
else if (!(yyin = fopen (file.c_str (), "r")))
    {
    error (std::string ("cannot open ") + file);
    exit (1);
    }
}

void
driver::scan_end ()
{
fclose (yyin);
}