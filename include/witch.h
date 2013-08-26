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

void free_curried_function(void *f);

void *curry(void *fptr, char *signature, ...);

#endif /* ! WITCH_H */
