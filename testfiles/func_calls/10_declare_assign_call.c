// EXPECT: ok — declare + assign from function call
int get42() {
	return 42;
}

int main() {
	int x = get42();
	return x;
}
