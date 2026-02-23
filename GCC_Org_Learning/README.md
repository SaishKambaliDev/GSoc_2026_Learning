`Compiler` A compiler like GNU Compiler collection (GCC) is simply a program that converts a c/cpp code into a machine code, it undergoes through various stages before converting it.

`Intermediate Representation` GCC does not directly work on the user provided c/cpp code instead it converts that into a even simmpler internal language which is `GIMPLE` which makes analysis and optimmization easier.

### So basically typical flow is
```
c/cpp source code
      |
      v
Gimple (IR)
      |
      v
Optimization and analysis of code
      |
      v
Machine Code

### `Need of IR`  
1. C syntax is complicated
2. Compiler Analysis is easier on simple code

# `Optimizations`  
Optimizers rewrite GIMPLE to produce a even faster machine code.  
So, basically GCC Optimizations passes transform GIMPLE to improve its performance and to remove redundant computations before generating the machine code.  

1. Remove useless code
```c
x = 5;
x = 6;  // Compiler keeps x = 6;
```  
2. Constant Folding
```c
int a = 2 + 3; // it becomes x = 6;
```  
3. Dead Code Elimination
```c
if(0)
{
    function();
}  //this part is eliminated
```  
4. Inline small functions
```c
add(a,b)   //compiler may replace with a + b which inturns reduces function call overhead
```

# Static Single Assignment (SSA)
SSA means each variable is assigned only once internally which helps the compiler track values precisely.
It can easily see:
- where values come from,
- data flow
- which value is used where
This helps in bug detection and optimization easier.

```c
x = 1;
x = 4;

//Compiler rewrites it as
x_1 = 1;
x_2 = 4;
```  

# `Static Analyzer`
`Static` means without running the program.  
`analyzer` means looking for bugs.  

GCC's static analyzer `-fanalyzer` sybolically explores programs paths to detect bugs like leaks and use after free without executing the code.  

About [Gimple](https://gcc.gnu.org/onlinedocs/gccint/GIMPLE.html)

# 1. Assignment in GIMPLE
```c
a = b + c;

//GIMPLE
_1 = b_1 + c_1;  //GIMPLE is three address code
                 //each line usually has at most 
                 // result = operand1 op operand2
a_1 = _1;

//gimple prefer one operation per line and no complex nesting
// Constant assignment

x_1 = 10;
```  
# 2. Conditional (if)
```c
if(x > 0)
    y = 1;

//Gimple

_1 = x_2 > 0;
if(_1 != 0)
    goto <bb 3>;
else
    goto <bb 4>;  //explicit jumps
```   
# 3. Functional Calls
```c
z = foo(x);

//GIMPLE
_1 = foo(x_2);
z_3 = _1;     // or foo(x_2);

//return 
return x; // return x_2; (GIMPLE)
```  

## Basic Blocks
GIMPLE code is divided into basic blocks.
```c
<bb 2>: //code executes top to bottom, no jumos inside, ends with jump or return.
```

## Control Flow Graphs
All basic blocks are connected by arrows (jumps).

`C Program example`  
```c
int main()
{
    int x = 5;
    if(x > 0)
        x = 10;
    return x;
}
```
`Gimple Code example`  
```c
<bb 2>:
x_1 = 5;
_2 = x_1 > 0;
if(_2 != 0)
    goto <bb 3>;
else
    goto <bb 4>;

<bb 3>:
x_3 = 10;
goto <bb 4>;

<bb 4>:
x_4 = PHI <x_1(2), x_3(3)>
return x_4;
```  

Here `_1` is a compiler created temporary.
SSA version numbers can start with any number.

A `PHI` node associates each incomming SSA value with its originating basic block; when control reaches the merge point/PHI node, the value corresponding to the executed block is selected.

```c
x_3 = PHI <x_1(2), x_2(3)>;

//if came from block 2 then use x_1
//if came from block 3 then use x_2