/* SPDX-License-Identifier: MIT */

/*
 * arch/x86/trap.S
 *
 * InfOS
 * Copyright (C) University of Edinburgh 2016.  All Rights Reserved.
 *
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
.code64
.text

#define CFI_RETURN_COLUMN 32
#define DWARF_X86_64_RAX     0
#define DWARF_X86_64_RDX     1
#define DWARF_X86_64_RCX     2
#define DWARF_X86_64_RBX     3
#define DWARF_X86_64_RSI     4
#define DWARF_X86_64_RDI     5
#define DWARF_X86_64_RBP     6
#define DWARF_X86_64_RSP     7
#define DWARF_X86_64_R8      8
#define DWARF_X86_64_R9      9
#define DWARF_X86_64_R10     10
#define DWARF_X86_64_R11     11
#define DWARF_X86_64_R12     12
#define DWARF_X86_64_R13     13
#define DWARF_X86_64_R14     14
#define DWARF_X86_64_R15     15
#define DWARF_X86_64_RIP     16

.macro save_context,has_arg
	.cfi_return_column CFI_RETURN_COLUMN
	.cfi_offset CFI_RETURN_COLUMN, 8   // RIP is stored 8 bytes *above* the CFA
	.cfi_offset DWARF_X86_64_RSP, 32   // RSP is stored 32 bytes *above* the CFA
.if !\has_arg
	.cfi_def_cfa DWARF_X86_64_RSP, -8
	push $0
	.cfi_adjust_cfa_offset 8
.else
	.cfi_def_cfa DWARF_X86_64_RSP, 0
.endif
	// We are the first code running in the interrupt handler. So after
	// our push above if needed, our stack looks like this.
	//
	// SS:RSP (original RSP)           (SS pushed first, padded to quad-word)
	// RFLAGS
	// CS (padded to a quadword)
	// RIP
	// error code quadword or zero   <-- %rsp
	//
	// (recall: x86 stack pointer points at the last value pushed)
	//
	// Let's define the canonical frame address as the value of %rsp shown above.
	push %rax
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_RAX, -8
	push %rbx
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_RBX, -16
	push %rcx
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_RCX, -24
	push %rdx
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_RDX, -32
	push %rsi
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_RSI, -40
	push %rdi
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_RDI, -48
	push %rbp
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_RBP, -56
	push %r8
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R8, -64
	push %r9
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R9, -72
	push %r10
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R10, -80
	push %r11
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R11, -88
	push %r12
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R12, -96
	push %r13
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R13, -104
	push %r14
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R14, -112
	push %r15
	.cfi_adjust_cfa_offset 8
	.cfi_offset DWARF_X86_64_R15, -120

	// Load the pointer to the thread context into RAX
	call get_current_thread_context
	mov %rax, %rcx
	mov 0x10(%rcx), %rdi

	test %rdi, %rdi
	jz 1f

	xor %edx, %edx
	mov $7, %eax
	xsave (%rdi)

1:
	// 0(%rcx) is the pointer to the native context, so push this
	// onto the stack.
	push (%rcx)
	.cfi_adjust_cfa_offset 8

	// Now, store the pointer to the new native context into
	// the current thread's native context pointer.
	mov %rsp, (%rcx)
.endm

.macro restore_context
	call get_current_thread_context
	mov %rax, %rcx
	mov 0x10(%rcx), %rdi

	test %rdi, %rdi
	jz 1f

	xor %edx, %edx
	mov $7, %eax
	xrstor (%rdi)

1:
	mov (%rcx), %rsp
	pop (%rcx)
	.cfi_adjust_cfa_offset -8

	pop %r15
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R15
	pop %r14
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R14
	pop %r13
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R13
	pop %r12
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R12
	pop %r11
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R11
	pop %r10
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R10
	pop %r9
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R9
	pop %r8
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_R8
	pop %rbp
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_RBP
	pop %rdi
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_RDI
	pop %rsi
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_RSI
	pop %rdx
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_RDX
	pop %rcx
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_RCX
	pop %rbx
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_RBX
	pop %rax
	.cfi_adjust_cfa_offset -8
	.cfi_same_value DWARF_X86_64_RAX
	add $8, %rsp   // i.e. discard the argument or the zero we pushed earlier
	.cfi_adjust_cfa_offset -8


.endm

// FIXME: We should not call this stuff 'irq' because it's really 'interrupt'.
// The 8259A PIC's "IRQs" live at a configurable offset within this space.
.macro defirq,nr,has_arg
.align 16
.global __irq\nr
__irq\nr:
	.cfi_startproc simple
	.cfi_signal_frame     // disables gdb's "previous frame inner to this frame" sanity check
	// Save the current context
	// Some IRQs come with an argument pre-pushed on the stack.
	// If \has_arg is false, we will push a literal zero to ensure a
	// uniform frame layout.
	save_context \has_arg

	// The first argument is the interrupt number.
	mov $\nr, %edi

	// Call the interrupt handler
	call __handle_raw_irq

	// Restore the current context -- this unconditionally drops
	// the argument or dummy zero we pushed, as leaving the argument
	// in place is not architecturally required.
	restore_context

	// Return from interrupt.
	iretq
	.cfi_endproc
.endm

// Which interrupts push an argument? "faults", including 8, 10, 11, 12, 13, 14, 17 and 30.
// 8:  double fault
// 10: invalid TSS
// 11: segment not present
// 12: stack-segment fault
// 13: general protection fault
// 14: page fault
// 17: alignment check
// 30: (reserved?)
// Useful reference: https://wiki.osdev.org/Interrupt_Vector_Table
defirq 0,0
defirq 1,0
defirq 2,0
defirq 3,0
defirq 4,0
defirq 5,0
defirq 6,0
defirq 7,0
defirq 8,1
defirq 9,0
defirq 10,1
defirq 11,1
defirq 12,1
defirq 13,1
defirq 14,1
defirq 15,0
defirq 16,0
defirq 17,1
defirq 18,0
defirq 19,0
defirq 20,0
defirq 21,0
defirq 22,0
defirq 23,0
defirq 24,0
defirq 25,0
defirq 26,0
defirq 27,0
defirq 28,0
defirq 29,0
defirq 30,1
defirq 31,0
defirq 32,0
defirq 33,0
defirq 34,0
defirq 35,0
defirq 36,0
defirq 37,0
defirq 38,0
defirq 39,0
defirq 40,0
defirq 41,0
defirq 42,0
defirq 43,0
defirq 44,0
defirq 45,0
defirq 46,0
defirq 47,0
defirq 48,0
defirq 49,0
defirq 50,0
defirq 51,0
defirq 52,0
defirq 53,0
defirq 54,0
defirq 55,0
defirq 56,0
defirq 57,0
defirq 58,0
defirq 59,0
defirq 60,0
defirq 61,0
defirq 62,0
defirq 63,0
defirq 64,0
defirq 65,0
defirq 66,0
defirq 67,0
defirq 68,0
defirq 69,0
defirq 70,0
defirq 71,0
defirq 72,0
defirq 73,0
defirq 74,0
defirq 75,0
defirq 76,0
defirq 77,0
defirq 78,0
defirq 79,0
defirq 80,0
defirq 81,0
defirq 82,0
defirq 83,0
defirq 84,0
defirq 85,0
defirq 86,0
defirq 87,0
defirq 88,0
defirq 89,0
defirq 90,0
defirq 91,0
defirq 92,0
defirq 93,0
defirq 94,0
defirq 95,0
defirq 96,0
defirq 97,0
defirq 98,0
defirq 99,0
defirq 100,0
defirq 101,0
defirq 102,0
defirq 103,0
defirq 104,0
defirq 105,0
defirq 106,0
defirq 107,0
defirq 108,0
defirq 109,0
defirq 110,0
defirq 111,0
defirq 112,0
defirq 113,0
defirq 114,0
defirq 115,0
defirq 116,0
defirq 117,0
defirq 118,0
defirq 119,0
defirq 120,0
defirq 121,0
defirq 122,0
defirq 123,0
defirq 124,0
defirq 125,0
defirq 126,0
defirq 127,0
defirq 128,0
defirq 129,0
defirq 130,0
defirq 131,0
defirq 132,0
defirq 133,0
defirq 134,0
defirq 135,0
defirq 136,0
defirq 137,0
defirq 138,0
defirq 139,0
defirq 140,0
defirq 141,0
defirq 142,0
defirq 143,0
defirq 144,0
defirq 145,0
defirq 146,0
defirq 147,0
defirq 148,0
defirq 149,0
defirq 150,0
defirq 151,0
defirq 152,0
defirq 153,0
defirq 154,0
defirq 155,0
defirq 156,0
defirq 157,0
defirq 158,0
defirq 159,0
defirq 160,0
defirq 161,0
defirq 162,0
defirq 163,0
defirq 164,0
defirq 165,0
defirq 166,0
defirq 167,0
defirq 168,0
defirq 169,0
defirq 170,0
defirq 171,0
defirq 172,0
defirq 173,0
defirq 174,0
defirq 175,0
defirq 176,0
defirq 177,0
defirq 178,0
defirq 179,0
defirq 180,0
defirq 181,0
defirq 182,0
defirq 183,0
defirq 184,0
defirq 185,0
defirq 186,0
defirq 187,0
defirq 188,0
defirq 189,0
defirq 190,0
defirq 191,0
defirq 192,0
defirq 193,0
defirq 194,0
defirq 195,0
defirq 196,0
defirq 197,0
defirq 198,0
defirq 199,0
defirq 200,0
defirq 201,0
defirq 202,0
defirq 203,0
defirq 204,0
defirq 205,0
defirq 206,0
defirq 207,0
defirq 208,0
defirq 209,0
defirq 210,0
defirq 211,0
defirq 212,0
defirq 213,0
defirq 214,0
defirq 215,0
defirq 216,0
defirq 217,0
defirq 218,0
defirq 219,0
defirq 220,0
defirq 221,0
defirq 222,0
defirq 223,0
defirq 224,0
defirq 225,0
defirq 226,0
defirq 227,0
defirq 228,0
defirq 229,0
defirq 230,0
defirq 231,0
defirq 232,0
defirq 233,0
defirq 234,0
defirq 235,0
defirq 236,0
defirq 237,0
defirq 238,0
defirq 239,0
defirq 240,0
defirq 241,0
defirq 242,0
defirq 243,0
defirq 244,0
defirq 245,0
defirq 246,0
defirq 247,0
defirq 248,0
defirq 249,0
defirq 250,0
defirq 251,0
defirq 252,0
defirq 253,0
defirq 254,0
defirq 255,0
