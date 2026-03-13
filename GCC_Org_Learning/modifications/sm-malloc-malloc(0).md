# Experiment: Detecting `malloc(0)` in GCC Static Analyzer

While exploring how the GCC static analyzer works internally, I experimented with modifying the existing `malloc` checker to detect calls where `malloc()` is used with a size of **0**.

This change was made inside the file:

```
gcc/analyzer/sm-malloc.cc
```

The goal of this experiment was not to redesign the entire checker but to understand how the analyzer inspects function calls and emits warnings.

---

# Custom Diagnostic for `malloc(0)`

To produce a warning message when `malloc(0)` is detected, I first created a diagnostic class called `malloc_zero_diagnostic`.

```cpp
class malloc_zero_diagnostic : public pending_diagnostic
{
public:
  malloc_zero_diagnostic () {}

  const char *get_kind () const final override 
  { 
    return "malloc-zero-diagnostic"; 
  }

  int get_controlling_option () const final override
  {
    return 0;
  }

  bool subclass_equal_p (const pending_diagnostic &) const final override
  {
    return true;
  }

  bool operator== (const malloc_zero_diagnostic &) const
  {
    return true;
  }

  bool emit (diagnostic_emission_context &ctxt) final override
  {
    return ctxt.warn ("call to %<malloc%> with a zero size");
  }
};
```

### What this class does

This class defines a **custom analyzer warning**.

When the analyzer detects `malloc(0)`, this diagnostic is triggered and GCC prints a warning like:

```
call to 'malloc' with a zero size
```

The class inherits from `pending_diagnostic`, which is the mechanism the analyzer uses to represent warnings before they are actually emitted.

---

# Adding the Detection Logic

After defining the diagnostic, I added logic to detect when `malloc()` is called with `0`.

```cpp
if (known_allocator_p (callee_fndecl, call))
{
  if (is_named_call_p (callee_fndecl, "malloc", call, 1))
  {
    tree arg0 = gimple_call_arg (&call, 0);

    if (TREE_CODE (arg0) == INTEGER_CST && integer_zerop (arg0))
    {
      sm_ctxt.warn (NULL_TREE,
                    std::make_unique<malloc_zero_diagnostic> ());
    }
  }

  on_allocator_call (sm_ctxt, call, &m_free);
  return true;
}
```

---

# Understanding the Logic Step by Step

### 1. Check if the function is a memory allocator

```
known_allocator_p(callee_fndecl, call)
```

This function checks whether the current function call is a **known memory allocator**, such as:

- `malloc`
- `calloc`
- `realloc`

If the function is not an allocator, the analyzer simply continues without doing anything special.

---

### 2. Confirm that the function is `malloc`

```
is_named_call_p(callee_fndecl, "malloc", call, 1)
```

This ensures that the function being called is specifically `malloc`.

The final parameter (`1`) indicates that `malloc` should have **one argument**.

---

### 3. Retrieve the argument passed to `malloc`

```
tree arg0 = gimple_call_arg(&call, 0);
```

This line retrieves the first argument of the function call.

For example, if the program contains:

```c
malloc(10);
```

then `arg0` represents the value `10`.

---

### 4. Check if the argument is `0`

```
TREE_CODE(arg0) == INTEGER_CST
```

This checks whether the argument is a constant integer value.

Then:

```
integer_zerop(arg0)
```

checks whether that integer value is equal to `0`.

If both conditions are true, the analyzer has detected:

```
malloc(0)
```

---

### 5. Emit the warning

```
sm_ctxt.warn(NULL_TREE,
             std::make_unique<malloc_zero_diagnostic>());
```

This line triggers the custom diagnostic defined earlier.

As a result, the analyzer produces the warning message defined in the diagnostic class.

---

### 6. Continue the normal analyzer logic

```
on_allocator_call(sm_ctxt, call, &m_free);
```

After emitting the warning, the analyzer continues with its normal `malloc` tracking logic.

This ensures that the modification does not interfere with the existing state machine behavior.

---

# Example Test Program

```c
#include <stdlib.h>

int main() {
    malloc(0);
}
```

When this program is compiled using the GCC static analyzer:

```
gcc -fanalyzer test.c
```

the analyzer detects the call and emits the warning:

```
call to 'malloc' with a zero size
```

---
