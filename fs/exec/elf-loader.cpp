/*
 * fs/exec/elf-loader.cpp
 *
 * InfOS
 * Copyright (C) University of Edinburgh 2016.  All Rights Reserved.
 *
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
#include <infos/fs/exec/elf-loader.h>
#include <infos/fs/file.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/process.h>
#include <infos/mm/mm.h>
#include <infos/mm/page-allocator.h>
#include <arch/x86/vma.h> /* HACK: make sure we have the x86 page table definitions in scope */
using namespace infos::kernel;
using namespace infos::fs;
using namespace infos::fs::exec;
using namespace infos::util;

ComponentLog infos::fs::exec::elf_log(syslog, "elf");

#define MAGIC_NUMBER 0x464c457f

#define ECLASS_32BIT 1
#define ECLASS_64BIT 2
#define EDATA_LITTLE 1
#define EDATA_BIG 2

ElfLoader::ElfLoader(File &f) : _file(f)
{
}

Process *ElfLoader::load(const String &cmdline)
{
	ELF64Header hdr;

	_file.seek(0, File::SeekAbsolute);
	int bytes = _file.read(&hdr, sizeof(hdr));

	if (bytes != sizeof(hdr))
	{
		elf_log.messagef(LogLevel::DEBUG, "Unable to read ELF header");
		return NULL;
	}

	if (hdr.ident.magic_number != MAGIC_NUMBER)
	{
		elf_log.messagef(LogLevel::DEBUG, "Invalid ELF magic number %x", hdr.ident.magic_number);
		return NULL;
	}

	if (hdr.ident.eclass != ECLASS_64BIT)
	{
		elf_log.message(LogLevel::DEBUG, "Only 64-bit ELF programs are supported");
		return NULL;
	}

	if (hdr.ident.data != EDATA_LITTLE)
	{
		elf_log.message(LogLevel::DEBUG, "Only little-endian ELF programs are supported");
		return NULL;
	}

	if (hdr.type != ELFType::ET_EXEC)
	{
		elf_log.messagef(LogLevel::DEBUG, "Only executables can be loaded (%d)", (int) hdr.type);
		return NULL;
	}

	if (hdr.machine != 0x3e)
	{
		elf_log.messagef(LogLevel::DEBUG, "Unsupported instruction set architecture (%d)", hdr.machine);
		return NULL;
	}

	if (hdr.version != 1)
	{
		elf_log.message(LogLevel::DEBUG, "Invalid ELF version");
		return NULL;
	}

	bool use_interp = false;

	Process *np = new Process("user", false, (Thread::thread_proc_t)hdr.entry_point, &_file);
	for (unsigned int i = 0; i < hdr.phnum; i++)
	{
		ELF64ProgramHeaderEntry ent;

		off_t off = hdr.phoff + (i * hdr.phentsize);
		if (_file.pread(&ent, sizeof(ent), off) != sizeof(ent))
		{
			delete np;

			elf_log.message(LogLevel::DEBUG, "Unable to read PH entry");
			return NULL;
		}

		switch (ent.type)
		{
		case ProgramHeaderEntryType::PT_LOAD:
		{
			uintptr_t file_end_vaddr = ent.vaddr + ent.filesz;
			uintptr_t nextpage = __align_down_page(ent.vaddr + __page_size);
			// for each page that any part of this segment overlaps...
			for (uintptr_t vaddr = ent.vaddr; vaddr < ent.vaddr + ent.memsz;
				vaddr = nextpage, nextpage += __page_size)
			{
				size_t sz;
				unsigned long offset;
				// we have to allocate a page backed by real memory
				if (!np->vma().is_mapped(vaddr)) np->vma().allocate_virt(vaddr, 1, -1);
				if (vaddr % __page_size == 0
					&& nextpage <= file_end_vaddr)
				{
					/* Easy case: here we need a whole page of data from the file */
					sz = __page_size;
					offset = ent.offset + (vaddr - ent.vaddr);
				}
				else if (vaddr == ent.vaddr && nextpage < file_end_vaddr)
				{
					offset = ent.offset;
					// the file data continues, but this time around, we
					// copy just the part in this page
					sz = __align_up_page(ent.vaddr) - ent.vaddr;
				}
				else if (vaddr >= file_end_vaddr)
				{
					// we're after the end of file data... this part needs to be
					// zero-initialized (allocate_phys does this)
					continue;
				}
				else
				{
					// last page with file data, not a whole page
					assert(vaddr < file_end_vaddr);
					assert(nextpage > file_end_vaddr);
					sz = file_end_vaddr - vaddr;
					offset = ent.offset + (vaddr - ent.vaddr);
				}
				char *buffer = new char[sz];
				_file.pread(buffer, sz, offset);
				np->vma().copy_to(vaddr, buffer, sz);
				delete buffer;
			}
		}
		break;

		case ProgramHeaderEntryType::PT_INTERP:
		{
			char *buffer = new char[ent.filesz];
			_file.pread(buffer, ent.filesz, ent.offset);

			if (strncmp(buffer, "__INFOS_DYNAMIC_LINKER__", ent.filesz) != 0)
			{
				delete buffer;
				delete np;

				elf_log.message(LogLevel::DEBUG, "Unsupported ELF interpreter");
				return NULL;
			}

			syslog.messagef(LogLevel::DEBUG, "Interp: %s", buffer);
			delete buffer;

			use_interp = true;
		}
		break;

		default:
			break;
		}
	}

	if (use_interp) {
		elf_log.message(LogLevel::DEBUG, "Dynamic linked executables not supported");
		return NULL;
	}

	np->main_thread().allocate_user_stack(0x100000, 0x2000);

	if (cmdline.length() > 0)
	{
		virt_addr_t cmdline_start = 0x102000;
		np->vma().allocate_virt(cmdline_start, 1);

		if (!np->vma().copy_to(cmdline_start, cmdline.c_str(), cmdline.length()))
		{
			return NULL;
		}

		np->main_thread().add_entry_argument((void *)cmdline_start);
	}
	else
	{
		np->main_thread().add_entry_argument(NULL);
	}

	return np;
}
