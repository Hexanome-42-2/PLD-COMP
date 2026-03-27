int foo() {
	int a;
	a = 200;
	return a;
}

int main() {
	int a;
	a = 100;
	return foo() + a;
}
