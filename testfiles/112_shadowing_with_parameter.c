// EXPECT: 5
int foo(int a) {
    {
        int a = 5;
        return a;
    }
    return a;
}

int main() {
    return foo(99);
}