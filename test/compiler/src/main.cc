#include <iostream>
#include <deque>
#include <memory>
#include <map>
#include <sstream>
#include "lsysparse.hpp"
#include "lsystype.hpp"

using namespace std;

int main(){
    string line;
    deque<shared_ptr<Node>> input_queue;
    shared_ptr<Node> result;
    
    // Read tokens from stdin
    // Prepend BOF token
    input_queue.push_back(make_shared<Node>("BOF", "BOF"));
    
    while(getline(cin, line)){
        if(line.empty()) continue;
        istringstream iss(line);
        string kind, lexeme;
        iss >> kind >> lexeme;
        input_queue.push_back(make_shared<Node>(kind, lexeme));
    }
    input_queue.push_back(make_shared<Node>("EOF", "EOF"));
    
    // Step 1: Parse
    cerr << "=== PARSING ===" << endl;
    if (parse(input_queue, result) != 0 || !result) {
        cerr << "Parse failed" << endl;
        return 1;
    }
    cerr << "Parse succeeded" << endl;
    
    // Step 2: Type checking
    cerr << "=== TYPE CHECKING ===" << endl;
    try {
        type_check_ast(result);
        cerr << "Type checking succeeded" << endl;
    } catch (const exception &e) {
        cerr << "Type checking failed: " << e.what() << endl;
        return 1;
    }
    
    // Step 3: Code generation (from lysysgen)
    cerr << "=== CODE GENERATION ===" << endl;
    // This will call lsysgen functions once implemented
    
    // Print the annotated tree
    print_node(result);
    
    return 0;
}
