// EXPECT: 1
int main() {
    int a = 1;
    {
        int a = 4;
    }
	return a;
}