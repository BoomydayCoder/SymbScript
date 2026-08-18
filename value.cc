#include "value.hh"
#include "vm.hh"

#include <iostream>
#include <utility>
using namespace std;

static_assert(sizeof(Value) == sizeof(uintptr_t), "Value must remain one machine word");


/*

Value::Value(int v): val(v), type(V_INT){}

Value::Value(bool v): val(v?1:0), type(V_INT){}

Value::Value(): val(0), type(V_INT){}
Value::Value(vector<Value> v): val(make_shared<vector<Value>>(v)), type(V_LIST){}

int Value::get_int() const{
    return get<int>(val);
}

Value::vector<Value>* Value::get_list() const{
    return get<vector<Value>*>(val);
}*/ // using variants, deprecated

Value::Value(int v){
    bits = static_cast<uintptr_t>(static_cast<uint32_t>(v)) << 2;
}

Value::Value(bool v): Value(static_cast<int>(v)){
}


Value::Value(){
    bits = TAG_INT;
}



Value::Value(Program p){
    Program* program = new Program(p);
    vm.progs.push_back(program);
    bits = reinterpret_cast<uintptr_t>(program) | TAG_FUNC;
}

Value::Value(vector<Value>&& v){
    vector<Value>* list = vm.allocate_list(std::move(v));
    bits = reinterpret_cast<uintptr_t>(list) | TAG_LIST;
}

int Value::get_int() const{
    return static_cast<int32_t>(bits >> 2);
}

vector<Value>* Value::get_list() const{
    return reinterpret_cast<vector<Value>*>(bits & ~TAG_MASK);
}

Program* Value::get_func() const{
    return reinterpret_cast<Program*>(bits & ~TAG_MASK);
}

void Value::add_to_int(int amount){
    *this = Value(get_int()+amount);
}

ValueType Value::get_type() const{
    switch (bits & TAG_MASK){
        case TAG_LIST: return V_LIST;
        case TAG_FUNC: return V_FUNC;
        default: return V_INT;
    }
}



void Value::print_self(ostream& os){
    switch (get_type()){
        case V_INT:
            os << get_int();
            break;
        case V_LIST:
            // print it out like a comma separated list
            os << "[";
            for(int i=0;i<get_list()->size();++i){
                get_list()->at(i).print_self(os);
                if (i != get_list()->size()-1){
                    os << ", ";
                }
            }
            os << "]";
            break;
        case V_FUNC:
            os << "function: {" << endl;
            get_func()->print_self(os);
            cout << "}" << endl;
            break;
    }
}

bool Value::is_int() const{
    return (bits & TAG_MASK) == TAG_INT;
}

bool Value::is_list() const{
    return (bits & TAG_MASK) == TAG_LIST;
}

bool Value::is_func() const{
    return (bits & TAG_MASK) == TAG_FUNC;
}

bool Value::operator==(const Value& v) const{
    const ValueType type = get_type();
    if (type != v.get_type()){
        return false; // if different types, return false
    }
    switch(type){
        case V_INT:
            return get_int() == v.get_int(); // if the same type, compare the values
        case V_LIST:
            return *get_list() == *v.get_list(); // if the same type, compare the values
        case V_FUNC:
            return get_func() == v.get_func(); // simply compare that they point to the same function
    }
    return false;
}

bool Value::operator<(const Value& v) const{
    // if both numbers
    if (is_int() && v.is_int()){
        return get_int() < v.get_int();
    }
    // if both lists
    if (is_list() && v.is_list()){
        return *get_list() < *v.get_list();
    }
    // return error
    cerr << "Invalid comparison" << endl;
    exit(1);

}

bool Value::operator>(const Value& v) const{
    return v < *this;
}

Value::operator bool() const{
    switch(get_type()){
        case V_INT:
            return get_int() != 0; // if the value is not 0, return true
        case V_LIST:
            return get_list()->size() != 0; // if the value is not 0, return true
        case V_FUNC:
            return 1; // functions are always true
    }
    return false;
}
