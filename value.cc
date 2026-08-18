#include "value.hh"
#include "vm.hh"

#include <iostream>
#include <utility>
using namespace std;


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

Value::Value(int v): type(V_INT){
    val.i = v;
}

Value::Value(bool v): type(V_INT){
    val.i = v?1:0;
}


Value::Value(): type(V_INT){
    val.i = 0;
}



Value::Value(Program p): type(V_FUNC){
    val.p = new Program(p);
    vm.progs.push_back(val.p);
}

Value::Value(vector<Value>&& v): type(V_LIST){
    val.l = vm.allocate_list(std::move(v));
}

int Value::get_int() const{
    return val.i;
}

vector<Value>* Value::get_list() const{
    return val.l;
}

Program* Value::get_func() const{
    return val.p;
}



void Value::print_self(ostream& os){
    switch (type){
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
    return type == V_INT;
}

bool Value::is_list() const{
    return type == V_LIST;
}

bool Value::is_func() const{
    return type == V_FUNC;
}

bool Value::operator==(const Value& v) const{
    if (type != v.type){
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
    if (type == V_INT && v.type == V_INT){
        return get_int() < v.get_int();
    }
    // if both lists
    if (type == V_LIST && v.type == V_LIST){
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
    switch(type){
        case V_INT:
            return get_int() != 0; // if the value is not 0, return true
        case V_LIST:
            return get_list()->size() != 0; // if the value is not 0, return true
        case V_FUNC:
            return 1; // functions are always true
    }
    return false;
}
