#include <stdio.h>
#include <stdlib.h>
#include "witch.h"

static int test1_f(int a, int b) {
    return a + 2 * b;
}

static int test2_f(int a, int b) {
    return a + 4 * b;
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int r;
    int (*testfp_c1)(int b);
    int (*testfp_c2)(int a);
    int (*testfp_d)(int a, int b);
    arcp_t dispatch;
    struct afptr fptr1;
    struct afptr fptr2;

    testfp_c1 = (int (*)(int)) papf_create(test1_f, "%i$, %i -> %i", 1);
    if(testfp_c1 == NULL) {
	fprintf(stderr, "Received NULL result on first curry attempt\n");
	exit(EXIT_FAILURE);
    }
    testfp_c2 = (int (*)(int)) papf_create(test1_f, "%i, %i$ -> %i", 2);
    if(testfp_c2 == NULL) {
	fprintf(stderr, "Received NULL result on second curry attempt\n");
	exit(EXIT_FAILURE);
    }

    if((r = testfp_c1(1)) != 3) {
	fprintf(stderr, "Instead of 3, 1 + 2 * (1) was %d\n", r);
	exit(EXIT_FAILURE);
    }
    if((r = testfp_c2(1)) != 5) {
	fprintf(stderr, "Instead of 5, (1) + 2 * 2 was %d\n", r);
	exit(EXIT_FAILURE);
    }

    papf_free(testfp_c1);
    papf_free(testfp_c2);

    afptr_init(&fptr1, test1_f, NULL);
    afptr_init(&fptr2, test2_f, NULL);

    arcp_init(&dispatch, &fptr1);

    testfp_d = (int (*)(int, int)) afptr_dispatch_create(&dispatch, "%i, %i -> %i");
    if(testfp_d == NULL) {
	fprintf(stderr, "Received NULL result on dispatch function creation\n");
	exit(EXIT_FAILURE);
    }

    if((r = testfp_d(1, 2)) != 5) {
	fprintf(stderr, "Instead of 5, (1) + 2 * (2) was %d\n", r);
	exit(EXIT_FAILURE);
    }

    arcp_store(&dispatch, &fptr2);

    if((r = testfp_d(1, 2)) != 9) {
	fprintf(stderr, "Instead of 5, (1) + 4 * (2) was %d\n", r);
	exit(EXIT_FAILURE);
    }

    arcp_store(&dispatch, NULL);
    arcp_release(&fptr1);
    arcp_release(&fptr2);
    afptr_dispatch_free(testfp_d);

    exit(EXIT_SUCCESS);
}
