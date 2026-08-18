#include "compiler.hh"
#include "vm.hh"
#include <algorithm>
#include <functional>
#include <string>

Compiler::Compiler(unordered_map<string, int>& glob_index, unordered_map<string, int> loc_index,
                vector<string> loc_names, vector<int> loc_counts): global_index(glob_index), local_index(loc_index), local_names(loc_names), local_counts(loc_counts){}



Compiler::Compiler(){
    local_counts.push_back(0);
}

bool Compiler::local_operand(Ast* expression, uint8_t& index) const{
    if (expression->type != ID || global_index.find(expression->id) != global_index.end()){
        return false;
    }
    auto found = local_index.find(expression->id);
    if (found == local_index.end()){
        return false;
    }
    index = static_cast<uint8_t>(found->second);
    return true;
}

bool Compiler::small_integer(Ast* expression, int8_t& value) const{
    if (expression->type != INT || expression->num < INT8_MIN || expression->num > INT8_MAX){
        return false;
    }
    value = static_cast<int8_t>(expression->num);
    return true;
}

bool Compiler::try_superinstruction(Ast* expression){
    if (expression->type == SET && expression->ch[0]->type == ID){
        uint8_t target;
        Ast* rhs = expression->ch[1];
        if (local_operand(expression->ch[0], target) && rhs->type == ADD){
            uint8_t source;
            int8_t amount;
            if (local_operand(rhs->ch[0], source) && source == target
                    && small_integer(rhs->ch[1], amount)){
                prog.push_byte(OP_INC_LOCAL);
                prog.push_byte(target);
                prog.push_byte(static_cast<uint8_t>(amount));
                return true;
            }
        }
    }

    if (expression->type != SET || expression->ch[0]->type != IND){
        return false;
    }
    Ast* target = expression->ch[0];
    uint8_t list_index, target_index;
    if (!local_operand(target->ch[0], list_index) || !local_operand(target->ch[1], target_index)){
        return false;
    }

    Ast* rhs = expression->ch[1];
    if (rhs->type == IND){
        uint8_t source_list, source_index;
        if (local_operand(rhs->ch[0], source_list) && source_list == list_index
                && local_operand(rhs->ch[1], source_index)){
            prog.push_byte(OP_COPY_IND_LOCAL);
            prog.push_byte(list_index);
            prog.push_byte(target_index);
            prog.push_byte(source_index);
            return true;
        }
    }

    uint8_t value_index;
    if (local_operand(rhs, value_index)){
        prog.push_byte(OP_SET_IND_LOCAL_VALUE);
        prog.push_byte(list_index);
        prog.push_byte(target_index);
        prog.push_byte(value_index);
        return true;
    }
    return false;
}

bool Compiler::has_return(Ast* exp) const{
    if (exp->type == RET){
        return true;
    }
    for (Ast* child: exp->ch){
        if (has_return(child)){
            return true;
        }
    }
    return false;
}

void Compiler::discover_inline_functions(Ast* exp){
    unordered_map<string, int> assignments;
    unordered_map<string, Ast*> candidates;

    function<void(Ast*)> collect_assignments = [&](Ast* node){
        if (node->type == SET && node->ch[0]->type == ID){
            const string& name = node->ch[0]->id;
            if (global_index.find(name) != global_index.end()){
                ++assignments[name];
                if (node->ch[1]->type == DEF){
                    candidates[name] = node->ch[1];
                }
            }
        }
        for (Ast* child: node->ch){
            collect_assignments(child);
        }
    };
    collect_assignments(exp);

    for (const auto& candidate: candidates){
        Ast* definition = candidate.second;
        Ast* sequence = definition->ch[1]->ch[0];
        bool simple_returns = true;
        for (size_t i = 0; i + 1 < sequence->ch.size(); ++i){
            if (has_return(sequence->ch[i])){
                simple_returns = false;
                break;
            }
        }
        if (!sequence->ch.empty() && sequence->ch.back()->type != RET
                && has_return(sequence->ch.back())){
            simple_returns = false;
        }
        if (assignments[candidate.first] == 1 && simple_returns){
            inline_functions[candidate.first] = definition;
        }
    }
}

