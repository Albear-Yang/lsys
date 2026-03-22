#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "lsysgen.hpp"
using namespace std;



Lsystem::Lsystem(shared_ptr<Node> root) {
    // search for declearations and add them to globals
    // search for procedures and add them to procedures
    // search for rules and add them to rules and draw_rules
    find_declarations(root);
    find_procedures(root);
    find_rules(root);
}

void Lsystem::find_procedures(shared_ptr<Node> root){
    if (root->value == "procedure"){
        process_procedure(root);
    }
    for (auto &child : root->children)
    {
        find_procedures(child);
    }
}

void Lsystem::find_declarations(shared_ptr<Node> root){
    if (root->value == "declaration"){
        process_declarations(root);
    }
    for (auto &child : root->children)
    {
        find_declarations(child);
    }
}

void Lsystem::find_rules(shared_ptr<Node> root){
    if (root->value == "rule"){
        process_transition_rules(root);
    }
    else if(root->value == "DRAW_RULE"){
        process_draw_rules(root);
    }
    for (auto &child : root->children)
    {
        find_rules(child);
    }
}
// MY BRAIN IS SO FRIED
void Lsystem::process_declarations(shared_ptr<Node> declaration){
    if (!declaration || declaration->value != "declaration" || declaration->children.size() < 2){
        return;
    } // whatever. should not even be necessary

    if (declaration->children.size() == 5 && declaration->children[0]->value == "type"){
        string id = declaration->children[1]->lexeme;
        try {
            float val = stof(declaration->children[3]->lexeme);
            globals[id] = val; // global variable gets added into a map of global variables
        } catch (...) {
            throw runtime_error("ERROR 16: invalid number '" + declaration->children[3]->lexeme + "'");
        }
    }
    else if(declaration->children.size() == 6 && declaration->children[0]->value == "SYMBOL"){
        //thats why we have a symbol template!
        return;
    }
}

void Lsystem::process_procedure(shared_ptr<Node> procedure){
    Procedure proc;
    const string proc_name = procedure->children[1]->lexeme;

    // now we gotta build the procedures FUCK my chud life
    vector<string> params;
    find_params(procedure->children[3], params); //okay should be stored in params
    proc.params = params;
    // all locals should be processed no? aka the decls
    vector<pair<string, Expr>> locals;
    find_decls(procedure->children[6], locals);
    proc.locals = locals;
    // process bomboclart statements
    vector<Statement> stmts;

}
Test Lsystem::compile_test(shared_ptr<Node> test) {
    // Implementation for compiling a test condition
    // A test is just a lambda func that returns a bool
    if(test->children.size() != 3){
        throw runtime_error("ERROR 19: test condition malformed");
    }
    Expr left = compile_expr(test->children[0]);
    Expr right = compile_expr(test->children[2]);
    if(test->children[1]->value == "EQ"){
        return { [left, right](Env& e) { return feq(left(e), right(e)); } };
    }
    else if(test->children[1]->value == "NE"){
        return { [left, right](Env& e) { return fne(left(e), right(e)); } };
    }
    else if(test->children[1]->value == "LT"){
        return { [left, right](Env& e) { return flt(left(e), right(e)); } };
    }
    else if(test->children[1]->value == "GT"){
        return { [left, right](Env& e) { return fgt(left(e), right(e)); } };
    }
    else if(test->children[1]->value == "LE"){
        return { [left, right](Env& e) { return fle(left(e), right(e)); } };
    }
    else if(test->children[1]->value == "GE"){
        return { [left, right](Env& e) { return fge(left(e), right(e)); } };
    }
    return { [](Env& e) { return true; } }; // placeholder
}

Statement Lsystem::compile_statement(shared_ptr<Node> statement) {
    // Implementation for compiling a single statement
    // A statement is just a lambda func that returns nothing
    if (statement->children[0]->value == "lvalue") {
        // assignment
        string var = grab_id(statement->children[0]);
        Expr rhs = compile_expr(statement->children[2]);
        return { [var, rhs](Env& e) { e[var] = rhs(e); } };
    }
    if (statement->children[0]->value == "IF") {
        // if statement
        auto cond  = compile_test(statement->children[2]);
        auto then_ = compile_statements(statement->children[5]);
        auto else_ = compile_statements(statement->children[9]);
        return { [cond, then_, else_](Env& e) {
            if (cond.execute(e)){
                for (auto &s : then_) s.execute(e);
            }
            else{
                for (auto &s : else_) s.execute(e);
            } 
        }};
    }
    if (statement->children[0]->value == "WHILE") {
        // while statement
        auto cond = compile_test(statement->children[2]);
        auto body = compile_statements(statement->children[5]);
        return { [cond, body](Env& e) {
            while (cond.execute(e)){
                for (auto &s : body) s.execute(e);
            }
        }};
    }

}

