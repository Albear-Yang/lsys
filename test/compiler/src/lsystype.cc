// lsystype.cc
#include "lsystype.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iterator>

using namespace std;

// ─── helpers ──────────────────────────────────────────────────────────────────

static bool isCapitalized(const string &s) {
    return !s.empty() && s[0] >= 'A' && s[0] <= 'Z';
}

static vector<string> split_line(const string &line) {
    istringstream iss(line);
    return vector<string>(istream_iterator<string>(iss), istream_iterator<string>());
}

// ─── build_tree ───────────────────────────────────────────────────────────────
// Reads the linearised parse tree produced by the parser (one node per line).
// Terminal lines look like:  KIND lexeme
// Non-terminal lines look like:  lhs child1 child2 ...

shared_ptr<Node> TypeChecker::build_tree(istream &in) {
    string line;
    if (!getline(in, line)) return nullptr;

    auto tokens = split_line(line);
    if (tokens.empty()) return nullptr;

    shared_ptr<Node> node;

    // A terminal node: two tokens, first is ALL_CAPS kind, second is lexeme
    if (tokens.size() == 2 && isCapitalized(tokens[0])) {
        node = make_shared<Node>(tokens[0], tokens[1]);
    } else {
        node = make_shared<Node>(tokens[0]);
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i] == ".EMPTY") continue;
            auto child = build_tree(in);
            if (child) node->children.push_back(child);
        }
    }
    return node;
}

// ─── naming conflict ──────────────────────────────────────────────────────────

void TypeChecker::is_conflicting_naming(string &id) {
    if (global_variables_.count(id))
        throw runtime_error("ERROR 15: '" + id + "' already declared as global variable");
    if (procedure_type_.count(id))
        throw runtime_error("ERROR 15: '" + id + "' already declared as procedure");
    if (symbol_template_.count(id))
        throw runtime_error("ERROR 15: '" + id + "' already declared as symbol");
}

// ─── global declarations ──────────────────────────────────────────────────────
// declaration -> type ID BECOMES NUM SEMI          (children[0..4])
// declaration -> SYMBOL ID LPAREN params RPAREN SEMI  (children[0..5])

void TypeChecker::process_declarations(shared_ptr<Node> decl) {
    if (!decl || decl->value != "declaration") return;

    if (decl->children.size() == 5 && decl->children[0]->value == "type") {
        // float global variable
        string id = decl->children[1]->lexeme;
        is_conflicting_naming(id);
        try {
            float val = stof(decl->children[3]->lexeme);
            global_variables_[id] = val;
            decl->children[1]->type_annotation = "float";
        } catch (...) {
            throw runtime_error("ERROR 16: invalid number '" + decl->children[3]->lexeme + "'");
        }

    } else if (decl->children.size() == 6 && decl->children[0]->value == "SYMBOL") {
        // symbol template declaration
        string sym = decl->children[1]->lexeme;
        is_conflicting_naming(sym);

        set<string> params;
        collect_params(decl->children[3], sym, params);
        symbol_template_[sym] = params;
    }
}

void TypeChecker::find_declarations(shared_ptr<Node> node) {
    if (!node) return;
    if (node->value == "declaration") {
        process_declarations(node);
        return; // don't recurse into a declaration
    }
    for (auto &c : node->children){
        find_declarations(c);  
    } 
}

// ─── collect_params ───────────────────────────────────────────────────────────
// Walks any paramlist / params / dcl subtree and collects ID names.

void TypeChecker::collect_params(shared_ptr<Node> node, string proc, set<string> &out) {
    if (!node) return;
    if (node->value == "dcl" && node->children.size() == 2) {
        // dcl -> type ID
        string name = node->children[1]->lexeme;
        if (out.count(name))
            throw runtime_error("ERROR 14: duplicate parameter '" + name + "'");
        out.insert(name);
        return;
    }
    for (auto &c : node->children) collect_params(c, proc, out);
}

// ─── process_params ───────────────────────────────────────────────────────────
// Builds procedure_type_ signature (all params are float in this grammar).
// paramlist -> dcl
// paramlist -> dcl COMMA paramlist

void TypeChecker::process_params(shared_ptr<Node> paramlist, string proc, int &) {
    if (!paramlist) return;
    if (paramlist->value == "paramlist") {
        // Every dcl in the paramlist contributes one "float" entry.
        // dcl -> type ID, and type -> FLOAT, so always float.
        procedure_type_[proc].push_back("float");
        if (paramlist->children.size() == 3) {
            // dcl COMMA paramlist — recurse on the tail
            process_params(paramlist->children[2], proc, *(reinterpret_cast<int*>(1)));
        }
    }
}

