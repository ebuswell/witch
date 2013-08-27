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

union argvalue {
    uintmax_t _uintmax; /* Put this first so initialization is easy. */
    unsigned char _uchar;
    signed char _schar;
    unsigned short _ushort;
    short _short;
    unsigned int _uint;
    int _int;
    unsigned long _ulong;
    long _long;
    unsigned long long _ullong;
    long long _llong;
    intmax_t _intmax;
    ptrdiff_t _ptrdiff;
    size_t _size;
    float _float;
    double _double;
    long double _ldouble;
    void *_ptr;
};

enum argtype {
    ARGT_UCHAR,
    ARGT_SCHAR,
    ARGT_USHORT,
    ARGT_SHORT,
    ARGT_UINT,
    ARGT_INT,
    ARGT_ULONG,
    ARGT_LONG,
    ARGT_ULLONG,
    ARGT_LLONG,
    ARGT_UINTMAX,
    ARGT_INTMAX,
    ARGT_PTRDIFF,
    ARGT_SIZE,
    ARGT_FLOAT,
    ARGT_DOUBLE,
    ARGT_LDOUBLE,
    ARGT_PTR,
    ARGT_VOID
};

struct stashed_arg {
    union argvalue value;
    unsigned int argnum;
};

static ffi_type *argt2ffit(enum argtype type) {
    switch(type) {
    case ARGT_UCHAR:
	return &ffi_type_uchar;
    case ARGT_SCHAR:
	return &ffi_type_schar;
    case ARGT_USHORT:
	return &ffi_type_ushort;
    case ARGT_SHORT:
	return &ffi_type_sshort;
    case ARGT_UINT:
	return &ffi_type_uint;
    case ARGT_INT:
	return &ffi_type_sint;
    case ARGT_ULONG:
	return &ffi_type_ulong;
    case ARGT_LONG:
	return &ffi_type_slong;
    case ARGT_ULLONG:
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
    case ARGT_LLONG:
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
    case ARGT_UINTMAX:
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
    case ARGT_INTMAX:
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
    case ARGT_PTRDIFF:
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
    case ARGT_SIZE:
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
    case ARGT_FLOAT:
	return &ffi_type_float;
    case ARGT_DOUBLE:
	return &ffi_type_double;
    case ARGT_LDOUBLE:
	return &ffi_type_longdouble;
    case ARGT_PTR:
	return &ffi_type_pointer;
    case ARGT_VOID:
	return &ffi_type_void;
    default:
	return NULL;
    }
}

static int scanf_parsenext(char *str, int offset, enum argtype *type, bool *stash) {
    char size;
    while(str[offset] != '\0') {
	if(str[offset++] != '%') {
	    continue;
	}
	if(str[offset] == '%') {
	    offset++;
	    continue;
	}

	switch(str[offset]) {
	case 'h':
	    if(str[++offset] == 'h') {
		size = 'H';
		offset++;
	    } else {
		size = 'h';
	    }
	    break;
	case 'l':
	    if(str[++offset] == 'l') {
		size = 'L';
		offset++;
	    } else {
		size = 'l';
	    }
	    break;
	case 'j':
	    size = 'j';
	    offset++;
	    break;
	case 'L':
	case 'q':
	    size = 'L';
	    offset++;
	    break;
	case 't':
	    size = 't';
	    offset++;
	    break;
	case 'z':
	    size = 'z';
	    offset++;
	    break;
	default:
	    size = ' ';
	}
	switch(str[offset++]) {
	case 'd':
	case 'i':
	    switch(size) {
	    case 'H':
		*type = ARGT_SCHAR;
		break;
	    case 'h':
		*type = ARGT_SHORT;
		break;
	    case ' ':
		*type = ARGT_INT;
		break;
	    case 'l':
		*type = ARGT_LONG;
		break;
	    case 'L':
		*type = ARGT_LLONG;
		break;
	    case 'j':
		*type = ARGT_INTMAX;
		break;
	    case 't':
		*type = ARGT_PTRDIFF;
		break;
	    case 'z':
		*type = ARGT_SIZE;
		break;
	    default:
		return -1;
	    }
	    break;
	case 'o':
	case 'u':
	case 'x':
	case 'X':
	    switch(size) {
	    case 'H':
		*type = ARGT_UCHAR;
		break;
	    case 'h':
		*type = ARGT_USHORT;
		break;
	    case ' ':
		*type = ARGT_UINT;
		break;
	    case 'l':
		*type = ARGT_ULONG;
		break;
	    case 'L':
		*type = ARGT_ULLONG;
		break;
	    case 'j':
		*type = ARGT_UINTMAX;
		break;
	    case 't':
		*type = ARGT_PTRDIFF;
		break;
	    case 'z':
		*type = ARGT_SIZE;
		break;
	    default:
		return -1;
	    }
	    break;
	case 'f':
	case 'e':
	case 'g':
	case 'E':
	case 'a':
	    switch(size) {
	    case ' ':
		*type = ARGT_FLOAT;
		break;
	    case 'l':
		*type = ARGT_DOUBLE;
		break;
	    case 'L':
		*type = ARGT_LDOUBLE;
		break;
	    default:
		return -1;
	    }
	    break;
	case 's':
	case 'c':
	case 'p':
	    if(size != ' ') {
		return -1;
	    }
	    *type = ARGT_PTR;
	    break;
	case 'v':
	    if(size != ' ') {
		return -1;
	    }
	    *type = ARGT_VOID;
	    break;
	default:
	    return -1;
	}
	if(stash != NULL) {
	    if(str[offset] == '$') {
		offset++;
		*stash = true;
	    } else {
		*stash = false;
	    }
	}
	return offset;
    }
    return 0;
}