string grab_id(shared_ptr<Node> lvalue){
    if(lvalue->value != "lvalue"){
        throw runtime_error("ERROR 18: lvalue messed up");
    }
    if(lvalue->children.size() == 1){
        return lvalue->children[0]->lexeme;
    }
    else{
        return grab_id(lvalue->children[1]);
    }
}

vector<Statement> Lsystem::compile_statements(shared_ptr<Node> statements) {
    vector<Statement> stmts;
    if (statements->value == "statement") {
        stmts.push_back(compile_statement(statements));
    } else {
        for (auto &child : statements->children) {
            vector<Statement> child_stmts = find_statements(child);

            // append child_stmts to stmts
            stmts.insert(stmts.end(), child_stmts.begin(), child_stmts.end());
        }
    }
    return stmts;
}

void find_decls(shared_ptr<Node> decls, vector<pair<string, Expr>> &locals) {
    if(decls->value == ".EMPTY"){
        return;
    }
    else if(decls->value == "dcls" && decls->children.size() == 5){
        string dcl_id = decls->children[1]->children[1]->lexeme;
        float dcl_value = stof(decls->children[3]->lexeme);
        Expr dcl_expr = [dcl_value](const Env&) {return dcl_value;};
        locals.push_back({dcl_id, dcl_expr});
        find_decls(decls->children[0], locals);
    }
}

void find_params(shared_ptr<Node> paramlist, vector<string> &params) {
    if (paramlist->value == ".EMPTY"){
        return;
    }
    if (paramlist->value == "dcl"){
        params.push_back(paramlist->children[1]->lexeme);
        return;
    }
    for(auto &child : paramlist->children){
        find_params(child, params);
    }
}

Expr Lsystem::compile_expr(shared_ptr<Node> expr) {
    // In a full implementation, this would recursively compile the expression tree

    if (expr->children.size() == 1) {
        return compile_term(expr->children[0]);
    } else if (expr->children.size() == 3 && expr->children[1]->value == "PLUS") {
        auto lhs = compile_expr(expr->children[0]);
        auto rhs = compile_term(expr->children[2]);
        return [lhs, rhs](const Env& e) { return lhs(e) + rhs(e); };
    }
    else if (expr->children.size() == 3 && expr->children[1]->value == "MINUS") {
        auto lhs = compile_expr(expr->children[0]);
        auto rhs = compile_term(expr->children[2]);
        return [lhs, rhs](const Env& e) { return lhs(e) - rhs(e); };
    }
    return [](const Env& e) { return 0.0f; }; // placeholder
}

Expr Lsystem::compile_term(shared_ptr<Node> term) {
    // Placeholder implementation for compiling a term
    if(term->children.size() == 1){
        return compile_factor(term->children[0]);
    }
    else if(term->children.size() == 3 && term->children[1]->value == "STAR"){
        auto lhs = compile_term(term->children[0]);
        auto rhs = compile_factor(term->children[2]);
        return [lhs, rhs](const Env& e) { return lhs(e) * rhs(e); };
    }
    else if(term->children.size() == 3 && term->children[1]->value == "SLASH"){
        auto lhs = compile_term(term->children[0]);
        auto rhs = compile_factor(term->children[2]);
        return [lhs, rhs](const Env& e) { return lhs(e) / rhs(e); };
    }
    else if(term->children.size() == 3 && term->children[1]->value == "PCT"){
        auto lhs = compile_term(term->children[0]);
        auto rhs = compile_factor(term->children[2]);
        return [lhs, rhs](const Env& e) { return fmod(lhs(e), rhs(e)); };
    }
    return [](const Env& e) { return 0.0f; }; // placeholder
}

Expr compile_factor(shared_ptr<Node> factor) {
    auto &ch = factor->children;
    if (ch[0]->value == "NUM") {
        float val = stof(ch[0]->lexeme);
        return [val](const Env&) { return val; };
    }
    if (ch[0]->value == "ID" && ch.size() == 1) {
        string name = ch[0]->lexeme;
        return [name](const Env& e) { return e.at(name); };
    }
    if (ch[0]->value == "ID" && ch.size() == 4) {
        // function call
        string fname = ch[0]->lexeme;
        vector<Expr> args = compile_arglist(ch[2]);
        return [fname, args](const Env& e) {
            // looked up from the procedure store at call time
            return ProcedureStore::call(fname, args, e);
        };
    }
    // ...
}

