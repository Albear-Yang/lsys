# L-System Compiler

A custom C-like domain-specific language (DSL) and GPU-accelerated rendering engine for procedural L-system generation using C++ and Metal.

---

## Features

- Custom C-like DSL for procedural plant generation
- Parametric L-system rules
- User-defined procedures and expressions
- Recursive rule expansion
- GPU-accelerated rendering using Metal
- Real-time procedural geometry generation
- Filled polygon rendering support
- Randomized procedural behavior

---

## Architecture

```text
Source Code
    ↓
Lexer
    ↓
Parser
    ↓
AST Generation
    ↓
Rule Evaluation
    ↓
L-System Expansion
    ↓
Geometry Generation
    ↓
Metal Renderer
```

---

## Program Structure

L-sys C requires a strict declaration order.

```c
global float x = 1.0;

symbol F(float x);
symbol G(float x);

rule: F(float x) -> ...;

draw_rule: F {
    ...
};

float fn(float v) {
    return v * 2.0;
}
```

Order:
1. Global variables
2. Symbol declarations
3. Rules
4. Draw rules
5. Procedures

---

## Global Variables

```c
global float angle = 27.5;
global float r     = 0.68;
global float minx  = 0.5;
```

Rules:
- All global variables are `float`
- `global` keyword is required
- Globals are visible inside:
  - rules
  - draw rules
  - procedures

---

## Symbol Declarations

Every symbol used in rules or the axiom must be declared.

```c
symbol F(float x);
symbol Branch(float len, float w);
symbol Leaf(float x);
```

Rules:
- All parameters are `float`
- Trailing semicolon required

---

## Rules

Rules define symbol rewriting behavior.

```c
rule: F(float x) when x > 0.5 ->
    F(x * r)
    [F(x * 0.55)]
    F(x * r);

rule: F(float x) -> F(x);
```

Features:
- Recursive rewriting
- Conditional rules
- Parametric symbols
- Arithmetic expressions
- Branching structures

---

## Draw Rules

Draw rules define how symbols are rendered.

```c
draw_rule: F {
    turn_yaw angle;
    line x;
};
```

Filled geometry is also supported:

```c
draw_rule: L {
    fill(0.14, 0.55, 0.18, 0.9) {
        line x * 0.50;
        turn_yaw 55.0;
        line x * 0.28;
        turn_yaw 70.0;
        line x * 0.30;
        turn_yaw 55.0;
        line x * 0.50;
    };
};
```

---

## Procedures

Procedures allow reusable computations.

```c
float shrink(float v) {
    return v * 0.7;
}
```

Example with conditionals:

```c
float clamp(float v, float mn) {
    if (v < mn) {
        v = mn;
    } else {
        v = v;
    }

    return v;
}
```

Rules:
- Procedures must come after all rules
- No semicolon after closing brace
- Local variables use `float`
- Procedures may mutate global variables

---

## Expressions

Supported operations:
- Addition (`+`)
- Subtraction (`-`)
- Multiplication (`*`)
- Division (`/`)

Example:

```c
x * 0.8 + angle
```

---

## Branching

Branching uses bracket notation.

```c
[
    plant(x * 0.7)
]
```

Behavior:
- `[` pushes turtle state
- `]` restores turtle state

---

## Example Program

```c
global float angle = 27.5;
global float r = 0.68;

symbol F(float x);

rule: F(float x) when x > 0.5 ->
    F(x * r)
    [F(x * 0.55)]
    F(x * r);

rule: F(float x) -> F(x);

draw_rule: F {
    turn_yaw angle;
    line x;
};
```

---

## Complex Example

```c
global float angle = 22.0;
global float pitch = 18.0;
global float r = 0.65;

symbol plant(float x);
symbol leaf(float x);
symbol flower(float x);

rule: plant(float x) ->
    plant(x * r)
    [
        leaf(x * 0.4)
    ]
    [
        flower(x * 0.35)
    ];

draw_rule: leaf {
    fill(0.14, 0.55, 0.18, 0.95) {
        line x * 0.32;
        turn_yaw 20.0;
        line x * 0.08;
        turn_yaw 40.0;
        line x * 0.32;
    };
};

draw_rule: flower {
    turn_pitch -90.0;

    fill(0.92, 0.32, 0.58, 0.5) {
        line x * 0.28;
        turn_yaw 72.0;
        line x * 0.28;
        turn_yaw 72.0;
        line x * 0.28;
        turn_yaw 72.0;
        line x * 0.28;
        turn_yaw 72.0;
        line x * 0.28;
    };
};
```

Additional examples can be found in:
- `examples/flower.lsys`
- `examples/tree.lsys`
- `examples/fern.lsys`

---

## Rendering

The renderer is implemented using Metal and streams generated geometry directly to the GPU for real-time rendering.

Supported rendering features:
- Line rendering
- Filled polygons
- Real-time procedural geometry generation
- Randomized procedural transformations
- Recursive branching structures

---

## Tech Stack

- C++
- Metal
- Objective-C++
- GLFW

---

## Build

```bash
mkdir build
cd build
cmake ..
make
./lsystem
```

---

## Notes

This language was designed specifically for procedural generation and interactive visualization of L-system structures and plant-like geometry.
