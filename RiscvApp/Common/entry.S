// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
#include "custom_ops.S"

.section .text
	.global hard_mul
	.global hard_mulh
	.global hard_mulhsu
	.global hard_mulhu
	.global multest
	.global start


/* Main program
 **********************************/

start:
	/* zero-initialize all registers */

	addi x1, zero, 0
	addi x2, zero, 0
	addi x3, zero, 0
	addi x4, zero, 0
	addi x5, zero, 0
	addi x6, zero, 0
	addi x7, zero, 0
	addi x8, zero, 0
	addi x9, zero, 0
	addi x10, zero, 0
	addi x11, zero, 0
	addi x12, zero, 0
	addi x13, zero, 0
	addi x14, zero, 0
	addi x15, zero, 0
	addi x16, zero, 0
	addi x17, zero, 0
	addi x18, zero, 0
	addi x19, zero, 0
	addi x20, zero, 0
	addi x21, zero, 0
	addi x22, zero, 0
	addi x23, zero, 0
	addi x24, zero, 0
	addi x25, zero, 0
	addi x26, zero, 0
	addi x27, zero, 0
	addi x28, zero, 0
	addi x29, zero, 0
	addi x30, zero, 0
	addi x31, zero, 0

	/* running tests from riscv-tests */

#ifdef ENABLE_RVTST
#  define TEST(n) \
	.global n; \
	addi x1, zero, 1000; \
	picorv32_timer_insn(zero, x1); \
	jal zero,n; \
	.global n ## _ret; \
	n ## _ret:
#else
#  define TEST(n) \
	.global n ## _ret; \
	n ## _ret:
#endif

#ifdef ENABLE_INSTRTST
	TEST(lui)
	TEST(auipc)
	TEST(j)
	TEST(jal)
	TEST(jalr)

	TEST(beq)
	TEST(bne)
	TEST(blt)
	TEST(bge)
	TEST(bltu)
	TEST(bgeu)

	TEST(lb)
	TEST(lh)
	TEST(lw)
	TEST(lbu)
	TEST(lhu)

	TEST(sb)
	TEST(sh)
	TEST(sw)

	TEST(addi)
	TEST(slti) // also tests sltiu
	TEST(xori)
	TEST(ori)
	TEST(andi)
	TEST(slli)
	TEST(srli)
	TEST(srai)

	TEST(add)
	TEST(sub)
	TEST(sll)
	TEST(slt) // what is with sltu ?
	TEST(xor)
	TEST(srl)
	TEST(sra)
	TEST(or)
	TEST(and)

	TEST(mulh)
	TEST(mulhsu)
	TEST(mulhu)
	TEST(mul)

	TEST(div)
	TEST(divu)
	TEST(rem)
	TEST(remu)

	TEST(simple)
#endif    // ENABLE_INSTRTST

	/* set stack pointer */
	lui sp,(32*1024)>>12

	/* set gp and tp */
	lui gp, %hi(0xdeadbeef)
	addi gp, gp, %lo(0xdeadbeef)
	addi tp, gp, 0

#ifdef ENABLE_SIEVE
	/* call sieve C code */
	jal ra,sieve
#endif

#ifdef ENABLE_MULTST
	/* call multest C code */
	jal ra,multest
#endif

#ifdef ENABLE_STATS
	/* call stats C code */
	jal ra,stats
#endif


     /* call main C code */
     jal ra,main


	/* print "DONE\n" */
	li a0,0xffff0040
	addi a1,zero,'D'
	addi a2,zero,'O'
	addi a3,zero,'N'
	addi a4,zero,'E'
	addi a5,zero,'\n'
	sw a1,0(a0)
	sw a2,0(a0)
	sw a3,0(a0)
	sw a4,0(a0)
	sw a5,0(a0)

	li a0, 0x20000000
	li a1, 123456789
	sw a1,0(a0)

	/* trap */
	ebreak


/* Hard mul functions for multest.c
 **********************************/

hard_mul:
	mul a0, a0, a1
	ret

hard_mulh:
	mulh a0, a0, a1
	ret

hard_mulhsu:
	mulhsu a0, a0, a1
	ret

hard_mulhu:
	mulhu a0, a0, a1
	ret


// extern uint32_t _picorv32_irq_mask( uint32_t iMask );
// setting a bit in `iMask` disables the corresponding interrupt
// returns the previous interrupt mask
.globl _picorv32_irq_mask
_picorv32_irq_mask:
    picorv32_maskirq_insn(a0, a0)
    ret

// extern uint32_t _picorv32_irq_disable( uint32_t irqsToDisable );
// Disable individual interrupts by setting specific bits in the interrupt mask register
// returns the current interrupt mask
.globl _picorv32_irq_disable
_picorv32_irq_disable:
	li t0, 0xFFFFFFFF
    picorv32_maskirq_insn(t0, t0)	// t0 = currentMask, also disable all interrupts
    or a0, a0, t0                   // a0 = bitsToSet | t0
    picorv32_maskirq_insn(zero, a0)	// set new interrupt mask
    ret    							// return( a0 );

// extern uint32_t _picorv32_irq_enable( uint32_t irqsToEnable );
// Enable individual interrupts by clearing specific bits in the interrupt mask register
// returns the current interrupt mask
.globl _picorv32_irq_enable
_picorv32_irq_enable:
	li t0, 0xFFFFFFFF
	xor a0, a0, t0					// a0 = ~bitsToClear;
    picorv32_maskirq_insn(t0, t0)	// t0 = currentMask, also disable all interrupts
    and a0, a0, t0                  // a0 = a0 & currentMask
    picorv32_maskirq_insn(zero, a0)	// set new interrupt mask
    ret    							// return( a0 );

// extern uint32_t _picorv32_irq_timer( uint32_t tVal );
// loads the timer with `tVal`. Counting down with every instruction,
// it will trigger int0 and stop once it reaches zero
// returns the previous timer value before the reload
.globl _picorv32_irq_timer
_picorv32_irq_timer:
    picorv32_timer_insn(a0, a0)
    ret
