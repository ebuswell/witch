# Makefile.am
# 
# Copyright 2013 Evan Buswell
# 
# This file is part of libwitch.
# 
# libwitch is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2.
# 
# libwitch is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with libwitch.  If not, see <http://www.gnu.org/licenses/>.

MAINTAINERCLEANFILES = aclocal.m4 config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing src/config.h.in~ compile
EXTRA_DIST = autogen.sh Doxyfile
AUTOMAKE_OPTIONS = foreign serial-tests subdir-objects

lib_LTLIBRARIES = libwitch.la
libwitch_la_SOURCES = src/witch.c
libwitch_la_LIBADD = $(ATOMICKIT_LIBS) $(LIBFFI_LIBS)
libwitch_la_LDFLAGS = -L$(libdir) -export-dynamic -version_info @VERSION_INFO@
libwitch_la_CFLAGS = $(CFLAGS) $(ATOMICKIT_CFLAGS) $(LIBFFI_CFLAGS) -Og -g3 -Wall -Wextra -Wmissing-prototypes -Wredundant-decls -Wdeclaration-after-statement -Iinclude
libwitchincludedir = $(includedir)
libwitchinclude_HEADERS = include/witch.h

pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = libwitch.pc

check_PROGRAMS = unittest
unittest_SOURCES = test/main.c
unittest_LDADD = $(ATOMICKIT_LIBS) $(LIBFFI_LIBS) -lwitch
unittest_LDFLAGS = $(LD_FLAGS) -rpath $(libdir)
unittest_CFLAGS = $(CFLAGS) $(ATOMICKIT_CFLAGS) $(LIBFFI_CFLAGS) -Og -g3 -Wall -Wextra -Wmissing-prototypes -Wredundant-decls -Wdeclaration-after-statement -Iinclude
TESTS = unittest
