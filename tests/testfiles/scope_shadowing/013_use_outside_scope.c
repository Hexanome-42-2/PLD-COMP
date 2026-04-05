// EXPECT: 3
int main() {
    int a = 0;
    {
        int b = 3;
    }

    return b;
}