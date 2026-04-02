// This tests the inclusion of the standard library header <stdio.h> and the usage of the dynamically linked putchar function.
#include <stdio.h>
int main() {
    putchar(65); // ASCII for 'A'
    putchar(10); // ASCII for newline
    return 0;
}