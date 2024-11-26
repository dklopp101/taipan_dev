#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define real float
#define f32  float
#define u64  uint64_t
#define i64  int64_t
#define u32  uint32_t
#define i32  int32_t
#define u16  uint16_t
#define i16  int16_t
#define u8   uint8_t
#define i8   int8_t
#define EOL  '\n'
#define TRUE  1
#define FALSE 0
#define NONE  0

//
// VM DATATYPE CODES.
//
#define VOID_TYPE   0
#define UINT_TYPE   1
#define INT_TYPE    2 
#define UBYTE_TYPE  3
#define REAL_TYPE   4
#define ADDR_TYPE   5
#define STRING_TYPE 6
#define BOOL_TYPE   7
#define ID_TYPE     8
#define UINT16_TYPE 9

//
// VM SIZE CONSTANTS.
//
#define UBYTE_SIZE  1
#define UINT_SIZE   4
#define ADDR_SIZE   UINT_SIZE
#define INT_SIZE    4
#define REAL_SIZE   4
#define OPCODE_SIZE 4
#define WORDSIZE    4
#define WORDSIZE_X2 8
#define WORDSIZE_X3 12
#define WS          WORDSIZE    
#define DWS         WORDSIZE_X2 // double wordsize.
#define TWS         WORDSIZE_X3 // triple wordsize.
#define WS_BITS     32
#define DWS_BITS    64
#define TWS_BITS    96

typedef struct
{
	u32 stack_size; // work-stack size.
	u32 ram_size;
	u8  recur_max;  // maximum recursion level.
	u8* ram;        // heap allocate process user memory.
} Process;

//  struct used for recording the current executing state of a process
//  although not currently used it will be possible in the future to 
//  make an instr to allocate a ProcessState object then populate it
//  with the process state vars of the vm. then have another instr to
//  set the vm to the state in a specified ProcessState object.
//  set the vm to the state in a specified ProcessState object.
typedef struct
{
	Process* pro;
	u8* wstk;
	u8* lb;
	u8* le;
	u8* ip;
	u8* sp;
	u8* rp;
	u32* cstk;
	u32  lc;
} ProcessState;

//
// DEBUG MACROS
//
#define FIRST_OPERAND_WORD(TYPE) (*((TYPE*) ip + OPCODE_SIZE))
#define SECOND_OPERAND_WORD(TYPE) (*((TYPE*) ip + OPCODE_SIZE + WORDSIZE))
#define THIRD_OPERAND_WORD(TYPE) (*((TYPE*) ip + OPCODE_SIZE + WORDSIZE + WORDSIZE))

#define SHOW_DEBUG_INFO // comment out this line then rebuild to disable debug info.
#ifdef SHOW_DEBUG_INFO
#define PRINT_DEBUG_OPCODE_INFO() \
	printf("\n\nexecuting-ram-addr[%d] %s", RAM_ADDR(ip), mnemonic_strings[*ip]))

#define PRINT_DEBUG_STACK_TOP_UINT() \
	printf("\nstack top: %u (uint)", *((u32*) sp))

#else
#define PRINT_DEBUG_OPCODE_INFO()    ;
#define PRINT_DEBUG_STACK_TOP_UINT() ;
#define PRINT_DEBUG_RESULT_UINT()    ;
#endif

//
// VM DATA-ACCESS MACROS.
//
// note:
//       ram-addr: refers to addr where first byte of ram is addr 0.
//       stack-addr: refers to addr where first byte of stack is addr 0.
//       absolute-addr: refers to the actual address held in a c ptr to the ram/stack-addr.

#define OPERAND_VAL(TYPE)      (*((TYPE*) ip)) // returns operand as TYPE, assumes ip is currently pointing at operand before call.
#define RAM_ADDR(POINTER)      ((u32) (((POINTER) - (pro->ram))) // takes a c ptr to somewhere in ram & returns ram-addr.
#define RAM_VAL(TYPE, ADDR)    (*((TYPE*)((pro->ram) + (ADDR)))) // takes a ram-addr & returns a value as TYPE.
#define STACK_VAL(TYPE, ADDR)  (*((TYPE*)(wstk + (ADDR))))       // takes a stack-addr & returns a value as TYPE.
#define STACK_TOP_VAL(TYPE)    (*((TYPE*)sp))
#define STACK_SECOND_VAL(TYPE) (*((TYPE*)(sp - 4)))
#define STACK_THIRD_VAL(TYPE)  (*((TYPE*)(sp - 8)))
#define STACK_FOURTH_VAL(TYPE) (*((TYPE*)(sp - 12)))
#define STACK_FIFTH_VAL(TYPE)  (*((TYPE*)(sp - 16)))
#define STACK_VAL_TOP_OFFSET(TYPE, OFFSET) (*((TYPE*) (sp - OFFSET))) 

//
// PROGRAM START ADDR MACRO
//
// note:
//      ram[0] holds the ram-addr for the starting instr.
//      start-instr = ram[ram[0]]
#define start_instr (pro->ram + (*(u32*) (pro->ram)));

#define nextop() goto *optable[*ip]

int exec_file(char* filepath, u32 ram_size, u32 stack_size, u32 recur_max);

int run_file(char* filepath, u32 ram_size, u32 stack_size, u32 recur_max);

Process* new_process(u32 ram_size, u32 stack_size, u32 recur_max);

int eval_process(Process * pro);

int main(int argc, char* argv[]);

#define newline() printf("\n")
#define newlines(N) for (u8 i=0; i < (N); i++) { newline(); }

