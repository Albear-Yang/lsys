//// SLR_tablegen.cpp
//// Compile with: g++ -std=c++17 tablegen.cc -o slrgen
//#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <string>
//#include <vector>
//#include <set>
//#include <map>
//#include <unordered_map>
//#include <algorithm>
//using namespace std;
//
//const std::string LSYS_CFG = R"END(.CFG
//start BOF declarations rules procedures EOF
//declarations .EMPTY
//declarations declaration declarations
//declaration GLOBAL type ID BECOMES NUM SEMI
//declaration SYMBOL ID LPAREN params RPAREN SEMI
//rules rule rules
//rules .EMPTY
//rule RULE conditions COLON ID LPAREN params RPAREN ARROW symbol_sequence SEMI
//conditions LPAREN test RPAREN
//rule DRAW_RULE COLON ID LBRACE draw_cmds RBRACE SEMI
//draw_cmds draw_cmd draw_cmds
//draw_cmds .EMPTY
//draw_cmd LINE expr SEMI
//draw_cmd TURN_PITCH expr SEMI
//draw_cmd TURN_YAW expr SEMI
//draw_cmd TURN_ROLL expr SEMI
//draw_cmd FILL LPAREN expr COMMA expr COMMA expr COMMA expr RPAREN LBRACE draw_cmds RBRACE SEMI
//draw_cmd LBRACK draw_cmds RBRACK
//symbol_sequence .EMPTY
//symbol_sequence symbol symbol_sequence
//symbol_sequence LBRACK symbol_sequence RBRACK symbol_sequence
//symbol ID LPAREN arglist RPAREN
//symbol ID
//procedures procedure procedures
//procedures .EMPTY
//procedure FLOAT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
//params .EMPTY
//params paramlist
//paramlist dcl
//paramlist dcl COMMA paramlist
//type FLOAT
//dcls .EMPTY
//dcls dcls dcl BECOMES NUM SEMI
//dcl type ID
//statements .EMPTY
//statements statements statement
//statement lvalue BECOMES expr SEMI
//statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
//statement WHILE LPAREN test RPAREN LBRACE statements RBRACE
//test expr EQ expr
//test expr NE expr
//test expr LT expr
//test expr LE expr
//test expr GE expr
//test expr GT expr
//test test AND test
//test test OR test
//expr term
//expr expr PLUS term
//expr expr MINUS term
//term factor
//term term STAR factor
//term term SLASH factor
//term term PCT factor
//factor ID
//factor NUM
//factor LPAREN expr RPAREN
//factor ID LPAREN RPAREN
//factor RANDOM LPAREN RPAREN
//factor ID LPAREN arglist RPAREN
//arglist expr
//arglist expr COMMA arglist
//lvalue ID
//lvalue LPAREN lvalue RPAREN
//)END";
//
//struct Rule {
//    string lhs;
//    vector<string> rhs;
//};
//
//// Item for LR(0): rule index and dot position
//struct Item {
//    int rule;
//    int dot;
//    bool operator<(Item const& o) const {
//        if (rule != o.rule) return rule < o.rule;
//        return dot < o.dot;
//    }
//    bool operator==(Item const& o) const {
//        return rule == o.rule && dot == o.dot;
//    }
//};
//
//// --- Utility functions ---
//static inline vector<string> splitTokens(const string &line) {
//    vector<string> out;
//    istringstream ss(line);
//    string t;
//    while (ss >> t) out.push_back(t);
//    return out;
//}
//
//// --- Parser for .CFG style string ---
//vector<Rule> parseCFGFromString(const string& cfg_text, const string& start_sym) {
//    vector<Rule> rules;
//    istringstream in(cfg_text);
//    string line;
//
//    while (getline(in, line)) {
//        if (line.empty()) continue;
//        // Skip .CFG header line
//        if (line.rfind(".CFG", 0) == 0) continue;
//        // Stop at section markers
//        if (line == ".END" || line == ".TRANSITIONS" || line == ".REDUCTIONS") break;
//        // Skip comments
//        if (line[0] == '#') continue;
//
//        auto toks = splitTokens(line);
//        if (toks.empty()) continue;
//
//        string lhs = toks[0];
//        vector<string> rhs;
//        for (size_t i = 1; i < toks.size(); ++i) {
//            if (toks[i] == ".EMPTY") continue; // empty production -> rhs stays empty
//            rhs.push_back(toks[i]);
//        }
//        rules.push_back({lhs, rhs});
//    }
//
//    // Augment grammar: S' -> start_sym at index 0
//    vector<Rule> augmented;
//    augmented.push_back({ "S'", { start_sym } });
//    for (auto &r : rules) augmented.push_back(r);
//    return augmented;
//}
//
//// --- FIRST and FOLLOW computation ---
//void computeTerminalsNonterminals(const vector<Rule>& rules,
//                                  set<string>& nonterms,
//                                  set<string>& terms) {
//    for (auto &r : rules) nonterms.insert(r.lhs);
//    for (auto &r : rules) {
//        for (auto &s : r.rhs) {
//            if (!nonterms.count(s) && s != ".EMPTY") {
//                terms.insert(s);
//            }
//        }
//    }
//    // Remove anything that turned out to be a nonterminal
//    for (auto &nt : nonterms) terms.erase(nt);
//}
//
//void computeFIRST(const vector<Rule>& rules,
//                  const set<string>& nonterms,
//                  const set<string>& terms,
//                  unordered_map<string,set<string>>& FIRST)
//{
//    for (auto &t : terms) FIRST[t].insert(t);
//    for (auto &nt : nonterms) FIRST[nt]; // ensure exists
//
//    bool changed = true;
//    while (changed) {
//        changed = false;
//        for (auto &r : rules) {
//            if (r.rhs.empty()) {
//                if (FIRST[r.lhs].insert(".EMPTY").second) changed = true;
//                continue;
//            }
//            bool allNullable = true;
//            for (auto &sym : r.rhs) {
//                for (auto &f : FIRST[sym]) {
//                    if (f == ".EMPTY") continue;
//                    if (FIRST[r.lhs].insert(f).second) changed = true;
//                }
//                if (!FIRST[sym].count(".EMPTY")) {
//                    allNullable = false;
//                    break;
//                }
//            }
//            if (allNullable) {
//                if (FIRST[r.lhs].insert(".EMPTY").second) changed = true;
//            }
//        }
//    }
//}
//
//void computeFOLLOW(const vector<Rule>& rules,
//                   const set<string>& nonterms,
//                   unordered_map<string,set<string>>& FIRST,
//                   unordered_map<string,set<string>>& FOLLOW)
//{
//    for (auto &nt : nonterms) FOLLOW[nt];
//    FOLLOW["S'"].insert("$"); // use $ as EOF sentinel in FOLLOW
//
//    bool changed = true;
//    while (changed) {
//        changed = false;
//        for (auto &r : rules) {
//            for (int i = 0; i < (int)r.rhs.size(); ++i) {
//                string B = r.rhs[i];
//                if (!nonterms.count(B)) continue;
//                bool nullableSuffix = true;
//                for (int j = i + 1; j < (int)r.rhs.size(); ++j) {
//                    string beta = r.rhs[j];
//                    for (auto &t : FIRST[beta]) {
//                        if (t == ".EMPTY") continue;
//                        if (FOLLOW[B].insert(t).second) changed = true;
//                    }
//                    if (!FIRST[beta].count(".EMPTY")) {
//                        nullableSuffix = false;
//                        break;
//                    }
//                }
//                if (nullableSuffix) {
//                    for (auto &t : FOLLOW[r.lhs]) {
//                        if (FOLLOW[B].insert(t).second) changed = true;
//                    }
//                }
//            }
//        }
//    }
//}
//
//// --- closure and goto for LR(0) items ---
//set<Item> closure(const set<Item>& I, const vector<Rule>& rules, const set<string>& nonterms) {
//    set<Item> C = I;
//    bool changed = true;
//    while (changed) {
//        changed = false;
//        vector<Item> items(C.begin(), C.end());
//        for (auto &it : items) {
//            const Rule &r = rules[it.rule];
//            if (it.dot < (int)r.rhs.size()) {
//                string B = r.rhs[it.dot];
//                if (nonterms.count(B)) {
//                    for (int i = 0; i < (int)rules.size(); ++i) {
//                        if (rules[i].lhs == B) {
//                            if (C.insert({i, 0}).second) changed = true;
//                        }
//                    }
//                }
//            }
//        }
//    }
//    return C;
//}
//
//set<Item> GOTO(const set<Item>& I, const string& X, const vector<Rule>& rules, const set<string>& nonterms) {
//    set<Item> J;
//    for (auto &it : I) {
//        const Rule &r = rules[it.rule];
//        if (it.dot < (int)r.rhs.size() && r.rhs[it.dot] == X) {
//            J.insert({it.rule, it.dot + 1});
//        }
//    }
//    if (J.empty()) return J;
//    return closure(J, rules, nonterms);
//}
//
//// --- Build canonical collection using a map for O(log n) state lookup ---
//void buildCanonicalCollection(const vector<Rule>& rules,
//                              const set<string>& nonterms,
//                              const set<string>& terms,
//                              vector< set<Item> >& states,
//                              map<pair<int,string>, int>& transitions)
//{
//    map<set<Item>, int> stateIndex;
//
//    set<Item> start; start.insert({0,0});
//    set<Item> s0 = closure(start, rules, nonterms);
//    states.push_back(s0);
//    stateIndex[s0] = 0;
//
//    // All grammar symbols
//    vector<string> symbols;
//    symbols.insert(symbols.end(), terms.begin(), terms.end());
//    symbols.insert(symbols.end(), nonterms.begin(), nonterms.end());
//
//    for (int i = 0; i < (int)states.size(); ++i) {
//        for (auto &X : symbols) {
//            set<Item> J = GOTO(states[i], X, rules, nonterms);
//            if (J.empty()) continue;
//
//            int found;
//            auto it = stateIndex.find(J);
//            if (it == stateIndex.end()) {
//                found = (int)states.size();
//                states.push_back(J);
//                stateIndex[J] = found;
//            } else {
//                found = it->second;
//            }
//            transitions[{i, X}] = found;
//        }
//    }
//}
//
//// --- Build SLR action / goto tables ---
//void buildTablesSLR(const vector<Rule>& rules,
//                    const set<string>& nonterms,
//                    const set<string>& terms,
//                    const unordered_map<string,set<string>>& FOLLOW,
//                    const vector< set<Item> >& states,
//                    const map<pair<int,string>, int>& transitions,
//                    map<pair<int,string>, int>& shiftTable,
//                    map<pair<int,string>, int>& gotoTable,
//                    map<pair<int,string>, int>& reduceTable)
//{
//    // Shifts and GOTOs from transitions
//    for (auto &kv : transitions) {
//        int st = kv.first.first;
//        const string &X = kv.first.second;
//        int to = kv.second;
//        if (terms.count(X)) {
//            shiftTable[{st, X}] = to;
//        } else if (nonterms.count(X)) {
//            gotoTable[{st, X}] = to;
//        }
//    }
//
//    // Reductions: completed items
//    for (int i = 0; i < (int)states.size(); ++i) {
//        for (auto &it : states[i]) {
//            const Rule &r = rules[it.rule];
//            if (it.dot != (int)r.rhs.size()) continue;
//
//            if (r.lhs == "S'") {
//                // Accept — emit on EOF terminal
//                auto key = make_pair(i, string("EOF"));
//                if (reduceTable.count(key)) {
//                    cerr << "WARNING: conflict at state " << i << " on EOF\n";
//                }
//                reduceTable[key] = it.rule;
//            } else {
//                const auto &followSet = FOLLOW.at(r.lhs);
//                for (auto &a : followSet) {
//                    string tok = (a == "$") ? "EOF" : a;
//                    auto key = make_pair(i, tok);
//                    // Check shift/reduce conflict
//                    if (shiftTable.count(key)) {
//                        cerr << "WARNING: shift/reduce conflict at state " << i
//                             << " on '" << tok << "' (rule " << it.rule
//                             << ": " << r.lhs << " ->"; 
//                        for (auto &s : r.rhs) cerr << " " << s;
//                        cerr << ")\n";
//                    }
//                    // Check reduce/reduce conflict
//                    if (reduceTable.count(key) && reduceTable[key] != it.rule) {
//                        cerr << "WARNING: reduce/reduce conflict at state " << i
//                             << " on '" << tok << "' between rule "
//                             << reduceTable[key] << " and rule " << it.rule << "\n";
//                    }
//                    reduceTable[key] = it.rule;
//                }
//            }
//        }
//    }
//}
//
//// --- Output ---
//void printTransitions(const map<pair<int,string>, int>& transitions) {
//    cout << ".TRANSITIONS\n";
//    vector<pair<pair<int,string>,int>> entries(transitions.begin(), transitions.end());
//    sort(entries.begin(), entries.end(),
//         [](auto &a, auto &b){
//             if (a.first.first != b.first.first) return a.first.first < b.first.first;
//             return a.first.second < b.first.second;
//         });
//    for (auto &e : entries) {
//        cout << e.first.first << " " << e.first.second << " " << e.second << "\n";
//    }
//}
//
//void printReductions(const map<pair<int,string>, int>& reductions,
//                     const vector<Rule>& rules) {
//    cout << ".REDUCTIONS\n";
//    vector<pair<pair<int,string>,int>> entries(reductions.begin(), reductions.end());
//    sort(entries.begin(), entries.end(),
//         [](auto &a, auto &b){
//             if (a.first.first != b.first.first) return a.first.first < b.first.first;
//             return a.first.second < b.first.second;
//         });
//    for (auto &e : entries) {
//        int state  = e.first.first;
//        string tok = e.first.second;
//        int ruleIdx = e.second;
//        const Rule &r = rules[ruleIdx];
//        // Format: state lookahead ruleIndex lhs rhs...
//        cout << state << " " << ruleIdx << " " << tok;
//        cout << "\n";
//    }
//}
//
//int main() {
//    const string CFG_TEXT = LSYS_CFG;
//
//    vector<Rule> rules = parseCFGFromString(CFG_TEXT, "start");
//
//    set<string> nonterms, terms;
//    computeTerminalsNonterminals(rules, nonterms, terms);
//
//    // Sanity check: S' must be a nonterminal
//    if (!nonterms.count("S'")) {
//        cerr << "ERROR: S' not found in nonterminals\n";
//        return 1;
//    }
//
//    unordered_map<string,set<string>> FIRST;
//    computeFIRST(rules, nonterms, terms, FIRST);
//
//    unordered_map<string,set<string>> FOLLOW;
//    computeFOLLOW(rules, nonterms, FIRST, FOLLOW);
//
//    vector< set<Item> > states;
//    map<pair<int,string>, int> transitions;
//    buildCanonicalCollection(rules, nonterms, terms, states, transitions);
//
//    map<pair<int,string>, int> shiftTable, gotoTable, reduceTable;
//    buildTablesSLR(rules, nonterms, terms, FOLLOW, states, transitions,
//                   shiftTable, gotoTable, reduceTable);
//
//    // Combine shift + goto for .TRANSITIONS output
//    map<pair<int,string>, int> allTransitions;
//    for (auto &kv : shiftTable) allTransitions[kv.first] = kv.second;
//    for (auto &kv : gotoTable)  allTransitions[kv.first] = kv.second;
//
//    printTransitions(allTransitions);
//    printReductions(reduceTable, rules);
//
//    cerr << "# States: " << states.size() << "\n";
//    cerr << "# Rules: "  << rules.size()  << "\n";
//
//    return 0;
//}
