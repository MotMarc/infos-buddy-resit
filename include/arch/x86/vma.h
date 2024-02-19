/* SPDX-License-Identifier: MIT */

/*
 * arch/x86/vma.h
 * 
 * InfOS
 * Copyright (C) University of Edinburgh 2016.  All Rights Reserved.
 * 
 * Tom Spink <tspink@inf.ed.ac.uk>
 *
 * This file cobbled together by Stephen Kell <srk31@srcf.ucam.org>
 */

#pragma once
#include <infos/mm/vma.h>

namespace infos {
        namespace arch {
                namespace x86 {

extern uint64_t *__template_pml4;

#define BITS(val, start, end) ((((uint64_t)val) >> start) & (((1 << (end - start + 1)) - 1)))

typedef uint16_t table_idx_t;

static inline void va_table_indices(virt_addr_t va, table_idx_t& pm, table_idx_t& pdp, table_idx_t& pd, table_idx_t& pt)
{
	pm = BITS(va, 39, 47);
	pdp = BITS(va, 30, 38);
	pd = BITS(va, 21, 29);
	pt = BITS(va, 12, 20);
}

/* A page table entry can have one or more flags set, in addition to its main field
 * that is the frame number (of the page, or of the lower-level page table).
 *
 * Reminder:
 * to understand page tables on x86-64, this picture is useful:
 * https://wiki.osdev.org/File:64-bit_page_tables2.png
 *
 * The base class mm::GenericPageTableEntry is defined in include/infos/mm/vma.h.
 * The trick of inheriting from a template that we instantiate *using ourselves*
 * is called the "Curiously Recurring Template Pattern" or CRTP, and was documented
 * by Jim Coplien in 1995. It is an example of what programming-language
 * theoreticians call "F-bound polymorphism". Here it effectively delays the
 * compilation of the base-class code until after template elaboration, where the
 * compiler knows what the derived class is (here GenericX86PageTableEntry) so can
 * check that the static_cast being done the base class methods is sensible. */
struct GenericX86PageTableEntry : mm::GenericPageTableEntry<GenericX86PageTableEntry> {

	union {
		uint64_t bits;
	};

	inline phys_addr_t base_address() const {
		return bits & ~0xfff;
	}

	inline void base_address(phys_addr_t addr) {
		bits &= 0xfff;
		bits |= addr & ~0xfff;
	}

	inline uint16_t flags() const {
		return bits & 0xfff;
	}

	inline void flags(uint16_t flags) {
		bits &= ~0xfff;
		bits |= flags & 0xfff;
	}
} __packed;
/* check that '__packed' did what it was supposed to, and that
 * nobody has unwisely added fields to the base class. */
static_assert(sizeof (GenericX86PageTableEntry) == 8);

struct PML4TableEntry : GenericX86PageTableEntry {
} __packed;

struct PDPTableEntry : GenericX86PageTableEntry {
} __packed;

struct PDTableEntry : GenericX86PageTableEntry {
} __packed;

struct PTTableEntry : GenericX86PageTableEntry {
} __packed;

} /* end namespace x86 */
} /* end namespace arch */
} /* end namespace infos */
