// EXPECT-RUN: 5
int fact(int n);

int main() {
    return fact(5);
}

int fact(int n) {
    return n;
}