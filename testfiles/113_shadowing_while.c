// EXPECT: 10
int main() {
    int a = 0;
    int i = 0;
    while (i < 5) {
        int a = 2;
        i = i + 1;
    }
    return a + i;
}