#ifndef VM_H
#define VM_H

#include "program.hh"
#include "value.hh"
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>
using namespace std;


class VM {

    private:
        Value pop(); // returns the top value of the stack and removes it
        Value peek(int i); // Looks at the ith top value of the stack
        uint16_t read_short();
        void mark_value(const Value& value, unordered_set<Program*>& marked_programs);
        void mark_program(Program* program, unordered_set<Program*>& marked_programs);

        struct ListObject {
            vector<Value> values;
            bool marked = false;
            bool in_use = true;
        };

        deque<ListObject> list_pool;
        vector<ListObject*> free_lists;
        unordered_map<vector<Value>*, ListObject*> list_lookup;
        size_t active_lists = 0;
        size_t next_gc = 1024;

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

        vector<Program*> progs; // we need to keep track of all the functions to delete them later

        vector<CallFrame> frames;

        bool run(); // 0: success, 1: error

        bool trace_execution;
        void print_self(ostream& os);

        void throw_error(string msg);

        vector<Value>* allocate_list(vector<Value>&& values);
        void collect_garbage(const vector<Value>* extra_roots = nullptr);

        void init_program(Program& p, size_t global_count);
};

extern VM vm; // make a global instance of the VM

#endif
