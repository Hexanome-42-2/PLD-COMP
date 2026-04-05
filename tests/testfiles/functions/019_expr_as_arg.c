int foo(int x) {
    return x;
}

int main() {
    int a = 1;
    return foo(a = 5);
}