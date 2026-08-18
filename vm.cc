
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
    for (Program* p: progs){
        delete p;
    }
        

   
} // we must remember to delete all the lists and programs!!!

vector<Value>* VM::allocate_list(vector<Value>&& values){
    if (active_lists + 1 > next_gc){
        collect_garbage(&values);
    }

    ListObject* object;
    if (free_lists.empty()){
        list_pool.push_back({std::move(values), false, true});
        object = &list_pool.back();
        list_lookup[&object->values] = object;
    }
    else {
        object = free_lists.back();
        free_lists.pop_back();
        if (values.empty()){
            object->values.clear();
        }
        else {
            object->values = std::move(values);
        }
        object->marked = false;
        object->in_use = true;
    }
    ++active_lists;
    return &object->values;
}

void VM::mark_program(Program* program, unordered_set<Program*>& marked_programs){
    if (program == nullptr || !marked_programs.insert(program).second){
        return;
    }
    for (const Value& value: program->consts){
        mark_value(value, marked_programs);
    }
}

void VM::mark_value(const Value& value, unordered_set<Program*>& marked_programs){
    if (value.is_list()){
        auto found = list_lookup.find(value.get_list());
        if (found == list_lookup.end() || !found->second->in_use || found->second->marked){
            return;
        }
        ListObject* object = found->second;
        object->marked = true;
        for (const Value& element: object->values){
            mark_value(element, marked_programs);
        }
    }
    else if (value.is_func()){
        mark_program(value.get_func(), marked_programs);
    }
}

void VM::collect_garbage(const vector<Value>* extra_roots){
    unordered_set<Program*> marked_programs;
    for (const Value& value: stk){
        mark_value(value, marked_programs);
    }
    for (size_t i = 0; i < globals.size(); ++i){
        if (global_defined[i]){
            mark_value(globals[i], marked_programs);
        }
    }
    if (extra_roots != nullptr){
        for (const Value& value: *extra_roots){
            mark_value(value, marked_programs);
        }
    }
    mark_program(prog, marked_programs);
    for (const CallFrame& frame: frames){
        mark_program(frame.caller, marked_programs);
    }

    for (ListObject& object: list_pool){
        if (!object.in_use){
            continue;
        }
        if (object.marked){
            object.marked = false;
            continue;
        }
        object.values.clear();
        object.in_use = false;
        free_lists.push_back(&object);
        --active_lists;
    }
    next_gc = max<size_t>(1024, active_lists * 2);
}

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
        const uint8_t opcode = *(ip++);
#if (defined(__GNUC__) || defined(__clang__)) && !defined(SYMBSCRIPT_PORTABLE_DISPATCH)
        static const void* const dispatch_table[OP_COUNT] = {
            &&dispatch_switch, &&dispatch_switch, &&dispatch_switch, &&dispatch_switch,
            &&dispatch_switch, &&dispatch_switch, &&dispatch_switch, &&dispatch_switch,
            &&dispatch_switch, &&dispatch_switch, &&dispatch_switch, &&dispatch_switch,
            &&dispatch_switch, &&dispatch_switch, &&dispatch_app_pop,
            &&dispatch_const, &&dispatch_null, &&dispatch_switch,
            &&dispatch_switch, &&dispatch_switch, &&dispatch_pop,
            &&dispatch_switch, &&dispatch_set_global_pop, &&dispatch_get_global,
            &&dispatch_switch, &&dispatch_set_local_pop, &&dispatch_get_local,
            &&dispatch_switch, &&dispatch_switch, &&dispatch_switch,
            &&dispatch_switch, &&dispatch_switch, &&dispatch_jump_false_pop,
            &&dispatch_switch, &&dispatch_loop, &&dispatch_check_global,
            &&dispatch_slide, &&dispatch_add_local_imm, &&dispatch_inc_local,
            &&dispatch_abs_local, &&dispatch_compare_local,
            &&dispatch_get_ind_local, &&dispatch_get_ind_local_offset,
            &&dispatch_copy_ind_local, &&dispatch_set_ind_local_value,
            &&dispatch_switch, &&dispatch_switch,
        };
        goto *dispatch_table[opcode];