void TypeChecker::find_params(shared_ptr<Node> paramlist, string proc, int &offset) {
    process_params(paramlist, proc, offset);
}

// ─── local declarations ───────────────────────────────────────────────────────
// All local variable types are float (only `type FLOAT` exists).
// tables_[proc] stores the set of declared local variable names.
//
// dcl  -> type ID
// dcls -> .EMPTY
// dcls -> dcls dcl BECOMES NUM SEMI  (children[0..4])

void TypeChecker::processDeclaration(shared_ptr<Node> dcl, string proc) {
    if (!dcl || dcl->value != "dcl" || dcl->children.size() != 2){
        return;
    }
    string name = dcl->children[1]->lexeme;
    if (tables_[proc].count(name))
        throw runtime_error("ERROR 1: '" + name + "' already declared in '" + proc + "'");
    tables_[proc].insert(name);
    dcl->children[1]->type_annotation = "float";
}

void TypeChecker::processDclsBECOME(shared_ptr<Node> dcls, string proc) {
    // dcls -> dcls dcl BECOMES NUM SEMI
    // children: [0]=dcls [1]=dcl [2]=BECOMES [3]=NUM [4]=SEMI
    if (!dcls || dcls->children.size() != 5) return;

    auto dcl  = dcls->children[1];
    auto rhs  = dcls->children[3]; // always NUM in this grammar

    processDeclaration(dcl, proc);

    // RHS must be NUM (float literal); type always matches since dcl type is always float
    if (rhs->value != "NUM")
        throw runtime_error("ERROR 2: expected NUM on RHS of local declaration");
    rhs->type_annotation = "float";

}

void TypeChecker::findDeclarations(shared_ptr<Node> node, string proc) {
    if (!node) return;
    if (node->value == "dcl" && node->children.size() == 2 &&
        node->children[0]->value == "type" && node->children[1]->value == "ID") {
        processDeclaration(node, proc);
        return;
    }
    if (node->value == "dcls" && node->children.size() == 5) {
        processDclsBECOME(node, proc);
        for (auto &c : node->children) findDeclarations(c, proc);
        return;
    }
    for (auto &c : node->children) findDeclarations(c, proc);
}

// ─── expression / term / factor annotation ────────────────────────────────────
// Since the only numeric type is float, every well-typed expression is "float".
// NULL is treated as a null pointer — we'll annotate it "null" and only allow it
// where a float is expected (the grammar doesn't distinguish pointer types here).

void TypeChecker::annotate_factor_tree(shared_ptr<Node> factor, string proc, string ret) {
    if (!factor) return;
    auto &ch = factor->children;

    if (ch.size() == 1) {
        auto &t = ch[0];
        if (t->value == "ID") {
            // Check local scope first, then global variables
            if (tables_[proc].count(t->lexeme)) {
                t->type_annotation = "float";
            } else if (global_variables_.count(t->lexeme)) {
                t->type_annotation = "float";
            } else {
                throw runtime_error("ERROR 5: undeclared variable '" + t->lexeme + "'");
            }
            factor->type_annotation = t->type_annotation;
        } else if (t->value == "NUM") {
            t->type_annotation = "float";
            factor->type_annotation = "float";
        } else if (t->value == "NULL") {
            t->type_annotation = "null";
            factor->type_annotation = "null";
        }

    } else if (ch.size() == 3 && ch[0]->value == "LPAREN") {
        // factor -> LPAREN expr RPAREN
        annotate_expr_tree(ch[1], proc, ret);
        factor->type_annotation = ch[1]->type_annotation;

    } else if (ch.size() == 3 && ch[0]->value == "ID") {
        // factor -> ID LPAREN RPAREN  (zero-argument call)
        string fname = ch[0]->lexeme;
        if (!procedure_type_.count(fname))
            throw runtime_error("ERROR: unknown function '" + fname + "'");
        if (!procedure_type_[fname].empty())
            throw runtime_error("ERROR: '" + fname + "' expects arguments");
        ch[0]->type_annotation = "";
        factor->type_annotation = "float";

    } else if (ch.size() == 4 && ch[0]->value == "ID") {
        // factor -> ID LPAREN arglist RPAREN
        string fname = ch[0]->lexeme;
        if (!procedure_type_.count(fname))
            throw runtime_error("ERROR: unknown function '" + fname + "'");
        auto expected = procedure_type_[fname];
        auto actual   = search_arg(ch[2], proc, ret);
        if (expected.size() != actual.size())
            throw runtime_error("ERROR: argument count mismatch calling '" + fname + "'");
        for (size_t i = 0; i < expected.size(); ++i)
            if (expected[i] != actual[i])
                throw runtime_error("ERROR: argument type mismatch calling '" + fname + "'");
        ch[0]->type_annotation = "";
        factor->type_annotation = "float";
    }
}

