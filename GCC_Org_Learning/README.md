# Learning GCC Internals & Static Analyzer

This repository documents my learning journey while exploring the internals of **GCC (GNU Compiler Collection)**. My main goal while studying this is to understand how compilers work internally and eventually contribute to **GCC's static analyzer (`-fanalyzer`)**, particularly as preparation for **Google Summer of Code (GSoC) 2026**.

These notes are written in a way that helped me understand the concepts step by step. Instead of being purely academic, they are meant to explain things in a practical and intuitive way as I explored GCC.

---

# What is a Compiler?

A compiler is a program that converts human-readable source code into machine code that a computer can execute.

For example, when we write a C program:

```c
int main() {
    return 0;
}
```

The compiler eventually turns this into instructions that the CPU understands.

However, modern compilers don't convert source code directly into machine code in one step. Instead, they go through several intermediate stages that allow the compiler to analyze, optimize, and transform the program.

---

# GCC Compilation Pipeline

Inside GCC, the compilation process roughly looks like this:

```
C Source Code
      ↓
Parser → AST
      ↓
GIMPLE (Intermediate Representation)
      ↓
GIMPLE SSA
      ↓
Optimizations
      ↓
RTL (Register Transfer Language)
      ↓
Assembly
      ↓
Machine Code
```

Each stage transforms the program into a different representation that is easier for the compiler to work with.

For someone interested in the static analyzer, the most important stages are:

* GIMPLE
* CFG
* SSA
* Analyzer

---

# Intermediate Representation (GIMPLE)

One of the key ideas in compiler design is the use of **Intermediate Representations (IR)**.

C syntax can be complicated. So GCC converts it into a simpler internal language called **GIMPLE**.

For example, consider the following C statement:

```c
a = b + c;
```

In GIMPLE SSA this might look like:

```
_1 = b_1 + c_1
a_1 = _1
```

GIMPLE is a form of **three-address code**, which means each instruction usually performs only one operation.

This makes analysis and optimization much easier.

---

# Compiler Optimizations

Once code is represented in GIMPLE, GCC performs many optimizations.

### Dead Assignment

```
x = 5;
x = 6;
```

The first assignment is useless, so the compiler keeps only:

```
x = 6
```

### Constant Folding

```c
int a = 2 + 3;
```

The compiler can compute this at compile time:

```
a = 5
```

### Dead Code Elimination

```c
if (0) {
    foo();
}
```

Since the condition is always false, the code is removed completely.

### Function Inlining

Sometimes the compiler replaces a function call with the function's body to reduce overhead.

---

# Static Single Assignment (SSA)

Another very important representation used by GCC is **SSA (Static Single Assignment)**.

In SSA form, every variable is assigned exactly once.

Example:

Normal code:

```
x = 1
x = 4
```

SSA form:

```
x_1 = 1
x_2 = 4
```

This allows the compiler to track values more precisely and perform stronger optimizations.

---

# Basic Blocks

In GCC, code is organized into **basic blocks**.

A basic block is a sequence of instructions that:

* runs from top to bottom
* contains no jumps inside
* ends with a jump or return

Example:

```
<bb 2>:
x_1 = 5
_2 = x_1 > 0
if (_2 != 0)
    goto <bb 3>
else
    goto <bb 4>
```

---

# Control Flow Graph (CFG)

Basic blocks are connected together to form a **Control Flow Graph (CFG)**.

For example:

```c
int main() {
    int x = 5;
    if (x > 0)
        x = 10;
    return x;
}
```

In GIMPLE this might look like:

```
<bb 2>:
x_1 = 5
_2 = x_1 > 0
if (_2 != 0)
    goto <bb 3>
else
    goto <bb 4>

<bb 3>:
x_3 = 10
goto <bb 4>

<bb 4>:
x_4 = PHI <x_1(2), x_3(3)>
return x_4
```

---

# PHI Nodes

When different control flow paths merge, GCC uses **PHI nodes**.

Example:

```
x_4 = PHI <x_1(2), x_3(3)>
```

This means:

* if execution came from block 2 → use `x_1`
* if execution came from block 3 → use `x_3`

PHI nodes are essential for SSA.

---

# GCC Static Analyzer (`-fanalyzer`)

GCC includes a **static analyzer** that detects bugs without running the program.

It can detect issues like:

