# PLD-COMP

## Developer Documentation


### Repository Layout
- `compiler/`: compiler sources, ANTLR grammar, generated parser code, and build output.
    - `ifcc.g4`: ANTLR grammar definition for the supported C subset.
    - `main.cpp`: compiler entrypoint that reads a source file, runs the parser, and starts analysis and code generation.
    - `StaticAnalysisVisitor.cpp/h`: semantic checks for undeclared/unused variables, duplicate definitions, and function call consistency.
    - `CodeGenVisitor.cpp/h`: code generation from the parsed AST to backend IR and assembly.
    - `SymbolTable.cpp/h`: symbol table implementation for scopes, declarations, and types.
    - `IR.cpp/h`, `CFG.cpp/h`: intermediate representation and control flow / register mapping.
    - `generated/`: ANTLR4-generated C++ parser, lexer, and visitor sources.
    - `build/`: object files and dependency files produced during compilation.
    - `tests/testfiles/`: compiler test suite, organized by feature families (`base-tests`, `expression/*`, `functions`, `if`, `while`, `variables`, `scope_shadowing`).
    - `tests/ifcc-test.py`: Python test harness that compiles/runs both GCC and IFCC, then compares behavior.


### `StaticAnalysisVisitor` (semantic validation)

This pass checks if the parsed program is semantically valid before code generation.

- Main idea: detect errors that grammar alone cannot catch.
- It builds/uses symbol tables to validate declarations and scopes.
- It validates function calls (exists, argument count, return type constraints).
- It tracks variable usage to emit warnings for potentially unused variables.
- It runs in two stages for functions:
  - register signatures first (so forward calls can be checked),
  - then analyze function bodies.


### `CodeGenVisitor` (AST -> CFG/IR)

This pass transforms the parsed AST into an internal control-flow representation.

- Main idea: convert high-level constructs into basic blocks + IR instructions.
- Function definitions create a new CFG.
- `if`/`while` become explicit test/body/end blocks with jumps.
- Expressions are lowered to IR operations, with some constant folding.
- Variables are resolved to stack-slot offsets using symbol table information.
- Function calls move arguments to ABI registers and emit call IR.


### `IR` (instruction layer + asm emission)

The IR layer is the bridge between CFG logic and final assembly text.

- `BasicBlock` stores an ordered list of IR instructions.
- `IRInstr` models one low-level operation (add, cmp, call, jump, load/store, etc.).
- Backend methods (`gen_asm_x86`, `gen_asm_arm`) translate each IR op to target assembly.
- This file contains target-specific details (register conventions, instruction forms).


### `SymbolTable` (scope + stack slots)

This component stores variable metadata for each scope.

- Keeps mapping `name -> info` (offset, type, used flag).
- Supports nested scopes through parent links.
- Allocates stack offsets for locals and temporaries.
- Provides lookup helpers (local-only or recursive through parents).
- Tracks usage to support warnings and stack size calculations.


### How These 4 Parts Work Together

1. Parser builds AST from source.
2. `StaticAnalysisVisitor` validates semantics and prepares symbol/function info.
3. `CodeGenVisitor` uses that info to build CFG + IR.
4. `IR` backend generation emits final assembly for selected architecture.
5. `SymbolTable` is used across analysis + codegen for consistent scope/offset decisions.


### Makefile Targets
- `make` or `make all`: build the `ifcc` compiler.
- `make ifcc`: produce the compiler binary.
- `make clean`: remove `build/`, `generated/`, and the `ifcc` binary.
- `make gui FILE=...`: generate ANTLR parser files and launch the parse-tree GUI for a source file.
- `make test`: build `ifcc` and run the full test suite against `../testfiles`.
- `make test-file FILE=...`: run the test harness on a single test file.
- `make test-arch ARCH=DEV_ARCH_X86_64|DEV_ARCH_ARM64`: run tests for one target architecture.
- `make test-all-arch`: run tests for both x86 and ARM.


### Architecture Support
- The compiler supports Linux x86_64 and ARM64 backends.
- Architecture selection is controlled by the `ARCH` make variable: `DEV_ARCH_X86_64` or `DEV_ARCH_ARM64`.
- `CFG.cpp` contains architecture-specific register mapping and assembly output logic.
- `ifcc-test.py` can validate ARM builds using cross-compilation and `qemu-arm`.


### Testing
Use the Python test harness to execute compiler tests:
```bash
python3 tests/ifcc-test.py fileOrRepoName [--arch=(DEV_ARCH_X86_64 | DEV_ARCH_ARM64)]
```
- `fileOrRepoName` can be a single C file or a directory of test files.
- `--arch` forces the test harness to validate a specific target architecture. This parameter can be used two times to run the tests using one architecture then the other.


For more options, run:
```bash
python3 tests/ifcc-test.py --help
```


#### ARM64 Testing Setup

This testing program is made for native x86 users, and emulates the ARM64 architecture using `arm-linux-gnueabi` and `qemu`.
- `arm-linux-gnueabi` is the ARM cross-compilation toolchain used to assemble/link code for the ARM target.
- `qemu` is the emulator that runs the resulting ARM binary, so tests can execute ARM-generated code even on an x86 system.

To install these tools on a Debian-based system:
```bash
sudo apt update
sudo apt install -y gcc-arm-linux-gnueabi qemu-user
```


#### Test Files Architecture

There are 148 tests in total, covering various language features and edge cases. The tests are organized as follows:
- `tests/testfiles/base-tests/` contains basic tests for simple constructs and sanity checks.
- `tests/testfiles/functions/` tests function definitions, calls, and argument handling.
- `tests/testfiles/if/` tests `if` statements and their combinations.
- `tests/testfiles/while/` tests `while` loops and their behavior.
- `tests/testfiles/variables/` tests variable declarations, scoping, and usage.
- `tests/testfiles/` is grouped by language feature so regressions are easier to locate.
- The `expression/` subtree is split by operator family:
  - `expression/arithmetic` contains tests for `+`, `-`, `*`, `/`, and `%`.
  - `expression/bitwise` contains tests for `&`, `|`, `^`
  - `expression/comparison` contains tests for `==`, `!=`, `<`, `>`
  - `expression/affectation` contains tests for `=`
  - `expression/combine_ops` contains tests that combine multiple operator types in one expression, to check precedence and associativity.
- `scope_shadowing/` contains block-scope and name-resolution corner cases.
- Test naming is `<NNN>_<short_description>.c`, where numbering is local to each directory.


#### Known Failing Tests (Current Limitations)

The following tests are expected to fail with the current implementation:

- `tests/testfiles/functions-020_include_stdlib`
  - Reason: only a small subset of external functions is registered from includes (`putchar` and `getchar`).

- `tests/testfiles/functions-021_over_6_args`
  - Reason on x86: backend currently passes arguments only through registers, without stack argument passing fallback.
  - x86 path supports only the first 6 integer argument registers, so extra arguments are dropped.
  - On ARM (current backend settings), this test passes because there are 8 registers to store parameters by default.

- `tests/testfiles/functions-022_over_8_args`
  - Reason: same limitation as above but beyond both backend register windows, so argument passing breaks on both x86 and ARM.

- `tests/testfiles/scope_shadowing-010_shadowing_uninitialized`
  - Source pattern: `int x = x + 1;` inside an inner block with shadowing.
  - This reads a shadowed variable during its own initialization, and IFCC currently does not match GCC behavior on this edge case.
  - Result is a runtime mismatch (`different results at execution`).


