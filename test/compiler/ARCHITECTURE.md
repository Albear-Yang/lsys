# L-System Compiler Pipeline

## Overview
This document describes how to connect the parser, type checker, and code generator modules.

## Module Architecture

### 1. **lsysparse.cc/hpp** - Parser
- **Responsibility**: Converts token stream → Parse Tree (AST)
- **Input**: Deque of tokens (kind, lexeme pairs)
- **Output**: `shared_ptr<Node>` representing the AST
- **Main Function**: `int parse(deque<shared_ptr<Node>>& input_queue, shared_ptr<Node>& result)`
- **Helper**: `void print_node(shared_ptr<Node> root)` - Pretty-print AST

### 2. **lsystype.cc/hpp** - Type Checker
- **Responsibility**: Validates types and annotates AST with type information
- **Input**: AST from parser
- **Output**: Type-annotated AST (nodes have `type_annotation` field)
- **Main Function**: `void type_check_ast(shared_ptr<Node> root)`
- **Throws**: `runtime_error` on type violations

Key type annotations:
- `"int"` - Integer type
- `"int*"` - Pointer to integer
- `""` - Empty/no annotation

### 3. **lysysgen.cc/hpp** - Code Generator
- **Responsibility**: Generates MIPS assembly from type-annotated AST
- **Input**: Type-annotated AST from type checker
- **Output**: MIPS assembly instructions (to stdout or provided stream)
- **Main Function**: `void generate_code(shared_ptr<Node> root, ostream &out)`

## Pipeline Flow

```
stdin (tokens)
    ↓
    │ lsysparse.cc
    ├─→ Input: Token stream (BOF ... EOF)
    ├─→ Parser builds AST
    └─→ Output: parse tree (shared_ptr<Node>)
    ↓
AST
    ↓
    │ lsystype.cc
    ├─→ Input: AST
    ├─→ Type checking + annotation
    └─→ Output: Type-annotated AST
    ↓
Type-annotated AST
    ↓
    │ lysysgen.cc
    ├─→ Input: Type-annotated AST
    ├─→ Code generation
    └─→ Output: MIPS assembly
    ↓
stdout (assembly)
```

## Driver Program (main.cc)

The driver coordinates all three stages:

```cpp
// 1. Parse tokens into AST
parse(input_queue, result);

// 2. Type check the AST
type_check_ast(result);

// 3. Generate code
generate_code(result, cout);
```

## Usage

### Individual modules (standalone):
```bash
# Just parse
cat tokens.txt | ./lsysparse

# Parse and type check
cat tokens.txt | ./lsysparse | ./lsystype

# Full pipeline
cat tokens.txt | ./lsysparse | ./lsystype | ./lysysgen
```

### Integrated pipeline:
```bash
cat tokens.txt | ./lsys_compiler
```

## Data Structures

### Node
```cpp
struct Node {
    string value;                        // Token kind or non-terminal name
    string lexeme;                       // Token lexeme (empty for non-terminals)
    vector<shared_ptr<Node>> children;   // Child nodes
    string type_annotation;              // Type info (set by type checker)
};
```

## Error Handling

- **Parser**: Returns error code, writes to stderr
- **Type Checker**: Throws `runtime_error` with descriptive message
- **Code Generator**: Throws `runtime_error` on issues

All exceptions caught in driver and reported to stderr.

## Compilation

```bash
# Compile individual modules
g++ -std=c++17 -o lsysparse src/lsysparse.cc
g++ -std=c++17 -o lsystype src/lsystype.cc src/lsysparse.cc
g++ -std=c++17 -o lysysgen src/lysysgen.cc src/lsysparse.cc src/lsystype.cc

# Compile integrated pipeline
g++ -std=c++17 -I./include -o lsys_compiler \
    src/main.cc src/lsysparse.cc src/lsystype.cc src/lysysgen.cc
```

## Next Steps

1. **Test individual modules** to verify each stage works independently
2. **Test the integrated pipeline** end-to-end
3. **Add error recovery** if needed
4. **Optimize code generation** for better assembly
