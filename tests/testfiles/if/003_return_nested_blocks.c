// Test: return in nested blocks
int main() {
    int x = 10;
    if (x > 5) {
        if (x < 20) {
            return 1;
        }
        return 2;
    }
    return 3;
}
