#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#include "symtab.h"
#include "ram.h"

void
Ram::
reset_all()
{
	for (size_t i = 0; i < memory_size; i++)
	{
		type_record[i] = 0;
		base[i]        = 0;
	}

	delete[] type_record;
	type_record = new u8[memory_size];
	free(base);
	base = (u8*) malloc(memory_size);
	loadin_program(file_path);
}

void
Ram::
typerecord_input_file()
{
	u32 curr_addr = METADATA_SIZE; // set curr_addr to the first byte after metadata.
	u8* ptr = prog;          // set ptr to first instr.

	// enter metadata type-record information.
	type_record[MD_START_INSTR_ADDR_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_START_INSTR_ADDR_OFFSET + i] = INSIDE_UINT_CODE;

	type_record[MD_MEMORY_SIZE_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_MEMORY_SIZE_OFFSET + i] = INSIDE_UINT_CODE;

	type_record[MD_PROG_SIZE_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_PROG_SIZE_OFFSET + i] = INSIDE_UINT_CODE;


	type_record[MD_INSTR_COUNT_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_INSTR_COUNT_OFFSET + i] = INSIDE_UINT_CODE;

	type_record[MD_FIRST_INSTR_ADDR_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_FIRST_INSTR_ADDR_OFFSET + i] = INSIDE_UINT_CODE;

	type_record[MD_LAST_INSTR_ADDR_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_LAST_INSTR_ADDR_OFFSET + i] = INSIDE_UINT_CODE;

	type_record[MD_FIRST_USER_BYTE_ADDR_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_FIRST_USER_BYTE_ADDR_OFFSET + i] = INSIDE_UINT_CODE;

	type_record[MD_FLAGS_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_FLAGS_OFFSET + i] = INSIDE_UINT_CODE;

	type_record[MD_CREATION_DATE_OFFSET] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[MD_CREATION_DATE_OFFSET + i] = INSIDE_UINT_CODE;

	if (symbols_kept)
	{
		// for simiplicity write all symtab bytes as INSIDE_SYMTAB_CODE.
		// TODO: possibly improve this so that symtab bytes are accurate type-recorded.
		for (; curr_addr < (METADATA_SIZE + symtab_size); curr_addr++)
			type_record[curr_addr] = INSIDE_SYMTAB_CODE;
	}

	// align curr_addr after finishing the symbols.
	if (curr_addr % 4 != 0)
		curr_addr += 4 - (curr_addr % 4);

process_next_op:
    // make type-record for the opcode.
    type_record[curr_addr++] = UINT8_CODE;

	for (u8 i = 0; i < (OPCODE_SIZE - 1); i++)
		type_record[curr_addr++] = INSIDE_UINT8_PADDING_CODE;

	// switch on the opcode to determine how to type-record the operand(s).
	switch (*ptr)
	{
		// handle die-opcode triggering end of type-record processing.	
		case die_op:
		case test_die_op:
			return;

		// handle all no-operand opcodes.
		case swtch_op:  case lbrk_op:  case pop_op:      case pop2_op:
		case inc_op:    case dec_op:   case add_op:      case sub_op:
		case mul_op:    case div_op:   case mod_op:      case incs_op:
		case decs_op:   case adds_op:  case subs_op:     case muls_op:
		case divs_op:   case mods_op:  case and_op:      case not_op:
		case xor_op:    case or_op:    case lshft_op:    case rshft_op:
		case lrot_op:   case rrot_op:  case ands_op:     case nots_op:
		case xors_op:   case ors_op:   case lshfts_op:   case rshfts_op:
		case lrots_op:  case rrots_op: case brkp_op:     case lcont_op:
		case pshfrs_op: case nop_op:   case ret_op:      case nspctst_op:
			ptr += OPCODE_SIZE;
			goto process_next_op;

		// handle all 1-uint-operand opcodes.
		case nspctr_op: case call_op:  case jmp_op:
		case je_op:     case jn_op:      case jl_op:    case jg_op:
		case jls_op:    case jgs_op:     case psh_op:   case popn_op:
		case pshfr_op:  case poptr_op:   case movtr_op: case pshfrr_op:
			type_record[curr_addr++] = UINT32_CODE;

			for (u8 i = 0; i < (UINT_SIZE - 1); i++)
				type_record[curr_addr++] = INSIDE_UINT_CODE;

			ptr += (OPCODE_SIZE + UINT_SIZE);
			goto process_next_op;

		// handle all 3-uint-operand opcodes.
	case loop_op:
		for (int n = 0; n < 3; n++)
		{
			type_record[curr_addr++] = UINT32_CODE;

			for (u8 i = 0; i < (UINT_SIZE - 1); i++)
				type_record[curr_addr++] = INSIDE_UINT_CODE;
		}

		ptr += (OPCODE_SIZE + UINT_SIZE + UINT_SIZE + UINT_SIZE);
		goto process_next_op;

		// handle all 2-uint-operand opcodes.
	case stktr_op:
	case cpyr_op:
	case setr_op:
		for (int n = 0; n < 2; n++)
		{
			type_record[curr_addr++] = UINT32_CODE;

			for (u8 i = 0; i < (UINT_SIZE - 1); i++)
				type_record[curr_addr++] = INSIDE_UINT_CODE;

			ptr += (OPCODE_SIZE + UINT_SIZE);
		}

		goto process_next_op;

	default:
		throw RamError::INVALID_OPCODE_IN_TYPEREC;
	}
}

