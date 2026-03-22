#include "lsysparse.hpp"

using namespace std;

void build_node_string(shared_ptr<Node> root, string &out){
    out += root->value;

    if(root->children.size() > 0){
        for(auto child : root->children){
            out += " " + child->value;
        }
        out += "\n";

        for(auto child : root->children){
            build_node_string(child, out);
        }
        return;
    }
    else if(!root->lexeme.empty()){
        out += " " + root->lexeme + "\n";
        return;
    }
    else{
        out += " .EMPTY\n";
    }
}

void Parser::read_cfg() {
    string line;
    getline(cfg_str, line); // skip .CFG header
    while (getline(cfg_str, line)){
        // FIX 1: removed "peek()==EOF break" — it was dropping the last rule
        if(line.empty()) continue;
        istringstream ss(line);
        string lhs;
        ss >> lhs;
        vector<string> rhs;
        string token;
        while(ss >> token){
            if(token == ".EMPTY") continue;
            rhs.push_back(token);
        }
        rules[rule_index++] = {lhs, rhs};
    }
}

void Parser::read_transitions() {
    string line;
    while (getline(transitions_str, line)){
        // FIX 1: removed "peek()==EOF break" — it was dropping the last entry
        if(line.empty()) continue;
        istringstream ss(line);
        int state, next_state;
        string symbol;
        ss >> state >> symbol >> next_state;
        transitions[state][symbol] = next_state;
    }
}

void Parser::read_reductions() {
    string line;
    while (getline(reductions_str, line)){
        // FIX 1: removed "peek()==EOF break" — it was dropping the last entry
        if(line.empty()) continue;
        istringstream ss(line);
        int state, rule;
        string tag;
        ss >> state >> rule >> tag;
        reduce_states.insert(state);
        reductions[state][tag] = rule;
    }
}

int Parser::parse_input(const std::string &input) {
    result.clear();

    std::deque<std::shared_ptr<Node>> input_queue;
    input_queue.push_back(make_shared<Node>("BOF","BOF"));

    string line;
    stringstream ss(input);
    while(getline(ss,line)){
        if(line.empty()) continue;
        istringstream iss(line);
        string kind, lexeme;
        iss >> kind >> lexeme;
        input_queue.push_back(make_shared<Node>(kind,lexeme));
    }

    input_queue.push_back(make_shared<Node>("EOF","EOF"));
    input_queue.push_back(make_shared<Node>("EOF","EOF")); // sentinel: needed after shifting EOF to provide lookahead for final reduce

    vector<int> state_stack;
    vector<shared_ptr<Node>> symbol_stack;
    state_stack.push_back(0);

    int step = 0;

    while(true){
        // FIX 2: guard against empty queue before reading lookahead
        if(input_queue.empty()){
            cerr << "ERROR: unexpected end of input at step " << step << "\n";
            return 1;
        }

        int state = state_stack.back();
        string lookahead = input_queue.front()->value;

        // REDUCE
        if(reductions.count(state) && reductions[state].count(lookahead)){
            int rule_id = reductions[state][lookahead];
            Rule rule = rules[rule_id];

            shared_ptr<Node> parent = make_shared<Node>(rule.lhs,"");
            for(size_t i = 0; i < rule.rhs.size(); i++){
                if(symbol_stack.empty()){
                    cerr << "REDUCE: unexpected empty symbol_stack\n";
                    return 1;
                }
                parent->children.insert(parent->children.begin(), symbol_stack.back());
                symbol_stack.pop_back();
                state_stack.pop_back();
            }

            symbol_stack.push_back(parent);

            // FIX 3: guard GOTO lookup — missing entry = error, not UB
            int top = state_stack.back();
            if(!transitions.count(top) || !transitions[top].count(rule.lhs)){
                cerr << "ERROR: no GOTO for '" << rule.lhs << "' from state " << top << "\n";
                return 1;
            }
            state_stack.push_back(transitions[top][rule.lhs]);
            continue;
        }

        // ACCEPT
        if(reductions.count(state) && reductions[state].count(".ACCEPT")){
            int rule_id = reductions[state][".ACCEPT"];
            Rule rule = rules[rule_id];

            shared_ptr<Node> root = make_shared<Node>(rule.lhs,"");
            for(size_t i = 0; i < rule.rhs.size(); i++){
                if(symbol_stack.empty()) break;
                root->children.insert(root->children.begin(), symbol_stack.back());
                symbol_stack.pop_back();
            }

            build_node_string(root, result);
            return 0;
        }

        // SHIFT
        if(transitions.count(state) && transitions[state].count(lookahead)){
            auto tok = input_queue.front();
            input_queue.pop_front();
            symbol_stack.push_back(tok);
            state_stack.push_back(transitions[state][lookahead]);
            step++;
            continue;
        }

        // ERROR
        cerr << "ERROR: state=" << state << " lookahead='" << lookahead << "' step=" << step << "\n";
        return 1;
    }
}