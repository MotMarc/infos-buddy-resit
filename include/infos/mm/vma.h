/* SPDX-License-Identifier: MIT */

#pragma once

#include <infos/define.h>
#include <infos/util/list.h>

/* Forward-declare all the types of page table we know about.
 * The details are kept in arch-specific files... mostly! */
namespace infos {
        namespace arch {
                namespace x86 {
	struct GenericX86PageTableEntry;
} } }

namespace infos
{
	namespace mm
	{
		using infos::arch::x86::GenericX86PageTableEntry;
		class FrameDescriptor; // in page-allocator.h

		/* A page table entry can have one or more flags set, in addition to its main field
		 * that is the frame number (of the page, or of the lower-level page table).
		 *
		 * We are in a generic header, so we might need to model the flags of various
		 * instruction sets' page tables. But actually the values used here aree identical to
		 * 64-bit x86's flags! So if InfOS is ported to other architectures we might need to
		 * translate them, but on Intel we don't, and that keeps things simple for us now.
		 *
		 * On 64-bit x86, the lower bits 0..6 inclusive and 8 have the same flag meaning
		 * at all levels in the page table. Bit 7 is Page Size (PS) in higher levels, and
		 * Page Attribute Table (PAT) at the lowest (PT) level. PAT is bit 12 at higher
		 * levels. This picture is useful: https://wiki.osdev.org/File:64-bit_page_tables2.png
		 *
		 * This enum is defined at namespace scope, not inside GenericPageTableEntry, so that
		 * ORing many flags together doesn't get noisy... we can just write PTE_PRESENT
		 * not GenericPageTableEntry::PTE_PRESENT.
		 *
		 * (It's a pity that in C++ we can't use "using" on a class-level definition,
		 * to pull these into scope only where we use them.) */
		enum PageTableEntryFlags {
			PTE_PRESENT		= 1<<0,
			PTE_WRITABLE	= 1<<1,
			PTE_ALLOW_USER	= 1<<2,
			PTE_WRITE_THROUGH	= 1<<3,
			PTE_CACHE_DISABLED	= 1<<4,
			PTE_ACCESSED	= 1<<5,
			PTE_DIRTY		= 1<<6,
			PTE_HUGE		= 1<<7,
			PTE_PS			= 1<<7 /* alias for 'huge' */,
			PTE_PT_PAT		= 1<<7 /* alias for 'huge' */,
			PTE_GLOBAL		= 1<<8,
			PTE_NONPT_PAT	= 1<<12
		};

		/* We want to define a "base class", but we can't use virtual dispatch
		 * because it will add a vtable to our struct's layout and will no longer
		 * match the hardware's layout of the page table entry (usually just a
		 * single word). Instead we use a funky template trick: at compile time,
		 * any use of this class must supply the specific derived class it's
		 * using. We then cast our 'this' point to that type. Since we use
		 * 'static_cast', the compiler only lets us do it if it can see that
		 * we're starting from a type that is related, e.g.
		 * GenericX86PageTableEntry. See include/arch/x86/vma.h for that. */
		template <typename Derived>
		struct GenericPageTableEntry {

			phys_addr_t base_address() const // getter
			{ return static_cast<const Derived*>(this)->base_address(); }
			void base_address(phys_addr_t addr) // setter
			{ return static_cast<Derived*>(this)->base_address(addr); }

			uint16_t flags() const // getter
			{ return static_cast<const Derived*>(this)->flags(); }
			void flags(uint16_t flags) // setter
			{ return static_cast<Derived*>(this)->flags(flags); }

			bool get_flag(PageTableEntryFlags mask) const { return !!(flags() & mask); }
			void set_flag(PageTableEntryFlags mask, bool v) {
				if (!v) {
					flags(flags() & ~mask); 
				} else {
					flags(flags() | mask);
				}
			}

			bool present() const { return get_flag(PageTableEntryFlags::PTE_PRESENT); }
			void present(bool v) { set_flag(PageTableEntryFlags::PTE_PRESENT, v); }

			bool writable() const { return get_flag(PageTableEntryFlags::PTE_WRITABLE); }
			void writable(bool v) { set_flag(PageTableEntryFlags::PTE_WRITABLE, v); }

			bool user() const { return get_flag(PageTableEntryFlags::PTE_ALLOW_USER); }
			void user(bool v) { set_flag(PageTableEntryFlags::PTE_ALLOW_USER, v); }

			bool huge() const { return get_flag(PageTableEntryFlags::PTE_HUGE); }
			void huge(bool v) { set_flag(PageTableEntryFlags::PTE_HUGE, v); }
		};

