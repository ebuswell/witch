#include <stdio.h>
#include <stdlib.h>
#include "witch.h"

static int test_f(int a, int b) {
    return a + 2 * b;
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int (*testf1)(int b);
    int (*testf2)(int a);

    testf1 = (int (*)(int)) curry(test_f, "%i$, %i -> %i", 1);
    if(testf1 == NULL) {
	fprintf(stderr, "Received NULL result on first curry attempt\n");
	exit(EXIT_FAILURE);
    }
    testf2 = (int (*)(int)) curry(test_f, "%i, %i$ -> %i", 2);
    if(testf2 == NULL) {
	fprintf(stderr, "Received NULL result on second curry attempt\n");
	exit(EXIT_FAILURE);
    }

    int r;
    if((r = testf1(1)) != 3) {
	fprintf(stderr, "Instead of 3, 1 + 2 * (1) was %d\n", r);
	exit(EXIT_FAILURE);
    }
    if((r = testf2(1)) != 5) {
	fprintf(stderr, "Instead of 5, (1) + 2 * 2 was %d\n", r);
	exit(EXIT_FAILURE);
    }

    free_curried_function(testf1);
    free_curried_function(testf2);
    exit(EXIT_SUCCESS);
}
