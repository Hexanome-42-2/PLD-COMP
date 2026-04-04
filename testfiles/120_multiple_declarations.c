// EXPECT-RUN: 10
int main() {
    int a = 3;
    int b = 6;
    int c = b = a + 2;

    return b + c;
}