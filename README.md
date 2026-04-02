# PLD-COMP

## User Documentation

### Project Description
We implemented our own compiler using ANTLR4 that compiles a subset of the C programming language for Linux x86 and ARM architectures.

### Prerequisites

#### For Windows Users (WSL)
```bash
sudo apt update
sudo apt upgrade
sudo apt install cmake default-jdk unzip
sudo apt remove antlr4 antlr4-runtime-dev
```

#### For Windows (WSL) and Linux Users
```bash
mkdir ~/antlr4-install
cd ~/antlr4-install
wget https://www.antlr.org/download/antlr-4.13.2-complete.jar
wget https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip
unzip antlr4-cpp-runtime-4.13.2-source.zip
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=~/antlr4-install/ ..
make -j8
make install
```

ANTLR4 is now installed in the `~/antlr4-install` directory.

Create a `config.mk` file in the `compiler` directory with the following content:
```
ANTLRJAR=/home/$(USER)/antlr4-install/antlr-4.13.2-complete.jar
ANTLRINC=/home/$(USER)/antlr4-install/include/antlr4-runtime/
ANTLRLIB=/home/$(USER)/antlr4-install/lib/libantlr4-runtime.a
```

### Usage

#### Compiling a C File
```bash
cd compiler
make                                      # Compile the project
./ifcc sourceFile.c > destinationFile.s   # Compile the C source file to assembly
gcc -o execName destinationFile.s         # Assemble and link to executable
./execName                                # Run the program
```
These commands can be used to compile and execute any program written using the C subset described hereafter.

### Supported C Subset

#### Data Types
- `int` (32-bit signed integers)
- `char` (character constants, stored as integers)

#### Constants
- Integer constants: `42`, `-17`
- Character constants: `'A'`, `'\n'`, `'\\'`

#### Operators

##### Arithmetic Operators
- Addition: `+`
- Subtraction: `-`
- Multiplication: `*`
- Division: `/`
- Modulo: `%`

##### Bitwise Operators
- Bitwise AND: `&`
- Bitwise OR: `|`
- Bitwise XOR: `^`

##### Comparison Operators
- Equal to: `==`
- Not equal to: `!=`
- Less than: `<`
- Greater than: `>`

##### Unary Operators
- Unary plus: `+x`
- Unary minus: `-x`
- Logical NOT: `!x`

#### Variable statements
- Variable declarations: `int x;`
- Variable assignments: `x = 42;` (anywhere in the code)
- Variable initializations: `int x = 42;`
- Multiple declarations: `int a, b, c;`
- Assignments can be used as expressions: `int result = (x = 5) + 3;`

#### Statements
- Expression statements: `x + 5;`
- Return statements: `return 42;` or `return;` (anywhere in the code)
- Function call statements: `foo('A');`
- Block statements: `{ int x; x = 1; }`

#### Control Flow

##### Conditional Statements
```c
if (condition) {
    // statements
} else {
    // statements
}
```

##### Loops
```c
while (condition) {
    // statements
}
```

#### Functions
- Function definitions with `int` or `void` return types
- Up to 6 parameters (7 on an ARM architecture)
- Function calls with argument passing
- Recursive functions supported

Example:
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    return add(5, 3);
}
```

#### Built-in Functions
- `putchar(int c)`: Output a character
- `getchar()`: Read a character from input

#### Preprocessing Directives
- `#include` directive (partial support)

#### Static Analysis
The compiler detects:
- Undeclared variables
- Unused variables
- Multiple variable definitions
