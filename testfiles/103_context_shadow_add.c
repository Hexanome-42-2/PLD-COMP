// EXPECT: 4
int main() {
    int a = 1, b = 2;
    {
        int a = 3;
        a = a + b;
        b = a - b - 1;
    }
	return a+b;
}