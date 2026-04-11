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

constexpr float EPS = 1e-6f;
bool feq(float a, float b) { return fabs(a-b) < EPS; }
bool fne(float a, float b) { return fabs(a-b) >= EPS; }
bool fgt(float a, float b) { return a > b+EPS; }
bool fge(float a, float b) { return a > b-EPS; }
bool flt(float a, float b) { return a < b-EPS; }
bool fle(float a, float b) { return a < b+EPS; }

Lsystem::Lsystem(shared_ptr<Node> root) {
    find_declarations(root);
    find_procedures(root);
    find_rules(root);
}

void Lsystem::find_procedures(shared_ptr<Node> root) {
    if (!root) return;
    if (root->value == "procedure") { process_procedure(root); return; }
    for (auto &child : root->children) find_procedures(child);
}

void Lsystem::find_declarations(shared_ptr<Node> root) {
    if (!root) return;
    if (root->value == "declaration") { process_declarations(root); return; }
    for (auto &child : root->children) find_declarations(child);
}

void Lsystem::find_rules(shared_ptr<Node> root) {
    if (!root) return;
    if (root->value == "rule") {
        if (root->children[0]->value == "RULE") process_transition_rules(root);
        else if (root->children[0]->value == "DRAW_RULE") process_draw_rules(root);
        return;
    }
    for (auto &child : root->children) find_rules(child);
}

void Lsystem::process_declarations(shared_ptr<Node> declaration) {
    if (!declaration || declaration->children.size() < 2) return;

    // GLOBAL type ID BECOMES NUM SEMI  (6 children)
    // [0]    [1]  [2] [3]    [4] [5]
    if (declaration->children.size() == 6 && declaration->children[0]->value == "GLOBAL") {
        string id = declaration->children[2]->lexeme;
        try {
            globals[id] = stof(declaration->children[4]->lexeme);
        } catch (...) {
            throw runtime_error("ERROR 16: invalid number '" + declaration->children[4]->lexeme + "'");
        }
    }
    // SYMBOL ID LPAREN params RPAREN SEMI  (6 children)
    // [0]    [1] [2]   [3]   [4]    [5]
    else if (declaration->children.size() == 6 && declaration->children[0]->value == "SYMBOL") {
        string sym = declaration->children[1]->lexeme;
        vector<string> params;
        find_params(declaration->children[3], params);
        symbol_template[sym] = params;
    }
}

void Lsystem::process_procedure(shared_ptr<Node> procedure) {
    // FLOAT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    // [0]  [1] [2]   [3]   [4]   [5]    [6]  [7]        [8]   [9] [10] [11]
    Procedure proc;
    const string proc_name = procedure->children[1]->lexeme;
    find_params(procedure->children[3], proc.params);
    find_decls(procedure->children[6], proc.locals);
    proc.stmts       = compile_statements(procedure->children[7]);
    proc.return_expr = compile_expr(procedure->children[9]);
    procedures[proc_name] = std::move(proc);
}

Test Lsystem::compile_test(shared_ptr<Node> test) {
    if (test->children.size() != 3)
        throw runtime_error("ERROR 19: test condition malformed");
    Expr left  = compile_expr(test->children[0]);
    Expr right = compile_expr(test->children[2]);
    string op  = test->children[1]->value;
    if (op == "EQ") return { [left,right](const Env& e){ return feq(left(e),right(e)); } };
    if (op == "NE") return { [left,right](const Env& e){ return fne(left(e),right(e)); } };
    if (op == "LT") return { [left,right](const Env& e){ return flt(left(e),right(e)); } };
    if (op == "GT") return { [left,right](const Env& e){ return fgt(left(e),right(e)); } };
    if (op == "LE") return { [left,right](const Env& e){ return fle(left(e),right(e)); } };
    if (op == "GE") return { [left,right](const Env& e){ return fge(left(e),right(e)); } };
    return { [](const Env& e){ return true; } };
}

