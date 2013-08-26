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

struct argdesc {
    union scanfvalue value;
    unsigned int argnum;
};

#define IV(type) (*((union {char *c; unsigned int *i;}) type).i)
#define IVC(a, b, c) (((unsigned int) a) + (((unsigned int) b) << 8) + (((unsigned int) c) << 16))

static inline ffi_type *scanft2ffit(char *type) {
    switch((IV(type) & IVC('\xFF', '\xFF', '\0')) | IVC('\0', '\0', ' ')) {
    case IVC('H', 'u', ' '):
	return &ffi_type_uchar;
    case IVC('H', 'i', ' '):
	return &ffi_type_schar;
    case IVC('h', 'u', ' '):
	return &ffi_type_ushort;
    case IVC('h', 'i', ' '):
	return &ffi_type_sshort;
    case IVC(' ', 'u', ' '):
	return &ffi_type_uint;
    case IVC(' ', 'i', ' '):
	return &ffi_type_sint;
    case IVC('l', 'u', ' '):
	return &ffi_type_ulong;
    case IVC('l', 'i', ' '):
	return &ffi_type_slong;
    case IVC(' ', 'f', ' '):
	return &ffi_type_float;
    case IVC('l', 'f', ' '):
	return &ffi_type_double;
    case IVC('L', 'f', ' '):
	return &ffi_type_longdouble;
    case IVC(' ', 'p', ' '):
	return &ffi_type_pointer;
    case IVC('L', 'u', ' '):
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
    case IVC('j', 'u', ' '):
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
    case IVC('z', 'u', ' '):
    case IVC('z', 'i', ' '):
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
    case IVC('t', 'u', ' '):
    case IVC('t', 'i', ' '):
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
    case IVC('L', 'i', ' '):
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
    case IVC('j', 'i', ' '):
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
    default:
	return &ffi_type_void;
    }
}

#define PARSENEXT(str, type)	do {					\
	type[0] = ' ';							\
	type[1] = ' ';							\
	type[2] = ' ';							\
	type[3] = '\0';							\
	if(*str == '\0') {						\
	    break;							\
	}								\
	switch(*str) {							\
	case 'h':							\
	    if(*++str == 'h') {						\
		type[0] = 'H';						\
		str++;							\
	    } else {							\
		type[0] = 'h';						\
	    }								\
	    break;							\
	case 'l':							\
	    if(*++str == 'l') {						\
		type[0] = 'L';						\
		str++;							\
	    } else {							\
		type[0] = 'l';						\
	    }								\
	    break;							\
	case 'j':							\
	    type[0] = 'j';						\
	    str++;							\
	    break;							\
	case 'L':							\
	case 'q':							\
	    type[0] = 'L';						\
	    str++;							\
	    break;							\
	case 't':							\
	    type[0] = 't';						\
	    str++;							\
	    break;							\
	case 'z':							\
	    type[0] = 'z';						\
	    str++;							\
	    break;							\
	}								\
	switch(*str++) {						\
	case 'd':							\
	case 'i':							\
	    type[1] = 'i';						\
	    break;							\
	case 'o':							\
	case 'u':							\
	case 'x':							\
	case 'X':							\
	    type[1] = 'u';						\
	    break;							\
	case 'f':							\
	case 'e':							\
	case 'g':							\
	case 'E':							\
	case 'a':							\
	    switch(type[0]) {						\
	    case 'L':							\
	    case 'l':							\
	    case ' ':							\
		type[1] = 'f';						\
		break;							\
	    default:							\
		type[0] = ' ';						\
		str = NULL;						\
	    }								\
	    break;							\
	case 's':							\
	case 'c':							\
	case 'p':							\
	    if(type[0] != ' ') {					\
		type[0] = ' ';						\
		str = NULL;						\
	    } else {							\
		type[1] = 'p';						\
	    }								\
	    break;							\
	case 'v':							\
	    if(type[0] != ' ') {					\
		type[0] = ' ';						\
		str = NULL;						\
	    } else {							\
		type[1] = 'v';						\
	    }								\
	    break;							\
	default:							\
	    str = NULL;							\
	}								\
	if(str == NULL) {						\
	    break;							\
	}								\
	if(*str == '$') {						\
	    type[2] = '$';						\
	    str++;							\
	}								\
    } while(0)

struct curried_function {
    ffi_closure closure;
    ffi_cif plain_cif;
    ffi_cif curried_cif;
    void *fptr;
    struct argdesc stashed_args[];
};

#define CURRIED_FSIZE(plain_nargs, curried_nargs)			\
    (sizeof(struct curried_function)					\
     + (sizeof(struct argdesc) + sizeof(ffi_type *)) * (plain_nargs)	\
     - (sizeof(struct argdesc) - sizeof(ffi_type *)) * (curried_nargs))

static void run_curried_function(ffi_cif *curried_cif __attribute__((unused)), void *result, void **curried_args, struct curried_function *f) {
    void **plain_args = alloca(sizeof(void *) * f->plain_cif.nargs);
    unsigned int plain_i = 0;
    unsigned int stashed_i = 0;
    while(stashed_i < (f->plain_cif.nargs - f->curried_cif.nargs)) {
	if(plain_i == f->stashed_args[stashed_i++].argnum) {
	    plain_args[plain_i++] = &f->stashed_args[stashed_i++].value;
	} else {
	    plain_args[plain_i++] = *(curried_args++);
	}
    }
    while(plain_i < f->plain_cif.nargs) {
	plain_args[plain_i++] = *(curried_args++);
    }
    ffi_call(&f->plain_cif, f->fptr, result, plain_args);
}

