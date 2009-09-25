/* Copyright (C) 2009  Pierre-Marc Fournier
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include "share/usterr.h"

extern void ust_fork(void);
extern void ust_potential_exec(void);

pid_t fork(void)
{
	static pid_t (*plibc_func)(void) = NULL;

	pid_t retval;

	if(plibc_func == NULL) {
		plibc_func = dlsym(RTLD_NEXT, "fork");
		if(plibc_func == NULL) {
			fprintf(stderr, "libcwrap: unable to find fork\n");
			return -1;
		}
	}

	retval = plibc_func();

	if(retval == 0)
		ust_fork();

	return retval;
}

int execve(const char *filename, char *const argv[], char *const envp[])
{
	static int (*plibc_func)(const char *filename, char *const argv[], char *const envp[]) = NULL;

	pid_t retval;

	if(plibc_func == NULL) {
		plibc_func = dlsym(RTLD_NEXT, "execve");
		if(plibc_func == NULL) {
			fprintf(stderr, "libcwrap: unable to find execve\n");
			return -1;
		}
	}

	ust_potential_exec();

	retval = plibc_func(filename, argv, envp);

	return retval;
}
