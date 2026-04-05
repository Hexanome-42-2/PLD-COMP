// EXPECT: static analysis error
int foo() {
	return 1;
}

int foo() {
    return 1;
}

int main() {
    return 0;
}