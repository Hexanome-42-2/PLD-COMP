// EXPECT: 4
int main() {
    int a;
    {
        {
            {
                a = 4;
            }
        }
    }
	return a;
}