static int scanf_parsenargs(char *str, int *nargs, int *stashed_nargs) {
    *nargs = -1;
    *stashed_nargs = 0;
    enum argtype type;
    bool stashed;
    int parse_i = 0;
    while((parse_i = scanf_parsenext(str, parse_i, &type, &stashed)) > 0) {
	if(type == ARGT_VOID) {
	    break;
	}
	(*nargs)++;
	if(stashed && stashed_nargs != NULL) {
	    (*stashed_nargs)++;
	}
    }
    if(parse_i < 0) {
	/* Malformed signature */
	return -1;
    }
    if(*nargs < 0) {
	/* No return argument */
	return -1;
    }
    if(parse_i != 0 && scanf_parsenext(str, parse_i, &type, &stashed) != 0) {
	/* void type before the return */
	return -1;
    }
    if(stashed) {
	/* Attempted to curry the return value -- while potentially
	 * useful in functions with side effects, this is not
	 * supported. */
	return -1;
    }
    return 0;
}

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
    int plain_nargs;
    int curried_nargs;
    int stashed_nargs;
    int plain_i = 0;
    int curried_i = 0;
    int stashed_i = 0;
    ffi_type **plain_args;
    ffi_type **curried_args;
    struct stashed_arg *stashed_args;
    struct curried_function *ret;

    ffi_type *ffit = ffit;
    enum argtype type;
    bool stashed;
    int parse_i;
    ffi_status fs;

    int r;

    r = scanf_parsenargs(signature, &plain_nargs, &stashed_nargs);
    if(r != 0) {
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

    parse_i = 0;
    while((parse_i = scanf_parsenext(signature, parse_i, &type, &stashed)) > 0) {
	ffit = argt2ffit(type);
	if(plain_i == plain_nargs) {
	    break;
	}
	if(stashed) {
	    /* get the value */
	    switch(type) {
	    case ARGT_UCHAR:
		stashed_args[stashed_i].value._uchar = va_arg(ap, unsigned int);
		break;
	    case ARGT_SCHAR:
		stashed_args[stashed_i].value._schar = va_arg(ap, int);
		break;
	    case ARGT_USHORT:
		stashed_args[stashed_i].value._ushort = va_arg(ap, unsigned int);
		break;
	    case ARGT_SHORT:
		stashed_args[stashed_i].value._short = va_arg(ap, int);
		break;
	    case ARGT_UINT:
		stashed_args[stashed_i].value._uint = va_arg(ap, unsigned int);
		break;
	    case ARGT_INT:
		stashed_args[stashed_i].value._int = va_arg(ap, int);
		break;
	    case ARGT_ULONG:
		stashed_args[stashed_i].value._ulong = va_arg(ap, unsigned long);
		break;
	    case ARGT_LONG:
		stashed_args[stashed_i].value._long = va_arg(ap, long);
		break;
	    case ARGT_ULLONG:
		stashed_args[stashed_i].value._ullong = va_arg(ap, unsigned long long);
		break;
	    case ARGT_LLONG:
		stashed_args[stashed_i].value._llong = va_arg(ap, long long);
		break;
	    case ARGT_UINTMAX:
		stashed_args[stashed_i].value._uintmax = va_arg(ap, uintmax_t);
		break;
	    case ARGT_INTMAX:
		stashed_args[stashed_i].value._intmax = va_arg(ap, intmax_t);
		break;
	    case ARGT_PTRDIFF:
		stashed_args[stashed_i].value._ptrdiff = va_arg(ap, ptrdiff_t);
		break;
	    case ARGT_SIZE:
		stashed_args[stashed_i].value._size = va_arg(ap, size_t);
		break;
	    case ARGT_FLOAT:
		stashed_args[stashed_i].value._float = va_arg(ap, double);
		break;
	    case ARGT_DOUBLE:
		stashed_args[stashed_i].value._double = va_arg(ap, double);
		break;
	    case ARGT_LDOUBLE:
		stashed_args[stashed_i].value._ldouble = va_arg(ap, long double);
		break;
	    case ARGT_PTR:
		stashed_args[stashed_i].value._ptr = va_arg(ap, void *);
		break;
	    case ARGT_VOID:
	    default:
		/* We shouldn't get here... */
		free_curried_function(ret);
		errno = EINVAL;
		return NULL;
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

    /* We've now parsed everything. Now create the function descriptors. */
    fs = ffi_prep_cif(&ret->curried_cif, FFI_DEFAULT_ABI, curried_nargs,
		     ffit, curried_args);
    if(fs != FFI_OK) {
	free_curried_function(ret);
	return NULL;
    }
    fs = ffi_prep_cif(&ret->plain_cif, FFI_DEFAULT_ABI, plain_nargs,
		     ffit, plain_args);
    if(fs != FFI_OK) {
	free_curried_function(ret);
	return NULL;
    }

    fs = ffi_prep_closure(&ret->closure, &ret->curried_cif, (void (*)(ffi_cif *, void *, void **, void *)) run_curried_function, ret);
    if(fs != FFI_OK) {
	free_curried_function(ret);
	return NULL;
    }

    ret->fptr = fptr;
    ret->stashed_nargs = stashed_nargs;

    return ret;
}

struct afptr_dispatch {
    ffi_closure closure;
    ffi_cif cif;
    ffi_type *args[];
};

static void run_dispatch_function(ffi_cif *cif, void *result, void **args, arcp_t *arcp) {
    struct afptr *afptr = (struct afptr *) arcp_load(arcp);
    if(afptr == NULL) {
	if(cif->rtype == &ffi_type_void) {
	    return;
	}
	switch(cif->rtype->size) {
	case 1:
	    *((uint8_t *) result) = '\0';
	case 2:
	    *((uint16_t *) result) = 0;
	case 4:
	    *((uint32_t *) result) = 0;
	case 8:
	    *((uint64_t *) result) = 0;
	case 16:
	    ((uint64_t *) result)[0] = 0;
	    ((uint64_t *) result)[1] = 0;
	/* default:
	      Do nothing */
	}
	return;
    }
    ffi_call(cif, afptr->fptr, result, args);
    arcp_release(afptr);
    return;
}

void afptr_free_dispatch_f(void *f) {
    afree(f,
	  sizeof(struct afptr_dispatch)
	  + sizeof(ffi_type *) * ((struct afptr_dispatch *) f)->cif.nargs);
}

void *afptr_create_dispatch_f(arcp_t *arcp, char *signature) {
    struct afptr_dispatch *ret;
    ffi_type *ffit = ffit;
    int nargs = -1;
    enum argtype type;
    int parse_i = 0;
    int arg_i;
    int r;

    r = scanf_parsenargs(signature, &nargs, NULL);
    if(r != 0) {
	return NULL;
    }

    ret = amalloc(sizeof(struct afptr_dispatch) + sizeof(ffi_type *) * nargs);
    arg_i = 0;
    parse_i = 0;
    while((parse_i = scanf_parsenext(signature, parse_i, &type, NULL)) > 0) {
	ffit = argt2ffit(type);
	if(arg_i == nargs) {
	    break;
	}
	ret->args[arg_i++] = ffit;
    }

    r = ffi_prep_cif(&ret->cif, FFI_DEFAULT_ABI, nargs,
		     ffit, ret->args);
    if(r != FFI_OK) {
	afptr_free_dispatch_f(ret);
	return NULL;
    }

    r = ffi_prep_closure(&ret->closure, &ret->cif, (void (*)(ffi_cif *, void *, void **, void *)) run_dispatch_function, arcp);
    if(r != FFI_OK) {
	afptr_free_dispatch_f(ret);
	return NULL;
    }

    return ret;
}