#define print_bits(VALUE) \
			for (int i = 7; i >= 0; i--) printf("%c", (((VALUE)) & (1u << i)) ? '1' : '0')

static
	const char*
	datatype_str[] =
{
	"void",
	"uint",
	"int",
	"ubyte",
	"real",
	"addr",
	"string",
	"bool",
	"identifier",
	"uint-16",
	"", "", "" , "", "", "", "" , "", "", "", "" ,
	"symtab-data"
};


static
	const char*
	bool_strings[] =
{
	"false",
	"true"
};

	static
		const char*
		datatype_rec_code_str[] =
	{
		"no-type",       // 0
		"uint",          // 1
		"int",           // 2
		"ubyte",         // 3
		"real",          // 4
		"addr",          // 5
		"string",        // 6
		"bool",          // 7
		"identifier",    // 8
		"uint-16",       // 9
		"inside-uint",   // 10
		"inside-int",    // 11
		"inside-uint16", // 12
		"inside-symtab", // 13
		"inside-ubyte-padding", // 14
		"no-sign",   // 15
		"no-value" // 16
	};

	// another late night eyes-hanging out of my head
	// function, seems to work, might rewrite in future.
	static
		int
		file_exists(const char* path)
	{
		FILE* file = fopen(path, "rb");
		int retval;

		if (file) {
			retval = 1;
			fclose(file);
		}
		else {
			retval = 0;
		}

		return retval;
	}

	// confirms validity of a file-path, set a required_extension(".exe") if needed.
	// if any extension is okay set required_extension to "".
	static
		int
		is_valid_filepath(const char* path, const char* required_extension)
	{
		const char* exten_ptr = NULL;
		int exten_len = 0;
		int pos = -1;
		int slash_hit = 0;
		int colon_hit = 0;

		// confirm that file-path is at least 3 chars long.
		if (strlen(path) < 3) return 0;

		// iterate through chars in file-path one by one.
		do {
			// check if path is longer then max path len.
			// increment pos var aferwards.
			if ((pos++) > 259) return 0;

			switch (*path) {
				// check for file-extension or '../'
			case '.':
				// check if next char is period denoting "../"
				if ((*(path + 1)) == '.') {
					// if pos isn't 0 then we have a char before the '.', confirm it's a slash or path is invalid.
					// eg: "../sfsd/" <- valid, "fssaf../saf/" <- invalid.
					if (pos) {
						if ((*(path - 1)) != '/') {
							if (*(path - 1) != '\\')
								return 0;
						}
					}

					// check if next char after the ".." is a '/' or '\'
					if ((*(path + 2)) != '/') {
						if (*(path + 2) != '\\')
							return 0;
					}

					// all is well so advance past it the "../"
					path++;
					pos++;
					continue;
				}

				// '.' char wasn't denoting "../" so it must be a file-extension.
				// if pos == 0 then we dont have a char before the '.' so path is invalid.
				if (!pos) return 0;

				// set pointer at start of file extension we it can be checked after loop finishes.
				// then continue next iteration of loop.
				exten_ptr = path;
				continue;

				// these chars are invalid in windows paths and technically valid in unix paths
				// but strongly discouraged in them, so will be treated as invalid in both.
			case '<': case '>': case '"': case '|': case '+':
			case '?': case '*': case '-': case '`': case ',':
			case '!': case '#': case '$': case '%': case '^':
			case '&': case '(': case ')': case '=': case ';':
				return 0;

				// check what platform we're on then check accordingly.
			case ':':
#if defined(_WIN32) || defined(_WIN64)
				// on windows ':' cannot come after a slash.
				if (slash_hit) return 0;
				// also path can only have 1 ':' so check if already hit one.
				if (colon_hit) return 0;
				// also cannot be the first char.
				if (!pos) return 0;
				// also check if next char is not a slash, which is invalid.
				if ((*(path + 1)) != '/') {
					if (*(path + 1) != '\\')
						return 0;
				}
				colon_hit = 1;
				continue;

#elif defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
				// on unix ':' is treated as a typical valid char.
				continue;
#endif

				// if theres a tilde confirm it's the first char.
			case '~':
				if (pos) return 0;

			case '\\':
				// if on unix systems '\' is invalid.
#if defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
// on unix ':' is treated as a typical char.
				return 0;
#endif
				continue;

			default:
				// check if we're inside the file extension and count the char so
				// extension length can be checked after the loop.
				if (exten_ptr)
					exten_len++;
			}
		} while (*(++path));

		// check that last char wasn't a period, that there was
		// an extension in the path and it was at least 1 char long.
		if ((*(path - 1) != '.') && (exten_ptr && exten_len)) {
			// check if there was a required extension, if not file-path is valid.
			if (strlen(required_extension)) {
				// check if the file-path's extension matches the required extension.
				return strcmp(exten_ptr, required_extension) == 0;
			}
			else {
				return 1;
			}
		}

		return 0;
	}


	static
		u8
		count_digits(i64 num)
	{
		u8 count = 0;
		if (num == 0) return 1;
		for (; (num /= 10) != 0; count++);
		return count;
	}

	//int
	//dump_wstk(void* bsp);
	//
	//int
	//eval_process(void* ram);
	//
	//int
	//run_fbin(const char* file_path);
	//
	//int main();

	// format string thingies:    
	//     printf("Signed Integer: %d\n", a);
	//     printf("Unsigned Integer: %u\n", b);
	//     printf("Float: %f\n", c);
	//     printf("Character: %c\n", d);
	//     printf("String: %s\n", char*);

#endif // VM.H