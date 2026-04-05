int main() {
    int x = 1;
    {
        int x = x + 1;
        return x;
    }
}