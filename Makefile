.PHONY: shared static all install-headers install-pkgconfig install-shared \
        install-static install-static-strip install-shared-strip \
        install-all-static install-all-shared install-all-static-strip \
        install-all-shared-strip install install-strip uninstall clean \
        check-shared check-static check

.SUFFIXES: .o .pic.o

include config.mk

VERSION=0.1

SRCS=src/witch.c

TESTSRCS=test/main.c

HEADERS=include/witch.h

OBJS=${SRCS:.c=.o}
PICOBJS=${SRCS:.c=.pic.o}
TESTOBJS=${TESTSRCS:.c=.o}

MAJOR=${shell echo ${VERSION}|cut -d . -f 1}

all: shared libwitch.pc

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

.c.pic.o:
	${CC} ${CFLAGS} -fPIC -c $< -o $@

libwitch.so: ${PICOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -fPIC -shared ${PICOBJS} ${LIBS} -o libwitch.so

libwitch.a: ${OBJS}
	rm -f libwitch.a
	${AR} ${ARFLAGS}c libwitch.a ${OBJS}

unittest-shared: libwitch.so ${TESTOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -L`pwd` -Wl,-rpath,`pwd` \
	      ${TESTOBJS} ${LIBS} -lwitch -o unittest-shared

unittest-static: libwitch.a ${TESTOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -static -L`pwd` \
	      ${TESTOBJS} ${STATIC} -lwitch -o unittest-static

libwitch.pc: libwitch.pc.in config.mk Makefile
	sed -e 's!@prefix@!${PREFIX}!g' \
	    -e 's!@libdir@!${LIBDIR}!g' \
	    -e 's!@includedir@!${INCLUDEDIR}!g' \
	    -e 's!@version@!${VERSION}!g' \
	    libwitch.pc.in >libwitch.pc

shared: libwitch.so

static: libwitch.a

install-headers:
	(umask 022; mkdir -p ${DESTDIR}${INCLUDEDIR})
	install -m 644 -t ${DESTDIR}${INCLUDEDIR} ${HEADERS}

install-pkgconfig: libwitch.pc
	(umask 022; mkdir -p ${DESTDIR}${PKGCONFIGDIR})
	install -m 644 libwitch.pc ${DESTDIR}${PKGCONFIGDIR}/libwitch.pc

install-shared: shared
	(umask 022; mkdir -p ${DESTDIR}${LIBDIR})
	install -m 755 libwitch.so \
	        ${DESTDIR}${LIBDIR}/libwitch.so.${VERSION}
	ln -frs ${DESTDIR}${LIBDIR}/libwitch.so.${VERSION} \
	        ${DESTDIR}${LIBDIR}/libwitch.so.${MAJOR}
	ln -frs ${DESTDIR}${LIBDIR}/libwitch.so.${VERSION} \
	        ${DESTDIR}${LIBDIR}/libwitch.so

install-static: static
	(umask 022; mkdir -p ${DESTDIR}${LIBDIR})
	install -m 644 libwitch.a ${DESTDIR}${LIBDIR}/libwitch.a

install-shared-strip: install-shared
	strip --strip-unneeded ${DESTDIR}${LIBDIR}/libwitch.so.${VERSION}

install-static-strip: install-static
	strip --strip-unneeded ${DESTDIR}${LIBDIR}/libwitch.a

install-all-static: static libwitch.pc install-static install-headers install-pkgconfig

install-all-shared: shared libwitch.pc install-shared install-headers install-pkgconfig

install-all-shared-strip: install-all-shared install-shared-strip

install-all-static-strip: install-all-static install-static-strip

install: install-all-shared

install-strip: install-all-shared-strip

uninstall: 
	rm -f ${DESTDIR}${LIBDIR}/libwitch.so.${VERSION}
	rm -f ${DESTDIR}${LIBDIR}/libwitch.so.${MAJOR}
	rm -f ${DESTDIR}${LIBDIR}/libwitch.so
	rm -f ${DESTDIR}${LIBDIR}/libwitch.a
	rm -f ${DESTDIR}${PKGCONFIGDIR}/libwitch.pc
	rm -f ${DESTDIR}${INCLUDEDIR}/witch.h

clean:
	rm -f libwitch.pc
	rm -f libwitch.so
	rm -f libwitch.a
	rm -f ${OBJS}
	rm -f ${PICOBJS}
	rm -f ${TESTOBJS}
	rm -f unittest-shared
	rm -f unittest-static

check-shared: unittest-shared
	./unittest-shared

check-static: unittest-static
	./unittest-static

check: check-shared
