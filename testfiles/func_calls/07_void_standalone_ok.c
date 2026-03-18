// EXPECT: ok — void function as standalone statement is valid
void donothing() {
	return;
}

int main() {
	donothing();
	return 0;
}
