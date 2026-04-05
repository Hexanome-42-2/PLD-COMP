// EXPECT: syntax error
int foo(int a = 2, int b) {
    return a + b;
}

int main() {
    return foo(1, 2);
}