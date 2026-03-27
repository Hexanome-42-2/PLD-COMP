// Test: return in a loop
int main() {
    int i = 0;
    while (i < 5) {
        return i;
        i = i + 1; // unreachable
    }
    return 99; // unreachable
}
