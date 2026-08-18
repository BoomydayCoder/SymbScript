#include <iostream>
#include "driver.hh"
#include "exptree.hh"
#include "program.hh"
#include "vm.hh"
#include "compiler.hh"
#include <chrono>

void print_help(char* argv[]){
    cout << "Usage: " << argv[0] << " [-p] [-s] [-e] [-c] [-x] [-t] [file]" << endl;
    cout << "Options:" << endl;
    cout << "-p: Trace parsing" << endl;
    cout << "-s: Trace scanning" << endl;
    cout << "-e: Trace expression tree" << endl;
    cout << "-c: Trace compilation" << endl;
    cout << "-x: Trace execution" << endl;
    cout << "-t: Trace time" << endl;
}

int main (int argc, char *argv[]) 
{
    //freopen("input.txt","r",stdin);
    if (argc == 2 && std::string(argv[1]) == "-h"){
        print_help(argv);
        return 0;
    }     
    bool trace_expression = false;
    bool trace_compilation = false;
    bool trace_execution = false; // temporary, replace later
    bool trace_time = false;

    driver drv;
    Ast* exp;
    for (++argv; argv[0]; ++argv){
        if (*argv == std::string ("-p"))
        drv.trace_parsing = true;
        else if (*argv == std::string ("-s"))
        drv.trace_scanning = true;
        else if (*argv == std::string ("-e"))
        trace_expression = true;
        else if (*argv == std::string ("-c"))
        trace_compilation = true;
        else if (*argv == std::string ("-x"))
        trace_execution = true; // flags to enable debug prints
        else if (*argv == std::string ("-t"))
        trace_time = true;
        else {
            bool parsed = drv.parse (*argv);
            if (!parsed) exp = drv.result;
            else return 1;
        }
    }   
    if (exp == nullptr){
        cerr << "No file passed" << endl;
        return 1;
    } // if no file passed, return error
   
    if (trace_expression) exp->print_self(cerr);



    

    
    
    Compiler comp;
    comp.resolve_globals(exp);
    comp.discover_inline_functions(exp);
    comp.compile(exp);
    delete exp; // resolve and compile expression

    if (trace_compilation){
        comp.prog.print_self(cerr); cerr << endl;
    }

    
    
    
    vm.init_program(comp.prog, comp.global_index.size());
    
    
    vm.trace_execution = trace_execution;
    
    
    auto start = chrono::high_resolution_clock::now();
    
    bool success = vm.run();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    if (trace_time){
        cerr << "Execution time: " << duration.count() << " microseconds" << endl;
    }
   
    return success; // run the code
    
    

    
    
    
}
