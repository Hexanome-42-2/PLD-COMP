
int main() {
    int a = 2;
    {
    a = a + 3;
    int a = 4;
    }
    return a;
}