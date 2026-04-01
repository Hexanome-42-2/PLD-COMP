// This tests the inclusion of the standard library header <stdio.h> and the usage of the dynamically linked getchar function without blocking on input.
#include <stdio.h>
#include <string.h>

int main(void) {
    /* Create a memory stream with the test data */
    const char *data = "abc\n";
    FILE *mem = fmemopen((void *)data, strlen(data), "r");

    /* Save the original stdin and replace it */
    FILE *orig = stdin;
    stdin = mem;

    /* Test getchar() */
    int c1 = getchar();  /* 'a' */
    int c2 = getchar();  /* 'b' */
    int c3 = getchar();  /* 'c' */
    int c4 = getchar();  /* '\n' */
    int c5 = getchar();  /* EOF */

    /* Restore stdin and clean up */
    stdin = orig;
    fclose(mem);

    /* Verify results… */
    putchar(c1);  /* Should print 'a' */
    putchar(c2);  /* Should print 'b' */
    putchar(c3);  /* Should print 'c' */
    putchar(c4);  /* Should print '\n' */
    return 0;
}