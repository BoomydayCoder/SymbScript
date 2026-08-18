#ifndef VALUE_H
#define VALUE_H

#include "program.hh"

#include <variant>
#include <iostream>
#include <vector>
#include <memory>




using namespace std;
class Program;


enum ValueType {
    V_INT,
    V_LIST,
    V_FUNC,
};







class Value {
    public:
        

        
        

        Value ();
        Value (int v);
        Value (bool v); // constructors
        Value (vector<Value> v);
        Value (Program p);

        ValueType type; // the type of the value
        //variant<int, vector<Value>*> val; // the value of the variable - we have switched to union for better performance
        union {
            int i;
            vector<Value>* l;
            Program* p;
        } val;
        
        
        
        


        bool is_int();
        bool is_list();
        bool is_func();

        int get_int() const;
        vector<Value>* get_list() const;
        Program* get_func() const;


        void print_self(ostream& os);

        

        bool operator==(const Value& v) const; // checks equality
        bool operator<(const Value& v) const; // checks less than
        bool operator>(const Value& v) const; // checks greater than
        explicit operator bool() const; // truthiness


};

#endif