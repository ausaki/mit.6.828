// User-level page fault handler support.
// Rather than register the C page fault handler directly with the
// kernel as the page fault handler, we register the assembly language
// wrapper in pfentry.S, which in turns calls the registered C
// function.

#include <inc/lib.h>


// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _pgfault_upcall(void);

// Pointer to currently installed C-language pgfault handler.
void (*_pgfault_handler)(struct UTrapframe *utf);

//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _pgfault_upcall routine when a page fault occurs.
//

void _set_pgfault_upcall(envid_t envid){
	int err;
	if((err = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P)) < 0){
		panic("set_pgfault_handler: %e", err);
	}
	if((err = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0){
		panic("set_pgfault_handler: %e", err);
	}
}
void
set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
{
	int r;

	if (_pgfault_handler == 0) {
		// First time through!
		// LAB 4: Your code here.
		_set_pgfault_upcall(0);
	}

	// Save handler pointer for assembly to call.
	_pgfault_handler = handler;
}

/*
 * increase user stack
 */
void default_pgfault_handler(struct UTrapframe *utf){
	uint32_t addr = utf->utf_fault_va;
	int r;

	int ustackbottom = USTACKTOP - 100 * PGSIZE;
	if(addr >= USTACKTOP){
		panic("default_pgfault_handler: invalid addr = %08x", addr);
	}
	if(addr < ustackbottom){
		panic("default_pgfault_handler: stackoverflow");
	}
	
	addr = ROUNDDOWN(addr, PGSIZE);
	int perm = PTE_U | PTE_W | PTE_P;
	if((r = sys_page_alloc(0, (void *)addr, perm)) < 0){
		panic("default_pgfault_handler: %e", r);
	}
}