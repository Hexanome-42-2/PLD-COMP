# Full Implementation Demo - Quick User Guide

The program in `tests/full_implementation_demo.c` is a small "code-vault" game used to demonstrate many compiler features at once.

## What It Does

- Prints a prompt: `> 6: `
- Reads exactly 6 input characters (using `getchar()`)
- Converts each character to a digit value (`'0'` -> 0, `'9'` -> 9)
- Clamps values to range `[0, 9]` (so even non-digit chars are handled)
- Computes a `score` with arithmetic + bitwise operations
- Prints:
  - `* ` or `. ` (a small marker based on a secondary bitwise score)
  - `NO` if locked, or `OK` if unlocked
- If unlocked, prints a badge letter on the next line (`A`-`Z`)

## How To Play

1. Run the compiled program.
2. Type 6 characters (usually digits like `990990`).
3. Press Enter.
4. Read the result:
   - `NO` means still locked
   - `OK` means unlocked

### Suggested input

`990990` is a known input that gives `OK`.

## Output Meaning

- `* ` or `. `:
  - Computed from a temporary/shadowed block score
  - It is just an extra indicator, not the final unlock decision alone
- `NO`:
  - Program returns immediately with `score % 256`
- `OK`:
  - Program prints a badge letter (`A`-`Z`) on the next line
  - Then returns `score % 256`