Statement Lsystem::compile_statement(shared_ptr<Node> statement) {
    auto &ch = statement->children;
    if (ch[0]->value == "lvalue") {
        string var = grab_id(ch[0]);
        Expr rhs   = compile_expr(ch[2]);
        return { [var,rhs](Env& e){ e[var] = rhs(e); } };
    }
    if (ch[0]->value == "IF") {
        auto cond  = compile_test(ch[2]);
        auto then_ = compile_statements(ch[5]);
        auto else_ = compile_statements(ch[9]);
        return { [cond,then_,else_](Env& e){
            if (cond.execute(e)) for (auto &s:then_) s.execute(e);
            else                  for (auto &s:else_) s.execute(e);
        }};
    }
    if (ch[0]->value == "WHILE") {
        auto cond = compile_test(ch[2]);
        auto body = compile_statements(ch[5]);
        return { [cond,body](Env& e){
            while (cond.execute(e)) for (auto &s:body) s.execute(e);
        }};
    }
    throw runtime_error("Unknown statement type in compile_statement");
}

string Lsystem::grab_id(shared_ptr<Node> lvalue) {
    if (lvalue->value != "lvalue") throw runtime_error("ERROR 18: lvalue messed up");
    if (lvalue->children.size() == 1) return lvalue->children[0]->lexeme;
    return grab_id(lvalue->children[1]);
}

vector<Statement> Lsystem::compile_statements(shared_ptr<Node> statements) {
    vector<Statement> stmts;
    if (!statements || statements->children.empty()) return stmts;
    // statements -> statements statement
    auto left = compile_statements(statements->children[0]);
    stmts.insert(stmts.end(), left.begin(), left.end());
    stmts.push_back(compile_statement(statements->children[1]));
    return stmts;
}

void Lsystem::find_decls(shared_ptr<Node> dcls, vector<pair<string,Expr>> &locals) {
    // dcls -> .EMPTY              (0 children)
    // dcls -> dcls dcl BECOMES NUM SEMI  (5 children)
    if (!dcls || dcls->children.empty()) return;
    if (dcls->children.size() == 5) {
        find_decls(dcls->children[0], locals); // recurse first — preserve order
        string id  = dcls->children[1]->children[1]->lexeme;
        float  val = stof(dcls->children[3]->lexeme);
        locals.push_back({id, [val](const Env&){ return val; }});
    }
}

void Lsystem::find_params(shared_ptr<Node> paramlist, vector<string> &params) {
    if (!paramlist) return;
    if (paramlist->value == "dcl" && paramlist->children.size() == 2) {
        params.push_back(paramlist->children[1]->lexeme);
        return;
    }
    for (auto &child : paramlist->children) find_params(child, params);
}

Expr Lsystem::compile_expr(shared_ptr<Node> expr) {
    if (expr->children.size() == 1)
        return compile_term(expr->children[0]);
    if (expr->children[1]->value == "PLUS") {
        auto lhs = compile_expr(expr->children[0]);
        auto rhs = compile_term(expr->children[2]);
        return [lhs,rhs](const Env& e){ return lhs(e)+rhs(e); };
    }
    if (expr->children[1]->value == "MINUS") {
        auto lhs = compile_expr(expr->children[0]);
        auto rhs = compile_term(expr->children[2]);
        return [lhs,rhs](const Env& e){ return lhs(e)-rhs(e); };
    }
    return [](const Env&){ return 0.0f; };
}

Expr Lsystem::compile_term(shared_ptr<Node> term) {
    if (term->children.size() == 1)
        return compile_factor(term->children[0]);
    auto lhs = compile_term(term->children[0]);
    auto rhs = compile_factor(term->children[2]);
    string op = term->children[1]->value;
    if (op == "STAR")  return [lhs,rhs](const Env& e){ return lhs(e)*rhs(e); };
    if (op == "SLASH") return [lhs,rhs](const Env& e){ return lhs(e)/rhs(e); };
    if (op == "PCT")   return [lhs,rhs](const Env& e){ return fmod(lhs(e),rhs(e)); };
    return [](const Env&){ return 0.0f; };
}

vector<Expr> Lsystem::compile_arglist(shared_ptr<Node> arglist) {
    vector<Expr> out;
    while (arglist) {
        out.push_back(compile_expr(arglist->children[0]));
        if (arglist->children.size() == 3) arglist = arglist->children[2];
        else break;
    }
    return out;
}

