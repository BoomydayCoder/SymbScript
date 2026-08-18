#include "program.hh"

#include <iostream>
using namespace std;


void Program::push_byte(uint8_t b){
    code.push_back(b);
}

int Program::push_jump(uint8_t b){
    int sz = code.size();
    code.push_back(b);
    code.push_back(OP_NULL);
    code.push_back(OP_NULL); // will be backpatched later
    return sz;
}

int Program::push_local_compare_jump(uint8_t left, uint8_t right, uint8_t comparison){
    const int start = code.size();
    code.push_back(OP_JMP_FALSE_LOCAL_CMP);
    code.push_back(OP_NULL);
    code.push_back(OP_NULL);
    code.push_back(left);
    code.push_back(right);
    code.push_back(comparison);
    return start;
}

int Program::push_index_compare_jump(uint8_t list, uint8_t left, int8_t left_offset,
                                     uint8_t right, int8_t right_offset, uint8_t comparison){
    const int start = code.size();
    code.push_back(OP_JMP_FALSE_IND_CMP);
    code.push_back(OP_NULL);
    code.push_back(OP_NULL);
    code.push_back(list);
    code.push_back(left);
    code.push_back(static_cast<uint8_t>(left_offset));
    code.push_back(right);
    code.push_back(static_cast<uint8_t>(right_offset));
    code.push_back(comparison);
    return start;
}

void Program::push_loop(int loop_start){
    int s = code.size() - loop_start + 3; // account for the size of the jump instruction
    if (s > UINT16_MAX){
        cerr << "Loop too far" << endl;
        exit(1);
    }
    code.push_back(OP_LOOP);
    code.push_back(OP_NULL);
    code.push_back(OP_NULL);
    push_short(s, code.size()-2); 
}

void Program::push_const(Value v){
    push_byte(OP_CONST);
    push_byte(add_const(v));
}

int Program::add_const(Value v){
    consts.push_back(v);
    return consts.size() - 1;
}

void Program::push_short(uint16_t s, int loc){
    code[loc] = s >> 8;
    code[loc+1] = s & 0xFF;
}

void Program::patch_jump(int jmp_start){
    int instruction_size = 3;
    if (code[jmp_start] == OP_JMP_FALSE_LOCAL_CMP){
        instruction_size = 6;
    }
    else if (code[jmp_start] == OP_JMP_FALSE_IND_CMP){
        instruction_size = 9;
    }
    int s = code.size() - jmp_start - instruction_size;
    if (s > UINT16_MAX){
        cerr << "Jump too far" << endl;
        exit(1);
    }
    push_short(s, jmp_start+1);
}

