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
    int s =  code.size() - jmp_start;
    s -= 3; // account for the size of the jump instruction
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
            case OP_GET_IND:
                os << "OP_GET_IND" << endl;
                break;
            case OP_JMP_F:
                os << "OP_JMP_F " << (int)code[++i]*(1<<8) + (int)code[++i] + 3 << endl;
                break;
            case OP_JMP_F_POP:
                os << "OP_JMP_F_POP " << (int)code[++i]*(1<<8) + (int)code[++i] + 3 << endl;
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
