// base copied from bison c++ demo

%skeleton "lalr1.cc" // -*- C++ -*-
%require "2.4.1"
%defines
%define parser_class_name "parser"

%code requires {
    #include <string>
    #include <memory>
    using namespace std;
    #include "exptree.hh"
    class driver;
}

%parse-param {driver& drv}
%lex-param {driver& drv}

%locations
%initial-action {
    @$.begin.filename = @$.end.filename = &drv.file;
};

%debug
%error-verbose

%union {
    int ival;
    string *sval;
    Ast *exptr;
}; // value types nodes can store

%code {
    #include "driver.hh"
}

%token        END      0 
%token        ASSIGN    
%token        PRINT     
%token        DEF

%token <ival> NUMBER     
%token <sval> ID         
%token        INPUT      
%type  <exptr> exp
%type  <exptr> stmt 
%type  <exptr> sequence
%type <exptr> block // defines data held by parse tree node
%type <exptr> explistf // this will also be used for function calls later
%type <exptr> explist
%type <exptr> idtype




%printer {debug_stream() << $$;} <ival>
%printer {debug_stream() << *$$;} <sval>
%printer {$$->print_self(debug_stream());} <exptr> // debug prints


%nonassoc ID
%right ASSIGN
%nonassoc EXPLIST
%left APPEND
%nonassoc '@'
%right '?' ':'
%left '|'
%left '&'
%right '!'
%left '=' GT LT GE LE NE
%left '+' '-'
%left '*' '/'
%right UMINUS // precedence and associativity of operators
%left '.'
%left '[' ']'
%left '(' ')'
%left ABS // absolute value




%start program;

%%

program: sequence END {drv.result = $1;}


sequence: {$$ = new Ast(SEQ);} // a sequence of statements
| sequence stmt {$$ = $1; $1->add(move($2));}  // recursively build up the list of statements

block: '{' sequence '}' {$$ = new Ast(BLK, {move($2)});}

stmt: ';' {$$ = new Ast(SEQ);} // a statement (not evaluating to a value)
    | exp ';' {$$ = new Ast(EXP, {move($1)});}
    | PRINT exp ';' {$$ = new Ast(PRINT, {move($2)});}
    | block {$$ = $1;}
    | '(' exp ')' '?' stmt ':' stmt {$$ = new Ast(IF, {move($2), move($5), move($7)});}
    | '(' exp ')' '?' stmt {$$ = new Ast(IF, {move($2), move($5), new Ast(SEQ)});}
    | '(' exp ')' '@' stmt {$$ = new Ast(WHL, {move($2), move($5)});}
    | '(' stmt exp ';' stmt ')' '@' stmt {$$ = new Ast(FOR, {move($2), move($3), move($5), move($8)});} 
    | '(' ID ':' exp ')' '@' stmt {$$ = new Ast(RNG, {new Ast(ID, $2), move($4), move($7)}); delete $2;}
    | DEF exp ';' {$$ = new Ast(RET, {move($2)});}

explistf: exp {$$ = new Ast(LST, {move($1)});} %prec EXPLIST  // a nonempty list of expressions
    | explistf ',' exp {$$ = $1; $1->add(move($3));} %prec EXPLIST

explist: {$$ = new Ast(LST);} %prec EXPLIST  // a list of expressions
    | explistf {$$ = $1;} %prec EXPLIST 

idtype: ID {$$ = new Ast(ID, $1); delete $1;} // represents an lvalue
    | exp '[' exp ']' {$$ = new Ast(IND, {move($1), move($3)});}
    | '.' ID {$$ = new Ast(SGET, {new Ast(ID, $2)}); delete $2;}
    | exp '.' ID {$$ = new Ast(GET, {move($1), new Ast(ID, $3)}); delete $3;}
    
exp: // an expression (evaluating to a specific value)
    idtype ASSIGN exp {$$ = new Ast(SET, {move($1), move($3)}); } // need to make this and variable/array access the same
    | exp APPEND exp {$$ = new Ast(APP, {move($1), move($3)}); }
    | exp '+' exp   { $$ = new Ast(ADD, {move($1), move($3)}); }
    | exp '-' exp   { $$ = new Ast(SUB, {move($1), move($3)}); }
    | exp '*' exp   { $$ = new Ast(MUL, {move($1), move($3)}); }
    | exp '/' exp   { $$ = new Ast(DIV, {move($1), move($3)}); }
    | exp '=' exp   { $$ = new Ast(EQ, {move($1), move($3)}); }
    | exp GT exp    { $$ = new Ast(GT, {move($1), move($3)}); }
    | exp LT exp    { $$ = new Ast(LT, {move($1), move($3)}); }
    | exp GE exp    { $$ = new Ast(GE, {move($1), move($3)}); }
    | exp LE exp    { $$ = new Ast(LE, {move($1), move($3)}); }
    | exp NE exp    { $$ = new Ast(NE, {move($1), move($3)}); }
    | '|' exp '|'   { $$ = new Ast(ABS, {move($2)}); } %prec ABS
    | exp '|' exp   { $$ = new Ast(OR, {move($1), move($3)}); }
    | exp '&' exp   { $$ = new Ast(AND, {move($1), move($3)}); }
   
    | '!' exp       { $$ = new Ast(NOT, {move($2)}); }
    | '-' exp %prec UMINUS { $$ = new Ast(NEG, {move($2)}); }
    | '(' exp ')'   { $$ = $2; } %prec '('
    | NUMBER      { $$ = new Ast(INT, $1);}
    | '[' explist ']' {$$ = $2;}
    | idtype       { $$ = $1; } // this is the lvalue
    | INPUT {$$ = new Ast(INP);}
    | '(' explist ')' DEF block {$$ = new Ast(DEF, {move($2), move($5)});} 
    | '(' exp ')' DEF block {$$ = new Ast(DEF, {new Ast(LST, {move($2)}), move($5)});}
    | exp '(' explist ')' {$$ = new Ast(CAL, {move($1), move($3)});} 

    | '#' block {$$ = new Ast(CLS, {move($2)}); } // class definition!
    
    

%%

void yy::parser::error (const yy::parser::location_type& l, const string& m)
{
    drv.error(l, m);
}