void TypeChecker::annotate_term_tree(shared_ptr<Node> term, string proc, string ret) {
    if (!term) return;
    auto &ch = term->children;

    if (ch.size() == 1) {
        // term -> factor
        annotate_factor_tree(ch[0], proc, ret);
        term->type_annotation = ch[0]->type_annotation;

    } else if (ch.size() == 3) {
        // term -> term OP factor
        annotate_term_tree(ch[0], proc, ret);
        annotate_factor_tree(ch[2], proc, ret);
        if (ch[0]->type_annotation != "float" || ch[2]->type_annotation != "float")
            throw runtime_error("ERROR 3: arithmetic operands must be float");
        term->type_annotation = "float";

    } else {
        throw runtime_error("ERROR 4: malformed term node");
    }
}

void TypeChecker::annotate_expr_tree(shared_ptr<Node> expr, string proc, string ret) {
    if (!expr) return;
    auto &ch = expr->children;

    if (ch.size() == 1) {
        // expr -> term
        annotate_term_tree(ch[0], proc, ret);
        expr->type_annotation = ch[0]->type_annotation;

    } else if (ch.size() == 3) {
        // expr -> expr PLUS/MINUS term
        annotate_expr_tree(ch[0], proc, ret);
        annotate_term_tree(ch[2], proc, ret);
        if (ch[0]->type_annotation != "float" || ch[2]->type_annotation != "float")
            throw runtime_error("ERROR: PLUS/MINUS operands must be float");
        expr->type_annotation = "float";
    }
}

// ─── arglist ──────────────────────────────────────────────────────────────────
// arglist -> expr
// arglist -> expr COMMA arglist

vector<string> TypeChecker::search_arg(shared_ptr<Node> arglist, string proc, string ret) {
    vector<string> out;
    if (!arglist) return out;

    if (arglist->children.size() == 1) {
        annotate_expr_tree(arglist->children[0], proc, ret);
        out.push_back(arglist->children[0]->type_annotation);
    } else if (arglist->children.size() == 3) {
        annotate_expr_tree(arglist->children[0], proc, ret);
        out.push_back(arglist->children[0]->type_annotation);
        auto rest = search_arg(arglist->children[2], proc, ret);
        out.insert(out.end(), rest.begin(), rest.end());
    }
    return out;
}

// ─── lvalue annotation ────────────────────────────────────────────────────────
// lvalue -> ID
// lvalue -> LPAREN lvalue RPAREN

void TypeChecker::annotate_lvalue_tree(shared_ptr<Node> lvalue, string proc, string ret) {
    if (!lvalue) return;
    auto &ch = lvalue->children;

    if (ch.size() == 1 && ch[0]->value == "ID") {
        string name = ch[0]->lexeme;
        if (tables_[proc].count(name)) {
            ch[0]->type_annotation = "float";
        } else if (global_variables_.count(name)) {
            ch[0]->type_annotation = "float";
        } else {
            throw runtime_error("ERROR 10: undeclared variable '" + name + "'");
        }
        lvalue->type_annotation = ch[0]->type_annotation;

    } else if (ch.size() == 3 && ch[0]->value == "LPAREN") {
        // lvalue -> LPAREN lvalue RPAREN
        annotate_lvalue_tree(ch[1], proc, ret);
        lvalue->type_annotation = ch[1]->type_annotation;
    }
}

// ─── test ─────────────────────────────────────────────────────────────────────
// test -> expr OP expr

void TypeChecker::check_test(shared_ptr<Node> test, string proc, string ret) {
    if (!test || test->children.size() != 3) return;
    annotate_expr_tree(test->children[0], proc, ret);
    annotate_expr_tree(test->children[2], proc, ret);
    if (test->children[0]->type_annotation != test->children[2]->type_annotation)
        throw runtime_error("ERROR: type mismatch in test condition");
}

// ─── statements ───────────────────────────────────────────────────────────────
// statement -> lvalue BECOMES expr SEMI          (4 children)
// statement -> IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE  (11)
// statement -> WHILE LPAREN test RPAREN LBRACE statements RBRACE  (7)

