#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <stdarg.h>
#include <alloca.h>
#include <string.h>
#include <errno.h>
#include <ffi.h>
#include <atomickit/atomic-malloc.h>
#include "witch.h"

#include <stdio.h>

union scanfvalue {
    uintmax_t _uintmax; /* Put this first so initialization is easy. */
    unsigned char _uchar;
    signed char _schar;
    unsigned short _ushort;
    short _short;
    unsigned int _uint;
    int _int;
    unsigned long _ulong;
    long _long;
    unsigned long long _ulonglong;
    long long _longlong;
    intmax_t _intmax;
    ptrdiff_t _ptrdiff;
    size_t _size;
    float _float;
    double _double;
    long double _longdouble;
    void *_ptr;
};

struct stashed_arg {
    union scanfvalue value;
    unsigned int argnum;
};

#define TYPE_SIZE 0
#define TYPE_TYPE 1
#define TYPE_STASH 2

static inline ffi_type *scanft2ffit(char *type) {
    switch(type[TYPE_SIZE]) {
    case 'H':
	if(type[TYPE_TYPE] == 'u') {
	    return &ffi_type_uchar;
	} else {
	    return &ffi_type_schar;
	}
    case 'h':
	if(type[TYPE_TYPE] == 'u') {
	    return &ffi_type_ushort;
	} else {
	    return &ffi_type_sshort;
	}
    case ' ':
	switch(type[TYPE_TYPE]) {
	case 'u':
	    return &ffi_type_uint;
	case 'i':
	    return &ffi_type_sint;
	case 'f':
	    return &ffi_type_float;
	case 'p':
	    return &ffi_type_pointer;
	case 'v':
	default:
	    return &ffi_type_void;
	}
    case 'l':
	switch(type[TYPE_TYPE]) {
	case 'u':
	    return &ffi_type_ulong;
	case 'i':
	    return &ffi_type_slong;
	case 'f':
	default:
	    return &ffi_type_double;
	}
    case 'L':
	switch(type[TYPE_TYPE]) {
	case 'u':
#if ULLONG_MAX == UINT64_MAX
	    return &ffi_type_uint64;
#elif ULLONG_MAX == UINT32_MAX
	    return &ffi_type_uint32;
#elif ULLONG_MAX == UINT16_MAX
	    return &ffi_type_uint16;
#elif ULLONG_MAX == UINT8_MAX
	    return &ffi_type_uint8;
#else
# error "ULLONG_MAX seems to be out of a reasonable range..."
#endif
	case 'i':
#if LLONG_MAX == INT64_MAX
	    return &ffi_type_sint64;
#elif LLONG_MAX == INT32_MAX
	    return &ffi_type_sint32;
#elif LLONG_MAX == INT16_MAX
	    return &ffi_type_sint16;
#elif LLONG_MAX == INT8_MAX
	    return &ffi_type_sint8;
#else
# error "LLONG_MAX seems to be out of a reasonable range..."
#endif
	case 'f':
	default:
	    return &ffi_type_longdouble;
	}
    case 'j':
	if(type[TYPE_TYPE] == 'u') {
#if UINTMAX_MAX == UINT64_MAX
	    return &ffi_type_uint64;
#elif UINTMAX_MAX == UINT32_MAX
	    return &ffi_type_uint32;
#elif UINTMAX_MAX == UINT16_MAX
	    return &ffi_type_uint16;
#elif UINTMAX_MAX == UINT8_MAX
	    return &ffi_type_uint8;
#else
# error "UINTMAX_MAX seems to be out of a reasonable range..."
#endif
	} else {
#if INTMAX_MAX == INT64_MAX
	    return &ffi_type_sint64;
#elif INTMAX_MAX == INT32_MAX
	    return &ffi_type_sint32;
#elif INTMAX_MAX == INT16_MAX
	    return &ffi_type_sint16;
#elif INTMAX_MAX == INT8_MAX
	    return &ffi_type_sint8;
#else
# error "INTMAX_MAX seems to be out of a reasonable range..."
#endif
	}
    case 'z':
#if SIZE_MAX == UINT64_MAX
	return &ffi_type_uint64;
#elif SIZE_MAX == UINT32_MAX
	return &ffi_type_uint32;
#elif SIZE_MAX == UINT16_MAX
	return &ffi_type_uint16;
#elif SIZE_MAX == UINT8_MAX
	return &ffi_type_uint8;
#elif SIZE_MAX == INT64_MAX
	return &ffi_type_sint64;
#elif SIZE_MAX == INT32_MAX
	return &ffi_type_sint32;
#elif SIZE_MAX == INT16_MAX
	return &ffi_type_sint16;
#elif SIZE_MAX == INT8_MAX
	return &ffi_type_sint8;
#else
# error "SIZE_MAX seems to be out of a reasonable range..."
#endif
    case 't':
    default:
#if PTRDIFF_MAX == UINT64_MAX
	return &ffi_type_uint64;
#elif PTRDIFF_MAX == UINT32_MAX
	return &ffi_type_uint32;
#elif PTRDIFF_MAX == UINT16_MAX
	return &ffi_type_uint16;
#elif PTRDIFF_MAX == UINT8_MAX
	return &ffi_type_uint8;
#elif PTRDIFF_MAX == INT64_MAX
	return &ffi_type_sint64;
#elif PTRDIFF_MAX == INT32_MAX
	return &ffi_type_sint32;
#elif PTRDIFF_MAX == INT16_MAX
	return &ffi_type_sint16;
#elif PTRDIFF_MAX == INT8_MAX
	return &ffi_type_sint8;
#else
# error "PTRDIFF_MAX seems to be out of a reasonable range..."
#endif
    }
}

