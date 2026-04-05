// EXPECT: 1
int foo() {
    return 0;
}

int foo(int a) {
    return a;
}

int main() {
    return foo(1);
}