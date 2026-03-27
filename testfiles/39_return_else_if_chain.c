// Test: return in else-if chain
int main() {
    int x = 0;
    if (x == 1) {
        return 1;
    } else if (x == 0) {
        return 2;
    } else {
        return 3;
    }
    return 4; // unreachable
}