// CS241 SLOP BELOW (but i lowk might still need it)

void find_statements(shared_ptr<Node> statements, string procedure,
                     map<string, map<string, string>> &tables,
                     map<string, int> &procedure_type);
string annotate_lvalue_tree(shared_ptr<Node> lvalue, string procedure,
                            map<string, map<string, string>> &tables,
                            map<string, int> &procedure_type);
void annotate_arglist_tree(shared_ptr<Node> arglist, string procedure,
                           map<string, map<string, string>> &tables,
                           map<string, int> &procedure_type);

void process_procedure_params(shared_ptr<Node> paramlist, string proc_name, int &param_offset,
map<string, map<string, string>> &tables, map<string, int> &procedure_type);

int count_arguments(shared_ptr<Node> arglist);
string annotate_lvalue_tree(shared_ptr<Node> lvalue, string procedure,
    map<string, map<string, string>> &tables,
    map<string, int> &procedure_type);

string newLabel();

void annotate_term_tree(shared_ptr<Node> term, string procedure, string return_type,
                        map<string, map<string, string>> &tables,
                        map<string, int> &procedure_type);
void annotate_expr_tree(shared_ptr<Node> expr, string procedure, string return_type,
                        map<string, map<string, string>> &tables,
                        map<string, int> &procedure_type) {
    if (expr->children.size() == 1) {
        shared_ptr<Node> term = expr->children[0];
        annotate_term_tree(term, procedure, return_type, tables, procedure_type);
        expr->type_annotation = term->type_annotation;
    } else if (expr->children.size() == 3 && expr->children[1]->value == "PLUS") {
        shared_ptr<Node> other_expr = expr->children[0];
        shared_ptr<Node> term = expr->children[2];
        if(other_expr->type_annotation == "int" && term->type_annotation == "int"){
            annotate_expr_tree(other_expr, procedure, return_type, tables, procedure_type);
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
            annotate_term_tree(term, procedure, return_type, tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n";
            cout << "add $3, $5, $3\n";
        }
        else if(other_expr->type_annotation == "int*"){
            annotate_expr_tree(other_expr, procedure, return_type, tables, procedure_type);
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
            annotate_term_tree(term, procedure, return_type, tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n";
            cout << "mult $3, $4 \n";
            cout << "mflo $3 \n";
            cout << "add $3, $5, $3\n";
        }
        else{
            annotate_expr_tree(other_expr, procedure, return_type, tables, procedure_type);
            cout << "mult $3, $4 \n";
            cout << "mflo $3 \n";
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
            annotate_term_tree(term, procedure, return_type, tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n";
            cout << "add $3, $5, $3\n";
        }

    
    } else if (expr->children.size() == 3 && expr->children[1]->value == "MINUS") {
        shared_ptr<Node> other_expr = expr->children[0];
        shared_ptr<Node> term = expr->children[2];
        if((other_expr->type_annotation == "int" && term->type_annotation == "int")){
            annotate_expr_tree(other_expr, procedure, return_type, tables, procedure_type);
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
            annotate_term_tree(term, procedure, return_type, tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n";
            cout << "sub $3, $5, $3\n";
        }
        else if((other_expr->type_annotation == "int*" && term->type_annotation == "int*")){
            annotate_expr_tree(other_expr, procedure, return_type, tables, procedure_type);
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
            annotate_term_tree(term, procedure, return_type, tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n";
            cout << "sub $3, $5, $3\n";
            cout << "div $3, $4 \n";
            cout << "mflo $3\n";
        }
        else if(other_expr->type_annotation == "int*"){
            annotate_expr_tree(other_expr, procedure, return_type, tables, procedure_type);
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
            annotate_term_tree(term, procedure, return_type, tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n";
            cout << "mult $3, $4 \n";
            cout << "mflo $3 \n";
            cout << "sub $3, $5, $3\n";
        }
        else{
            annotate_expr_tree(other_expr, procedure, return_type, tables, procedure_type);
            cout << "mult $3, $4 \n";
            cout << "mflo $3 \n";
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
            annotate_term_tree(term, procedure, return_type, tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n";
            cout << "sub $3, $5, $3\n";
        }
    }
}

void annotate_factor_tree(shared_ptr<Node> factor, string procedure, string return_type,
                          map<string, map<string, string>> &tables,
                          map<string, int> &procedure_type) {
    if (factor->children.size() == 1) {
        shared_ptr<Node> terminal = factor->children[0];
        if (terminal->value == "ID") {
            string offset = tables[procedure][terminal->lexeme];
            //cout << offset << "yo help me" << endl;
            cout << "lw $3, " << offset << "($29)\n";
            return;
        } else if (terminal->value == "NUM") {
            string word_value = terminal->lexeme;
            cout << "lis $3\n";
            cout << ".word " << word_value << "\n";
            return;
        } else if (terminal->value == "NULL") {
            cout << "lis $3\n";
            cout << ".word 1" << "\n";
            return;
        }
    } else if (factor->children.size() == 3 && factor->children[0]->value == "LPAREN") {
        annotate_expr_tree(factor->children[1], procedure, return_type, tables, procedure_type);
        return;
    } else if (factor->children.size() == 3 && factor->children[0]->value == "GETCHAR") {
        cout << "lis $5\n";
        cout << ".word 0xffff0004\n";
        cout << "lw $3, 0($5)\n";
        return;
    } else if (factor->children.size() == 3 &&
               factor->children[0]->value == "ID" &&
               factor->children[1]->value == "LPAREN" &&
               factor->children[2]->value == "RPAREN") {
        string procName = factor->children[0]->lexeme;
        cout << "sw $29, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "sw $31, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "lis $5\n.word " << "proc"<<procName << "\n";
        cout << "jalr $5\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $31, -4($30)\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $29, -4($30)\n";
        return;
    } else if (factor->children.size() == 4 &&
               factor->children[0]->value == "ID" &&
               factor->children[1]->value == "LPAREN" &&
               factor->children[3]->value == "RPAREN") {
        string procName = factor->children[0]->lexeme;
        auto arglist = factor->children[2];
        cout << "sw $31, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "sw $29, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        annotate_arglist_tree(arglist, procedure, tables, procedure_type);
        cout << "lis $5\n.word " << "proc"<< procName << "\n";
        cout << "jalr $5\n";
        int argCount = count_arguments(arglist);
        for (int i = 0; i < argCount; i++) {
            cout << "add $30, $30, $4\n";
        }
        cout << "add $30, $30, $4\n";
        cout << "lw $29, -4($30)\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $31, -4($30)\n";
        return;
    }
    else if(factor->children.size() == 2 && factor->children[0]->value == "STAR"){
        shared_ptr<Node> factor_node = factor->children[1];
        annotate_factor_tree(factor_node, procedure, return_type, tables, procedure_type);
        cout << "lw $3, 0($3) \n";
    }
    else if(factor->children.size() == 2 && factor->children[0]->value == "AMP"){
        
        shared_ptr<Node> factor_node = factor->children[1];
        string id = annotate_lvalue_tree(factor_node, procedure, tables, procedure_type);

        if(factor_node->children[0]->value == "STAR"){
            annotate_factor_tree(factor_node->children[1], procedure, return_type, tables, procedure_type);
        }
        else{
            cout << "lis $3 \n";
            cout << ".word " << tables[procedure][id] << endl;
            cout << "add $3, $29, $3 \n";
        }
        //COMEBACK

    }
    else if(factor->children.size() == 5 && factor->children[0]->value == "NEW"){
        shared_ptr<Node> expr = factor->children[3];
        annotate_expr_tree(expr, procedure, "int", tables, procedure_type);

        cout << "sw $1, -4($30)\n";
        cout << "sub $30, $30, $4\n";

        cout << "add $1, $3, $0\n";

        cout << "sw $31, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "lis $5\n";
        cout << ".word new\n";
        cout << "jalr $5\n";
        cout << "bne $3, $0, 1\n";
        cout << "add $3, $11, $0\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $31, -4($30)\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $1, -4($30)\n";

    }

}

void annotate_term_tree(shared_ptr<Node> term, string procedure, string return_type,
                        map<string, map<string, string>> &tables,
                        map<string, int> &procedure_type) {
    if (term->children.size() == 1) {
        annotate_factor_tree(term->children[0], procedure, return_type, tables, procedure_type);
        term->type_annotation = term->children[0]->type_annotation;
    } else if (term->children.size() == 3 && term->children[1]->value == "STAR") {
        annotate_term_tree(term->children[0], procedure, return_type, tables, procedure_type);
        cout << "sw $3, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        annotate_factor_tree(term->children[2], procedure, return_type, tables, procedure_type);
        cout << "add $30, $30, $4\n";
        cout << "lw $5, -4($30)\n";
        cout << "mult $5, $3\n";
        cout << "mflo $3\n";
    } else if (term->children.size() == 3 && term->children[1]->value == "SLASH") {
        annotate_term_tree(term->children[0], procedure, return_type, tables, procedure_type);
        cout << "sw $3, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        annotate_factor_tree(term->children[2], procedure, return_type, tables, procedure_type);
        cout << "add $30, $30, $4\n";
        cout << "lw $5, -4($30)\n";
        cout << "div $5, $3\n";
        cout << "mflo $3\n";
    } else if (term->children.size() == 3 && term->children[1]->value == "PCT") {
        annotate_term_tree(term->children[0], procedure, return_type, tables, procedure_type);
        cout << "sw $3, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        annotate_factor_tree(term->children[2], procedure, return_type, tables, procedure_type);
        cout << "add $30, $30, $4\n";
        cout << "lw $5, -4($30)\n";
        cout << "div $5, $3\n";
        cout << "mfhi $3\n";
    }
}

// Updated process_paramlist with checks for null and ".EMPTY"
void process_paramlist(shared_ptr<Node> paramlist, string proc_name, int &param_offset,
                       map<string, map<string, string>> &tables, map<string, int> &procedure_type) {
    if (!paramlist || paramlist->value == ".EMPTY"){
        return;
    }
    if (paramlist->children.size() == 1) {
        process_procedure_params(paramlist->children[0], proc_name, param_offset, tables, procedure_type);
    }
}

void process_procedure_params(shared_ptr<Node> paramlist, string proc_name, int &param_offset,
    map<string, map<string, string>> &tables, map<string, int> &procedure_type) {
        if (paramlist->children.size() == 1) {
            string param_name = paramlist->children[0]->children[1]->lexeme;
            tables[proc_name][param_name] = to_string(param_offset);
    
        } else if (paramlist->children.size() == 3) {
            process_procedure_params(paramlist->children[2], proc_name, param_offset, tables, procedure_type);
            param_offset += 4;

            string param_name = paramlist->children[0]->children[1]->lexeme;
            tables[proc_name][param_name] = to_string(param_offset);
        }
}

void processDclsBECOME(shared_ptr<Node> dcls, string procedure,
                       map<string, map<string, string>> &tables, map<string, int> &procedure_type) {
    if (dcls->value == "dcls" && dcls->children.size() == 5) {
        if(dcls->children[3]->value == "NUM"){
            shared_ptr<Node> dcl_node = dcls->children[1];
            string dcl_id = dcl_node->children[1]->lexeme;
            string num = dcls->children[3]->lexeme;
            int map_size = tables[procedure].size();
            string offset = to_string((map_size - procedure_type[procedure]) * -4);
            tables[procedure][dcl_id] = offset;
            cout << "lis $3\n";
            cout << ".word " << num << "\n";
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
        }
        else if(dcls->children[3]->value == "NULL"){
            shared_ptr<Node> dcl_node = dcls->children[1];
            string dcl_id = dcl_node->children[1]->lexeme;
            string num = dcls->children[3]->lexeme;
            int map_size = tables[procedure].size();
            string offset = to_string((map_size - procedure_type[procedure]) * -4);
            tables[procedure][dcl_id] = offset;
            cout << "lis $3\n";
            cout << ".word 1" << "\n";
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n";
        }
    }
}

string findDeclarations(shared_ptr<Node> tree, string procedure,
                        map<string, map<string, string>> &tables, map<string, int> &procedure_type) {
    string epilogue = "";
    if (tree->value == "dcls" && tree->children.size() == 5) {
        processDclsBECOME(tree, procedure, tables, procedure_type);
        epilogue += "add $30, $30, $4\n";
        for (shared_ptr<Node> child : tree->children){
            epilogue += findDeclarations(child, procedure, tables, procedure_type);
        }
    } else {
        for (shared_ptr<Node> child : tree->children){
            epilogue += findDeclarations(child, procedure, tables, procedure_type);
        }
    }
    return epilogue;
}

int label_counter = 0;
string newLabel() {
    return "label" + to_string(label_counter++);
}

string process_parameters(shared_ptr<Node> procedure,
                          map<string, map<string, string>> &tables,
                          map<string, int> &procedure_type) {
    if (procedure->value == "main") {
        shared_ptr<Node> reg1 = procedure->children[3];
        shared_ptr<Node> reg2 = procedure->children[5];
        tables["wain"][reg1->children[1]->lexeme] = "8";
        tables["wain"][reg2->children[1]->lexeme] = "4";
        procedure_type["wain"] = 2;
        // cout << "sw $1, -4($30)\n";
        // cout << "sub $30, $30, $4\n";
        // cout << "sw $2, -4($30)\n";
        // cout << "sub $30, $30, $4\n";
        // cout << "sub $29, $30, $4\n";
        string epilogue;
        epilogue += "add $30, $30, $4\n";
        epilogue += "add $30, $30, $4\n";
        return epilogue;
    }
    else{

    }
    return "";
}

void annotate_test_tree(shared_ptr<Node> test, string falseLabel, string procedure,
                          map<string, map<string, string>> &tables,
                          map<string, int> &procedure_type) {
    annotate_expr_tree(test->children[0], procedure, "int", tables, procedure_type);
    if (test->children[0]->type_annotation == "int*"){
        cout << "sw $3, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        annotate_expr_tree(test->children[2], procedure, "int", tables, procedure_type);
        cout << "add $30, $30, $4\n";
        cout << "lw $5, -4($30)\n";
        string comp = test->children[1]->value;
        if (comp == "EQ")
            cout << "bne $5, $3, " << falseLabel << "\n";
        else if (comp == "NE")
            cout << "beq $5, $3, " << falseLabel << "\n";
        else if (comp == "LT") {
            cout << "sltu $8, $5, $3\n";
            cout << "beq $8, $0, " << falseLabel << "\n";
        } else if (comp == "LE") {
            cout << "sltu $8, $3, $5\n";
            cout << "bne $8, $0, " << falseLabel << "\n";
        } else if (comp == "GT") {
            cout << "sltu $8, $3, $5\n";
            cout << "beq $8, $0, " << falseLabel << "\n";
        } else if (comp == "GE") {
            cout << "sltu $8, $5, $3\n";
            cout << "bne $8, $0, " << falseLabel << "\n";
        }
    }
    else{
        cout << "sw $3, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        annotate_expr_tree(test->children[2], procedure, "int", tables, procedure_type);
        cout << "add $30, $30, $4\n";
        cout << "lw $5, -4($30)\n";
        string comp = test->children[1]->value;
        if (comp == "EQ")
            cout << "bne $5, $3, " << falseLabel << "\n";
        else if (comp == "NE")
            cout << "beq $5, $3, " << falseLabel << "\n";
        else if (comp == "LT") {
            cout << "slt $8, $5, $3\n";
            cout << "beq $8, $0, " << falseLabel << "\n";
        } else if (comp == "LE") {
            cout << "slt $8, $3, $5\n";
            cout << "bne $8, $0, " << falseLabel << "\n";
        } else if (comp == "GT") {
            cout << "slt $8, $3, $5\n";
            cout << "beq $8, $0, " << falseLabel << "\n";
        } else if (comp == "GE") {
            cout << "slt $8, $5, $3\n";
            cout << "bne $8, $0, " << falseLabel << "\n";
        }
    }
}

void check_statements(shared_ptr<Node> statement, string procedure,
                      map<string, map<string, string>> &tables,
                      map<string, int> &procedure_type) {
    if (statement->children.size() == 4 && statement->children[0]->value == "lvalue") {
        shared_ptr<Node> lvalue = statement->children[0];
        if(lvalue->children[0]->value == "STAR"){
            shared_ptr<Node> factor_node = lvalue->children[1];
            annotate_factor_tree(factor_node, procedure, "int", tables, procedure_type); 
            cout << "sw $3, -4($30)\n";
            cout << "sub $30, $30, $4\n"; //save the address?
            shared_ptr<Node> expr = statement->children[2];
            annotate_expr_tree(expr, procedure, "int", tables, procedure_type);
            cout << "add $30, $30, $4\n";
            cout << "lw $5, -4($30)\n"; // take the address?
            cout << "sw $3, 0($5)\n";

        }
        else{
            string lvalue_id = annotate_lvalue_tree(lvalue, procedure, tables, procedure_type);
            shared_ptr<Node> expr = statement->children[2];
            annotate_expr_tree(expr, procedure, "int", tables, procedure_type);
            cout << "sw $3, " << tables[procedure][lvalue_id] << "($29)\n";
        }
    } else if (statement->children.size() >= 11 && statement->children[0]->value == "IF") {
        string else_label = newLabel();
        string end_label = newLabel();
        annotate_test_tree(statement->children[2], else_label, procedure, tables, procedure_type);
        find_statements(statement->children[5], procedure, tables, procedure_type);
        cout << "beq $0, $0, " << end_label << "\n";
        cout << else_label << ":\n";
        find_statements(statement->children[9], procedure, tables, procedure_type);
        cout << end_label << ":\n";
    } else if (statement->children.size() >= 7 && statement->children[0]->value == "WHILE") {
        string start_label = newLabel();
        string end_label = newLabel();
        cout << start_label << ":\n";
        annotate_test_tree(statement->children[2], end_label, procedure, tables, procedure_type);
        find_statements(statement->children[5], procedure, tables, procedure_type);
        cout << "beq $0, $0, " << start_label << "\n";
        cout << end_label << ":\n";
    } else if (statement->children.size() >= 5 && statement->children[0]->value == "PUTCHAR") {
        shared_ptr<Node> expr = statement->children[2];
        annotate_expr_tree(expr, procedure, "int", tables, procedure_type);
        cout << "lis $5\n";
        cout << ".word 0xffff000c\n";
        cout << "sw $3, 0($5)\n";
    } else if (statement->children.size() >= 5 && statement->children[0]->value == "PRINTLN") {
        cout << "sw $1, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        shared_ptr<Node> expr = statement->children[2];
        annotate_expr_tree(expr, procedure, "int", tables, procedure_type);
        cout << "add $1, $3, $0\n";
        cout << "sw $31, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "lis $5\n";
        cout << ".word print\n";
        cout << "jalr $5\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $31, -4($30)\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $1, -4($30)\n";
    }
    else if(statement->children.size() >= 5 && statement->children[0]->value == "DELETE"){
        shared_ptr<Node> expr = statement->children[3];
        annotate_expr_tree(expr, procedure, "int", tables, procedure_type);
        string skipDelete = newLabel();
        cout << "beq $3, $11, " << skipDelete << "\n";
        cout << "sw $1, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "add $1, $3, $0\n";
        cout << "sw $31, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "lis $5\n";
        cout << ".word delete\n";
        cout << "jalr $5\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $31, -4($30)\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $1, -4($30)\n";
        cout << skipDelete << ":\n";
    }
}

void print_node(shared_ptr<Node> root) {
    cout << root->value;
    if (!root->lexeme.empty())
        cout << " " << root->lexeme << root->type_annotation << "\n";
    else if (!root->children.empty()) {
        for (auto child : root->children){
            cout << " " << child->value;
        }
        cout << root->type_annotation << "\n";
        for (auto child : root->children){
            print_node(child);
        }
    } else{
        cout << " .EMPTY\n";
    }   
}

void find_statements(shared_ptr<Node> statements, string procedure,
                     map<string, map<string, string>> &tables,
                     map<string, int> &procedure_type) {
    if (statements->children.empty())
        return;
    else if (statements->children.size() == 2) {
        find_statements(statements->children[0], procedure, tables, procedure_type);
        check_statements(statements->children[1], procedure, tables, procedure_type);
    }
}

void annotate_arglist_tree(shared_ptr<Node> arglist, string procedure,
                           map<string, map<string, string>> &tables,
                           map<string, int> &procedure_type) {
    annotate_expr_tree(arglist->children[0], procedure, "int", tables, procedure_type);
    cout << "sw $3, -4($30)\n";
    cout << "sub $30, $30, $4\n";
    if (arglist->children.size() == 3){
        annotate_arglist_tree(arglist->children[2], procedure, tables, procedure_type);
    }
}

int count_arguments(shared_ptr<Node> arglist) {
    if (arglist->children.size() == 1)
        return 1;
    else if (arglist->children.size() == 3)
        return 1 + count_arguments(arglist->children[2]);
    return 0;
}

void process_procedure(shared_ptr<Node> procedure, map<string, map<string, string>> &tables,
                       map<string, int> &procedure_type) {
    string proc_name;
    if (procedure->value == "main") {
        proc_name = "wain";
        cout << "wain:" << "\n";
        cout << "lis $4\n.word 4\n";
        cout << "lis $11\n.word 1\n";
        cout << "lis $5\n.word init\n";

        

        cout << "sw $1, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "sw $2, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "sub $29, $30, $4\n";

        if(procedure->children[3]->children[1]->type_annotation != "int*"){
            cout << "add $2, $0, $0\n";    
        }
        else{
            cout << "lw $2, 4($29)\n";
        }
        cout << "sw $31, -4($30)\n";
        cout << "sub $30, $30, $4\n";
        cout << "jalr $5\n";
        cout << "add $30, $30, $4\n";
        cout << "lw $31, -4($30)\n";
        
        string epilogue = process_parameters(procedure, tables, procedure_type);
        shared_ptr<Node> expr = procedure->children[11];
        epilogue += findDeclarations(procedure->children[8], proc_name, tables, procedure_type);
        find_statements(procedure->children[9], proc_name, tables, procedure_type);
        annotate_expr_tree(expr, "wain", "int", tables, procedure_type);
        cout << epilogue;
        cout << "jr $31\n";
    } else if (procedure->value == "procedure") {
        proc_name = procedure->children[1]->lexeme;

        //cout << "help me BRO PLEASE THIS IS SO STUPID" << endl;
        tables[proc_name] = map<string, string>();
        //cout << "help me BRO PLEASE THIS IS SO STUPID" << endl;
        int offset = 4;
        //cout << "help me BRO PLEASE THIS IS SO STUPID" << endl;
        process_paramlist(procedure->children[3], proc_name, offset, tables, procedure_type);
        //cout << "help me BRO PLEASE THIS IS SO STUPID" << endl;
        procedure_type[proc_name] = tables[proc_name].size();
        cout << "proc"<< proc_name << ":\n";
        cout << "sub $29, $30, $4\n";
        string epilogue = findDeclarations(procedure->children[6], proc_name, tables, procedure_type);
        find_statements(procedure->children[7], proc_name, tables, procedure_type);
        annotate_expr_tree(procedure->children[9], proc_name, "int", tables, procedure_type);
        cout << epilogue;
        cout << "jr $31\n";
    }
}


string  annotate_lvalue_tree(shared_ptr<Node> lvalue, string procedure,
                            map<string, map<string, string>> &tables,
                            map<string, int> &procedure_type) {
    if (lvalue->children.size() == 1) {
        shared_ptr<Node> id = lvalue->children[0];
        return id->lexeme;
    } else if (lvalue->children.size() == 3) {
        shared_ptr<Node> factor = lvalue->children[1];
        return annotate_lvalue_tree(factor, procedure, tables, procedure_type);
    }
    else if(lvalue->children.size() == 2 && lvalue->children[0]->value == "STAR"){
        shared_ptr<Node> factor_node = lvalue->children[1];
        annotate_factor_tree(factor_node, procedure, "int", tables, procedure_type);
        cout << "lw $3, 0($3) \n";
    }
    return "";
}

// Wrapper function for external integration
void generate_code(shared_ptr<Node> root, ostream &out) {
    map<string, map<string, string>> tables;
    map<string, int> procedure_type;
    
    if (!root) {
        cerr << "Error: empty parse tree input.\n";
        throw runtime_error("Empty AST for code generation");
    }
    
    // Redirect stdout to provided output stream
    streambuf* old = cout.rdbuf(out.rdbuf());
    
    try {
        cout << ".import print\n";
        cout << ".import init\n";
        cout << ".import new\n";
        cout << ".import delete\n";
        
        auto procs = find_procedures(root, tables, procedure_type);
        for (auto proc : procs){
            if(proc){
                process_procedure(proc, tables, procedure_type);
            }
        }
    } catch (...) {
        cout.rdbuf(old);
        throw;
    }
    
    cout.rdbuf(old);
}

int main() {
    map<string, map<string, string>> tables;
    map<string, int> procedure_type;
    shared_ptr<Node> root = build_tree(cin);
    //print_node(root);
    if (!root) {
        cerr << "Error: empty parse tree input.\n";
        return 1;
    }
    cout << ".import print\n";
    cout << ".import init\n";
    cout << ".import new\n";
    cout << ".import delete\n";
    auto procs = find_procedures(root, tables, procedure_type);
    for (auto proc : procs){
        if(proc){
            process_procedure(proc, tables, procedure_type);
        }
    }
    return 0;
}

constexpr float EPS = 1e-6f;

bool feq(float a, float b) {
    return fabs(a - b) < EPS;
}

bool fne(float a, float b) {
    return fabs(a - b) >= EPS;
}

bool fgt(float a, float b) {
    return a > b + EPS;
}

bool fge(float a, float b) {
    return a > b - EPS;
}

bool flt(float a, float b) {
    return a < b - EPS;
}

bool fle(float a, float b) {
    return a < b + EPS;
}