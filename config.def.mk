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

ATOMICKIT_CFLAGS!=pkg-config --cflags atomickit
ATOMICKIT_LIBS!=pkg-config --libs atomickit
ATOMICKIT_STATIC!=pkg-config --static atomickit
LIBFFI_CFLAGS!=pkg-config --cflags libffi
LIBFFI_LIBS!=pkg-config --libs libffi
LIBFFI_STATIC!=pkg-config --static libffi

CFLAGS+=${ATOMICKIT_CFLAGS} ${LIBFFI_CFLAGS}
CFLAGS+=-Wall -Wextra -Wmissing-prototypes -Wredundant-decls -Wdeclaration-after-statement
CFLAGS+=-fplan9-extensions
CFLAGS+=-Iinclude

LIBS=${ATOMICKIT_LIBS} ${LIBFFI_LIBS}
STATIC=${ATOMICKIT_STATIC} ${LIBFFI_STATIC}
