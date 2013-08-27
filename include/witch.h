/*
 * libwitch.h
 * 
 * Copyright 2013 Evan Buswell
 * 
 * This file is part of libwitch.
 * 
 * libwitch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 2.
 * 
 * libwitch is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with libwitch.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WITCH_H
#define WITCH_H 1

#include <atomickit/atomic-rcp.h>

void free_curried_function(void *f);

void *curry(void *fptr, char *signature, ...);

struct afptr {
    struct arcp_region;
    void *fptr;
};

static inline void afptr_init(struct afptr *afptr, void *fptr, void (*destroy)(struct afptr *)) {
    afptr->fptr = fptr;
    arcp_region_init(fptr, (void (*)(struct arcp_region *)) destroy);
}

static inline void *afptr_fptr(struct afptr *afptr) {
    return afptr->fptr;
}

void *afptr_create_dispatch_f(arcp_t *arcp, char *signature);

void afptr_free_dispatch_f(void *f);

#endif /* ! WITCH_H */
