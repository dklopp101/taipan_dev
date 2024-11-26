#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)


#ifndef RAM_H
#define RAM_H

#include "asm.h"

#include "infostrings.h"
#include "vm.h"

// forget wtf these are being used for?!?!?
#define PUSH           50
#define POP            60

// .FBIN MEMORY-MAP

// 0x00 (0)                      : START_INSTR_ADDR        (u32) --
// 0x04 (4)                      : PROG_SIZE               (u32)   |
// 0x08 (8)                      : FLAGS                   (u16)   |
// 0x0A (10)                     : CREATION_DATE           (u16)   |--- METADATA SEGMENT
// 0x0C (12)                     : FIRST_INSTR_ADDR        (u32)   |
// 0x10 (16)                     : LAST_INSTR_ADDR         (u32)   |
// 0x14 (20)                     : FIRST_USER_BYTE_ADDR    (u32) --
// 
// 0x18 (24)                     : SYMTAB_SIZE             (u32) ------ SYMBOL-TABLE_SEGMENT
//      (28)                     : SERIALISED SYMTAB       (Symbol() objects and their strings.)

enum class RamError
{
	GENERAL_ERR = 1,
	INVALID_ADDR,
	INVALID_OPCODE_IN_TYPEREC,
	DATATYPE_REC_MISMATCH,
	INPUT_FILE_OPEN,
	PROGRAM_WRITE_TO_RAM,
	MASTER_ALLOCATION,
	INVALID_DATATYPE
};

int load_program(const char* file_path, u8* ram);
u8* alloc_ram_memory(u32 size);

struct Ram
{
	u8* user_start; // first-user-byte.
	u8* prog; // first-prog-instr.
	u8* symtab_data; // serialised-symtab;
	u32 mem_size; // same as file size. all ram.
	u32 prog_size;
	u32 instr_count;
	u32 prog_start_addr;
	u32 first_instr_addr;
	u32 last_instr_addr;
	u32 first_user_addr;
	u32 symtab_size;
	u16 FLAGS;
	u16 creation_date;
	bool symbols_kept;
	size_t filesize;
	const char* file_path;
	void loadin_program(const char* file_path_);

	u8* type_record;
	u8* ubyte_ptr;
	u32* uint_ptr;
	i32* int_ptr;
	//u8* master;
	u8* base;
	u32 memory_size;


	// address watch stuff.
	std::vector<u32>* watch_vec;
	void watch_addr(u32 addr);
	void unwatch_addr(u32 addr);
	void watched_addrs(); // prints just the addresses not the contents.
	void print_watch_addrs();

	Ram(u32 memory_size, u8* master_ptr);
	virtual ~Ram();

	void typerecord_input_file();
	void validate_addr(u32 addr);
	void validate_datatype(u32 addr, u8 type);
	u64  get_abs_addr(u32 relative_addr);
	u8*  get_ptr(u32 addr);
	void print_addr(u32 addr);
	void set_ubyte(u32 addr, u8 value);
	void set_uint(u32 addr, u32 value);
	void set_int(u32 addr, i32 value);
	u8   get_ubyte(u32 addr);
	u32  get_uint(u32 addr);
	i32  get_int(u32 addr);

	void print_all();
	void print_typerecord(size_t start_addr, size_t end_addr);
	void set_addr_typerec(u32 addr, u8 typerec_code);
	void print_values(size_t start_addr, size_t end_addr);
	void reset_all();
	void modify_size(u32 newsize);
};

char* decode_datestamp_LOCAL(u16 val); // redefnition of function from asm.h, couldnt get it to work so copied the fucker..

//int main();

#endif // RAM_H
