#ifndef EXPTREE_H
#define EXPTREE_H

#include <iostream>
#include <vector>
#include <memory>
using namespace std;

enum node_type { // Types of AST nodes
    ADD, SUB, MUL, DIV, NEG, INT, SEQ, ID, IND, SET, EXP, PRINT, BLK, INP, EQ, NOT, IF, AND, OR, GT, LT, GE, LE, NE, WHL, FOR, RNG, LST, APP, CAL, DEF, RET, ABS, CLS,GET,SGET
};

class Ast { // A class that stores the AST
    public:
        node_type type;

        int num;
        string id;
        vector<Ast*> ch;

        Ast(node_type t, int v);
        Ast(node_type t, string* i);
        Ast(node_type t, vector<Ast*> c);
        Ast(node_type t);
        Ast();
        ~Ast(); // constructors and destructor

        void print_self(ostream &os); // Print the AST
        void add(Ast* c); // Add a child to the node
};


#endif