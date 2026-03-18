// EXPECT: ok — assign from function call
int get7() {
	return 7;
}

int main() {
	int x;
	x = get7();
	return x;
}
