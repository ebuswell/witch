/*
 * test_witch_h.c
 *
 * Copyright 2014 Evan Buswell
 *
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2.
 * 
 * Atomic Kit is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "test.h"
#include "witch.h"

static int test1_f(int a, int b) {
	return a + 2 * b;
}

static int test2_f(int a, int b) {
	return a + 4 * b;
}

int (*testfp_c1)(int b);
int (*testfp_c2)(int a);

struct afptr fptr1;
struct afptr fptr2;

arcp_t dispatch;
int (*testfp_d)(int a, int b);

/*************************/

static void test_papf_create() {
	CHECKPOINT();
	testfp_c1 = (int (*)(int)) papf_create(test1_f,
					       "%i$, %i -> %i", 1);
	ASSERT(testfp_c1 != NULL);
	CHECKPOINT();
	testfp_c2 = (int (*)(int)) papf_create(test1_f,
					       "%i, %i$ -> %i", 2);
	ASSERT(testfp_c2 != NULL);
}

static void test_afptr_init() {
	afptr_init(&fptr1, test1_f, NULL);
	afptr_init(&fptr2, test2_f, NULL);
	ASSERT(afptr_fptr(&fptr1) == test1_f);
	ASSERT(afptr_fptr(&fptr2) == test2_f);
}

/*************************/

static void test_papf_create_fixture(void (*test)()) {
	CHECKPOINT();
	testfp_c1 = (int (*)(int)) papf_create(test1_f,
					       "%i$, %i -> %i", 1);
	CHECKPOINT();
	testfp_c2 = (int (*)(int)) papf_create(test1_f,
					       "%i, %i$ -> %i", 2);
	test();
}

static void test_papf_invoke() {
	CHECKPOINT();
	ASSERT(testfp_c1(1) == 3);
	CHECKPOINT();
	ASSERT(testfp_c2(1) == 5);
}

static void test_papf_free() {
	CHECKPOINT();
	papf_free(testfp_c1);
	CHECKPOINT();
	papf_free(testfp_c2);
}

/*************************/

static void test_afptr_init_fixture(void (*test)()) {
	CHECKPOINT();
	afptr_init(&fptr1, test1_f, NULL);
	CHECKPOINT();
	afptr_init(&fptr2, test2_f, NULL);
	test();
}

static void test_afptr_fptr() {
	CHECKPOINT();
	ASSERT(afptr_fptr(&fptr1) == test1_f);
	CHECKPOINT();
	ASSERT(afptr_fptr(&fptr2) == test2_f);
}

static void test_afptr_dispatch_create() {
	CHECKPOINT();
	arcp_init(&dispatch, &fptr1);
	CHECKPOINT();
	testfp_d = (int (*)(int, int)) afptr_dispatch_create(&dispatch,
							     "%i, %i -> %i");
	ASSERT(testfp_d != NULL);
}

/*************************/

static void test_afptr_dispatch_create_fixture(void (*test)()) {
	CHECKPOINT();
	afptr_init(&fptr1, test1_f, NULL);
	CHECKPOINT();
	afptr_init(&fptr2, test2_f, NULL);
	arcp_init(&dispatch, &fptr1);
	CHECKPOINT();
	testfp_d = (int (*)(int, int)) afptr_dispatch_create(&dispatch,
							     "%i, %i -> %i");
	test();
}

static void test_afptr_dispatch_invoke() {
	CHECKPOINT();
	ASSERT(testfp_d(1, 2) == 5);
	arcp_store(&dispatch, &fptr2);
	CHECKPOINT();
	ASSERT(testfp_d(1, 2) == 9);
}

static void test_afptr_dispatch_free() {
	CHECKPOINT();
	afptr_dispatch_free(testfp_d);
}

/*************************/

int run_witch_h_test_suite() {
	int r;
	void (*witch_uninit_tests[])() = { test_papf_create, test_afptr_init,
					   NULL };
	char *witch_uninit_test_names[] = { "papf_create", "afptr_init",
					    NULL };

	void (*papf_create_tests[])() = { test_papf_invoke, test_papf_free,
					  NULL };
	char *papf_create_test_names[] = { "papf_invoke", "papf_free",
					   NULL };
	void (*afptr_init_tests[])() = { test_afptr_fptr, test_afptr_dispatch_create,
					 NULL };
	char *afptr_init_test_names[] = { "afptr_fptr", "afptr_dispatch_create",
					  NULL };
	void (*afptr_dispatch_create_tests[])() = { test_afptr_dispatch_invoke,
						    test_afptr_dispatch_free,
						    NULL };
	char *afptr_dispatch_create_test_names[] = { "afptr_dispatch_invoke",
						     "afptr_dispatch_free",
						     NULL };

	r = run_test_suite(NULL,
			   witch_uninit_test_names,
			   witch_uninit_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_papf_create_fixture,
			   papf_create_test_names,
			   papf_create_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_afptr_init_fixture,
			   afptr_init_test_names,
			   afptr_init_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_afptr_dispatch_create_fixture,
			   afptr_dispatch_create_test_names,
			   afptr_dispatch_create_tests);
	if (r != 0) {
		return r;
	}

	return 0;
}
