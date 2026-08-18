#ifndef VM_H
#define VM_H

#include "program.hh"
#include "value.hh"
#include <vector>
#include <iostream>
using namespace std;


class VM {

    private:
        Value pop(); // returns the top value of the stack and removes it
        Value peek(int i); // Looks at the ith top value of the stack
        uint16_t read_short();

    public:
        struct CallFrame {
            Program* caller;
            vector<uint8_t>::iterator return_ip;
            size_t stack_start;
        };

        VM ();
        ~VM();
        

        Program* prog;
        vector<uint8_t>::iterator ip; // the instruction pointer

        vector<Value> stk; // the value stack
        vector<Value> globals; // globals are compiled to direct byte-sized indices
        vector<uint8_t> global_defined; // preserves late-bound undefined-global errors

        vector<vector<Value>*> lists; // we need to keep track of all the lists to delete them later
        vector<Program*> progs; // we need to keep track of all the functions to delete them later

        vector<CallFrame> frames;

        bool run(); // 0: success, 1: error

        bool trace_execution;
        void print_self(ostream& os);

        void throw_error(string msg);

        void init_program(Program& p, size_t global_count);
};

extern VM vm; // make a global instance of the VM

#endif
