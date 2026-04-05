// EXPECT: 3
int main() {
    int a = 0;
    {
        int a = 3;

        return a;
    }
}