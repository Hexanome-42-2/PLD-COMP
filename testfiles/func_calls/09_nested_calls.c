// EXPECT: ok — nested function calls as arguments
int h() {
	return 3;
}

int add(int a, int b) {
	return a + b;
}

int main() {
	int result = add(h(), h());
	return result;
}