dispatch_switch:
#endif
        switch(opcode){
            case OP_NULL:
                stk.push_back(Value());
                break;
            case OP_ADD:  // basic operations
                BINARY_OP(+);
                break;
            case OP_ADD_LOCAL_IMM: {
                const uint8_t index = *(ip++);
                const int amount = static_cast<int8_t>(*(ip++));
                const Value& value = stk[frames.back().stack_start+index];
                if (!value.is_int()){
                    throw_error("Operands must be integers");
                }
                stk.push_back(Value(value.get_int()+amount));
                break;
            }
            case OP_INC_LOCAL: {
                const uint8_t index = *(ip++);
                const int amount = static_cast<int8_t>(*(ip++));
                Value& value = stk[frames.back().stack_start+index];
                if (!value.is_int()){
                    throw_error("Operands must be integers");
                }
                value.add_to_int(amount);
                break;
            }
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
            case OP_ABS_LOCAL: {
                const uint8_t index = *(ip++);
                const Value& value = stk[frames.back().stack_start+index];
                if (value.is_int()){
                    stk.push_back(Value(abs(value.get_int())));
                }
                else if (value.is_list()){
                    stk.push_back(Value(static_cast<int>(value.get_list()->size())));
                }
                else {
                    throw_error("Operand must be an integer or a list");
                }
                break;
            }
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
            case OP_CHECK_GLOBAL: {
                const uint8_t index = *(ip++);
                if (!global_defined[index]){
                    throw_error("Undefined global");
                }
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
            case OP_GET_IND_LOCAL:
            case OP_GET_IND_LOCAL_OFFSET: {
                const uint8_t list_local = *(ip++);
                const uint8_t index_local = *(ip++);
                int index_offset = 0;
                if (*(ip-3) == OP_GET_IND_LOCAL_OFFSET){
                    index_offset = static_cast<int8_t>(*(ip++));
                }
                const size_t frame = frames.back().stack_start;
                const Value& list_value = stk[frame+list_local];
                const Value& index_value = stk[frame+index_local];
                if (!list_value.is_list() || !index_value.is_int()){
                    throw_error("Invalid operation");
                }
                const int index = index_value.get_int()+index_offset;
                vector<Value>* const list = list_value.get_list();
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
            case OP_COPY_IND_LOCAL: {
                const uint8_t list_local = *(ip++);
                const uint8_t target_local = *(ip++);
                const uint8_t source_local = *(ip++);
                const size_t frame = frames.back().stack_start;
                Value& list_value = stk[frame+list_local];
                const Value& target_value = stk[frame+target_local];
                const Value& source_value = stk[frame+source_local];
                if (!list_value.is_list() || !target_value.is_int() || !source_value.is_int()){
                    throw_error("Invalid operation");
                }
                vector<Value>* const list = list_value.get_list();
                const int source = source_value.get_int();
                if (source < 0 || static_cast<size_t>(source) >= list->size()){
                    throw_error("Index out of bounds");
                }
                const Value copied = (*list)[source];
                const int target = target_value.get_int();
                if (target < 0 || static_cast<size_t>(target) >= list->size()){
                    throw_error("Index out of bounds");
                }
                (*list)[target] = copied;
                break;
            }
            case OP_SET_IND_LOCAL_VALUE: {
                const uint8_t list_local = *(ip++);
                const uint8_t target_local = *(ip++);
                const uint8_t value_local = *(ip++);
                const size_t frame = frames.back().stack_start;
                Value& list_value = stk[frame+list_local];
                const Value& target_value = stk[frame+target_local];
                const Value copied = stk[frame+value_local];
                if (!list_value.is_list() || !target_value.is_int()){
                    throw_error("Invalid operation");
                }
                vector<Value>* const list = list_value.get_list();
                const int target = target_value.get_int();
                if (target < 0 || static_cast<size_t>(target) >= list->size()){
                    throw_error("Index out of bounds");
                }
                (*list)[target] = copied;
                break;
            }
            case OP_CMP_LOCAL_LOCAL: {
                const uint8_t left_local = *(ip++);
                const uint8_t right_local = *(ip++);
                const uint8_t operation = *(ip++);
                const size_t frame = frames.back().stack_start;
                const Value& a = stk[frame+left_local];
                const Value& b = stk[frame+right_local];
                bool result;
                switch (operation){
                    case OP_EQ: result = a == b; break;
                    case OP_NE: result = !(a == b); break;
                    case OP_GRTR: result = a > b; break;
                    case OP_GE: result = !(a < b); break;
                    case OP_LESS: result = a < b; break;
                    case OP_LE: result = !(a > b); break;
                    default: result = false; break;
                }
                stk.push_back(Value(result));
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
            case OP_SLIDE: {
                const uint8_t count = *(ip++);
                const Value result = pop();
                stk.resize(stk.size()-count);
                stk.push_back(result);
                break;
            }
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
        continue;

#if (defined(__GNUC__) || defined(__clang__)) && !defined(SYMBSCRIPT_PORTABLE_DISPATCH)
dispatch_null:
        stk.push_back(Value());
        continue;

dispatch_const:
        stk.push_back(prog->consts[(*(ip++))]);
        continue;

dispatch_pop:
        stk.pop_back();
        continue;

dispatch_app_pop: {
        Value toapp = pop();
        Value list = pop();
        if (!list.is_list()){
            throw_error("Operand must be a list");
        }
        list.get_list()->push_back(toapp);
        continue;
    }

dispatch_set_global_pop: {
        const uint8_t index = *(ip++);
        globals[index] = pop();
        global_defined[index] = 1;
        continue;
    }

dispatch_get_global: {
        const uint8_t index = *(ip++);
        if (!global_defined[index]){
            throw_error("Undefined global");
        }
        stk.push_back(globals[index]);
        continue;
    }

dispatch_set_local_pop: {
        const uint8_t index = *(ip++);
        const Value value = pop();
        stk[index+frames.back().stack_start] = value;
        continue;
    }

dispatch_get_local:
        stk.push_back(stk[(*(ip++))+frames.back().stack_start]);
        continue;

dispatch_jump_false_pop: {
        const bool condition = pop().get_int();
        const uint16_t offset = read_short();
        if (!condition){
            ip += offset;
        }
        continue;
    }

dispatch_loop:
        ip -= read_short();
        continue;

dispatch_check_global: {
        const uint8_t index = *(ip++);
        if (!global_defined[index]){
            throw_error("Undefined global");
        }
        continue;
    }

dispatch_slide: {
        const uint8_t count = *(ip++);
        const Value result = pop();
        stk.resize(stk.size()-count);
        stk.push_back(result);
        continue;
    }

dispatch_add_local_imm: {
        const uint8_t index = *(ip++);
        const int amount = static_cast<int8_t>(*(ip++));
        const Value& value = stk[frames.back().stack_start+index];
        if (!value.is_int()){
            throw_error("Operands must be integers");
        }
        stk.push_back(Value(value.get_int()+amount));
        continue;
    }

dispatch_inc_local: {
        const uint8_t index = *(ip++);
        const int amount = static_cast<int8_t>(*(ip++));
        Value& value = stk[frames.back().stack_start+index];
        if (!value.is_int()){
            throw_error("Operands must be integers");
        }
        value.add_to_int(amount);
        continue;
    }

dispatch_abs_local: {
        const uint8_t index = *(ip++);
        const Value& value = stk[frames.back().stack_start+index];
        if (value.is_int()){
            stk.push_back(Value(abs(value.get_int())));
        }
        else if (value.is_list()){
            stk.push_back(Value(static_cast<int>(value.get_list()->size())));
        }
        else {
            throw_error("Operand must be an integer or a list");
        }
        continue;
    }

dispatch_compare_local: {
        const uint8_t left_local = *(ip++);
        const uint8_t right_local = *(ip++);
        const uint8_t operation = *(ip++);
        const size_t frame = frames.back().stack_start;
        const Value& a = stk[frame+left_local];
        const Value& b = stk[frame+right_local];
        bool result;
        switch (operation){
            case OP_EQ: result = a == b; break;
            case OP_NE: result = !(a == b); break;
            case OP_GRTR: result = a > b; break;
            case OP_GE: result = !(a < b); break;
            case OP_LESS: result = a < b; break;
            case OP_LE: result = !(a > b); break;
            default: result = false; break;
        }
        stk.push_back(Value(result));
        continue;
    }

dispatch_get_ind_local:
dispatch_get_ind_local_offset: {
        const bool has_offset = opcode == OP_GET_IND_LOCAL_OFFSET;
        const uint8_t list_local = *(ip++);
        const uint8_t index_local = *(ip++);
        const int offset = has_offset ? static_cast<int8_t>(*(ip++)) : 0;
        const size_t frame = frames.back().stack_start;
        const Value& list_value = stk[frame+list_local];
        const Value& index_value = stk[frame+index_local];
        if (!list_value.is_list() || !index_value.is_int()){
            throw_error("Invalid operation");
        }
        const int index = index_value.get_int()+offset;
        vector<Value>* const list = list_value.get_list();
        if (index < 0 || static_cast<size_t>(index) >= list->size()){
            throw_error("Index out of bounds");
        }
        stk.push_back((*list)[index]);
        continue;
    }

dispatch_copy_ind_local: {
        const uint8_t list_local = *(ip++);
        const uint8_t target_local = *(ip++);
        const uint8_t source_local = *(ip++);
        const size_t frame = frames.back().stack_start;
        Value& list_value = stk[frame+list_local];
        const Value& target_value = stk[frame+target_local];
        const Value& source_value = stk[frame+source_local];
        if (!list_value.is_list() || !target_value.is_int() || !source_value.is_int()){
            throw_error("Invalid operation");
        }
        vector<Value>* const list = list_value.get_list();
        const int source = source_value.get_int();
        if (source < 0 || static_cast<size_t>(source) >= list->size()){
            throw_error("Index out of bounds");
        }
        const Value copied = (*list)[source];
        const int target = target_value.get_int();
        if (target < 0 || static_cast<size_t>(target) >= list->size()){
            throw_error("Index out of bounds");
        }
        (*list)[target] = copied;
        continue;
    }

dispatch_set_ind_local_value: {
        const uint8_t list_local = *(ip++);
        const uint8_t target_local = *(ip++);
        const uint8_t value_local = *(ip++);
        const size_t frame = frames.back().stack_start;
        Value& list_value = stk[frame+list_local];
        const Value& target_value = stk[frame+target_local];
        const Value copied = stk[frame+value_local];
        if (!list_value.is_list() || !target_value.is_int()){
            throw_error("Invalid operation");
        }
        vector<Value>* const list = list_value.get_list();
        const int target = target_value.get_int();
        if (target < 0 || static_cast<size_t>(target) >= list->size()){
            throw_error("Index out of bounds");
        }
        (*list)[target] = copied;
        continue;
    }
#endif
    }
    
    return 0;
}

void VM::throw_error(string msg){
    cerr << msg << endl;
    exit(1);
}
