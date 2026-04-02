// This test checks that a function that is declared but not defined is correctly flagged as an error.
int foo(); // Declaration without definition

int main() {
    foo(); // Error because foo is not defined
    return 0;
}