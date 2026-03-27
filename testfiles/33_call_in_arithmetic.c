// EXPECT: ok — function call used in arithmetic expression
int get3() {
	return 3;
}

int main() {
	int x = get3() + get3();
	return x;
}