Ram::
Ram(u32 memory_size,
	u8* _memory)
:
    memory_size(memory_size), base(_memory),
    int_ptr(nullptr),         uint_ptr(nullptr),
    ubyte_ptr(nullptr),       symbols_kept(false),
    prog_size(0),             prog_start_addr(0),
    first_instr_addr(0),      last_instr_addr(0),
    first_user_addr(0),       symtab_size(0),
	FLAGS(0),                 user_start(0),
	prog(NULL),               symtab_data(NULL),
	creation_date(0),         filesize(0),
	file_path(0)
{
    type_record = new u8[memory_size];
	watch_vec   = new std::vector<u32>();
}

Ram::
~Ram()
{
    delete[] type_record;
	delete   watch_vec;
    free(base);
}

void
Ram::
watch_addr(u32 addr)
{
	for (size_t i = 0; i < watch_vec->size(); ++i)
		if (watch_vec->at(i) == addr) return;

	watch_vec->push_back(addr);
}

void
Ram::
unwatch_addr(u32 addr)
{
	for (size_t i = 0; i < watch_vec->size(); ++i)
	{
		if (watch_vec->at(i) == addr)
			watch_vec->erase(std::remove(watch_vec->begin(), watch_vec->end(), addr), watch_vec->end());
	}
}

void
Ram::
watched_addrs()
{
	for (size_t i = 0; i < watch_vec->size(); ++i)
		printf("\n address: %u", watch_vec->at(i));
}

void
Ram::
print_watch_addrs()
{
	for (size_t i = 0; i < watch_vec->size(); ++i)
		print_addr(watch_vec->at(i));
}

void
Ram::
validate_addr(u32 addr)
{
    if (addr >= memory_size)
        throw RamError::INVALID_ADDR;
}

void
Ram::
validate_datatype(u32 addr, u8 type)
{
	u8 x = type_record[addr];

	if (type_record[addr] != type)
		throw RamError::DATATYPE_REC_MISMATCH;
}

u64
Ram::
get_abs_addr(u32 relative_addr)
{
    return (u64)(base + relative_addr);
}

u8*
Ram::
get_ptr(u32 addr)
{
    validate_addr(addr);
    return (u8*)((uintptr_t)base) + addr;
}

void
Ram::
print_addr(u32 addr)
{
    validate_addr(addr);

    switch (type_record[addr])
    {
		case UINT32_CODE:
			printf(UINT_RAM_VALUE_INFOSTR, addr, get_uint(addr));
			break;

		case INT32_CODE:
			printf(INT_RAM_VALUE_INFOSTR, addr, get_int(addr));
			break;

		case UINT8_CODE:
			printf(BYTE_RAM_VALUE_INFOSTR, addr, get_ubyte(addr));
			break;

		default:
			throw RamError::GENERAL_ERR;
    }
}