bool Compiler::try_inline_call(Ast* callee, Ast* arguments){
    if (callee->type != ID){
        return false;
    }
    auto found = inline_functions.find(callee->id);
    if (found == inline_functions.end()){
        return false;
    }
    Ast* definition = found->second;
    if (find(inline_stack.begin(), inline_stack.end(), definition) != inline_stack.end()
            || definition->ch[0]->ch.size() != arguments->ch.size()){
        return false;
    }

    for (Ast* argument: arguments->ch){
        compile(argument);
    }
    prog.push_byte(OP_CHECK_GLOBAL);
    prog.push_byte(global_index[callee->id]);

    const auto saved_local_index = local_index;
    const size_t saved_name_count = local_names.size();
    const int outer_local_count = local_counts.back();
    begin_scope();

    for (Ast* parameter: definition->ch[0]->ch){
        local_index[parameter->id] = local_counts.back()++;
        local_names.push_back(parameter->id);
    }

    inline_stack.push_back(definition);
    Ast* sequence = definition->ch[1]->ch[0];
    const bool has_final_return = !sequence->ch.empty() && sequence->ch.back()->type == RET;
    const size_t statement_count = sequence->ch.size() - (has_final_return ? 1 : 0);
    for (size_t i = 0; i < statement_count; ++i){
        compile(sequence->ch[i]);
    }
    if (has_final_return){
        compile(sequence->ch.back()->ch[0]);
    }
    else {
        prog.push_byte(OP_NULL);
    }
    inline_stack.pop_back();

    const int locals_to_remove = local_counts.back() - outer_local_count;
    if (locals_to_remove > UINT8_MAX){
        cerr << "Too many locals in inlined function" << endl;
        exit(1);
    }
    prog.push_byte(OP_SLIDE);
    prog.push_byte(locals_to_remove);

    local_counts.pop_back();
    local_names.resize(saved_name_count);
    local_index = saved_local_index;
    scope--;
    return true;
}
void Compiler::resolve_globals(Ast* exp){ // this is needed to allocate global variables (as they can be referenced before they are defined)
    switch(exp->type){
        case BLK:
        case FOR:
        case DEF:
            break;
        case IF:
            resolve_globals(exp->ch[0]);
            break;

        case SET:
            if(global_index.find(exp->ch[0]->id) == global_index.end()){
                global_index[exp->ch[0]->id] = global_index.size();
            }
            if (global_index[exp->ch[0]->id] > UINT8_MAX){
                cerr << "Too many global variables" << endl;
                exit(1); // Allocate global variables
            }
            for(auto c: exp->ch){
                resolve_globals(c);
            } 
            break;
        default:
            for(auto c: exp->ch){
                resolve_globals(c); // Recursively resolve globals
            }
            break;
    }
}


void Compiler::begin_scope(){ // Add a new scope
    scope++;
    local_counts.push_back(local_counts.back()); 
}

void Compiler::end_scope(){ // Delete scope and free the correct number of variables
    int to_remove = local_counts.back() - local_counts[local_counts.size()-2];
        for (int i=0; i<to_remove; ++i){
            prog.push_byte(OP_POP);
            local_index.erase(local_names.back());
            local_names.pop_back();
        }
    local_counts.pop_back();
    scope--;
} 

void Compiler::set_var(string name){ // Set a local variable
    if (global_index.find(name) != global_index.end()){ // it is a global, so already resolved
        prog.push_byte(OP_SET_GLOBAL);
        prog.push_byte(global_index[name]);
        return;
    }
    
    // it is a local variable
    if (local_index.find(name) == local_index.end()){
        local_index[name] = local_counts.back()++;
        local_names.push_back(name);
        prog.push_byte(OP_DEF_LOCAL); // If not found, define local variable
    } 
    else{
        prog.push_byte(OP_SET_LOCAL);
        prog.push_byte(local_index[name]);
    }

}

void Compiler::get_var(string name){ // Get a local variable
    if (global_index.find(name) != global_index.end()){ // it is a global, so already resolved
            prog.push_byte(OP_GET_GLOBAL);
            prog.push_byte(global_index[name]);
            return;
        }
        // it is a local variable
    if (local_index.find(name) == local_index.end()){
        cerr << "Undefined variable " << name << endl;
        exit(1);
    }
    prog.push_byte(OP_GET_LOCAL);
    prog.push_byte(local_index[name]);
}