void TypeChecker::check_statements(shared_ptr<Node> stmt, string proc, string ret) {
    if (!stmt) return;
    auto &ch = stmt->children;

    if (ch.size() == 4 && ch[0]->value == "lvalue") {
        // lvalue BECOMES expr SEMI
        annotate_lvalue_tree(ch[0], proc, ret);
        annotate_expr_tree(ch[2], proc, ret);
        if (ch[0]->type_annotation != ch[2]->type_annotation)
            throw runtime_error("ERROR 16: type mismatch in assignment");

    } else if (ch.size() == 11 && ch[0]->value == "IF") {
        // IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
        check_test(ch[2], proc, ret);
        find_statements(ch[5], proc, ret);
        find_statements(ch[9], proc, ret);

    } else if (ch.size() == 7 && ch[0]->value == "WHILE") {
        // WHILE LPAREN test RPAREN LBRACE statements RBRACE
        check_test(ch[2], proc, ret);
        find_statements(ch[5], proc, ret);
    }
}

// statements -> .EMPTY          (0 children)
// statements -> statements statement  (2 children)

void TypeChecker::find_statements(shared_ptr<Node> stmts, string proc, string ret) {
    if (!stmts || stmts->children.empty()) return;
    if (stmts->children.size() == 2) {
        find_statements(stmts->children[0], proc, ret);
        check_statements(stmts->children[1], proc, ret);
    }
}

// ─── draw_cmd type-checking ───────────────────────────────────────────────────
// draw_cmd -> LINE/TURN_*/TURN_ROLL expr SEMI
// draw_cmd -> FILL LPAREN expr COMMA expr COMMA expr COMMA expr RPAREN LBRACE draw_cmds RBRACE SEMI
// draw_cmd -> LBRACK draw_cmds RBRACK

void TypeChecker::check_draw_cmd(shared_ptr<Node> cmd, string proc) {
    if (!cmd) return;
    auto &ch = cmd->children;
    string tag = ch[0]->value;

    if ((tag == "LINE" || tag == "TURN_PITCH" || tag == "TURN_YAW" || tag == "TURN_ROLL")
        && ch.size() == 3) {
        annotate_expr_tree(ch[1], proc, "float");
        if (ch[1]->type_annotation != "float")
            throw runtime_error("ERROR: draw command expression must be float");

    } else if (tag == "FILL" && ch.size() == 14) {
        // FILL LPAREN e , e , e , e RPAREN LBRACE draw_cmds RBRACE SEMI
        for (int idx : {2, 4, 6, 8}) {
            annotate_expr_tree(ch[idx], proc, "float");
            if (ch[idx]->type_annotation != "float")
                throw runtime_error("ERROR: FILL argument must be float");
        }
        check_draw_cmds(ch[11], proc);

    } else if (tag == "LBRACK" && ch.size() == 3) {
        check_draw_cmds(ch[1], proc);
    }
}

// draw_cmds -> .EMPTY
// draw_cmds -> draw_cmd draw_cmds

void TypeChecker::check_draw_cmds(shared_ptr<Node> cmds, string proc) {
    if (!cmds || cmds->children.empty()) return;
    if (cmds->children.size() == 2) {
        check_draw_cmd(cmds->children[0], proc);
        check_draw_cmds(cmds->children[1], proc);
    }
}

// ─── rules ────────────────────────────────────────────────────────────────────
// rule -> RULE COLON ID LPAREN params RPAREN ARROW symbol_sequence SEMI
//          [0]  [1] [2] [3]   [4]   [5]   [6]      [7]            [8]
// rule -> DRAW_RULE COLON ID LBRACE draw_cmds RBRACE SEMI
//          [0]      [1]  [2] [3]      [4]     [5]   [6]

void TypeChecker::process_rules(shared_ptr<Node> rule) {
    if (!rule) return;
    auto &ch = rule->children;

    if (ch[0]->value == "RULE") {
        string pred = ch[2]->lexeme;
        // Predecessor symbol must be declared
        if (!symbol_template_.count(pred))
            throw runtime_error("ERROR 17: unknown symbol '" + pred + "' in rule");

        // Collect the rule's parameter names and check they are a subset of
        // the symbol's declared template parameters
        set<string> rule_params;
        collect_params(ch[4], pred, rule_params);
        auto &tmpl = symbol_template_[pred];
        if (!includes(tmpl.begin(), tmpl.end(), rule_params.begin(), rule_params.end())){
            throw runtime_error("ERROR 17: rule params not a subset of symbol template for '" + pred + "'");
        }
        // Type-check each symbol in the successor sequence
        check_symbol_sequence(ch[7], pred, rule_params);

    } else if (ch[0]->value == "DRAW_RULE") {
        string sym = ch[2]->lexeme;
        // Draw rules reference a symbol — it must exist
        if (!symbol_template_.count(sym)){
            throw runtime_error("ERROR 17: unknown symbol '" + sym + "' in draw rule");
        }
        // Type-check the draw commands; expressions may reference global variables
        // (draw rules don't have their own local scope in this grammar)
        check_draw_cmds(ch[4], "");
    }
}

