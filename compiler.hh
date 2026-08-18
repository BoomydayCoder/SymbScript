#ifndef COMPILER_H
#define COMPILER_H

#include "exptree.hh"
#include "program.hh"

#include "unordered_map"
using namespace std;

class Compiler { // A class that represents the compiler as it compiles the code
    public:
        
        Compiler(unordered_map<string, int>& glob_index, unordered_map<string, int> loc_index,
                vector<string> loc_names, vector<int> loc_counts); // constructs the compiler to be used in a call frame.
                // NOTE: the glob_index is by reference (as globals are stored between all functions) while the loc_index is not (as we only use pure functions)
        Compiler(); // default constructor

        Program prog; // Stores bytecode instructions

        void resolve_globals(Ast* exp); // Since global variables are late bound, they need to be resolved before compilation
        void discover_inline_functions(Ast* exp);

        void compile(Ast* exp); // Turns the code into bytecode

        void begin_scope();
        void end_scope(); // Utility functions to begin and end scope

        void set_var(string name); // Utility function to set a variable
        void get_var(string name); // Utility function to get a variable


        unordered_map<string, int> global_index; 
        unordered_map<string, int> local_index; // Map local and global variables to their indices in the bytecode

        vector<string> local_names;
        vector<int> local_counts; // To manage allocating and freeing local variables from the stack

        int scope = 0; // The current scope level
        int range_for_ct = 0; // a strange variable needed to assign looping variables for range based for loops

        unordered_map<string, Ast*> inline_functions;
        vector<Ast*> inline_stack;

    private:
        bool try_inline_call(Ast* callee, Ast* arguments);
        bool try_superinstruction(Ast* expression);
        bool local_operand(Ast* expression, uint8_t& index) const;
        bool local_index_operand(Ast* expression, uint8_t& index, int8_t& offset) const;
        bool small_integer(Ast* expression, int8_t& value) const;
        int push_condition_jump(Ast* condition);
        bool has_return(Ast* exp) const;
};
#endif
