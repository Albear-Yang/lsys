#ifndef LSYSGEN_HPP
#define LSYSGEN_HPP

#include <memory>
#include <map>
#include <string>
#include <vector>
#include "lsysparse.hpp"
#include "lsystype.hpp"

using Env = unordered_map<string, float>;
using Expr = function<float(const Env&)>;

class Lsystem{
public:
    map<string, float>      globals;
    map<string, Procedure>  procedures;
    map<string, vector<LsysRule>> rules;       // symbol -> list of applicable rules
    map<string, vector<DrawCmd>> draw_rules;
    Lsystem(shared_ptr<Node> root);

private:
    //function to process the procedures and them into the procedures map
    void process_procedure(shared_ptr<Node> procedure);
    //find the procedures and run process procedures on them
    void find_procedures(shared_ptr<Node> root);
    //find 
    void find_declarations(shared_ptr<Node> root);
    void process_declarations(shared_ptr<Node> declaration);

    void find_rules(shared_ptr<Node> root);
    void process_transition_rules(shared_ptr<Node> rule);
    void process_draw_rules(shared_ptr<Node> draw_rule);

    Expr compile_expr(shared_ptr<Node> expr);

    Expr compile_factor(shared_ptr<Node> factor);

    Expr compile_term(shared_ptr<Node> term);

    Test compile_test(shared_ptr<Node> test);

    vector<Statement> compile_statements(shared_ptr<Node> statements);
    Statement compile_statement(shared_ptr<Node> statement);

};

struct DrawCmd {
    function<void(Turtle&, const Env&)> execute;
};

struct Statement {
    function<void(Env&)> execute;
};
struct Test {
    function<bool(Env&)> execute;
};

struct Procedure {
    vector<string>              params;   // ordered param names
    vector<pair<string,Expr>>   locals;   // name, init-expr
    vector<Statement>           stmts;
    Expr                        return_expr;

    float call(vector<float> args) const {
        Env env;
        for (size_t i = 0; i < params.size(); ++i)
            env[params[i]] = args[i];
        for (auto &[name, init] : locals)
            env[name] = init(env);
        for (auto &stmt : stmts)
            stmt.execute(env);
        return return_expr(env);
    }
};

struct SymbolInstance {
    string          name;
    vector<Expr>    args;   // evaluated at rewrite time

    vector<float> eval_args(const Env& e) const {
        vector<float> out;
        for (auto &a : args) out.push_back(a(e));
        return out;
    }
};

struct LsysRule {
    string              predecessor;
    vector<string>      params;        // names bound during rewrite
    vector<SymbolInstance> successor;

    // Returns the expanded successor given the symbol's current args
    vector<SymbolInstance> apply(vector<float> arg_vals) const {
        Env env;
        for (size_t i = 0; i < params.size(); ++i)
            env[params[i]] = arg_vals[i];
        vector<SymbolInstance> out;
        for (auto &s : successor) {
            SymbolInstance inst;
            inst.name = s.name;
            for (auto &a : s.args) inst.args.push_back(
                [v = a(env)](const Env&){ return v; }); // bake in evaluated args
            out.push_back(inst);
        }
        return out;
    }
};
#endif // LSYSGEN_HPP

// code gen should parse an axiom of starting letters
// then with procedures should be able to find a step procedure that deals with public symbols
// should be able to process rules the same way we process procedures, should be stored in a map or something maybe?
// processes... how to store?
// instead of creating assembly code you store expressions, terms and factors as lambda functions
// procedures will affect the scope which is represented by Env an unodrederd map of string and flaots...
// draw commands are a whole other issue that will hurt my tiny miniscule brain
