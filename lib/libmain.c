// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, uvpd, and uvpt.

#include <inc/lib.h>

extern void umain(int argc, char **argv);

const volatile struct Env *thisenv;
const char *binaryname = "<unknown>";

void
libmain(int argc, char **argv)
{
	// set thisenv to point at our Env structure in envs[].
	// LAB 3: Your code here.
	envid_t envid = sys_getenvid();
	thisenv = &envs[ENVX(envid)];
	if (thisenv->env_status == ENV_FREE || thisenv->env_id != envid) {
		panic("libmain: wrong envid");
	}

	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];
	
	set_pgfault_handler(default_pgfault_handler);

	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}