Expr Lsystem::compile_factor(shared_ptr<Node> factor) {
    auto &ch = factor->children;

    if (ch[0]->value == "NUM") {
        float val = stof(ch[0]->lexeme);
        return [val](const Env&){ return val; };
    }
    if (ch[0]->value == "NULL")
        return [](const Env&){ return 0.0f; };

    if (ch[0]->value == "LPAREN")
        return compile_expr(ch[1]);

    if (ch[0]->value == "ID") {
        if (ch.size() == 1) {
            string name = ch[0]->lexeme;
            return [name, this](const Env& e) {
                auto it = e.find(name);
                if (it != e.end()) return it->second;
                auto git = globals.find(name);
                if (git != globals.end()) return git->second;
                throw runtime_error("undefined variable '" + name + "'");
            };
        }
        // built-in rand() — must come before general zero-arg call
        if (ch.size() == 3 && ch[0]->lexeme == "rand") {
            return [](const Env&) -> float {
                return (float)std::rand() / ((float)RAND_MAX + 1.0f);
            };
        }
        if (ch.size() == 3) {  // ID LPAREN RPAREN — zero-arg procedure call
            string fname = ch[0]->lexeme;
            return [fname, this](const Env& e){
                return procedures.at(fname).call({}, globals);
            };
        }
        if (ch.size() == 4) {  // ID LPAREN arglist RPAREN
            string fname = ch[0]->lexeme;
            vector<Expr> args = compile_arglist(ch[2]);
            return [fname, args, this](const Env& e){
                vector<float> vals;
                for (auto &a : args) vals.push_back(a(e));
                return procedures.at(fname).call(vals, globals);
            };
        }
    }

    throw runtime_error("compile_factor: unrecognised factor shape");
}

void Lsystem::process_transition_rules(shared_ptr<Node> rule) {
    auto &ch = rule->children;
    LsysRule r;
    r.predecessor = ch[2]->lexeme;
    find_params(ch[4], r.params);

    if (ch[0]->value == "RULE" && ch.size() == 11) {
        // RULE COLON ID LPAREN params RPAREN WHEN test ARROW symbol_sequence SEMI
        //  [0]  [1] [2] [3]   [4]   [5]    [6]  [7] [8]       [9]          [10]
        // compile condition
        auto cond = compile_test(ch[7]);
        r.condition = cond.execute;
        r.successor = compile_symbol_sequence(ch[9]);
    } else {
        // RULE COLON ID LPAREN params RPAREN ARROW symbol_sequence SEMI
        //  [0]  [1] [2] [3]   [4]   [5]    [6]       [7]          [8]
        r.condition = nullptr; // always applies
        r.successor = compile_symbol_sequence(ch[7]);
    }
    rules[r.predecessor].push_back(std::move(r));
}

void Lsystem::process_draw_rules(shared_ptr<Node> rule) {
    // DRAW_RULE COLON ID LBRACE draw_cmds RBRACE SEMI
    //    [0]    [1]  [2]  [3]     [4]      [5]   [6]
    string sym = rule->children[2]->lexeme;
    DrawRule dr;
    auto it = symbol_template.find(sym);
    if (it != symbol_template.end()) dr.params = it->second;
    dr.cmds = compile_draw_cmds(rule->children[4]);
    draw_rules[sym] = std::move(dr);
}

vector<DrawCmd> Lsystem::compile_draw_cmds(shared_ptr<Node> cmds) {
    vector<DrawCmd> out;
    while (cmds && cmds->children.size() == 2) {
        out.push_back(compile_draw_cmd(cmds->children[0]));
        cmds = cmds->children[1];
    }
    return out;
}

