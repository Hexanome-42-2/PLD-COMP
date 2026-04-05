// EXPECT: error — get5 expects 0 args, got 1
int get5() {
	return 5;
}

int main() {
	return get5(1);
}
