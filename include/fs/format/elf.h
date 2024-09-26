#pragma once

#include <klibc/stdlib.h>

typedef		uint32_t	ELF32_Addr_t;
typedef		uint32_t	ELF32_Off_t;
typedef 	uint16_t	ELF32_Half_t;
typedef		uint32_t	ELF32_Word_t;

#define ELF_MAGIC	(uint32_t)0x464C457F //0x7F + "ELF"

#define SHN_UNDEF	0

#define	ET_NONE		0		//No file type
#define	ET_REL		1		//Relocatable file
#define	ET_EXEC		2		//Executable file
#define	ET_DYN		3		//Shared object file
#define	ET_CORE		4		//Core file
#define	ET_LOOS		0xFE00	//Operating system-specific
#define	ET_HIOS		0xFEFF	//Operating system-specific
#define	ET_LOPROC	0xFF00	//Processor-specific
#define	ET_HIPROC	0xFFFF	//Processor-specific

#define	ELFCLASSNONE	0	//Invalid class
#define ELFCLASS32		1	//32-bit objects
#define ELFCLASS64		2	//64-bit objects

#define	ELFDATANONE 	0	//Invalid data encoding
#define	ELFDATA2LSB		1	//Least significant bit
#define	ELFDATA2MSB 	2	//Most significant bit

#define	ESHT_NULL		0
#define	ESHT_PROGBITS 	1
#define	ESHT_SYMTAB 	2
#define	ESHT_STRTAB		3
#define	ESHT_RELA		4
#define	ESHT_HASH		5
#define	ESHT_DYNAMIC 	6
#define	ESHT_NOTE	 	7
#define	ESHT_NOBITS 	8
#define	ESHT_REL 		9

#define	SHF_WRITE				0x1
#define	SHF_ALLOC 				0x2
#define	SHF_EXECINSTR			0x4
#define	SHF_MERGE				0x10
#define	SHF_STRINGS				0x20
#define	SHF_INFO_LINK			0x40
#define	SHF_LINK_ORDER			0x80
#define	SHF_OS_NONCONFORMING	0x100
#define	SHF_GROUP				0x200
#define	SHF_TLS					0x400
#define	SHF_COMPRESSED			0x800
#define	SHF_MASKOS				0x0ff00000
#define	SHF_MASKPROC			0xf0000000

#define	PT_NULL 	0
#define	PT_LOAD 	1
#define	PT_DYNAMIC 	2
#define	PT_INTERP 	3
#define	PT_NOTE 	4
#define	PT_SHLIB 	5
#define	PT_PHDR 	6
#define	PT_TLS 		7
#define	PT_LOOS 	0x60000000
#define	PT_HIOS 	0x6fffffff
#define	PT_LOPROC 	0x70000000
#define	PT_HIPROC 	0x7fffffff

typedef struct {
	struct {
		uint32_t magic;
		uint8_t class;
		uint8_t data;
		uint8_t version;
		uint8_t osabi;
		uint8_t abiversion;
		uint8_t unused[7];
	} ident;
	ELF32_Half_t	type;
	ELF32_Half_t	machine;
	ELF32_Word_t	version;
	ELF32_Addr_t	entry;
	ELF32_Off_t		phoff;
	ELF32_Off_t		shoff;
	ELF32_Word_t	flags;
	ELF32_Half_t	ehsize;
	ELF32_Half_t	phentsize;
	ELF32_Half_t	phnum;
	ELF32_Half_t	shentsize;
	ELF32_Half_t	shnum;
	ELF32_Half_t	shstrndx;
} ELF32Header_t;

typedef struct {
	ELF32_Word_t	type;
	ELF32_Off_t		offset;
	ELF32_Addr_t	vaddr;
	ELF32_Addr_t	paddr;
	ELF32_Word_t	filesz;
	ELF32_Word_t	memsz;
	ELF32_Word_t	flags;
	ELF32_Word_t	align;
} ELF32ProgramHeader_t;

typedef struct {
	ELF32_Word_t	name;
	ELF32_Word_t	type;
	ELF32_Word_t	flags;
	ELF32_Addr_t	addr;
	ELF32_Off_t		offset;
	ELF32_Word_t	size;
	ELF32_Word_t	link;
	ELF32_Word_t	info;
	ELF32_Word_t	addralign;
	ELF32_Word_t	entsize;
} ELF32SectionHeader_t;

typedef struct {
	ELF32_Word_t	name;
	ELF32_Addr_t	value;
	ELF32_Word_t	size;
	uint8_t			info;
	uint8_t			other;
	ELF32_Half_t	shndx;
} ELF32Symbol_t;

