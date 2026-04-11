#ifndef LSYSGEN_HPP
#define LSYSGEN_HPP

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include "lsysparse.hpp"

using namespace std;
using Env  = unordered_map<string, float>;
using Expr = function<float(const Env&)>;

struct Turtle {
    virtual void forward(float dist)                       = 0;
    virtual void yaw(float deg)                            = 0;
    virtual void pitch(float deg)                          = 0;
    virtual void roll(float deg)                           = 0;
    virtual void fill(float r, float g, float b, float a) = 0;
    virtual void push()                                    = 0;
    virtual void pop()                                     = 0;
    virtual ~Turtle() = default;
};

struct Statement { function<void(Env&)>                execute; };
struct Test      { function<bool(const Env&)>          execute; };
struct DrawCmd   { function<void(Turtle&, const Env&)> execute; };

struct Procedure {
    vector<string>            params;
    vector<pair<string,Expr>> locals;
    vector<Statement>         stmts;
    Expr                      return_expr;

    float call(vector<float> args, const map<string,float> &globals = {}) const {
        Env env;
        for (auto &[k,v] : globals) env[k] = v;
        for (size_t i = 0; i < params.size(); ++i) env[params[i]] = args[i];
        for (auto &[name, init] : locals) env[name] = init(env);
        for (auto &s : stmts) s.execute(env);
        return return_expr(env);
    }
};

struct SymbolInstance {
    string       name{""};
    vector<Expr> args{};
    bool         lbrack{false};
    bool         rbrack{false};

    vector<float> eval_args(const Env& e) const {
        vector<float> out;
        for (auto &a : args) out.push_back(a(e));
        return out;
    }
};

struct LsysRule {
    string                    predecessor;
    vector<string>            params;
    vector<SymbolInstance>    successor;
    // optional condition — nullptr means always applies
    function<bool(const Env&)> condition{nullptr};

    bool applies(const Env &env) const {
        if (!condition) return true;
        return condition(env);
    }

    vector<SymbolInstance> apply(vector<float> arg_vals,
                                  const map<string,float> &globals) const {
        Env env;
        for (auto &[k,v] : globals) env[k] = v;
        for (size_t i = 0; i < params.size(); ++i) env[params[i]] = arg_vals[i];
        vector<SymbolInstance> out;
        for (auto &s : successor) {
            if (s.lbrack || s.rbrack) { out.push_back(s); continue; }
            SymbolInstance inst;
            inst.name = s.name;
            for (auto &a : s.args)
                inst.args.push_back([v = a(env)](const Env&){ return v; });
            out.push_back(inst);
        }
        return out;
    }
};

struct DrawRule {
    vector<string>  params;
    vector<DrawCmd> cmds;

    void execute(Turtle &t, const vector<float> &arg_vals,
                 const map<string,float> &globals) const {
        Env env;
        for (auto &[k,v] : globals) env[k] = v;
        for (size_t i = 0; i < params.size(); ++i) env[params[i]] = arg_vals[i];
        for (auto &c : cmds) c.execute(t, env);
    }
};

class Lsystem {
public:
    map<string, float>            globals;
    map<string, Procedure>        procedures;
    map<string, vector<string>>   symbol_template;
    map<string, vector<LsysRule>> rules;
    map<string, DrawRule>         draw_rules;

    explicit Lsystem(shared_ptr<Node> root);

    vector<SymbolInstance> step(const vector<SymbolInstance> &current) const;
    void draw(const vector<SymbolInstance> &syms, Turtle &t) const;

private:
    void find_declarations(shared_ptr<Node> root);
    void process_declarations(shared_ptr<Node> decl);
    void find_procedures(shared_ptr<Node> root);
    void process_procedure(shared_ptr<Node> proc);
    void find_rules(shared_ptr<Node> root);
    void process_transition_rules(shared_ptr<Node> rule);
    void process_draw_rules(shared_ptr<Node> rule);

    Expr              compile_expr(shared_ptr<Node> expr);
    Expr              compile_term(shared_ptr<Node> term);
    Expr              compile_factor(shared_ptr<Node> factor);
    vector<Expr>      compile_arglist(shared_ptr<Node> arglist);

    Test              compile_test(shared_ptr<Node> test);
    Statement         compile_statement(shared_ptr<Node> stmt);
    vector<Statement> compile_statements(shared_ptr<Node> stmts);

    DrawCmd           compile_draw_cmd(shared_ptr<Node> cmd);
    vector<DrawCmd>   compile_draw_cmds(shared_ptr<Node> cmds);

    SymbolInstance         compile_symbol_instance(shared_ptr<Node> sym);
    vector<SymbolInstance> compile_symbol_sequence(shared_ptr<Node> seq);

    void   find_params(shared_ptr<Node> node, vector<string> &params);
    void   find_decls(shared_ptr<Node> dcls, vector<pair<string,Expr>> &locals);
    string grab_id(shared_ptr<Node> lvalue);
};

#endif // LSYSGEN_HPP