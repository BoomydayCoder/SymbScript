

#include "exptree.hh"

using namespace std;

// constructors and destructor
Ast::Ast(node_type t, int v){
    type = t;
    num = v;
    id = "";
}

Ast::Ast(node_type t, string* i){
    type = t;
    id = *i;
    num = 0;
}

Ast::Ast(node_type t, vector<Ast*> c){
    type = t;
    ch = move(c);
}

Ast::Ast(node_type t){
    type = t;
    num = 0;
    id = "";
}

Ast::Ast(){
    type = INT;
    num = 0;
    id = "";
}

Ast::~Ast(){
    for(auto c: ch){
        delete c; // make sure to not cause memory leaks
    }
}

void Ast::add(Ast* c){
    ch.push_back(c);
}
// make it stream specified

void Ast::print_self(ostream& os){
    switch(type){ // recursively print the abstract segment tree (for debugging)
        case SEQ:
            for(auto c: ch){
                c->print_self(os);
                os << endl;
            }
            break;
        case EXP:
            ch[0]->print_self(os);
            break;
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case SET:
        case EQ:
        case AND:
        case OR:
        case GT:
        case LT:
        case GE:
        case LE:
        case NE:
        case APP:
            os << "(";
            ch[0]->print_self(os);
            os << " ";
            switch(type){
                case ADD:
                    os << "+";
                    break;
                case SUB:
                    os << "-";
                    break;
                case MUL:
                    os << "*";
                    break;
                case DIV:
                    os << "/";
                    break;
                case SET:
                    os << ":=";
                    break;
                case EQ:
                    os << "==";
                    break;
                case AND:
                    os << "&&";
                    break;
                case OR:
                    os << "||";
                    break;
                case GT:
                    os << ">";
                    break;
                case LT:
                    os << "<";
                    break;
                case GE:
                    os << ">=";
                    break;
                case LE:
                    os << "<=";
                    break;
                case NE:
                    os << "!=";
                    break;
                case APP:
                    os << ":+";
                    break;

            }
            os << " ";
            ch[1]->print_self(os);
            os << ")";
            break;
        case NEG:
            os << "-";
            ch[0]->print_self(os);
            break;
        case INT:
            os << num;
            break;
        case LST:
            os << "[";
            for(int i=0;i<ch.size();++i){
                ch[i]->print_self(os);
                if (i != ch.size()-1){
                    os << ", ";
                }
            }
            os << "]";
            break;
        case ID:
            os << id;
            break;
        case IND:
            ch[0]->print_self(os);
            os << "[";
            ch[1]->print_self(os);
            os << "]";
            break;
        case PRINT:
            os << "print ";
            ch[0]->print_self(os);
            break;
        case BLK:
            os << "{" << endl;
            ch[0]->print_self(os);
            os << "}";
            break;
        case INP:
            os << "input";
            break;
        case NOT:
            os << "!";
            ch[0]->print_self(os);
            break;
        case IF:
            os << "if ";
            ch[0]->print_self(os);
            os << " then ";
            ch[1]->print_self(os);
            os << " else ";
            ch[2]->print_self(os);
            break;
        case WHL:
            os << "while ";
            ch[0]->print_self(os);
            os << " do ";
            ch[1]->print_self(os);
            break;
        case FOR:
            os << "for [";
            ch[0]->print_self(os);
            os << ", ";
            ch[1]->print_self(os);
            os << ", ";
            ch[2]->print_self(os);
            os << "] do ";
            ch[3]->print_self(os);
            break;
        case RNG:
            os << "for [";
            ch[0]->print_self(os);
            os << " in "; 
            ch[1]->print_self(os);
            os << "] do ";
            ch[2]->print_self(os);
            break;
        case CAL:
            ch[0]->print_self(os);
            os << "(";
            for(int i=0;i<ch[1]->ch.size();++i){
                ch[1]->ch[i]->print_self(os);
                if (i != ch[1]->ch.size()-1){
                    os << ", ";
                }
            }
            os << ")";
            break;
        case DEF:
            os << "(";
            for(int i=0;i<ch[0]->ch.size();++i){
                ch[0]->ch[i]->print_self(os);
                if (i != ch[0]->ch.size()-1){
                    os << ", ";
                }
            }
            os << ")";
            os << " => ";
            ch[1]->print_self(os);
            break;
        case RET:
            os << "return ";
            ch[0]->print_self(os);
            break;
        case ABS:
            os << "|";
            ch[0]->print_self(os);
            os << "|";
            break;
        case CLS:
            os << "class ";
            ch[0]->print_self(os);
            break;

        case GET:
            ch[0]->print_self(os);
            os << ".";
            ch[1]->print_self(os);
            break;
        case SGET:
            os << ".";
            ch[0]->print_self(os);
            break;


        
    }
}