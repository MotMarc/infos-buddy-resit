/* SPDX-License-Identifier: MIT */

#pragma once

#include <infos/define.h>
#include <infos/util/list.h>

namespace infos
{
	namespace mm
	{
		class FrameDescriptor; // in page-allocator.h
		
		namespace MappingFlags
		{
			enum MappingFlags
			{
				None		= 0,
				Present		= 1,
				User		= 2,
				Writable	= 4
			};
			
			static inline MappingFlags operator|(const MappingFlags& l, const MappingFlags& r)
			{
				return (MappingFlags)((uint64_t)l | (uint64_t)r);
			}
		}
		
		/* In InfOS, a virtual address space is called a 'virtual memory area' or VMA.
		 * Each process has its own VMA, and the implementation of a VMA involves
		 * maintaining a multi-level page table. As this is 64-bit x86, the page table
		 * has four levels (from top to bottom: PML4, PDP, PD, PT).
		 * (Note: in Linux, 'VMA' means something slightly different!)
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
			void insert_mapping(virt_addr_t va, phys_addr_t pa, MappingFlags::MappingFlags flags);
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
			
			void dump_pdp(int pml4, virt_addr_t pdp_va);
			void dump_pd(int pml4, int pdp, virt_addr_t pd_va);
			void dump_pt(int pml4, int pdp, int pd, virt_addr_t pt_va);
		};
	}
}
