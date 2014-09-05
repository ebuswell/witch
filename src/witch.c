#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <stdarg.h>
#include <alloca.h>
#include <string.h>
#include <errno.h>
#include <ffi.h>
#include <atomickit/malloc.h>
#include "witch.h"

#include <stdio.h>

/* union representing all possible values of an argument */
union argvalue {
	uintmax_t _uintmax; /* put this first so initialization is easy */
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

/* enum of all the argument types we understand */
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

/* an argument as it is stored in a partially applied function */
struct stashed_arg {
	union argvalue value; /* the argument */
	unsigned int argnum; /* the slot in which this argument should go */
};

/* transform our argtype to FFI's ffi_type */
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

/* Scan the str starting at offset, returning the next argtype in *type and
 * whether or not the argument is marked to be stashed in *stash. If stash is
 * NULL, ignore stash parsing entirely. Returns the number of characters that
 * were parsed, zero if all arguments have been parsed, or negative on
 * error. */
static int scanf_parsenext(char *str, int offset, enum argtype *type,
                           bool *stash) {
	char size;
	while(str[offset] != '\0') {
		if(str[offset++] != '%') {
			/* ignore characters between percents */
			continue;
		}
		if(str[offset] == '%') {
			/* a double percent is a literal percent; ignore
 			 * it */
			offset++;
			continue;
		}

		/* first deal with a potential size modification */
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
			/* no size modification, use the default size */
			size = ' ';
		}
		switch(str[offset++]) {
		case 'd':
		case 'i':
			/* signed integral type */
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
			/* unsigned integral type */
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
			/* floating type */
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
			/* string/pointer type */
			if(size != ' ') {
				return -1;
			}
			*type = ARGT_PTR;
			break;
		case 'v':
			/* void type, for return value */
			if(size != ' ') {
				return -1;
			}
			*type = ARGT_VOID;
			break;
		default:
			return -1;
		}
		/* see if it's marked to be stashed */
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

/* Scan through string and return the number of arguments in *nargs and the
 * number of stashed arguments in *stashed_nargs. If stashed_nargs is NULL,
 * ignore stashing. Returns zero on success nonzero on error. */
static int scanf_parsenargs(char *str, int *nargs, int *stashed_nargs) {
	enum argtype type;
	bool stashed;
	int parse_i = 0;
	*nargs = -1;
	if(stashed_nargs != NULL) {
		*stashed_nargs = 0;
	}
	while((parse_i = scanf_parsenext(str, parse_i, &type, &stashed))
	      > 0) {
		(*nargs)++;
		if(type == ARGT_VOID) {
			break;
		}
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
	if(parse_i != 0
	   && scanf_parsenext(str, parse_i, &type, &stashed) != 0) {
		/* void type before the return */
		return -1;
	}
	if(stashed) {
		/* Attempted to partially apply the return value -- while
 		 * potentially useful in functions with side effects, this is
 		 * not supported. */
		return -1;
	}
	return 0;
}

/***********************
 * Partial Application *
 ***********************/

/* partial function application struct (cast to void * / function *) */
struct papf {
	ffi_closure closure; /* the magic ffi structure */
	ffi_cif plain_cif; /* the argument descriptor for the plain
			      function */
	ffi_cif papf_cif; /* the argument descriptor for the partially applied
			     function (this) */
	void *fptr; /* a pointer to the plain function */
	unsigned int stashed_nargs; /* the number of stashed args */
	struct stashed_arg stashed_args[]; /* [stashed_nargs] */
	/* ffi_type plain_args[plain_nargs] -- this is appended to the struct,
 	                                       but C doesn't have semantics
					       for that. */
	/* ffi_type papf_args[papf_nargs] -- this is appended to the struct
 	                                     after plain_args, but C doesn't
					     have semantics for that. */
};

/* get the total size of the struct for the given numbers of arguments */
#define PAPF_SIZE(plain_nargs, papf_nargs, stashed_nargs)	\
	(sizeof(struct papf)					\
	 + sizeof(struct stashed_arg) * (stashed_nargs)		\
	 + sizeof(ffi_type *) * (plain_nargs)			\
	 + sizeof(ffi_type *) * (papf_nargs))

/* the function that is called by ffi when the partially applied function is
 * called. */
static void run_papf(ffi_cif *papf_cif __attribute__((unused)), void *result,
                     void **papf_args, struct papf *f) {
	void *plain_args[f->plain_cif.nargs];
	unsigned int plain_i = 0;
	unsigned int papf_i = 0;
	unsigned int stashed_i = 0;
	/* go through each plain_i and get the appropriate argument for it */
	while(stashed_i < f->stashed_nargs) {
		/* while there are stashed variables to apply... */
		if(plain_i == f->stashed_args[stashed_i].argnum) {
			/* the next stashed argument should be applied here */
			plain_args[plain_i++]
				= &f->stashed_args[stashed_i++].value;
		} else {
			/* this argument was not stashed; get it from the
 			 * arguments passed in to us */
			plain_args[plain_i++] = papf_args[papf_i++];
		}
	}
	/* no more stashed variables; tack on the rest of the passed
 	 * variables */
	while(plain_i < f->plain_cif.nargs) {
		plain_args[plain_i++] = papf_args[papf_i++];
	}
	/* call the original function via ffi */
	ffi_call(&f->plain_cif, f->fptr, result, plain_args);
}

void papf_free(void *f) {
	afree(f, PAPF_SIZE(((struct papf *) f)->plain_cif.nargs,
	                   ((struct papf *) f)->papf_cif.nargs,
	                   ((struct papf *) f)->stashed_nargs));
}

void *papf_create(void *fptr, char *signature, ...) {
	va_list ap;
	int plain_nargs;
	int papf_nargs;
	int stashed_nargs;
	int plain_i = 0;
	int papf_i = 0;
	int stashed_i = 0;
	ffi_type **plain_args;
	ffi_type **papf_args;
	struct stashed_arg *stashed_args;
	struct papf *ret;

	ffi_type *ffit = ffit;
	enum argtype type;
	bool stashed;
	int parse_i;
	ffi_status fs;

	int r;

	/* check signature and refuse to move further if it has problems... */
	r = scanf_parsenargs(signature, &plain_nargs, &stashed_nargs);
	if(r != 0) {
		errno = EINVAL;
		return NULL;
	}
	papf_nargs = plain_nargs - stashed_nargs;

	/* now we can allocate this */
	ret = amalloc(PAPF_SIZE(plain_nargs, papf_nargs, stashed_nargs));
	if(ret == NULL) {
		return NULL;
	}

	/* get plain_args, stashed_args, and papf_args from where they should
 	 * be in the structure */
	plain_args = (ffi_type **) (((void *) ret)
	                            + sizeof(struct papf)
	                            + sizeof(struct stashed_arg)
	                              * stashed_nargs);
	papf_args = (ffi_type **) (((void *) ret)
	                              + sizeof(struct papf)
	                              + sizeof(struct stashed_arg)
	                                * stashed_nargs
	                              + sizeof(ffi_type *) * plain_nargs);
	stashed_args = ret->stashed_args;

	/* start the variadic parsing */
	va_start(ap, signature);

	/* parse the signature */
	parse_i = 0;
	while((parse_i = scanf_parsenext(signature, parse_i, &type, &stashed))
	      > 0) {
		ffit = argt2ffit(type);
		if(plain_i == plain_nargs) {
			break;
		}
		if(stashed) {
			/* get the value */
			switch(type) {
			case ARGT_UCHAR:
				stashed_args[stashed_i].value._uchar
					= va_arg(ap, unsigned int);
				break;
			case ARGT_SCHAR:
				stashed_args[stashed_i].value._schar
					= va_arg(ap, int);
				break;
			case ARGT_USHORT:
				stashed_args[stashed_i].value._ushort
					= va_arg(ap, unsigned int);
				break;
			case ARGT_SHORT:
				stashed_args[stashed_i].value._short
					= va_arg(ap, int);
				break;
			case ARGT_UINT:
				stashed_args[stashed_i].value._uint
					= va_arg(ap, unsigned int);
				break;
			case ARGT_INT:
				stashed_args[stashed_i].value._int
					= va_arg(ap, int);
				break;
			case ARGT_ULONG:
				stashed_args[stashed_i].value._ulong
					= va_arg(ap, unsigned long);
				break;
			case ARGT_LONG:
				stashed_args[stashed_i].value._long
					= va_arg(ap, long);
				break;
			case ARGT_ULLONG:
				stashed_args[stashed_i].value._ullong
					= va_arg(ap, unsigned long long);
				break;
			case ARGT_LLONG:
				stashed_args[stashed_i].value._llong
					= va_arg(ap, long long);
				break;
			case ARGT_UINTMAX:
				stashed_args[stashed_i].value._uintmax
					= va_arg(ap, uintmax_t);
				break;
			case ARGT_INTMAX:
				stashed_args[stashed_i].value._intmax
					= va_arg(ap, intmax_t);
				break;
			case ARGT_PTRDIFF:
				stashed_args[stashed_i].value._ptrdiff
					= va_arg(ap, ptrdiff_t);
				break;
			case ARGT_SIZE:
				stashed_args[stashed_i].value._size
					= va_arg(ap, size_t);
				break;
			case ARGT_FLOAT:
				stashed_args[stashed_i].value._float
					= va_arg(ap, double);
				break;
			case ARGT_DOUBLE:
				stashed_args[stashed_i].value._double
					= va_arg(ap, double);
				break;
			case ARGT_LDOUBLE:
				stashed_args[stashed_i].value._ldouble
					= va_arg(ap, long double);
				break;
			case ARGT_PTR:
				stashed_args[stashed_i].value._ptr
					= va_arg(ap, void *);
				break;
			case ARGT_VOID:
			default:
				/* we shouldn't get here... */
				papf_free(ret);
				errno = EINVAL;
				return NULL;
			}
			stashed_args[stashed_i++].argnum = plain_i;
			plain_args[plain_i++] = ffit;
		} else {
			/* value will be passed through */
			papf_args[papf_i++] = ffit;
			plain_args[plain_i++] = ffit;
		}
	}

	va_end(ap);

	/* We've now parsed everything. Now create the function
 	 * descriptors. */
	fs = ffi_prep_cif(&ret->papf_cif, FFI_DEFAULT_ABI, papf_nargs,
	                  ffit, papf_args);
	if(fs != FFI_OK) {
		papf_free(ret);
		return NULL;
	}
	fs = ffi_prep_cif(&ret->plain_cif, FFI_DEFAULT_ABI, plain_nargs,
	                  ffit, plain_args);
	if(fs != FFI_OK) {
		papf_free(ret);
		return NULL;
	}

	fs = ffi_prep_closure(
		&ret->closure,
		&ret->papf_cif,
		(void (*)(ffi_cif *, void *, void **, void *)) run_papf,
		ret);
	if(fs != FFI_OK) {
		papf_free(ret);
		return NULL;
	}

	/* set up the last few things */
	ret->fptr = fptr;
	ret->stashed_nargs = stashed_nargs;

	return ret;
}

/************
 * Dispatch *
 ************/

void afptr_init(struct afptr *afptr, void *fptr,
                void (*destroy)(struct afptr *)) {
	afptr->fptr = fptr;
	arcp_region_init(afptr, (void (*)(struct arcp_region *)) destroy);
}

/* the dispatch function struct */
struct afptr_dispatch {
	ffi_closure closure; /* ffi magic */
	ffi_cif cif; /* function descriptor */
	ffi_type *args[]; /* argument types */
};

/* the function that is called by ffi when the dispatch function is called. */
static void run_dispatch(ffi_cif *cif, void *result,
                         void **args, arcp_t *arcp) {
	struct afptr *afptr;
	afptr = (struct afptr *) arcp_load(arcp);
	if(afptr == NULL) {
		/* no function to dispatch to; set default return value and
 		 * return */
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

void afptr_dispatch_free(void *f) {
	afree(f,
	      sizeof(struct afptr_dispatch)
	      + sizeof(ffi_type *)
	        * ((struct afptr_dispatch *) f)->cif.nargs);
}

void *afptr_dispatch_create(arcp_t *arcp, char *signature) {
	struct afptr_dispatch *ret;
	ffi_type *ffit = ffit;
	int nargs = -1;
	enum argtype type;
	int parse_i = 0;
	int arg_i;
	int r;

	/* check signature and refuse to move further if it has problems... */
	r = scanf_parsenargs(signature, &nargs, NULL);
	if(r != 0) {
		errno = EINVAL;
		return NULL;
	}

	/* allocate dispatch function */
	ret = amalloc(sizeof(struct afptr_dispatch)
	              + sizeof(ffi_type *) * nargs);
	/* parse signature */
	arg_i = 0;
	parse_i = 0;
	while((parse_i = scanf_parsenext(signature, parse_i, &type, NULL))
	      > 0) {
		ffit = argt2ffit(type);
		if(arg_i == nargs) {
			break;
		}
		ret->args[arg_i++] = ffit;
	}

	r = ffi_prep_cif(&ret->cif, FFI_DEFAULT_ABI, nargs,
	                 ffit, ret->args);
	if(r != FFI_OK) {
		afptr_dispatch_free(ret);
		return NULL;
	}

	r = ffi_prep_closure(
		&ret->closure,
		&ret->cif,
		(void (*)(ffi_cif *, void *, void **, void *)) run_dispatch,
		arcp);
	if(r != FFI_OK) {
		afptr_dispatch_free(ret);
		return NULL;
	}

	return ret;
}