#define PARSENEXT(str, type)	do {					\
	if(*str++ == '\0') {						\
	    break;							\
	}								\
	switch(*str) {							\
	case 'h':							\
	    if(*++str == 'h') {						\
		type[TYPE_SIZE] = 'H';					\
		str++;							\
	    } else {							\
		type[TYPE_SIZE] = 'h';					\
	    }								\
	    break;							\
	case 'l':							\
	    if(*++str == 'l') {						\
		type[TYPE_SIZE] = 'L';					\
		str++;							\
	    } else {							\
		type[TYPE_SIZE] = 'l';					\
	    }								\
	    break;							\
	case 'j':							\
	    type[TYPE_SIZE] = 'j';					\
	    str++;							\
	    break;							\
	case 'L':							\
	case 'q':							\
	    type[TYPE_SIZE] = 'L';					\
	    str++;							\
	    break;							\
	case 't':							\
	    type[TYPE_SIZE] = 't';					\
	    str++;							\
	    break;							\
	case 'z':							\
	    type[TYPE_SIZE] = 'z';					\
	    str++;							\
	    break;							\
	default:							\
	    type[TYPE_SIZE] = ' ';					\
	}								\
	switch(*str) {							\
	case 'd':							\
	case 'i':							\
	    type[TYPE_TYPE] = 'i';					\
	    break;							\
	case 'o':							\
	case 'u':							\
	case 'x':							\
	case 'X':							\
	    type[TYPE_TYPE] = 'u';					\
	    break;							\
	case 'f':							\
	case 'e':							\
	case 'g':							\
	case 'E':							\
	case 'a':							\
	    switch(type[TYPE_SIZE]) {					\
	    case 'L':							\
	    case 'l':							\
	    case ' ':							\
		type[TYPE_TYPE] = 'f';					\
		break;							\
	    default:							\
		type[TYPE_SIZE] = ' ';					\
		type[TYPE_TYPE] = ' ';					\
	    }								\
	    break;							\
	case 's':							\
	case 'c':							\
	case 'p':							\
	    if(type[TYPE_SIZE] != ' ') {				\
		type[TYPE_SIZE] = ' ';					\
		type[TYPE_TYPE] = ' ';					\
	    } else {							\
		type[TYPE_TYPE] = 'p';					\
	    }								\
	    break;							\
	case 'v':							\
	    if(type[TYPE_SIZE] != ' ') {				\
		type[TYPE_SIZE] = ' ';					\
		type[TYPE_TYPE] = ' ';					\
	    } else {							\
		type[TYPE_TYPE] = 'v';					\
	    }								\
	    break;							\
	default:							\
	    type[TYPE_TYPE] = ' ';					\
	}								\
	if(type[TYPE_TYPE] == ' ') {					\
	    break;							\
	}								\
	if(*++str == '$') {						\
	    type[TYPE_STASH] = '$';					\
	    str++;							\
	} else {							\
	    type[TYPE_STASH] = ' ';					\
	}								\
    } while(0)

