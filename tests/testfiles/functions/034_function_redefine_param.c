// EXPECT: static analysis error
int foo(int a) {
    int a = 0;
    return a;
}

int main() {
    return foo(1);
}