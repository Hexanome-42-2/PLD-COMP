// EXPECT: 2

int foo(int a) {
    a = a + 1;
    {
        int a = a - 1;
    }
    return a;
}


int main() {
	return foo(1);
}