/**
 * @file witch.h
 */
/*
 * witch.h
 * 
 * Copyright 2014 Evan Buswell
 * 
 * This file is part of libwitch.
 * 
 * libwitch is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2.
 * 
 * libwitch is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with libwitch.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WITCH_H
#define WITCH_H 1

#include <atomickit/rcp.h>

/**
 * Create a partial function application.
 *
 * @param fptr the function to create a partial application of.
 * @param signature the signature describing the partial application.
 * @param ... the variables to apply to the function in accord with the
 * signature.
 *
 * @returns the partial function application.
 */
void *papf_create(void *fptr, char *signature, ...);

/**
 * Free a previously created partial function application.
 *
 * @param f the partial function application to free.
 */
void papf_free(void *f);

/**
 * Atomic function pointer.
 */
struct afptr {
	struct arcp_region;
	void *fptr; /**< The function pointer the atomic function pointer
		     *   points to. */
};

/**
 * Initialize atomic function pointer.
 *
 * @param afptr the atomic function pointer to initialize.
 * @param fptr the function pointer the atomic function pointer points to.
 * @param destroy destruction function for the atomic function pointer.
 */
void afptr_init(struct afptr *afptr, void *fptr,
		void (*destroy)(struct afptr *));

/**
 * Get the function pointer for an atomic function pointer.
 *
 * @param afptr the atomic function pointer to get the function pointer from.
 *
 * @returns the function pointer.
 */
static inline void *afptr_fptr(struct afptr *afptr) {
	return afptr->fptr;
}

/**
 * Create a function pointer dispatch function.
 *
 * A function pointer dispatch relays calls to the atomic function pointer
 * held by an arcp pointer.
 *
 * @param arcp the arcp pointer which will hold the atomic function pointer to
 * dispatch to.
 * @param signature the signature of the atomic function pointer.
 *
 * @returns a function pointer dispatch to the function held by the arcp
 * pointer.
 */
void *afptr_dispatch_create(arcp_t *arcp, char *signature);

/**
 * Free the function pointer dispatch.
 *
 * @param f the function pointer dispatch to free.
 */
void afptr_dispatch_free(void *f);

#endif /* ! WITCH_H */
