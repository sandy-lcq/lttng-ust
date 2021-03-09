/*
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (C) 2014 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#define _LGPL_SOURCE
#include <error.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include <usterr-signal-safe.h>
#include <lttng/ust-clock.h>
#include <urcu/system.h>
#include <urcu/arch.h>

#include "clock.h"
#include "getenv.h"

struct lttng_trace_clock *lttng_trace_clock;

static
struct lttng_trace_clock user_tc;

static
void *clock_handle;

int lttng_ust_trace_clock_set_read64_cb(uint64_t (*read64)(void))
{
	if (CMM_LOAD_SHARED(lttng_trace_clock))
		return -EBUSY;
	user_tc.read64 = read64;
	return 0;
}

int lttng_ust_trace_clock_set_freq_cb(uint64_t (*freq)(void))
{
	if (CMM_LOAD_SHARED(lttng_trace_clock))
		return -EBUSY;
	user_tc.freq = freq;
	return 0;
}

int lttng_ust_trace_clock_set_uuid_cb(int (*uuid)(char *uuid))
{
	if (CMM_LOAD_SHARED(lttng_trace_clock))
		return -EBUSY;
	user_tc.uuid = uuid;
	return 0;
}

int lttng_ust_trace_clock_set_name_cb(const char *(*name)(void))
{
	if (CMM_LOAD_SHARED(lttng_trace_clock))
		return -EBUSY;
	user_tc.name = name;
	return 0;
}

int lttng_ust_trace_clock_set_description_cb(const char *(*description)(void))
{
	if (CMM_LOAD_SHARED(lttng_trace_clock))
		return -EBUSY;
	user_tc.description = description;
	return 0;
}

int lttng_ust_enable_trace_clock_override(void)
{
	if (CMM_LOAD_SHARED(lttng_trace_clock))
		return -EBUSY;
	if (!user_tc.read64)
		return -EINVAL;
	if (!user_tc.freq)
		return -EINVAL;
	if (!user_tc.name)
		return -EINVAL;
	if (!user_tc.description)
		return -EINVAL;
	/* Use default uuid cb when NULL */
	cmm_smp_mb();	/* Store callbacks before trace clock */
	CMM_STORE_SHARED(lttng_trace_clock, &user_tc);
	return 0;
}

void lttng_ust_clock_init(void)
{
	const char *libname;
	void (*libinit)(void);

	if (clock_handle)
		return;
	libname = lttng_ust_getenv("LTTNG_UST_CLOCK_PLUGIN");
	if (!libname)
		return;
	clock_handle = dlopen(libname, RTLD_NOW);
	if (!clock_handle) {
		PERROR("Cannot load LTTng UST clock override library %s",
			libname);
		return;
	}
	dlerror();
	libinit = (void (*)(void)) dlsym(clock_handle,
		"lttng_ust_clock_plugin_init");
	if (!libinit) {
		PERROR("Cannot find LTTng UST clock override library %s initialization function lttng_ust_clock_plugin_init()",
			libname);
		return;
	}
	libinit();
}