void Program::print_self(ostream& os){ // print out the generated program
    
    for(int i=0; i<code.size();++i){
        // add the same spacing for each line
        os << i << ":\t";
        switch(code[i]){
            case OP_ADD:
                os << "OP_ADD" << endl;
                break;
            case OP_ADD_LOCAL_IMM:
                os << "OP_ADD_LOCAL_IMM " << (int)code[++i] << " " << (int)(int8_t)code[++i] << endl;
                break;
            case OP_ADD_LOCAL_IMM_INT:
                os << "OP_ADD_LOCAL_IMM_INT " << (int)code[++i] << " " << (int)(int8_t)code[++i] << endl;
                break;
            case OP_INC_LOCAL:
                os << "OP_INC_LOCAL " << (int)code[++i] << " " << (int)(int8_t)code[++i] << endl;
                break;
            case OP_INC_LOCAL_INT:
                os << "OP_INC_LOCAL_INT " << (int)code[++i] << " " << (int)(int8_t)code[++i] << endl;
                break;
            case OP_SUB:
                os << "OP_SUB" << endl;
                break;
            case OP_MUL:
                os << "OP_MUL" << endl;
                break;
            case OP_DIV:
                os << "OP_DIV" << endl;
                break;
            case OP_NEG:
                os << "OP_NEG" << endl;
                break;
            case OP_EQ:
                os << "OP_EQ" << endl;
                break;
            case OP_NE:
                os << "OP_NE" << endl;
                break;
            case OP_GRTR:
                os << "OP_GRTR" << endl;
                break;
            case OP_GE:
                os << "OP_GE" << endl;
                break;
            case OP_LESS:
                os << "OP_LESS" << endl;
                break;
            case OP_LE:
                os << "OP_LE" << endl;
                break;
            case OP_ABS:
                os << "OP_ABS" << endl;
                break;
            case OP_ABS_LOCAL:
                os << "OP_ABS_LOCAL " << (int)code[++i] << endl;
                break;
            case OP_APP:
                os << "OP_APP" << endl;
                break;
            case OP_APP_POP:
                os << "OP_APP_POP" << endl;
                break;
            case OP_NOT:
                os << "OP_NOT" << endl;
                break;
            case OP_POP:
                os << "OP_POP" << endl;
                break;
            case OP_CONST:
                os << "OP_CONST ";
                consts[code[++i]].print_self(os);
                os << endl;
                break;
            case OP_LIST:
                os << "OP_LIST " << (int)code[++i] << endl;
                break;
            case OP_PRINT:
                os << "OP_PRINT" << endl;
                break;
            case OP_INPUT:
                os << "OP_INPUT" << endl;
                break;
            case OP_SET_GLOBAL:
                os << "OP_SET_GLOBAL " << (int)code[++i] << endl;
                break;
            case OP_SET_GLOBAL_POP:
                os << "OP_SET_GLOBAL_POP " << (int)code[++i] << endl;
                break;
            case OP_GET_GLOBAL:
                os << "OP_GET_GLOBAL " << (int)code[++i] << endl;
                break;
            case OP_CHECK_GLOBAL:
                os << "OP_CHECK_GLOBAL " << (int)code[++i] << endl;
                break;
            case OP_SET_LOCAL:
                os << "OP_SET_LOCAL " << (int)code[++i] << endl;
                break;
            case OP_SET_LOCAL_POP:
                os << "OP_SET_LOCAL_POP " << (int)code[++i] << endl;
                break;
            case OP_GET_LOCAL:
                os << "OP_GET_LOCAL " << (int)code[++i] << endl;
                break;
            case OP_DEF_LOCAL:
                os << "OP_DEF_LOCAL " << endl;
                break;
            case OP_SET_IND:
                os << "OP_SET_IND" << endl;
                break;
            case OP_SET_IND_POP:
                os << "OP_SET_IND_POP" << endl;
                break;
            case OP_GET_IND_LOCAL:
                os << "OP_GET_IND_LOCAL " << (int)code[++i] << " " << (int)code[++i] << endl;
                break;
            case OP_GET_IND_LOCAL_OFFSET:
                os << "OP_GET_IND_LOCAL_OFFSET " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)(int8_t)code[++i] << endl;
                break;
            case OP_GET_IND_LOCAL_INT:
                os << "OP_GET_IND_LOCAL_INT " << (int)code[++i] << " " << (int)code[++i] << endl;
                break;
            case OP_GET_IND_LOCAL_OFFSET_INT:
                os << "OP_GET_IND_LOCAL_OFFSET_INT " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)(int8_t)code[++i] << endl;
                break;
            case OP_COPY_IND_LOCAL:
                os << "OP_COPY_IND_LOCAL " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)code[++i] << endl;
                break;
            case OP_SET_IND_LOCAL_VALUE:
                os << "OP_SET_IND_LOCAL_VALUE " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)code[++i] << endl;
                break;
            case OP_CMP_LOCAL_LOCAL:
                os << "OP_CMP_LOCAL_LOCAL " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)code[++i] << endl;
                break;
            case OP_GET_IND:
                os << "OP_GET_IND" << endl;
                break;
            case OP_JMP_F:
                os << "OP_JMP_F " << (int)code[++i]*(1<<8) + (int)code[++i] + 3 << endl;
                break;
            case OP_JMP_F_POP:
                os << "OP_JMP_F_POP " << (int)code[++i]*(1<<8) + (int)code[++i] + 3 << endl;
                break;
            case OP_JMP_FALSE_LOCAL_CMP:
                os << "OP_JMP_FALSE_LOCAL_CMP " << (int)code[++i]*(1<<8) + (int)code[++i] + 6
                   << " " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)code[++i] << endl;
                break;
            case OP_JMP_FALSE_IND_CMP:
                os << "OP_JMP_FALSE_IND_CMP " << (int)code[++i]*(1<<8) + (int)code[++i] + 9
                   << " " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)(int8_t)code[++i] << " " << (int)code[++i]
                   << " " << (int)(int8_t)code[++i] << " " << (int)code[++i] << endl;
                break;
            case OP_JMP_FALSE_LOCAL_INT_CMP:
                os << "OP_JMP_FALSE_LOCAL_INT_CMP " << (int)code[++i]*(1<<8) + (int)code[++i] + 6
                   << " " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)code[++i] << endl;
                break;
            case OP_JMP_FALSE_IND_INT_CMP:
                os << "OP_JMP_FALSE_IND_INT_CMP " << (int)code[++i]*(1<<8) + (int)code[++i] + 9
                   << " " << (int)code[++i] << " " << (int)code[++i]
                   << " " << (int)(int8_t)code[++i] << " " << (int)code[++i]
                   << " " << (int)(int8_t)code[++i] << " " << (int)code[++i] << endl;
                break;
            case OP_JMP:
                os << "OP_JMP " << (int)code[++i]*(1<<8) + (int)code[++i] + 3 << endl;
                break;
            case OP_LOOP:
                os << "OP_LOOP " << (int)code[++i]*(1<<8) + (int)code[++i] - 3 << endl; 
                break;
            case OP_SLIDE:
                os << "OP_SLIDE " << (int)code[++i] << endl;
                break;
            case OP_RETURN:
                os << "OP_RETURN" << endl;
                break;
            case OP_CALL:
                os << "OP_CALL " << (int)code[++i] << endl;
                break;
            case OP_NULL:
                os << "OP_NULL" << endl;
                break;
            



        }
    }
}
