// EXPECT-RUN: 2
int main() {
    int a;
    int b = a = 5 - (a = 3);
    return b;
}