void free_curried_function(void *f) {
    afree(f, CURRIED_FSIZE(((struct curried_function *) f)->plain_cif.nargs,
			   ((struct curried_function *) f)->curried_cif.nargs));
}

void *curry(void *fptr, char *signature, ...) {
    /* Check signature and refuse to move further if it has problems... */
    va_list ap;
    int plain_nargs = -1;
    int stashed_nargs = 0;
    char *next = signature;
    char type[4];
    while((next = strchr(next, '%')) != NULL) {
	if(next[1] == '%') {
	    next += 2;
	    continue;
	}
	if(type[1] == 'v') {
	    errno = EINVAL;
	    return NULL;
	}
	PARSENEXT(next, type);
	if(type[1] == ' ') {
	    errno = EINVAL;
	    return NULL;
	}
	plain_nargs++;
	if(type[2] == '$') {
	    stashed_nargs++;
	}
    }
    if(type[2] == '$') {
	errno = EINVAL;
	return NULL;
    }
    if(plain_nargs < 0) {
	errno = EINVAL;
	return NULL;
    }

    int curried_nargs = plain_nargs - stashed_nargs;
    /* Now we can allocate and fill this. */
    struct curried_function *f = amalloc(CURRIED_FSIZE(plain_nargs, curried_nargs));
    if(f == NULL) {
	return NULL;
    }
    ffi_type **plain_args = (ffi_type **) (((void *) f)
					   + sizeof(struct argdesc) * stashed_nargs);
    ffi_type **curried_args = (ffi_type **) (((void *) f)
					     + sizeof(struct argdesc) * stashed_nargs
					     + sizeof(ffi_type *) * plain_nargs);
    f->fptr = fptr;

    va_start(ap, signature);

    int plain_i = 0;
    int curried_i = 0;
    ffi_type *ffit = NULL;
    struct argdesc *stashed = f->stashed_args;
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
	plain_args[plain_i++] = ffit;
	if(type[2] == '$') {
	    /* get the value */
	    switch(IV(type)) {
	    case IVC('H', 'u', '$'):
		(stashed++)->value._uchar = va_arg(ap, unsigned int);
		break;
	    case IVC('H', 'i', '$'):
		(stashed++)->value._schar = va_arg(ap, int);
		break;
	    case IVC('h', 'u', '$'):
		(stashed++)->value._ushort = va_arg(ap, unsigned int);
		break;
	    case IVC('h', 'i', '$'):
		(stashed++)->value._short = va_arg(ap, int);
		break;
	    case IVC(' ', 'u', '$'):
		(stashed++)->value._uint = va_arg(ap, unsigned int);
		break;
	    case IVC(' ', 'i', '$'):
		(stashed++)->value._int = va_arg(ap, int);
		break;
	    case IVC('l', 'u', '$'):
		(stashed++)->value._ulong = va_arg(ap, unsigned long);
		break;
	    case IVC('l', 'i', '$'):
		(stashed++)->value._long = va_arg(ap, long);
		break;
	    case IVC('L', 'u', '$'):
		(stashed++)->value._ulonglong = va_arg(ap, unsigned long long);
		break;
	    case IVC('j', 'u', '$'):
		(stashed++)->value._uintmax = va_arg(ap, uintmax_t);
		break;
	    case IVC('t', 'u', '$'):
		(stashed++)->value._ptrdiff = va_arg(ap, ptrdiff_t);
		break;
	    case IVC('z', 'u', '$'):
		(stashed++)->value._size = va_arg(ap, size_t);
		break;
	    case IVC('L', 'i', '$'):
		(stashed++)->value._longlong = va_arg(ap, long long);
		break;
	    case IVC('j', 'i', '$'):
		(stashed++)->value._intmax = va_arg(ap, intmax_t);
		break;
	    case IVC('t', 'i', '$'):
		(stashed++)->value._ptrdiff = va_arg(ap, ptrdiff_t);
		break;
	    case IVC('z', 'i', '$'):
		(stashed++)->value._size = va_arg(ap, size_t);
		break;
	    case IVC(' ', 'f', '$'):
		(stashed++)->value._float = va_arg(ap, double);
		break;
	    case IVC('l', 'f', '$'):
		(stashed++)->value._double = va_arg(ap, double);
		break;
	    case IVC(' ', 'p', '$'):
		(stashed++)->value._ptr = va_arg(ap, void *);
		break;
	    }
	} else {
	    /* Value is passed through */
	    curried_args[curried_i++] = ffit;
	}
    }

    va_end(ap);

    /* We've slurped and configured, configured and slurped.
       Now get this show on the road. */
    ffi_status r;
    r = ffi_prep_cif(&f->curried_cif, FFI_DEFAULT_ABI, curried_nargs,
		     ffit, curried_args);
    if(r != FFI_OK) {
	free_curried_function(f);
	return NULL;
    }
    r = ffi_prep_cif(&f->plain_cif, FFI_DEFAULT_ABI, plain_nargs,
		     ffit, plain_args);
    if(r != FFI_OK) {
	free_curried_function(f);
	return NULL;
    }

    r = ffi_prep_closure(&f->closure, &f->curried_cif, (void (*)(ffi_cif *, void *, void **, void *)) run_curried_function, f);
    if(r != FFI_OK) {
	free_curried_function(f);
	return NULL;
    }

    return f;
}
