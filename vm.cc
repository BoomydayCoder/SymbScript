
#include "vm.hh"


#include <variant>
#include <algorithm>
#include "value.hh"
using namespace std;
using vptr = vector<Value>*;

VM vm;




#define BINARY_OP(op) do { \
    if (!peek(0).is_int() || !peek(1).is_int()){ \
        throw_error("Operands must be integers"); \
    } \
    int b = pop().get_int(); \
    int a = pop().get_int(); \
    stk.push_back(Value(a op b)); \
} while (false) // macro to handle binary operations

VM::VM(){
    stk.reserve(UINT8_MAX+1);
}

void VM::init_program(Program& p, size_t global_count){
    prog = &p;
    globals.assign(global_count, Value());
    global_defined.assign(global_count, 0);
    
    
    ip = prog->code.begin();
    // output the type of ip
    
    
    

    frames.push_back({nullptr, {}, 0});
}

VM::~VM(){
    
    
    for (vector<Value>* l: lists)
        delete l;
    for (Program* p: progs){
        delete p;
    }
        

   
} // we must remember to delete all the lists and programs!!!

Value VM::pop(){
    Value v = stk.back();
    stk.pop_back();
    return v;
} 

Value VM::peek(int i){
    return stk[stk.size()-i-1]; 
}

void VM::print_self(ostream& os){
    os << "ip: " << ip-prog->code.begin() << endl;
    os << "stack: ";
    for(auto v: stk){
        v.print_self(os);
        os << " ";
    }
    os << endl;
}

uint16_t VM::read_short(){
    return (*(ip++))*(1<<8) + (*(ip++));
}


