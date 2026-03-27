// EXPECT: error — void function used in expression (assign)
void donothing() {
	return;
}

int main() {
	int a = donothing();
	return 0;
}
