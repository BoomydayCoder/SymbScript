#ifndef PROGRAM_H
#define PROGRAM_H

#include <vector>
#include <cstdint>
#include "value.hh"
#include <iostream>
using namespace std;

enum op_code {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_EQ, OP_NE, OP_LESS, OP_LE, OP_GRTR, OP_GE, OP_NOT, OP_NEG, OP_ABS,
    OP_APP, OP_APP_POP,
    OP_CONST, OP_NULL, OP_LIST,
    OP_PRINT, OP_INPUT, 
    OP_POP, 
    OP_SET_GLOBAL, OP_SET_GLOBAL_POP, OP_GET_GLOBAL,
    OP_SET_LOCAL, OP_SET_LOCAL_POP, OP_GET_LOCAL, OP_DEF_LOCAL,
    OP_SET_IND, OP_SET_IND_POP, OP_GET_IND,
    OP_JMP_F, OP_JMP_F_POP, OP_JMP, OP_LOOP,
    OP_CHECK_GLOBAL, OP_SLIDE,
    OP_ADD_LOCAL_IMM, OP_INC_LOCAL, OP_ABS_LOCAL,
    OP_CMP_LOCAL_LOCAL, OP_GET_IND_LOCAL, OP_GET_IND_LOCAL_OFFSET,
    OP_COPY_IND_LOCAL, OP_SET_IND_LOCAL_VALUE,
    OP_JMP_FALSE_LOCAL_CMP, OP_JMP_FALSE_IND_CMP,
    OP_ADD_LOCAL_IMM_INT, OP_INC_LOCAL_INT,
    OP_GET_IND_LOCAL_INT, OP_GET_IND_LOCAL_OFFSET_INT,
    OP_JMP_FALSE_LOCAL_INT_CMP, OP_JMP_FALSE_IND_INT_CMP,
    OP_RETURN, OP_CALL,
    OP_COUNT,
}; // the bytecode operations

class Value;
class Program {
    private:
        int add_const(Value v); // returns the index of the constant, internal utility function to add to the constant table
        void push_short(uint16_t s, int loc); // push a short to the bytecode program at locaiton loc
 
    public:
        vector<uint8_t> code; // the bytecode program itself
        vector<Value> consts; // the constant table

        uint8_t arity = -1; // the arity of the function, -1 if not a function

        void push_byte(uint8_t b); 
        int push_jump(uint8_t b); // add the instructions required for a byte or a jump
        int push_local_compare_jump(uint8_t left, uint8_t right, uint8_t comparison);
        int push_index_compare_jump(uint8_t list, uint8_t left, int8_t left_offset,
                                    uint8_t right, int8_t right_offset, uint8_t comparison);

        void push_loop(int loop_start); // add the instructions required for a loop
        void push_const(Value v); // add a constant 

        void patch_jump(int jmp_start); // backpatch jump

        void print_self(ostream& os);
        
};

#endif