struct curried_function {
    ffi_closure closure;
    ffi_cif plain_cif;
    ffi_cif curried_cif;
    void *fptr;
    unsigned int stashed_nargs;
    struct stashed_arg stashed_args[];
};

#define CURRIED_FSIZE(plain_nargs, curried_nargs, stashed_nargs)	\
    (sizeof(struct curried_function)					\
     + sizeof(struct stashed_arg) * (stashed_nargs)			\
     + sizeof(ffi_type *) * (plain_nargs)				\
     + sizeof(ffi_type *) * (curried_nargs))

static void run_curried_function(ffi_cif *curried_cif __attribute__((unused)), void *result, void **curried_args, struct curried_function *f) {
    void **plain_args = alloca(sizeof(void *) * f->plain_cif.nargs);
    unsigned int plain_i = 0;
    unsigned int curried_i = 0;
    unsigned int stashed_i = 0;
    while(stashed_i < f->stashed_nargs) {
	if(plain_i == f->stashed_args[stashed_i].argnum) {
	    plain_args[plain_i++] = &f->stashed_args[stashed_i++].value;
	} else {
	    plain_args[plain_i++] = curried_args[curried_i++];
	}
    }
    while(plain_i < f->plain_cif.nargs) {
	plain_args[plain_i++] = curried_args[curried_i++];
    }
    ffi_call(&f->plain_cif, f->fptr, result, plain_args);
}

void free_curried_function(void *f) {
    afree(f, CURRIED_FSIZE(((struct curried_function *) f)->plain_cif.nargs,
			   ((struct curried_function *) f)->curried_cif.nargs,
			   ((struct curried_function *) f)->stashed_nargs));
}

