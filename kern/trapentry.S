/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>
#include <kern/macro.h>



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
	pushq $(num);							\
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
	pushq $0;							\
	pushq $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
   Find the PUSHA,POPA macros in kern/macro.h. Since amd64 doesn't support
   pusha,popa so you will find these macros handy.
 */
	TRAPHANDLER_NOEC(TH_0, 0)
	TRAPHANDLER_NOEC(TH_1, 1)
	TRAPHANDLER_NOEC(TH_2, 2)
	TRAPHANDLER_NOEC(TH_3, 3)
	TRAPHANDLER_NOEC(TH_4, 4)
	TRAPHANDLER_NOEC(TH_5, 5)
	TRAPHANDLER_NOEC(TH_6, 6)
	TRAPHANDLER_NOEC(TH_7, 7)
	TRAPHANDLER(TH_8, 8)
	TRAPHANDLER_NOEC(TH_9, 9) /* RESERVED! */
	TRAPHANDLER(TH_10, 10)
	TRAPHANDLER(TH_11, 11)
	TRAPHANDLER(TH_12, 12)
	TRAPHANDLER(TH_13, 13)
	TRAPHANDLER(TH_14, 14)
	TRAPHANDLER_NOEC(TH_15, 15) /* RESERVED! */
	TRAPHANDLER_NOEC(TH_16, 16)
	TRAPHANDLER(TH_17, 17)
	TRAPHANDLER_NOEC(TH_18, 18)
	TRAPHANDLER_NOEC(TH_19, 19)
	TRAPHANDLER_NOEC(TH_48, 48)
	TRAPHANDLER(TH_500, 500)

/*
 * Lab 3: Your code here for _alltraps
 *
 * Hint: Be sure to review the x64 calling convention from lab1
 *       for how to pass a pointer to the trapframe.
 */
_alltraps:
	sub $16,%rsp
	movw %ds,8(%rsp)
	movw %es,0(%rsp)
	PUSHA
	movq $GD_KD,%rax
	movq %rax,%es
	movq %rax,%ds
	movq %rsp,%rdi
	call trap
	
	