void
Ram::
loadin_program(const char* file_path_)
{
	load_program(file_path_, base);

	if (!file_path)
		file_path = file_path_;

	prog_start_addr = *(u32*)(base + MD_START_INSTR_ADDR_OFFSET);
	mem_size = *(u32*)(base + MD_MEMORY_SIZE_OFFSET);
	prog_size = *(u32*)(base + MD_PROG_SIZE_OFFSET);
	instr_count = *(u32*)(base + MD_INSTR_COUNT_OFFSET);
	first_instr_addr = *(u32*)(base + MD_FIRST_INSTR_ADDR_OFFSET);
	last_instr_addr = *(u32*)(base + MD_LAST_INSTR_ADDR_OFFSET);
	first_user_addr = *(u32*)(base + MD_FIRST_USER_BYTE_ADDR_OFFSET);
	FLAGS = *(u32*)(base + MD_FLAGS_OFFSET);
	creation_date = *(u32*)(base + MD_CREATION_DATE_OFFSET);
	symbols_kept = (bool)FLAGS;

	if (symbols_kept)
	{
		symtab_size = *(u32*)(base + SYMTAB_SIZE_OFFSET);
		symtab_data = base + SYMTAB_DATA_OFFSET;
	}

	user_start = base + first_user_addr;
	prog = base + first_instr_addr;
	filesize = METADATA_SIZE + prog_size + symtab_size;

	typerecord_input_file();
}

void
Ram::
set_ubyte(u32 addr, u8 value)
{
    validate_addr(addr);
    ubyte_ptr = (base + addr);
    *ubyte_ptr = value;
    type_record[addr] = UINT8_CODE;
}

void
Ram::
set_addr_typerec(u32 addr, u8 typerec_code)
{
	validate_addr(addr);
	type_record[addr] = typerec_code;
}

void
Ram::
set_uint(u32 addr, u32 value)
{
    validate_addr(addr);

    uint_ptr = (u32*)(base + addr);
    *uint_ptr = value;
    type_record[addr] = UINT32_CODE;

    for (int i = 1; i < UINT_SIZE; i++)
        type_record[addr + i] = INSIDE_UINT_CODE;
}

void
Ram::
set_int(u32 addr, i32 value)
{
    validate_addr(addr);

    int_ptr = (i32*)(base + addr);
    *int_ptr = value;
    type_record[addr] = INT32_CODE;

    for (int i = 1; i < INT_SIZE; i++)
        type_record[addr + i] = INSIDE_INT_CODE;
}

u8
Ram::
get_ubyte(u32 addr)
{
    validate_addr(addr);
    validate_datatype(addr, UINT8_CODE);
	return *(base + addr);
}

u32
Ram::
get_uint(u32 addr)
{
    validate_addr(addr);
    validate_datatype(addr, UINT32_CODE);
	return *((u32*)(base + addr));
}

i32
Ram::
get_int(u32 addr)
{
    validate_addr(addr);
    validate_datatype(addr, INT32_CODE);
    return *((i32*) (base + addr));
}

