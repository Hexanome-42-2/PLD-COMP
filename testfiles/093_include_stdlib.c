// This tests the include of a header other than stdio.h. Returns error due to the compiler not actually analyzing the included header.
#include <stdlib.h>

int main() {
   return abs(-1);
}