bool VM::run(){
    
    for(;ip<prog->code.end();){
        
        if (trace_execution) print_self(cerr);
        switch((*(ip++))){
            case OP_NULL:
                stk.push_back(Value());
                break;
            case OP_ADD:  // basic operations
                BINARY_OP(+);
                break;
            case OP_SUB:
                BINARY_OP(-);
                break;
            case OP_MUL:
                BINARY_OP(*);
                break;
            case OP_DIV:
                if (peek(0).get_int() == 0){
                    throw_error("Division by zero");
                }
                BINARY_OP(/);
                break;
            case OP_NEG:
                if (peek(0).is_int()){
                    stk.push_back(Value(-pop().get_int()));
                }
                // if it's a list, pop the last element
                else if (peek(0).is_list()){
                    vptr l = pop().get_list();
                    if (l->size() == 0){
                        throw_error("List is empty");
                    }
                    stk.push_back(l->back());
                    l->pop_back();
                }
                else {
                    throw_error("Operand must be an integer or a list");
                }
                
                break;
            case OP_EQ: {
                const Value b = pop();
                const Value a = pop();
                stk.push_back(Value(a == b));
                break;
            }
            case OP_NE: {
                const Value b = pop();
                const Value a = pop();
                stk.push_back(Value(!(a == b)));
                break;
            }
            case OP_GRTR: {
                const Value b = pop();
                const Value a = pop();
                stk.push_back(Value(a > b));
                break;
            }
            case OP_GE: {
                const Value b = pop();
                const Value a = pop();
                stk.push_back(Value(!(a < b)));
                break;
            }
            case OP_LESS: {
                const Value b = pop();
                const Value a = pop();
                stk.push_back(Value(a < b));
                break;
            }
            case OP_LE: {
                const Value b = pop();
                const Value a = pop();
                stk.push_back(Value(!(a > b)));
                break;
            }
            case OP_APP: {
                Value toapp = pop();
                if (!peek(0).is_list()){
                    throw_error("Operand must be a list");
                }
                (peek(0).get_list())->push_back(toapp);
                break;
            }
            case OP_APP_POP: {
                Value toapp = pop();
                Value list = pop();
                if (!list.is_list()){
                    throw_error("Operand must be a list");
                }
                list.get_list()->push_back(toapp);
                break;
            }
            case OP_NOT:
                if (!peek(0).is_int()){
                    throw_error("Operand must be an integer");
                }
                stk.push_back(Value(!pop()));  
                break; 
            case OP_ABS:
                // operand must be an int or a list
                if (peek(0).is_int()){
                    stk.push_back(Value(abs(pop().get_int())));
                }
                else if (peek(0).is_list()){
                    // return the length of the list
                    stk.push_back(Value((int)pop().get_list()->size()));
                }
                else {
                    throw_error("Operand must be an integer or a list");
                }
                break;
            case OP_CONST:
                
                stk.push_back(prog->consts[(*(ip++))]);
                break;
            case OP_LIST :{
                int n = (*(ip++));
                vector<Value> l(n);
                for(int i=n-1;i>=0;--i){
                    l[i] = pop();
                }
                stk.push_back(Value(std::move(l)));
                break;
            }


            case OP_PRINT:
                cout << ">> ";
                pop().print_self(cout); 
                cout << endl;
                break;
            case OP_INPUT:
                cout << "<< " << flush;
                int v;
                cin >> v;
                stk.push_back(Value(v));
                break;
            case OP_POP:
                stk.pop_back();
                break;

            case OP_SET_GLOBAL: {
                const uint8_t index = *(ip++);
                globals[index] = peek(0);
                global_defined[index] = 1;
                break;
            }
            case OP_SET_GLOBAL_POP: {
                const uint8_t index = *(ip++);
                globals[index] = pop();
                global_defined[index] = 1;
                break;
            }
            case OP_GET_GLOBAL: {
                const uint8_t index = *(ip++);
                if (!global_defined[index]){ // globals remain late bound
                    throw_error("Undefined global");
                }
                stk.push_back(globals[index]);
                break;
            }

            case OP_SET_LOCAL:
                stk[(*(ip++))+frames.back().stack_start] = peek(0);
                break;
            case OP_SET_LOCAL_POP: {
                const uint8_t index = *(ip++);
                const Value value = pop();
                stk[index+frames.back().stack_start] = value;
                break;
            }
            case OP_GET_LOCAL:
                stk.push_back(stk[(*(ip++))+frames.back().stack_start]);
                break;
            case OP_DEF_LOCAL:
                stk.push_back(peek(0));
                break;
            case OP_GET_IND: {
                Value vnum = pop(), vlist = pop();
                if (!vlist.is_list() || !vnum.is_int()){
                    throw_error("Invalid operation");
                }
                const int index = vnum.get_int();
                vector<Value>* const list = vlist.get_list();
                if (index < 0 || static_cast<size_t>(index) >= list->size()){
                    throw_error("Index out of bounds");
                }
                stk.push_back((*list)[index]);
                break;
            }
            case OP_SET_IND: { // the value to be set to is at the bottom of the stack
                Value vval = pop(), vnum = pop(), vlist = pop(); // temporary variables, will be optimised by the compiler
                if (!vlist.is_list() || !vnum.is_int()){
                    throw_error("Invalid operation");
                }
                const int index = vnum.get_int();
                vector<Value>* const list = vlist.get_list();
                if (index < 0 || static_cast<size_t>(index) >= list->size()){
                    throw_error("Index out of bounds");
                }
                (*list)[index] = vval;
                stk.push_back(vval); // This could be a cause of slowness - check if it's a great performance loss
                break;
            }
            case OP_SET_IND_POP: {
                Value vval = pop(), vnum = pop(), vlist = pop();
                if (!vlist.is_list() || !vnum.is_int()){
                    throw_error("Invalid operation");
                }
                const int index = vnum.get_int();
                vector<Value>* const list = vlist.get_list();
                if (index < 0 || static_cast<size_t>(index) >= list->size()){
                    throw_error("Index out of bounds");
                }
                (*list)[index] = vval;
                break;
            }
            case OP_JMP_F: // note: this does not actually use the c++ if statement - it can be implemented without
                if (!peek(0).get_int()){
                    ip += read_short();
                }
                else {
                    read_short();
                }
                // ip += (bool(pop())-1)*read_short();  - will benchmark later
                break;
            case OP_JMP_F_POP: {
                const bool condition = pop().get_int();
                const uint16_t offset = read_short();
                if (!condition){
                    ip += offset;
                }
                break;
            }
            case OP_JMP:
                ip += read_short(); 
                break;
            case OP_LOOP:
                ip -= read_short(); 
                break;
            case OP_CALL: {
                if (!peek(0).is_func()){
                    throw_error("Operand must be a function");
                }
                Program* p = pop().get_func();
                uint8_t arity = (*(ip++));
                if (arity != p->arity){
                    throw_error("Invalid arity");
                }
                frames.push_back({prog, ip, stk.size()-arity});
                prog = p;
                ip = p->code.begin();
                
                break;
            }

            case OP_RETURN: {
                // case: if done in main body
                if (frames.size() == 1){
                    return 0;
                }
                Value ret_val = pop();
                const CallFrame frame = frames.back();
                stk.resize(frame.stack_start);
                frames.pop_back();
                prog = frame.caller;
                ip = frame.return_ip;
                stk.push_back(ret_val);
                break;
            }
                
            default:

                throw_error("Invalid opcode");
                break;
        }
    }
    
    return 0;
}

void VM::throw_error(string msg){
    cerr << msg << endl;
    exit(1);
}