void
Ram::
print_all()
{
	u8     symbols_kept = 0;
	size_t addr = 0;

	printf("\n:: ram contents :: size: %u\n", memory_size);

	printf("\n\METADATA TABLE:");
	// print metadata.
	u8* bsptr = base + MD_START_INSTR_ADDR_OFFSET;
	printf("\nram[%zu] = (start-addr) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_MEMORY_SIZE_OFFSET;
	printf("\nram[%zu] = (mem-size) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_PROG_SIZE_OFFSET;
	printf("\nram[%zu] = (prog-size) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_INSTR_COUNT_OFFSET;
	printf("\nram[%zu] = (instr_count) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_FIRST_INSTR_ADDR_OFFSET;
	printf("\nram[%zu] = (first-instr-addr) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_LAST_INSTR_ADDR_OFFSET;
	printf("\nram[%zu] = (last-instr-addr) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_FIRST_USER_BYTE_ADDR_OFFSET;
	printf("\nram[%zu] = (first-user-byte-addr) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_FLAGS_OFFSET;
	printf("\nram[%zu] = (FLAGS) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	bsptr = base + MD_CREATION_DATE_OFFSET;
	printf("\nram[%zu] = (creation date-stamp) %u", addr, *((u32*)bsptr));
	for (u8 i = 1; i < UINT_SIZE; i++)
		printf("\n\ttype_record[%zu] = %s\n", addr + i, datatype_rec_code_str[type_record[addr + i]]);
	addr += 4;

	// print everything after metadata table, which is symtab(if kept) and program.
	print_typerecord(addr, memory_size);
}

void
Ram::
print_typerecord(size_t start_addr, size_t end_addr)
{
	for (size_t i = 0; i < memory_size; i++)
	{
		if (i < start_addr) continue;
		if (i == end_addr)  return;

		if (i > (METADATA_SIZE + symtab_size))
			if (i % 4 == 0) printf("\n");

		printf("\ntype_record[%zu] = %s", i, datatype_rec_code_str[type_record[i]]);
	}
}

void
Ram::
modify_size(u32 newsize)
{
	base = (u8*) realloc(base, newsize);
	memory_size = newsize;
}

void
Ram::
print_values(size_t start_addr, size_t end_addr)
{
	size_t addr = start_addr-1;

	while (addr++ < end_addr)
	{
		switch (type_record[addr])
		{
			case UINT32_CODE:
				printf("\n\nram[%zu] = %u (uint32)", addr, *((u32*) (base + addr)));
				continue;

			case INT32_CODE:
				printf("\n\nram[%zu] = %d (int32)", addr, *((i32*)(base + addr)));
				continue;

			case UINT8_CODE:
				printf("\n\nram[%zu] = %d (ubyte)", addr, *(base + addr));
				continue;

			case INSIDE_UINT_CODE:
				printf("\nram[%zu] = (inside-uint) bitstring: ", addr);

				for (int i = 7; i >= 0; i--)
					printf("%c", ((*(base + addr)) & (1u << i)) ? '1' : '0');

				continue;

			case INSIDE_INT_CODE:
				printf("\nram[%zu] = (inside-int) bitstring: ", addr);

				for (int i = 7; i >= 0; i--)
					printf("%c", ((*(base + addr)) & (1u << i)) ? '1' : '0');

				continue;

			case INSIDE_UINT8_PADDING_CODE:
				printf("\nram[%zu] = (ubyte-padding-byte) bitstring: ", addr);

				for (int i = 7; i >= 0; i--)
					printf("%c", ((*(base + addr)) & (1u << i)) ? '1' : '0');

				continue;

			case INSIDE_SYMTAB_CODE:
				printf("\nram[%zu] = (inside-symtab) bitstring: ", addr);

				for (int i = 7; i >= 0; i--)
					printf("%c", ((*(base + addr)) & (1u << i)) ? '1' : '0');

				continue;

			default:
				printf("\nram[%zu] = (unknown) bitstring: ", addr);

				for (int i = 7; i >= 0; i--)
					printf("%c", ((*(base + addr)) & (1u << i)) ? '1' : '0');

				continue;
		}
	}
}

int
load_program(const char* file_path, u8* ram)
{
    FILE*  file;
    size_t file_size;
    size_t bytes_read;

    file = fopen(file_path, "rb");

    if (file == NULL)
		throw RamError::INPUT_FILE_OPEN;

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    bytes_read = fread(ram, sizeof(u8), file_size, file);

    if (bytes_read != file_size)
		throw RamError::PROGRAM_WRITE_TO_RAM;

    fclose(file);
    return 0;
}

u8*
alloc_ram_memory(u32 size)
{
	u8* ram = (u8*) malloc(size);

	if (ram == NULL)
		throw RamError::MASTER_ALLOCATION;

    return ram;
}

char* // duplicate of function in asm.cpp, couldnt include it for some reason fucken..
decode_datestamp_LOCAL(u16 datestamp)
{
	int day = datestamp & 0x1F;
	int month = (datestamp >> 5) & 0xF;
	int year = (datestamp >> 9) & 0x7F;

	if (year < 100)
		year += 2000;
	else
		year += 1900;

	char* datestr = (char*)malloc(DATESTR_MALLOC_LEN);

	sprintf(datestr, DATESTR_INFOSTR, day, month, year);
	return datestr;
}

void
RAM_mem_test()
{
	printf("\n setting up ram memory test..");
	Ram* ram = new Ram(1000, alloc_ram_memory(1000));

	ram->loadin_program("C:/Users/klopp/furst_built/bella.fbin");
	//ram->print_all();

	ram->set_uint(ram->first_user_addr, 6566);
	ram->set_addr_typerec(ram->first_user_addr + 4, INSIDE_SYMTAB_CODE);
	ram->print_values(ram->first_instr_addr, ram->filesize + 200);
	//ram->print_typerecord(ram->first_instr_addr, ram->filesize);

	delete ram;
	printf("\n closing up ram memory test...");
}

//int main()
//{
//	RAM_mem_test();
//	return 0;
//}
