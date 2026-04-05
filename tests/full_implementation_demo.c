// 990990 gives an OK
#include <stdio.h>

int mix6(int a, int b, int c, int d, int e, int f) {
    int score = (a + b * 3 - c) % 97;          // +, -, *, %
    score = (score ^ d) + (e & 31) - (f | 7);  // ^, &, |
    if (score < 0) return -score;              // return anywhere & unary -
    return +score;                             // unary +
}

void showBadge(int a, int b, int c, int d, int e, int f) {
    int letter = mix6(a, b, c, d, e, f) % 26;
    putchar('A' + letter); // const char
    putchar('\n');
}

int main() {
    putchar('>');
    putchar(' ');
    putchar('6');
    putchar(':');
    putchar(' ');

    int i = 0;
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0; // decl & init

    while (i < 6) {
        int ch = getchar();
        if (ch == -1) {     // comparison ==
            putchar('E');
            putchar('O');
            putchar('F');
            putchar('\n');
            return 1;
        }

        // affectation return value
        int v;
        if ((v = ch - '0') < 0) v = 0; // comparison <
        if (v > 9) v = 9;              // comparison >

        if (i == 0) a = v;
        else if (i == 1) b = v;
        else if (i == 2) c = v;
        else if (i == 3) d = v;
        else if (i == 4) e = v;
        else f = v;

        i = i + 1;
    }

    int score = mix6(a, b, c, d, e, f);

    {   // bloc & shadowing
        int score = (a | b) ^ (c & d);
        if (score != 0) putchar('*');  // comparison !=
        else putchar('.');
        putchar(' ');
    }

    int unlocked = 0;
    if (!(score < 30)) unlocked = 1;  // unary !
    if (score > 120) unlocked = 1;

    if (unlocked == 0) {
        putchar('N'); putchar('O'); putchar('\n');
        return score % 256;
    } else {
        putchar('O'); putchar('K'); putchar('\n');
    }

    showBadge(a, b, c, d, e, f); // void function with 6 params
    return score % 256;
}