DrawCmd Lsystem::compile_draw_cmd(shared_ptr<Node> cmd) {
    auto &ch  = cmd->children;
    string tag = ch[0]->value;

    if (tag == "LINE") {
        Expr dist = compile_expr(ch[1]);
        return { [dist](Turtle &t, const Env &e){ t.forward(dist(e)); } };
    }
    if (tag == "TURN_YAW") {
        Expr angle = compile_expr(ch[1]);
        return { [angle](Turtle &t, const Env &e){ t.yaw(angle(e)); } };
    }
    if (tag == "TURN_PITCH") {
        Expr angle = compile_expr(ch[1]);
        return { [angle](Turtle &t, const Env &e){ t.pitch(angle(e)); } };
    }
    if (tag == "TURN_ROLL") {
        Expr angle = compile_expr(ch[1]);
        return { [angle](Turtle &t, const Env &e){ t.roll(angle(e)); } };
    }
    if (tag == "FILL") {
        // FILL LPAREN e , e , e , e RPAREN LBRACE draw_cmds RBRACE SEMI
        //  [0]  [1] [2][3][4][5][6][7][8]  [9]   [10]  [11] [12] [13]
        Expr r = compile_expr(ch[2]);
        Expr g = compile_expr(ch[4]);
        Expr b = compile_expr(ch[6]);
        Expr a = compile_expr(ch[8]);
        vector<DrawCmd> inner = compile_draw_cmds(ch[11]);
        return { [r,g,b,a,inner](Turtle &t, const Env &e){
            t.fill(r(e),g(e),b(e),a(e));
            for (auto &c : inner) c.execute(t,e);
        }};
    }
    if (tag == "LBRACK") {
        vector<DrawCmd> inner = compile_draw_cmds(ch[1]);
        return { [inner](Turtle &t, const Env &e){
            t.push();
            for (auto &c : inner) c.execute(t,e);
            t.pop();
        }};
    }
    throw runtime_error("compile_draw_cmd: unknown tag '" + tag + "'");
}

vector<SymbolInstance> Lsystem::compile_symbol_sequence(shared_ptr<Node> seq) {
    // symbol_sequence -> .EMPTY                                      (0 children)
    // symbol_sequence -> symbol symbol_sequence                      (2 children)
    // symbol_sequence -> LBRACK symbol_sequence RBRACK symbol_sequence (4 children)
    vector<SymbolInstance> out;
    if (!seq || seq->children.empty()) return out;

    if (seq->children.size() == 4) {
        // bracket branch: push marker, inner, pop marker, rest
        SymbolInstance push_inst; push_inst.lbrack = true;
        SymbolInstance pop_inst;  pop_inst.rbrack  = true;
        out.push_back(push_inst);
        auto inner = compile_symbol_sequence(seq->children[1]);
        out.insert(out.end(), inner.begin(), inner.end());
        out.push_back(pop_inst);
        auto rest = compile_symbol_sequence(seq->children[3]);
        out.insert(out.end(), rest.begin(), rest.end());
        return out;
    }

    // symbol symbol_sequence — iterative tail
    out.push_back(compile_symbol_instance(seq->children[0]));
    auto rest = compile_symbol_sequence(seq->children[1]);
    out.insert(out.end(), rest.begin(), rest.end());
    return out;
}

SymbolInstance Lsystem::compile_symbol_instance(shared_ptr<Node> sym) {
    // symbol -> ID
    // symbol -> ID LPAREN arglist RPAREN
    SymbolInstance inst;
    inst.name = sym->children[0]->lexeme;
    if (sym->children.size() == 4)
        inst.args = compile_arglist(sym->children[2]);
    return inst;
}

vector<SymbolInstance> Lsystem::step(const vector<SymbolInstance> &current) const {
    vector<SymbolInstance> next;
    for (auto &sym : current) {
        if (sym.lbrack || sym.rbrack) { next.push_back(sym); continue; }

        auto it = rules.find(sym.name);
        if (it == rules.end()) { next.push_back(sym); continue; }

        // Build env for condition evaluation (globals + current args)
        Env env;
        for (auto &[k,v] : globals) env[k] = v;
        auto arg_vals = sym.eval_args(env);
        const auto &rule_list = it->second;

        // find first rule whose condition passes
        const LsysRule *matched = nullptr;
        for (auto &r : rule_list) {
            // bind params into env for condition check
            Env cond_env = env;
            for (size_t i = 0; i < r.params.size() && i < arg_vals.size(); ++i)
                cond_env[r.params[i]] = arg_vals[i];
            if (r.applies(cond_env)) { matched = &r; break; }
        }

        if (!matched) { next.push_back(sym); continue; } // no rule applies — keep symbol

        auto expanded = matched->apply(arg_vals, globals);
        next.insert(next.end(), expanded.begin(), expanded.end());
    }
    return next;
}

void Lsystem::draw(const vector<SymbolInstance> &syms, Turtle &t) const {
    for (auto &sym : syms) {
        if (sym.lbrack) { t.push(); continue; }
        if (sym.rbrack) { t.pop();  continue; }
        auto it = draw_rules.find(sym.name);
        if (it == draw_rules.end()) continue;
        Env env;
        for (auto &[k,v] : globals) env[k] = v;
        it->second.execute(t, sym.eval_args(env), globals);
    }
}