typedef struct {
	ELF32SectionHeader_t*	sectab_head;
	uint32_t				sectab_size;

	ELF32ProgramHeader_t*	progtab_head;
 	uint32_t				progtab_size;

	ELF32Symbol_t*			symtab_head;
	uint32_t				symtab_size;

	uint8_t*				strtab_head;
	uint32_t				strtab_size;

	uint8_t*				hstrtab_head;
} ELF32SectionObj_t;

typedef struct {
	ELF32Header_t*			header;
	ELF32SectionObj_t		sections;
	uint32_t				start;
	uint32_t				end;
} ELF32Obj_t;

#define ELF32_HEADER(h)				((h)->header)
#define ELF32_OFFSET(p, off)		((uint8_t*)(p) + (off))
#define ELF32_ENTRY(h, idx)			(&(h)[idx])
#define ELF32_TABLE(h, tbl)			((h)->sections.tbl##tab_head)
#define ELF32_TABLE_SIZE(h, tbl)	((h)->sections.tbl##tab_size)

#define ELF32_PROGTAB(h)			((ELF32ProgramHeader_t*)ELF32_OFFSET(ELF32_HEADER(h), ELF32_HEADER(h)->phoff));
#define ELF32_PROGTAB_NENTRIES(h)	(ELF32_TABLE_SIZE(h, prog))

#define ELF32_SECTAB(h)				((ELF32SectionHeader_t*)ELF32_OFFSET(ELF32_HEADER(h), ELF32_HEADER(h)->shoff));
#define ELF32_SECTAB_NENTRIES(h)	(ELF32_TABLE_SIZE(h, sec))

#define ELF32_STRTAB_SIZE(h)		(ELF32_TABLE_SIZE(h, str))

#define ELF32_SECTION(h, n)			((ELF32SectionHeader_t*)ELF32_ENTRY(ELF32_TABLE(h, sec), n))
#define ELF32_PROGRAM(h, n)			((ELF32ProgramHeader_t*)ELF32_ENTRY(ELF32_TABLE(h, prog), n))

#define ELF32_HSTRTAB(h)			((uint8_t*)ELF32_OFFSET(ELF32_HEADER(h), ELF32_SECTION(h, (ELF32_HEADER(h)->shstrndx))->offset))
#define ELF32_SYMTAB_NENTRIES(h)	(ELF32_TABLE_SIZE(h, sym))

#define ELF32_LOOKUP_SECTION_BY_NAME(h, n) (ELF32SectionHeader_t*)ELF32_OFFSET(ELF32_TABLE(h, sec), ELFLookupSectionByName(h, n)->offset);

#define ELF32_ST_BIND(i)	((i) >> 4)
#define ELF32_ST_TYPE(i)	((i) & 0xf)
#define ELF32_ST_INFO(b,t)	(((b) << 4) + ((t) & 0xf))

#define	STB_LOCAL	0
#define STB_GLOBAL 	1
#define STB_WEAK 	2
#define STB_LOOS 	10
#define STB_HIOS 	12
#define STB_LOPROC 	13
#define STB_HIPROC 	15

#define STT_TYPE(i)		((i) & 0xF)
#define STT_BIND(i)		((i) >> 4)
#define STT_INFO(b, t)	(((b) << 4) + ((t) & 0xF))
#define STT_CHECKTYPE(d, t)	(STT_TYPE(((ELF32Symbol_t*)d)->info) == (t))

#define STT_NOTYPE 	0
#define STT_OBJECT 	1
#define STT_FUNC 	2
#define STT_SECTION 3
#define STT_FILE 	4
#define STT_COMMON 	5
#define STT_TLS 	6
#define STT_LOOS 	10
#define STT_HIOS 	12
#define STT_LOPROC 	13
#define STT_HIPROC 	15

bool					ELF32CheckMagic(ELF32Header_t* hdr);
ELF32SectionHeader_t* 	ELFLookupSectionByName(ELF32Obj_t* hdr, char* name);
ELF32SectionHeader_t* 	ELFLookupSectionByType(ELF32Obj_t* hdr, uint8_t type);
ELF32SectionHeader_t* 	ELFFindNearestSectionByAddress(ELF32Obj_t* hdr, uint32_t address);
ELF32Symbol_t* 			ELFLookupSymbolByName(ELF32Obj_t* hdr, uint8_t type, char* name);
ELF32Symbol_t* 			ELFFindNearestSymbolByAddress(ELF32Obj_t* hdr, uint8_t type, uint32_t address);
bool 					ELFLoad(uint32_t address, ELF32Obj_t* out);