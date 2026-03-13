# GCC Internals & Static Analyzer Learning

This repository documents my learning journey while exploring the internals of the **GNU Compiler Collection (GCC)**.  
My primary focus is understanding how compilers work internally and experimenting with the **GCC Static Analyzer (`-fanalyzer`)**.

The goal of this learning project is to gain a deep understanding of compiler internals and eventually contribute to GCC as preparation for **Google Summer of Code (GSoC) 2026**.

---

# GCC Static Analyzer Experiment: Toy Checker (`exit()`)

To understand how GCC static analyzer checkers are implemented, I created a **toy analyzer checker** that detects calls to the `exit()` function.

This experiment helped me understand:

- How GCC analyzer state machines are implemented
- How new analyzer checkers are registered
- How GCC's build system compiles analyzer components
- How the analyzer interacts with GIMPLE function calls

The following sections describe the steps taken to integrate this checker into GCC.

---

# 1. Creating the Checker File

A new analyzer checker file was created:

```
gcc/analyzer/sm-exit.cc
```

This file implements a **state machine checker** that observes calls to the `exit()` function during static analysis.

The checker is designed to hook into analyzer callbacks such as:

```
on_call()
```

which is triggered whenever the analyzer encounters a function call in the analyzed program.

---

# 2. Registering the Checker in `sm.cc`

All analyzer checkers are registered in the file:

```
gcc/analyzer/sm.cc
```

Inside the function responsible for initializing analyzer state machines, the following line was added:

```cpp
out.push_back (make_va_list_state_machine (logger));
out.push_back (make_exit_state_machine (logger));
```

This registers the new `exit` checker so the analyzer engine can execute it during analysis.

Without this step, the checker would exist but would **never be used by the analyzer**.

---

# 3. Declaring the Checker in `sm.h`

The checker factory function must also be declared in the analyzer header file:

```
gcc/analyzer/sm.h
```

The following declaration was added near the bottom of the file:

```cpp
extern std::unique_ptr<state_machine> make_exit_state_machine (logger *);
```

This declaration allows other parts of the analyzer framework to reference the new checker.

---

# 4. Adding the Checker to the GCC Build System

In order for GCC to compile the new checker, the corresponding object file must be added to the build system.

This was done in:

```
gcc/Makefile.in
```

Inside the analyzer object list, the following entry was added:

```
analyzer/sm-exit.o
```

This ensures that the new checker source file is compiled as part of the GCC analyzer build process.

---

# 5. Rebuilding GCC

After integrating the checker, GCC was rebuilt so that the new analyzer component would be included in the compiler.

First, the checker object file was compiled:

```bash
make -j$(nproc) analyzer/sm-exit.o
```

Then the main GCC frontend was rebuilt:

```bash
make -j$(nproc) cc1
```

This produces an updated GCC compiler that includes the custom analyzer checker.

---

# 6. Testing the Checker

Example test program:

```c
#include <stdlib.h>

int main() {
    exit(1);
}
```

Compile the program using the GCC static analyzer:

```bash
gcc -fanalyzer test.c
```

During analysis, the analyzer processes the program and executes the logic defined in `sm-exit.cc`.

---

# 7. Files Modified

The following files were modified or added during this experiment:

```
gcc/analyzer/sm-exit.cc      (new analyzer checker implementation)
gcc/analyzer/sm.cc           (registered the checker)
gcc/analyzer/sm.h            (declared checker factory function)
gcc/Makefile.in              (added checker object to build system)
```

---

# 8. Learning Outcomes

Through this experiment I learned:

- How GCC static analyzer state machines work
- How new analyzer checkers are integrated into GCC
- How the analyzer observes GIMPLE-level function calls
- How GCC's build system compiles analyzer components

This experiment served as an introduction to implementing custom analyzer checkers and exploring GCC internals.

---

# Future Goals

- Implement additional analyzer checks
- Explore the analyzer's **exploded graph** implementation
- Understand the **region model** used for memory abstraction
- Contribute patches to the GCC static analyzer

---

# References

GCC Internals Documentation  
https://gcc.gnu.org/onlinedocs/gccint/

GCC Static Analyzer Documentation  
https://gcc.gnu.org/wiki/StaticAnalyzer