int foo() {
	int a;
	a = 63;
	return a;
}

int foo() {
	int a;
	a = foo();
	return a;
}

int main() {
	int a;
	a = foo();
	return a;
}
