/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
# 0 - 8
TRAPHANDLER_NOEC(IDT_DIVIDE, T_DIVIDE)
TRAPHANDLER_NOEC(IDT_DEBUG, T_DEBUG)
TRAPHANDLER_NOEC(IDT_NMI, T_NMI)
TRAPHANDLER_NOEC(IDT_BRKPT, T_BRKPT)
TRAPHANDLER_NOEC(IDT_OFLOW, T_OFLOW)
TRAPHANDLER_NOEC(IDT_BOUND, T_BOUND)
TRAPHANDLER_NOEC(IDT_ILLOP, T_ILLOP)
TRAPHANDLER_NOEC(IDT_DEVICE, T_DEVICE)
TRAPHANDLER(IDT_DBLFLT, T_DBLFLT)
# 9 unused
# 10 - 14
TRAPHANDLER(IDT_TSS, T_TSS)
TRAPHANDLER(IDT_SEGNP, T_SEGNP)
TRAPHANDLER(IDT_STACK, T_STACK)
TRAPHANDLER(IDT_GPFLT, T_GPFLT)
TRAPHANDLER(IDT_PGFLT, T_PGFLT)
# 15 unused
# 16 - 19
TRAPHANDLER_NOEC(IDT_FPERR, T_FPERR)
TRAPHANDLER(IDT_ALIGN, T_ALIGN)
TRAPHANDLER_NOEC(IDT_MCHK, T_MCHK)
TRAPHANDLER_NOEC(IDT_SIMDERR, T_SIMDERR)
# syscall
TRAPHANDLER_NOEC(IDT_SYSCALL, T_SYSCALL)
/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
    # push values to make the stack look like a struct Trapframe
    # load GD_KD into %ds and %es
    # pushl %esp to pass a pointer to the Trapframe as an argument to trap()
    # call trap (can trap ever return?)

	pushl %ds
	pushl %es
	pushal
	movw $(GD_KD), %ax
	movw %ax, %ds
	movw %ax, %es
	pushl %esp
	call trap



