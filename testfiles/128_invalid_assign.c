// EXPECT-RUN: Syntax error
int main() {
    int a = 0;
    a+1 = 2;

    return a;
}