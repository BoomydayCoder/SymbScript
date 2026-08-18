#ifndef VALUE_H
#define VALUE_H

#include "program.hh"

#include <cstdint>
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
    private:
        static constexpr uintptr_t TAG_MASK = 0x3;
        static constexpr uintptr_t TAG_INT = 0x0;
        static constexpr uintptr_t TAG_LIST = 0x1;
        static constexpr uintptr_t TAG_FUNC = 0x2;

        uintptr_t bits = TAG_INT;
        ValueType get_type() const;

    public:
        

        
        

        Value ();
        Value (int v);
        Value (bool v); // constructors
        Value (vector<Value>&& v);
        Value (Program p);

        bool is_int() const;
        bool is_list() const;
        bool is_func() const;

        int get_int() const;
        vector<Value>* get_list() const;
        Program* get_func() const;
        void add_to_int(int amount);


        void print_self(ostream& os);

        

        bool operator==(const Value& v) const; // checks equality
        bool operator<(const Value& v) const; // checks less than
        bool operator>(const Value& v) const; // checks greater than
        explicit operator bool() const; // truthiness


};

#endif
