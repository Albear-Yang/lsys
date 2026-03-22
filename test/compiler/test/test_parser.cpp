// test_parser.cpp
// g++ -std=c++17 test_parser.cpp lsysparse.cpp -o test_parser && ./test_parser
#include "lsysparse.hpp"
#include <iostream>
using namespace std;

struct TestCase {
    string name;
    string input;
    bool expect_ok;
};

static int passed = 0, failed = 0;

void run(const TestCase &tc) {
    Parser p;
    int rc = p.parse_input(tc.input);
    bool ok = (rc == 0);
    if (ok == tc.expect_ok) {
        cout << "[PASS] " << tc.name << "\n";
        ++passed;
    } else {
        cout << "[FAIL] " << tc.name
             << "  (got " << (ok?"OK":"ERROR")
             << ", expected " << (tc.expect_ok?"OK":"ERROR") << ")\n";
        if (ok) cout << "  tree: " << p.result.substr(0,120) << "\n";
        ++failed;
    }
}

void run_tree(const TestCase &tc, const string &must_contain) {
    Parser p;
    int rc = p.parse_input(tc.input);
    bool ok = (rc == 0);
    bool found = ok && p.result.find(must_contain) != string::npos;
    if (ok == tc.expect_ok && (!tc.expect_ok || found)) {
        cout << "[PASS] " << tc.name << "\n"; ++passed;
    } else {
        cout << "[FAIL] " << tc.name;
        if (ok != tc.expect_ok)
            cout << "  (got "<<(ok?"OK":"ERROR")<<", expected "<<(tc.expect_ok?"OK":"ERROR")<<")";
        if (tc.expect_ok && ok && !found)
            cout << "  (tree missing '"<<must_contain<<"')";
        cout << "\n"; ++failed;
    }
}

// Build a token string from KIND LEXEME pairs
string toks(initializer_list<pair<string,string>> ts) {
    string out;
    for (auto &[k,v] : ts) out += k + " " + v + "\n";
    return out;
}