		/* In InfOS, a virtual address space is called a 'virtual memory area' or VMA.
		 * (Note: in Linux, 'VMA' means something slightly different!)
		 *
		 * Each process has its own VMA, and the implementation of a VMA involves
		 * maintaining a multi-level page table. As this is 64-bit x86, the page table
		 * has four levels (from top to bottom: PML4, PDP, PD, PT).
		 * (Since this header file is generic, i.e. not under the arch/x86 hierarchy,
		 * it should be free of these x86-specific details. But it isn't factored
		 * perfectly at present!)
		 *
		 * The interface for allocating physical memory is provided by a separate object,
		 * of class PageAllocator. (The algorithm used to allocate physical pages
		 * is, in turn, separated from that!  An abstract class PageAllocatorAlgorithm
		 * is defined, with one provided implementation, SimplePageAllocator.) */
		class VMA
		{
		public:
			VMA();
			virtual ~VMA();
			typedef __make_arch_ident(Generic, PageTableEntry) PageTableEntry;
			/* expands to 'GenericX86PageTableEntry' etc... */
			
			phys_addr_t pgt_base() const { return _pgt_phys_base; }
			
			/** Allocate some pages in this VMA, each backed by frames of physical memory.
			 *
			 * @arg order The logarithm (base 2) of how many pages to allocate.
			 *
			 * This works by calling allocate() on the active page allocator. Then
			 * we do some bookkeeping: make a FrameAllocation structure to describe the
			 * allocation, append to the list of page allocations * (a descriptor and a ), then call pnzero.
			 * The 'order' argument is the log base 2 of the number of pages to
			 * allocate. */
			FrameDescriptor *allocate_phys(int order);
			/* Allocates a whole number of pages at a given virtual address, with permissions.
			 * Permissions are bitwise ORed from mm::MappingFlags::MappingFlags. */
			bool allocate_virt(virt_addr_t va, int nr_pages, int perm = -1);
			/* Allocates a whole number of pages at any free virtual address,
			 * with permissions. NOTE: this is unimplemented in vma.cpp. */
			bool allocate_virt_any(int nr_pages, int perm = -1);
			/* Install a mapping from a (virtual) page to a (physical) frame, with permissions. */
			void insert_mapping(virt_addr_t va, phys_addr_t pa, unsigned long flags);
			/* Does this virtual address map to anything? Update pa to the physical address. */
			bool get_mapping(virt_addr_t va, phys_addr_t& pa);
			/* Does this virtual address map to anything? */
			bool is_mapped(virt_addr_t va);
			/* Like allocate_virt, but don't actually allocate physical memory.
			 * Just ensure the page tables exist and nothing is mapped there yet. */
			bool create_unused_ptes(virt_addr_t va, int nr_pages);

			/* The following two calls provide a "cookie API" for inactive
			 * page table entries. The entry's "present" bit remains clear,
			 * so the hardware will ignore it, but the other bits can be
			 * used to store an arbitrary 32-bit value. The page table does
			 * not know or care what this value means -- the client code that
			 * is setting and getting the cookie can use it to represent
			 * whatever it likes. (This is the same sense of "cookie" as in
			 * a web browser -- a cookie is an opaque piece of data, where
			 * the data's meaning is known only to one party but the data is
			 * stored by the other party.) In the week 3 task, you can use
			 * this to store information about disk blocks.... */

			/* Store an arbitrary ('cookie') value into the PTE for the given
			 * virtual address. The PTE *must* exist already
			 * (see create_unused_ptes()) and must be invalid (non-present).
			 * Return true if and only if success. */
			bool set_pte_cookie(virt_addr_t va, uint32_t cookie);
			/* Like get_mapping, but updates cookie to the PTE's stored cookie (iff it
			 * has one, else returns false). NOTE the use of a reference parameter. */
			bool get_pte_cookie(virt_addr_t va, uint32_t& cookie);
			
			void install_default_kernel_mapping();
			
			bool copy_to(virt_addr_t dest_va, const void *src, size_t size);
			
			/* Print the page tables to the MM message log. */
			void dump();
					
		private:
			struct FrameAllocation
			{
				FrameDescriptor *descriptor_base;
				int allocation_order;
			};
			/* This allocation list is a list of <base, run length> pairs
			 * within the FrameDescriptor vector, except that the run length
			 * is really an 'order' (log base 2 of the length). */
			util::List<FrameAllocation> _frame_allocations;
			
			phys_addr_t _pgt_phys_base;
			virt_addr_t _pgt_virt_base;
			// FIXME: move these x86-specific details elsewhere....
			void dump_pdp(int pml4, virt_addr_t pdp_va);
			void dump_pd(int pml4, int pdp, virt_addr_t pd_va);
			void dump_pt(int pml4, int pdp, int pd, virt_addr_t pt_va);
		};
	}
}
