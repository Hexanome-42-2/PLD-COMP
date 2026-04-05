// EXPECT: error — add expects 2 args, got 1
int add(int a, int b) {
	return a + b;
}

int main() {
	return add(1);
}