void Compiler::compile(Ast* exp){  
    switch(exp->type){ // Loop over types of tree node
        case ADD: // Basic operations (recursively compile tree nodes then add the operand)
            {
            uint8_t local;
            int8_t immediate;
            if (local_operand(exp->ch[0], local) && small_integer(exp->ch[1], immediate)){
                prog.push_byte(OP_ADD_LOCAL_IMM);
                prog.push_byte(local);
                prog.push_byte(static_cast<uint8_t>(immediate));
                break;
            }
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_ADD);
            break;
            }
        case SUB:
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_SUB);
            break;
        case MUL:
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_MUL);
            break;
        case DIV:
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_DIV);
            break;
        case NEG:
            compile(exp->ch[0]);
            prog.push_byte(OP_NEG);
            break;
        case EQ:
            {
            uint8_t left, right;
            if (local_operand(exp->ch[0], left) && local_operand(exp->ch[1], right)){
                prog.push_byte(OP_CMP_LOCAL_LOCAL);
                prog.push_byte(left); prog.push_byte(right); prog.push_byte(OP_EQ);
                break;
            }
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_EQ);
            break;
            }
        case GT:
            {
            uint8_t left, right;
            if (local_operand(exp->ch[0], left) && local_operand(exp->ch[1], right)){
                prog.push_byte(OP_CMP_LOCAL_LOCAL);
                prog.push_byte(left); prog.push_byte(right); prog.push_byte(OP_GRTR);
                break;
            }
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_GRTR);
            break;
            }
        case LT:
            {
            uint8_t left, right;
            if (local_operand(exp->ch[0], left) && local_operand(exp->ch[1], right)){
                prog.push_byte(OP_CMP_LOCAL_LOCAL);
                prog.push_byte(left); prog.push_byte(right); prog.push_byte(OP_LESS);
                break;
            }
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_LESS);
            break;
            }
        case GE:
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_GE);
            break;
        case LE:
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_LE);
            break;
        case NE:
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_NE);
            break;
        case APP:
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_APP);
            break;
        case NOT:
            // Peephole comparison inversion avoids materialising and then
            // negating an intermediate boolean.
            switch (exp->ch[0]->type){
                case EQ: case NE: case GT: case LT: case GE: case LE: {
                    compile(exp->ch[0]->ch[0]);
                    compile(exp->ch[0]->ch[1]);
                    switch (exp->ch[0]->type){
                        case EQ: prog.push_byte(OP_NE); break;
                        case NE: prog.push_byte(OP_EQ); break;
                        case GT: prog.push_byte(OP_LE); break;
                        case LT: prog.push_byte(OP_GE); break;
                        case GE: prog.push_byte(OP_LESS); break;
                        case LE: prog.push_byte(OP_GRTR); break;
                        default: break;
                    }
                    break;
                }
                default:
                    compile(exp->ch[0]);
                    prog.push_byte(OP_NOT);
                    break;
            }
            break;
        case ABS:
            {
            uint8_t local;
            if (local_operand(exp->ch[0], local)){
                prog.push_byte(OP_ABS_LOCAL);
                prog.push_byte(local);
                break;
            }
            compile(exp->ch[0]);
            prog.push_byte(OP_ABS);
            break;
            }
        case SEQ:
            for(auto c: exp->ch){
                compile(c);
            }
            break;
        case PRINT:
            compile(exp->ch[0]);
            prog.push_byte(OP_PRINT);
            break;
        case INP:
            prog.push_byte(OP_INPUT);
            break;
        case BLK: {
                begin_scope();
                for(auto c: exp->ch){
                    compile(c);
                }
                end_scope();
                break;
            }

        case INT:
            prog.push_const(Value(exp->num)); // Adds a constant to the program
            break;
        case LST:
            begin_scope();
            for (auto c: exp->ch){
                compile(c);
            }
            end_scope();
            prog.push_byte(OP_LIST);
            if (exp->ch.size() > UINT8_MAX){
                cerr << "Too many elements in list initialiser" << endl;
                exit(1);
            }
            prog.push_byte(exp->ch.size());
            break;
        case EXP:
            if (try_superinstruction(exp->ch[0])){
                break;
            }
            compile(exp->ch[0]);
            // Assignment and append statements can discard their result as part of
            // the operation instead of executing a separate OP_POP.
            if (exp->ch[0]->type == SET){
                if (exp->ch[0]->ch[0]->type == IND){
                    prog.code.back() = OP_SET_IND_POP;
                }
                else if (prog.code.back() == OP_DEF_LOCAL){
                    prog.code.pop_back(); // the existing RHS becomes the local slot
                }
                else if (prog.code.size() >= 2 && prog.code[prog.code.size()-2] == OP_SET_LOCAL){
                    prog.code[prog.code.size()-2] = OP_SET_LOCAL_POP;
                }
                else if (prog.code.size() >= 2 && prog.code[prog.code.size()-2] == OP_SET_GLOBAL){
                    prog.code[prog.code.size()-2] = OP_SET_GLOBAL_POP;
                }
                else {
                    prog.push_byte(OP_POP);
                }
            }
            else if (exp->ch[0]->type == APP){
                prog.code.back() = OP_APP_POP;
            }
            else {
                prog.push_byte(OP_POP);
            }
            break;

        case SET:
            switch (exp->ch[0]->type){
                case ID: {
                    compile(exp->ch[1]);
                    set_var(exp->ch[0]->id);

                    break;
                }
                case IND:
                    compile(exp->ch[0]->ch[0]);
                    compile(exp->ch[0]->ch[1]);
                    compile(exp->ch[1]);
                    prog.push_byte(OP_SET_IND);
                    break;
            }
            
            
            
            break;
        case ID:
        
            get_var(exp->id);
            break;

        case IND: 
            {
            uint8_t list_index, element_index;
            if (local_operand(exp->ch[0], list_index) && local_operand(exp->ch[1], element_index)){
                prog.push_byte(OP_GET_IND_LOCAL);
                prog.push_byte(list_index);
                prog.push_byte(element_index);
                break;
            }
            if (local_operand(exp->ch[0], list_index)
                    && (exp->ch[1]->type == ADD || exp->ch[1]->type == SUB)){
                uint8_t base_index;
                int8_t offset;
                if (local_operand(exp->ch[1]->ch[0], base_index)
                        && small_integer(exp->ch[1]->ch[1], offset)){
                    if (exp->ch[1]->type == SUB){
                        if (offset == INT8_MIN){
                            compile(exp->ch[0]);
                            compile(exp->ch[1]);
                            prog.push_byte(OP_GET_IND);
                            break;
                        }
                        offset = -offset;
                    }
                    prog.push_byte(OP_GET_IND_LOCAL_OFFSET);
                    prog.push_byte(list_index);
                    prog.push_byte(base_index);
                    prog.push_byte(static_cast<uint8_t>(offset));
                    break;
                }
            }
            compile(exp->ch[0]);
            compile(exp->ch[1]);
            prog.push_byte(OP_GET_IND);
            break;
            }
           
        
        case IF: {

            begin_scope();

            compile(exp->ch[0]);
            int j_else = prog.push_jump(OP_JMP_F_POP);

            begin_scope(); 
            compile(exp->ch[1]); // then block
            end_scope(); 

            int j_end = prog.push_jump(OP_JMP); // jump to end after then block

            prog.patch_jump(j_else); // begin else block

            begin_scope();
            compile(exp->ch[2]); // else block
            end_scope();


            prog.patch_jump(j_end);

            end_scope();

            break;
        }
        case AND: {
            compile(exp->ch[0]);
            int j_end = prog.push_jump(OP_JMP_F); // if first condition false then jump to end
            prog.push_byte(OP_POP);
            compile(exp->ch[1]);
            prog.patch_jump(j_end);
            break;
        }
        case OR: {
            compile(exp->ch[0]);
            int j_skip = prog.push_jump(OP_JMP_F); // if first condition false, skip over jump to end
            int j_end = prog.push_jump(OP_JMP); // if first condition true then jump to end
            prog.patch_jump(j_skip);
            prog.push_byte(OP_POP);
            compile(exp->ch[1]);
            prog.patch_jump(j_end);
            break;
        }
        case WHL: {

            begin_scope();

            int loop_start = prog.code.size();
            compile(exp->ch[0]); // condition
            int j_end = prog.push_jump(OP_JMP_F_POP);

            begin_scope();
            compile(exp->ch[1]);
            end_scope();

            prog.push_loop(loop_start); // loop to beginning

            prog.patch_jump(j_end);

            end_scope();
            break;
        }
        case FOR: {
            begin_scope();
            compile(exp->ch[0]); // initialiser

            int loop_start = prog.code.size();
            compile(exp->ch[1]); // condition
            int j_end = prog.push_jump(OP_JMP_F_POP);

            begin_scope();
            compile(exp->ch[3]); // body
            end_scope();
            compile(exp->ch[2]); // increment
            prog.push_loop(loop_start); // loop to start

            prog.patch_jump(j_end);
            end_scope();
            break;
        }
        case RNG: {
            // for a in b do c
            begin_scope();
            // initialiser
            string num = to_string(range_for_ct++);
            string loop_name = num + "i";
            string list_name = num + "l";

            prog.push_byte(OP_NULL);
            set_var(loop_name);
            prog.push_byte(OP_POP);

            compile(exp->ch[1]); // list
            set_var(list_name);
            prog.push_byte(OP_POP);


            prog.push_byte(OP_NULL);
            set_var(exp->ch[0]->id); // iterator
            prog.push_byte(OP_POP);


            int loop_start = prog.code.size();
            // condition: loop_name < size of list
            get_var(loop_name);
            get_var(list_name);
            prog.push_byte(OP_ABS);
            prog.push_byte(OP_LESS);

            int j_end = prog.push_jump(OP_JMP_F_POP);

            begin_scope();
            // name := list[loop_name]
            // list, then number
            get_var(list_name);
            get_var(loop_name);
            prog.push_byte(OP_GET_IND);
            set_var(exp->ch[0]->id);
            prog.push_byte(OP_POP);


            compile(exp->ch[2]); // body

            end_scope();

            // loop_name += 1
            get_var(loop_name);
            prog.push_const(Value(1));
            prog.push_byte(OP_ADD);
            set_var(loop_name);
            prog.push_byte(OP_POP);


            prog.push_loop(loop_start); // loop to start    

            prog.patch_jump(j_end);


            
            end_scope();
            
            break;
        }
        case RET:
            compile(exp->ch[0]);
            prog.push_byte(OP_RETURN);
            break;
        case DEF: {
            // this creates a function object.
            Compiler comp(global_index, unordered_map<string, int>(), vector<string>(), vector<int>(1,0));
            comp.inline_functions = inline_functions;
            comp.inline_stack = inline_stack;
            comp.inline_stack.push_back(exp);
            
            // no global variables resolved, so all variables will be local by default (unless in the global table of course)

            // first check if the argument list is all ids, all with different names, and differing from global variables

            // check that the arity is not too large
            if (exp->ch[0]->ch.size() > UINT8_MAX){
                cerr << "Too many arguments" << endl;
                exit(1);
            }
            comp.prog.arity = exp->ch[0]->ch.size();
            for(auto c: exp->ch[0]->ch){
                if (c->type != ID){
                    cerr << "Invalid argument list" << endl;
                    exit(1);
                }
                if (global_index.find(c->id) != global_index.end()){
                    cerr << "Argument shadows global variable" << endl;
                    exit(1);
                }
                if (comp.local_index.find(c->id) != comp.local_index.end()){
                    cerr << "Duplicate argument name" << endl;
                    exit(1);
                }
                comp.local_index[c->id] = comp.local_counts.back()++;
                comp.local_names.push_back(c->id);
            }
            
            // let's compile the program now
      
            comp.compile(exp->ch[1]);

            comp.prog.push_byte(OP_NULL);
            comp.prog.push_byte(OP_RETURN); // return a null value if no return statement
            
            // now add the function to the constant table
            prog.push_const(Value(comp.prog)); // functions are immutables
            break;
        }
        case CAL: {
            if (try_inline_call(exp->ch[0], exp->ch[1])){
                break;
            }
            // compile all the parameters, then the function name itself
            for(auto c: exp->ch[1]->ch){
                compile(c);
            }
            compile(exp->ch[0]);
            // now do the call
            // check that the arity isn't too large
            if (exp->ch[1]->ch.size() > UINT8_MAX){
                cerr << "Too many arguments" << endl;
                exit(1);
            }
            prog.push_byte(OP_CALL);
            prog.push_byte(exp->ch[1]->ch.size()); // the arity

        
            break;
        }
    }
}
