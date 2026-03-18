// EXPECT: error — void function used in expression
void logv(int v) {
	return;
}

int main() {
	return logv(3);
}
