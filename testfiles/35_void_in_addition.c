// EXPECT: error — void function used in arithmetic expression
void noop() {
	return;
}

int main() {
	int x = 1 + noop();
	return x;
}