// symbol_sequence -> .EMPTY
// symbol_sequence -> symbol symbol_sequence
// symbol -> ID
// symbol -> ID LPAREN arglist RPAREN

void TypeChecker::check_symbol_sequence(shared_ptr<Node> seq, string context_proc,
                                         const set<string> &visible_params) {
    if (!seq || seq->children.empty()) return;
    if (seq->children.size() == 2) {
        check_symbol_instance(seq->children[0], context_proc, visible_params);
        check_symbol_sequence(seq->children[1], context_proc, visible_params);
    }
}

void TypeChecker::check_symbol_instance(shared_ptr<Node> sym, string context_proc,
                                          const set<string> &visible_params) {
    if (!sym) return;
    auto &ch = sym->children;

    string name = ch[0]->lexeme;
    // Symbol must be declared
    if (!symbol_template_.count(name))
        throw runtime_error("ERROR 18: unknown symbol '" + name + "' in successor");

    if (ch.size() == 4) {
        // symbol -> ID LPAREN arglist RPAREN
        // Each argument expression may only reference rule params (and globals).
        // We build a temporary local scope from visible_params (all float).
        // Re-use the existing annotation machinery with a synthetic scope.
        string fake_scope = "__rule_" + context_proc;
        tables_[fake_scope].clear();
        for (auto &p : visible_params) tables_[fake_scope].insert(p);

        auto arg_types = search_arg(ch[2], fake_scope, "float");
        tables_.erase(fake_scope);

        // Argument count must match symbol's declared parameter count
        if (arg_types.size() != symbol_template_[name].size())
            throw runtime_error("ERROR 18: argument count mismatch for symbol '" + name + "'");
    }
}

void TypeChecker::find_rules(shared_ptr<Node> node) {
    if (!node) return;
    if (node->value == "rule") {
        process_rules(node);
        return;
    }
    for (auto &c : node->children) find_rules(c);
}

// ─── procedures ───────────────────────────────────────────────────────────────
// procedure -> FLOAT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
//               [0]  [1] [2]  [3]    [4]    [5]   [6]    [7]       [8]   [9] [10]  [11]

void TypeChecker::process_procedure(shared_ptr<Node> proc) {
    if (!proc || proc->value != "procedure")
        throw runtime_error("ERROR 6: expected procedure node");

    string name = proc->children[1]->lexeme;

    // Duplicate function check
    if (procedure_type_.count(name))
        throw runtime_error("ERROR 14: duplicate procedure '" + name + "'");

    // Register the procedure with an empty signature first (handles recursion)
    procedure_type_[name] = {};
    tables_[name] = {};

    // Build parameter signature
    // params -> .EMPTY | paramlist
    auto params = proc->children[3];
    if (!params->children.empty()) {
        int dummy = 0;
        process_params(params->children[0], name, dummy);
        // Add param names to the local scope
        set<string> param_names;
        collect_params(params->children[0], name, param_names);
        for (auto &p : param_names) tables_[name].insert(p);
    }

    // Process local variable declarations (dcls subtree)
    findDeclarations(proc->children[6], name);

    // Annotate return expression and check it is float
    annotate_expr_tree(proc->children[9], name, "float");
    if (proc->children[9]->type_annotation != "float")
        throw runtime_error("ERROR 14: return expression must be float in '" + name + "'");

    // Type-check statements
    find_statements(proc->children[7], name, "float");
}

void TypeChecker::find_procedures(shared_ptr<Node> node) {
    if (!node) return;
    if (node->value == "procedure") {
        process_procedure(node);
        return; // don't recurse into a procedure's subtree again
    }
    for (auto &c : node->children) find_procedures(c);
}

// ─── public entry point ───────────────────────────────────────────────────────
// Called by the constructor after build_tree() succeeds.

void TypeChecker::type_check() {
    if (!root) throw runtime_error("ERROR: no parse tree to check");

    // Pass 1: global variable and symbol template declarations
    find_declarations(root);

    // Pass 2: procedure signatures and bodies
    find_procedures(root);

    // Pass 3: rewrite rules
    find_rules(root);
}