int main() {
    // 1. Empty program
    run({ "empty_program", "", true });

    // 2. Minimal procedure: float foo() { return 1; }
    run({ "minimal_procedure",
        toks({{"FLOAT","float"},{"ID","foo"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},{"RETURN","return"},{"NUM","1"},{"SEMI",";"},{"RBRACE","}"}}),
        true });

    // 3. Procedure with typed param and local decl
    //    float bar(float x) { float y = 2; return y; }
    run({ "procedure_with_param_and_dcl",
        toks({{"FLOAT","float"},{"ID","bar"},{"LPAREN","("},
                  {"FLOAT","float"},{"ID","x"},
              {"RPAREN",")"},{"LBRACE","{"},
                  {"FLOAT","float"},{"ID","y"},{"BECOMES","="},{"NUM","2"},{"SEMI",";"},
                  {"RETURN","return"},{"ID","y"},{"SEMI",";"},
              {"RBRACE","}"}}),
        true });

    // 4. Arithmetic: a + b * c
    run_tree({ "arithmetic_expr",
        toks({{"FLOAT","float"},{"ID","f"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},{"RETURN","return"},
                  {"ID","a"},{"PLUS","+"},{"ID","b"},{"STAR","*"},{"ID","c"},
              {"SEMI",";"},{"RBRACE","}"}}),
        true }, "expr");

    // 5. If / else
    run({ "if_else",
        toks({{"FLOAT","float"},{"ID","g"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},
                  {"IF","if"},{"LPAREN","("},{"ID","x"},{"LT","<"},{"NUM","0"},{"RPAREN",")"},
                  {"LBRACE","{"},{"RBRACE","}"},
                  {"ELSE","else"},
                  {"LBRACE","{"},{"RBRACE","}"},
                  {"RETURN","return"},{"NUM","0"},{"SEMI",";"},
              {"RBRACE","}"}}),
        true });

    // 6. While loop
    run({ "while_loop",
        toks({{"FLOAT","float"},{"ID","h"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},
                  {"WHILE","while"},{"LPAREN","("},{"ID","i"},{"NE","!="},{"NUM","0"},{"RPAREN",")"},
                  {"LBRACE","{"},{"RBRACE","}"},
                  {"RETURN","return"},{"NUM","0"},{"SEMI",";"},
              {"RBRACE","}"}}),
        true });

    // 7. Symbol declaration: SYMBOL F(float x)
    run({ "symbol_decl",
        toks({{"SYMBOL","symbol"},{"ID","F"},{"LPAREN","("},
                  {"FLOAT","float"},{"ID","x"},
              {"RPAREN",")"},{"SEMI",";"}}),
        true });

    // 8. Simple L-system rule: rule: F(float x) -> F F
    run({ "simple_rule",
        toks({{"RULE","rule"},{"COLON",":"},{"ID","F"},{"LPAREN","("},
                  {"FLOAT","float"},{"ID","x"},
              {"RPAREN",")"},{"ARROW","->"},
                  {"ID","F"},{"ID","F"},
              {"SEMI",";"}}),
        true });

    // 9. Rule with arglist in symbol instance
    run({ "rule_with_arglist",
        toks({{"RULE","rule"},{"COLON",":"},{"ID","F"},{"LPAREN","("},
                  {"FLOAT","float"},{"ID","x"},
              {"RPAREN",")"},{"ARROW","->"},
                  {"ID","F"},{"LPAREN","("},{"ID","x"},{"PLUS","+"},{"NUM","1"},{"RPAREN",")"},
              {"SEMI",";"}}),
        true });

    // 10. Draw rule block
    run({ "draw_rule",
        toks({{"DRAW_RULE","draw"},{"COLON",":"},{"ID","F"},{"LBRACE","{"},
                  {"LINE","line"},{"NUM","1"},{"SEMI",";"},
                  {"TURN_PITCH","turn_pitch"},{"NUM","90"},{"SEMI",";"},
              {"RBRACE","}"},{"SEMI",";"}}),
        true });

    // 11. NULL as factor
    run({ "null_factor",
        toks({{"FLOAT","float"},{"ID","f"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},{"RETURN","return"},{"NULL","NULL"},{"SEMI",";"},{"RBRACE","}"}}),
        true });

    // 12. Function call in expression
    run({ "function_call_in_expr",
        toks({{"FLOAT","float"},{"ID","f"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},{"RETURN","return"},
                  {"ID","foo"},{"LPAREN","("},{"ID","x"},{"COMMA",","},{"NUM","2"},{"RPAREN",")"},
              {"SEMI",";"},{"RBRACE","}"}}),
        true });

    // 13. Nested parens in expr
    run({ "nested_parens",
        toks({{"FLOAT","float"},{"ID","f"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},{"RETURN","return"},
                  {"LPAREN","("},{"ID","a"},{"PLUS","+"},{"ID","b"},{"RPAREN",")"},
              {"SEMI",";"},{"RBRACE","}"}}),
        true });

    // 14. ERROR: stray token
    run({ "error_stray_token", toks({{"SEMI",";"}}), false });

    // 15. ERROR: missing SEMI after assignment
    run({ "error_missing_semi",
        toks({{"FLOAT","float"},{"ID","f"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},
                  {"ID","x"},{"BECOMES","="},{"NUM","1"}, // no SEMI
                  {"RETURN","return"},{"NUM","0"},{"SEMI",";"},
              {"RBRACE","}"}}),
        false });

    // 16. ERROR: unmatched brace
    run({ "error_unmatched_brace",
        toks({{"FLOAT","float"},{"ID","f"},{"LPAREN","("},{"RPAREN",")"},
              {"LBRACE","{"},
                  {"RETURN","return"},{"NUM","0"},{"SEMI",";"}
              // missing RBRACE
              }),
        false });

    cout << "\n" << passed << "/" << (passed+failed) << " tests passed.\n";
    return failed > 0 ? 1 : 0;
}