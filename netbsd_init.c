#include <sys/types.h>

#include <sys/exec_elf.h>
#include <sys/exec.h>

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <mini-os/os.h>
#include <mini-os/mm.h>

static char *the_env[1] = { NULL } ;
extern void *environ;
void _libc_init(void);
extern char *__progname;

/* XXX */
static struct ps_strings thestrings;
static AuxInfo myaux[2];
extern struct ps_strings *__ps_strings;
extern size_t pthread__stacksize;

#include "netbsd_init.h"

typedef void (*initfini_fn)(void);
extern const initfini_fn __init_array_start[1];
extern const initfini_fn __init_array_end[1];
extern const initfini_fn __fini_array_start[1];
extern const initfini_fn __fini_array_end[1];

void *__dso_handle;

static void
runinit(void)
{
	const initfini_fn *fn;

	for (fn = __init_array_start; fn < __init_array_end; fn++)
		(*fn)();
}

static void
runfini(void)
{
	const initfini_fn *fn;

	for (fn = __fini_array_start; fn < __fini_array_end; fn++)
		(*fn)();
}

extern const long __eh_frame_start[0];
__weakref_visible void register_frame_info(const void *, const void *)
	__weak_reference(__register_frame_info);
static long dwarf_eh_object[8];

void
_netbsd_init(void)
{

	thestrings.ps_argvstr = (void *)((char *)&myaux - 2);
	__ps_strings = &thestrings;
	pthread__stacksize = 2*STACK_SIZE;

	rump_boot_setsigmodel(RUMP_SIGMODEL_IGNORE);
	rump_init();

	environ = the_env;
	_lwp_rumpxen_scheduler_init();
	if (register_frame_info)
		register_frame_info(__eh_frame_start, &dwarf_eh_object);
	runinit();
	_libc_init();

	/* XXX: we should probably use csu, but this is quicker for now */
	__progname = "rumpxenstack";

#ifdef RUMP_SYSPROXY
	rump_init_server("tcp://0:12345");
#endif

	/*
	 * give all threads a chance to run, and ensure that the main
	 * thread has gone through a context switch
	 */
	sched_yield();
}

void
_netbsd_fini(void)
{
	runfini();
	rump_sys_reboot(0, 0);
}
