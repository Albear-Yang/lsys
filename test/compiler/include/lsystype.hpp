#ifndef LSYSTYPE_HPP
#define LSYSTYPE_HPP

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <sstream>
#include "lsysparse.hpp"

using namespace std;

class TypeChecker {
private:
    // per-procedure local variable names (all float; type stored implicitly)
    map<string, set<string>>    tables_;
    // procedure name -> ordered list of param types (always "float")
    map<string, vector<string>> procedure_type_;
    // global float variables: name -> initial value
    map<string, float>          global_variables_;
    // symbol templates: symbol name -> set of declared parameter names
    map<string, set<string>>    symbol_template_;

    shared_ptr<Node> root;

    // ── tree builder ──────────────────────────────────────────────────────────
    shared_ptr<Node> build_tree(istream &input);

    // ── naming ────────────────────────────────────────────────────────────────
    void is_conflicting_naming(string &id);

    // ── global declarations ───────────────────────────────────────────────────
    void find_declarations(shared_ptr<Node> node);
    void process_declarations(shared_ptr<Node> decl);

    // ── param handling ────────────────────────────────────────────────────────
    void collect_params(shared_ptr<Node> node, string proc, set<string> &out);
    void find_params(shared_ptr<Node> paramlist, string proc, int &offset);
    void process_params(shared_ptr<Node> paramlist, string proc, int &offset);

    // ── local declarations ────────────────────────────────────────────────────
    void processDeclaration(shared_ptr<Node> dcl, string proc);
    void processDclsBECOME(shared_ptr<Node> dcls, string proc);
    void findDeclarations(shared_ptr<Node> node, string proc);

    // ── expression annotation ─────────────────────────────────────────────────
    void annotate_factor_tree(shared_ptr<Node> factor, string proc, string ret);
    void annotate_term_tree(shared_ptr<Node> term,   string proc, string ret);
    void annotate_expr_tree(shared_ptr<Node> expr,   string proc, string ret);
    vector<string> search_arg(shared_ptr<Node> arglist, string proc, string ret);

    // ── lvalue annotation ─────────────────────────────────────────────────────
    void annotate_lvalue_tree(shared_ptr<Node> lvalue, string proc, string ret);

    // ── statements ────────────────────────────────────────────────────────────
    void check_test(shared_ptr<Node> test, string proc, string ret);
    void check_statements(shared_ptr<Node> stmt, string proc, string ret);
    void find_statements(shared_ptr<Node> stmts, string proc, string ret);

    // ── draw commands ─────────────────────────────────────────────────────────
    void check_draw_cmd(shared_ptr<Node> cmd, string proc);
    void check_draw_cmds(shared_ptr<Node> cmds, string proc);

    // ── rules ─────────────────────────────────────────────────────────────────
    void find_rules(shared_ptr<Node> node);
    void process_rules(shared_ptr<Node> rule);
    void check_symbol_sequence(shared_ptr<Node> seq, string ctx_proc,
                                const set<string> &visible_params);
    void check_symbol_instance(shared_ptr<Node> sym, string ctx_proc,
                                const set<string> &visible_params);

    // ── procedures ────────────────────────────────────────────────────────────
    void find_procedures(shared_ptr<Node> node);
    void process_procedure(shared_ptr<Node> proc);

public:
    // Constructs from the linearised parse-tree string produced by Parser.
    TypeChecker(const string &parse_tree_str) {
        try {
            istringstream iss(parse_tree_str);
            root = build_tree(iss);
        } catch (const runtime_error &e) {
            cerr << e.what() << "\n";
            root.reset();
        }
    }

    // Constructs from an already-built Node tree.
    TypeChecker(shared_ptr<Node> tree) : root(tree) {}

    // Run all type-checking passes. Throws runtime_error on any violation.
    void type_check();

    // Accessors for downstream passes (code generation, interpreter, etc.)
    const map<string, set<string>>    &tables()          const { return tables_; }
    const map<string, vector<string>> &procedure_types() const { return procedure_type_; }
    const map<string, float>          &globals()         const { return global_variables_; }
    const map<string, set<string>>    &symbol_templates()const { return symbol_template_; }
    shared_ptr<Node>                   tree()            const { return root; }
};

#endif // LSYSTYPE_HPP