/* SPDX-License-Identifier: MIT */

/*
 * arch/x86/syscall-trap.S
 * 
 * InfOS
 * Copyright (C) University of Edinburgh 2016.  All Rights Reserved.
 * 
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
.code64
.text

# Currently we don't use 'syscall'. User-space programs
# must use int 0x81 instead. So this is an infinite loop.
.global __syscall_trap
__syscall_trap:
// R11 = RFLAGS
// RCX = RIP

	hlt

1:	jmp 1b

	sysretq

__syscall_user_stack: .quad 0
