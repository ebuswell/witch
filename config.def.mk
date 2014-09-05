PREFIX?=/usr/local
INCLUDEDIR?=${PREFIX}/include
LIBDIR?=${PREFIX}/lib
DESTDIR?=
PKGCONFIGDIR?=${LIBDIR}/pkgconfig

CC?=gcc
CFLAGS?=-Og -g3
LDFLAGS?=
AR?=ar
ARFLAGS?=rv

ATOMICKIT_CFLAGS=${shell pkg-config --cflags atomickit}
ATOMICKIT_LIBS=${shell pkg-config --libs atomickit}
ATOMICKIT_STATIC=${shell pkg-config --static atomickit}
LIBFFI_CFLAGS=${shell pkg-config --cflags libffi}
LIBFFI_LIBS=${shell pkg-config --libs libffi}
LIBFFI_STATIC=${shell pkg-config --static libffi}

CFLAGS+=${ATOMICKIT_CFLAGS} ${LIBFFI_CFLAGS}
CFLAGS+=-Wall -Wextra -Wmissing-prototypes -Wredundant-decls -Wdeclaration-after-statement
CFLAGS+=-fplan9-extensions
CFLAGS+=-Iinclude

LIBS=${ATOMICKIT_LIBS} ${LIBFFI_LIBS}
STATIC=${ATOMICKIT_STATIC} ${LIBFFI_STATIC}
