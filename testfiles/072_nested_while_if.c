int main() {
    int a;
    a = 0;

    while (a < 8) {
        while (a < 4) {
            if (a == 1) {
                a = a + 3;
            }
            a = a + 1;
        }
        a = a + 1;
    }

    return a;
}