void *curry(void *fptr, char *signature, ...) {
    /* Check signature and refuse to move further if it has problems... */
    va_list ap;
    int plain_nargs = -1;
    int curried_nargs;
    int stashed_nargs = 0;
    int plain_i = 0;
    int curried_i = 0;
    int stashed_i = 0;
    ffi_type **plain_args;
    ffi_type **curried_args;
    struct stashed_arg *stashed_args;
    struct curried_function *ret;
    ffi_type *ffit = ffit;
    ffi_status r;
    char *next;
    char type[4] = { ' ', ' ', ' ', '\0' };

    next = signature;
    while((next = strchr(next, '%')) != NULL) {
	if(next[1] == '%') {
	    next += 2;
	    continue;
	}
	if(type[TYPE_TYPE] == 'v') {
	    errno = EINVAL;
	    return NULL;
	}
	PARSENEXT(next, type);
	if(type[TYPE_TYPE] == ' ') {
	    errno = EINVAL;
	    return NULL;
	}
	plain_nargs++;
	if(type[TYPE_STASH] == '$') {
	    stashed_nargs++;
	}
    }
    if(type[TYPE_STASH] == '$') {
	errno = EINVAL;
	return NULL;
    }
    if(plain_nargs < 0) {
	errno = EINVAL;
	return NULL;
    }

    curried_nargs = plain_nargs - stashed_nargs;

    /* Now we can allocate and fill this. */
    ret = amalloc(CURRIED_FSIZE(plain_nargs, curried_nargs, stashed_nargs));
    if(ret == NULL) {
	return NULL;
    }

    plain_args = (ffi_type **) (((void *) ret)
				+ sizeof(struct curried_function)
				+ sizeof(struct stashed_arg) * stashed_nargs);
    curried_args = (ffi_type **) (((void *) ret)
				  + sizeof(struct curried_function)
				  + sizeof(struct stashed_arg) * stashed_nargs
				  + sizeof(ffi_type *) * plain_nargs);
    stashed_args = ret->stashed_args;

    va_start(ap, signature);

    next = signature;
    while((next = strchr(next, '%')) != NULL) {
	if(next[1] == '%') {
	    next += 2;
	    continue;
	}
	PARSENEXT(next, type);
	ffit = scanft2ffit(type);
	if(plain_i == plain_nargs) {
	    break;
	}
	if(type[TYPE_STASH] == '$') {
	    /* get the value */
	    switch(type[TYPE_SIZE]) {
	    case 'H':
		if(type[TYPE_TYPE] == 'u') {
		    stashed_args[stashed_i].value._uchar = va_arg(ap, unsigned int);
		} else {
		    stashed_args[stashed_i].value._schar = va_arg(ap, int);
		}
		break;
	    case 'h':
		if(type[TYPE_TYPE] == 'u') {
		    stashed_args[stashed_i].value._ushort = va_arg(ap, unsigned int);
		} else {
		    stashed_args[stashed_i].value._short = va_arg(ap, int);
		}
		break;
	    case ' ':
		switch(type[TYPE_TYPE]) {
		case 'u':
		    stashed_args[stashed_i].value._uint = va_arg(ap, unsigned int);
		    break;
		case 'i':
		    stashed_args[stashed_i].value._int = va_arg(ap, int);
		    break;
		case 'f':
		    stashed_args[stashed_i].value._float = va_arg(ap, double);
		    break;
		case 'p':
		default:
		    stashed_args[stashed_i].value._ptr = va_arg(ap, void *);
		}
		break;
	    case 'l':
		switch(type[TYPE_TYPE]) {
		case 'u':
		    stashed_args[stashed_i].value._ulong = va_arg(ap, unsigned long);
		    break;
		case 'i':
		    stashed_args[stashed_i].value._long = va_arg(ap, long);
		    break;
		case 'f':
		default:
		    stashed_args[stashed_i].value._double = va_arg(ap, double);
		}
		break;
	    case 'L':
		switch(type[TYPE_TYPE]) {
		case 'u':
		    stashed_args[stashed_i].value._ulonglong = va_arg(ap, unsigned long long);
		    break;
		case 'i':
		    stashed_args[stashed_i].value._longlong = va_arg(ap, long long);
		    break;
		case 'f':
		default:
		    stashed_args[stashed_i].value._longdouble = va_arg(ap, long double);
		}
		break;
	    case 'j':
		if(type[TYPE_TYPE] == 'u') {
		    stashed_args[stashed_i].value._uintmax = va_arg(ap, uintmax_t);
		} else {
		    stashed_args[stashed_i].value._intmax = va_arg(ap, intmax_t);
		}
		break;
	    case 'z':
		stashed_args[stashed_i].value._size = va_arg(ap, size_t);
		break;
	    case 't':
	    default:
		stashed_args[stashed_i].value._ptrdiff = va_arg(ap, ptrdiff_t);
	    }
	    stashed_args[stashed_i++].argnum = plain_i;
	    plain_args[plain_i++] = ffit;
	} else {
	    /* Value is passed through */
	    curried_args[curried_i++] = ffit;
	    plain_args[plain_i++] = ffit;
	}
    }

    va_end(ap);

    /* We've slurped and configured, configured and slurped.
       Now get this show on the road. */
    r = ffi_prep_cif(&ret->curried_cif, FFI_DEFAULT_ABI, curried_nargs,
		     ffit, curried_args);
    if(r != FFI_OK) {
	free_curried_function(ret);
	return NULL;
    }
    r = ffi_prep_cif(&ret->plain_cif, FFI_DEFAULT_ABI, plain_nargs,
		     ffit, plain_args);
    if(r != FFI_OK) {
	free_curried_function(ret);
	return NULL;
    }

    r = ffi_prep_closure(&ret->closure, &ret->curried_cif, (void (*)(ffi_cif *, void *, void **, void *)) run_curried_function, ret);
    if(r != FFI_OK) {
	free_curried_function(ret);
	return NULL;
    }

    ret->fptr = fptr;
    ret->stashed_nargs = stashed_nargs;

    return ret;
}
