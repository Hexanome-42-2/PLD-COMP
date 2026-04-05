// 990990 gives an OK
#include <stdio.h>

int mix6(int a, int b, int c, int d, int e, int f) {
    int score = (a + b * 3 - c) % 97;          // +, -, *, %
    score = (score ^ d) + (e & 31) - (f | 7);  // ^, &, |
    if (score < 0) return -score;              // return n'importe où + unaire -
    return +score;                             // unaire +
}

void showBadge(int a, int b, int c, int d, int e, int f) {
    int letter = mix6(a, b, c, d, e, f) % 26;
    putchar('A' + letter); // constante caractère
    putchar('\n');
}

int main() {
    putchar('>');
    putchar(' ');
    putchar('6');
    putchar(':');
    putchar(' ');

    int i = 0;
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0; // init à la déclaration

    while (i < 6) {
        int ch = getchar(); // déclaration n'importe où
        if (ch == -1) {     // comparaison ==
            putchar('E');
            putchar('O');
            putchar('F');
            putchar('\n');
            return 1;       // return n'importe où
        }

        // affectation avec retour de valeur
        int v;
        if ((v = ch - '0') < 0) v = 0; // comparaison <
        if (v > 9) v = 9;              // comparaison >

        if (i == 0) a = v;
        else if (i == 1) b = v;
        else if (i == 2) c = v;
        else if (i == 3) d = v;
        else if (i == 4) e = v;
        else f = v;

        i = i + 1;
    }

    int score = mix6(a, b, c, d, e, f);

    {   // bloc + shadowing
        int score = (a | b) ^ (c & d);
        if (score != 0) putchar('*');  // comparaison !=
        else putchar('.');
        putchar(' ');
    }

    int unlocked = 0;
    if (!(score < 30)) unlocked = 1;  // unaire !
    if (score > 120) unlocked = 1;

    if (unlocked == 0) {
        putchar('N'); putchar('O'); putchar('\n');
        return score % 256;
    } else {
        putchar('O'); putchar('K'); putchar('\n');
    }

    showBadge(a, b, c, d, e, f); // fonction void à 6 params
    return score % 256;
}
