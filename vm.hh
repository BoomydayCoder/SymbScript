#ifndef VM_H
#define VM_H

#include "program.hh"
#include "value.hh"
#include <vector>
#include <iostream>
#include <unordered_map>
using namespace std;


class VM {

    private:
        Value pop(); // returns the top value of the stack and removes it
        Value peek(int i); // Looks at the ith top value of the stack
        uint16_t read_short();

    public:
        VM ();
        ~VM();
        

        Program* prog;
        vector<uint8_t>::iterator ip; // the instruction pointer

        vector<Program*> c_stk; // the call stack
        vector<vector<uint8_t>::iterator> ips; // the instruction pointer

        vector<Value> stk; // the value stack
        unordered_map<int, Value> globals; // this is an unordered map to prevent redeclarations

        vector<vector<Value>*> lists; // we need to keep track of all the lists to delete them later
        vector<Program*> progs; // we need to keep track of all the functions to delete them later

        vector<int> frames; // stores the position of the current call frame in the stack

        bool run(); // 0: success, 1: error

        bool trace_execution;
        void print_self(ostream& os);

        void throw_error(string msg);

        void init_program(Program& p);        
};

extern VM vm; // make a global instance of the VM

#endif