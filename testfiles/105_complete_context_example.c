// EXPECT: 4
int add(int a, int b) {
	int c;
    {c = a + b;}
    return c;
}

int main() {
    int g = 1, h;
    {h=(g=2);}
	return add(h, g);
}