* memory leaks
* double free
* use-after-free
* null pointer dereference
* uninitialized variables

Instead of executing the program normally, the analyzer **symbolically explores possible execution paths**.

---

# Analyzer Architecture

The analyzer works on top of GCC's internal structures.

The general flow looks like this:

```
GIMPLE
   ↓
CFG
   ↓
Supergraph
   ↓
Exploded Graph
```

### CFG

Represents control flow inside a function.

### Supergraph

Extends the CFG across function calls.

### Exploded Graph

Represents different possible execution states.

---

# Exploded Graph

The analyzer explores different program paths.

Each node in the exploded graph represents:

```
(program location, program state)
```

Example:

```
(<bb 4>, state A)
(<bb 4>, state B)
(<bb 4>, state C)
```

This means the analyzer reached the same code location with different possible states.

---

# Region Model

The analyzer does not track real memory addresses.

Instead, it uses an abstract memory model called the **region model**.

Memory is divided into regions such as:

* stack regions
* heap regions
* global regions
* subregions

Example:

```c
*p = 5;
```

Internally represented as:

```
region(pointee_of_p) = 5
```

This abstraction allows the analyzer to detect memory errors.

---

# State Machines in the Analyzer

Many analyzer checks are implemented using **state machines**.

For example, a memory allocation state machine might track:

* unallocated
* allocated
* freed

Transitions might look like:

```
malloc → allocated
free → freed
free again → error
```

In GCC this logic exists in files like:

```
gcc/analyzer/sm-malloc.cc
```

---

# Analyzer Hooks

When the analyzer processes code, it notifies different checkers through hooks.

Examples include:

* `on_stmt()`
* `on_call()`
* `on_condition()`
* `on_return()`

For example:

```cpp
void on_call(sm_context &sm_ctxt, const gcall &call)
```

This function runs whenever the analyzer sees a function call in GIMPLE.

---

# Important GCC Data Structures

### `tree`

The `tree` type is GCC's universal internal node.

It can represent:

* variables
* constants
* expressions
* declarations
* functions

---

### `gcall`

Represents a function call in GIMPLE.

Example:

```c
malloc(10);
```

---

### `gimple_call_arg`

Retrieves arguments passed to a function.

Example:

```c
tree arg0 = gimple_call_arg(call, 0);
```

---

### `integer_zerop`

Checks if a value is constant zero.

Useful for detecting cases like:

```
malloc(0)
```

---

### `sm_context`

`sm_context` acts as the interface between the analyzer engine and checkers.

It allows checkers to:

* read resource state
* update resource state
* emit warnings

Example operations:

```
sm_ctxt.get_state(...)
sm_ctxt.set_next_state(...)
sm_ctxt.warn(...)
```

---

# Building GCC from Source

Building GCC usually follows this process:

```
GCC Source
     ↓
configure
     ↓
make
     ↓
new GCC compiler
```

### configure

Checks the system and generates Makefiles.

### make

Compiles source files and builds GCC.

---

# Bootstrapping

GCC is written in C/C++, which means GCC can compile itself.

The bootstrap process usually looks like:

```
Stage 1
system gcc → builds gcc_stage1

Stage 2
gcc_stage1 → builds gcc_stage2

Stage 3
gcc_stage2 → builds gcc_stage3
```

If stage2 and stage3 outputs match, the compiler is considered correct.

---

# GCC Driver and Internal Programs

The `gcc` command is mainly a driver program.

It calls other internal tools:

```
gcc
 ↓
cc1
 ↓
assembler
 ↓
linker
```

### `cc1`

The real C compiler.

It handles:

* parsing
* GIMPLE generation
* optimizations
* static analysis

---

# Useful GCC Flags

Some useful flags while exploring GCC internals:

```
-o    specify output file
-c    compile only
-S    stop at assembly
-v    show internal commands
```

Example:

```
gcc -S program.c
```

This produces assembly output.

---

# Dumping GCC Internal Representations

GCC can dump its internal stages for debugging.

Example:

```
-fdump-tree-all
```

Command:

```
gcc -fdump-tree-all program.c
```

This generates files showing **GIMPLE**, **SSA**, and other internal representations.

---

# References

**GCC Internals Documentation**
https://gcc.gnu.org/onlinedocs/gccint/

**GCC Static Analyzer**
https://gcc.gnu.org/wiki/StaticAnalyzer
