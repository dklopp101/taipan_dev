#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "opcode.h"

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

//
// VM DATA-ACCESS MACROS.
//
// note:
//       ram-addr: refers to addr where first byte of ram is addr 0.
//       stack-addr: refers to addr where first byte of stack is addr 0.
//       absolute-addr: refers to the actual address held in a c ptr to the ram/stack-addr.

#define OPERAND_VAL(TYPE)      (*((TYPE*) ip)) // returns operand as TYPE, assumes ip is currently pointing at operand before call.
#define RAM_ADDR(POINTER)      ((u32) (((POINTER) - (pro->ram)))) // takes a c ptr to somewhere in ram & returns ram-addr.
#define RAM_VAL(TYPE, ADDR)    (*((TYPE*)((pro->ram) + (ADDR)))) // takes a ram-addr & returns a value as TYPE.
#define STACK_VAL(TYPE, ADDR)  (*((TYPE*)(wstk + (ADDR))))       // takes a stack-addr & returns a value as TYPE.
#define STACK_TOP_VAL(TYPE)    (*((TYPE*)sp))
#define STACK_SECOND_VAL(TYPE) (*((TYPE*)(sp - 4)))
#define STACK_THIRD_VAL(TYPE)  (*((TYPE*)(sp - 8)))
#define STACK_FOURTH_VAL(TYPE) (*((TYPE*)(sp - 12)))
#define STACK_FIFTH_VAL(TYPE)  (*((TYPE*)(sp - 16)))
#define STACK_VAL_TOP_OFFSET(TYPE, OFFSET) (*((TYPE*) (sp - OFFSET))) 

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


#define PRINT_DEBUG_OPCODE_INFO() \
	printf("\n\nexecuting-ram-addr[%d] %s", RAM_ADDR(ip), mnemonic_strings[*ip])

#define PRINT_DEBUG_STACK_TOP_UINT() \
	printf("\nstack top: %u (uint)", *((u32*) sp))

//
// DEBUG MACROS
//
#define FIRST_OPERAND_WORD(TYPE) (*((TYPE*) ip + OPCODE_SIZE))
#define SECOND_OPERAND_WORD(TYPE) (*((TYPE*) ip + OPCODE_SIZE + WORDSIZE))
#define THIRD_OPERAND_WORD(TYPE) (*((TYPE*) ip + OPCODE_SIZE + WORDSIZE + WORDSIZE))
//
//#define SHOW_DEBUG_INFO // comment out this line then rebuild to disable debug info.
//#ifdef SHOW_DEBUG_INFO
//#define PRINT_DEBUG_OPCODE_INFO() \
//	printf("\n\nexecuting-ram-addr[%d] %s", RAM_ADDR(ip), mnemonic_strings[*ip]))
//
//#define PRINT_DEBUG_STACK_TOP_UINT() \
//	printf("\nstack top: %u (uint)", *((u32*) sp))
//
//#else
//#define PRINT_DEBUG_OPCODE_INFO()    ;
//#define PRINT_DEBUG_STACK_TOP_UINT() ;
//#define PRINT_DEBUG_RESULT_UINT()    ;
//#endif
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

int eval_process(Process* pro);

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


int
exec_file(char* filepath, u32 ram_size, u32 stack_size, u32 recur_max)
{
	FILE* file;
	size_t file_size;
	size_t bytes_read;

	Process* pro = new_process(ram_size, stack_size, recur_max);
	if (!pro) return 2;

	if (!is_valid_filepath(filepath, ".fbin")) return 1;

	file = fopen(filepath, "rb");
	if (file == 0) return 0;

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	bytes_read = fread(pro->ram, sizeof(u8), file_size, file);

	fclose(file);

	return eval_process(pro);
}

int
run_file(char* filepath, u32 ram_size, u32 stack_size, u32 recur_max)
{
	Process* pro = new_process(ram_size, stack_size, recur_max);
	FILE* file = fopen(filepath, "rb");
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);
	fread(pro->ram, sizeof(u8), file_size, file);
	fclose(file);
	return eval_process(pro);
}

Process*
new_process(u32 ram_size, u32 stack_size, u32 recur_max)
{
	Process* pro = (Process*) malloc(sizeof(Process));
	if (pro == NULL) return NULL;

	pro->ram = (u8*) malloc(ram_size);
	if (pro->ram == NULL) return NULL;

	pro->ram_size   = ram_size;
	pro->stack_size = stack_size;
	pro->recur_max  = recur_max;
	
	return pro;
}
#define STACK_DEFAULT_SIZE 1000
#define RECUR_MAX_DEFAULT  100
int
eval_process(Process* pro)
{
	build_optable();

	u8   wstk[STACK_DEFAULT_SIZE]; // work-stack.
	u8*  cstk[RECUR_MAX_DEFAULT];  // call-stack.
    u8*  sp = wstk;             // stack-pointer.
	u8*  rp = cstk[0];          // return-pointer.
	u8*  ip = start_instr       // instr-pointer.
    u32  lc = 0;                // loop-counter 
	u8*  lb, *le;               // loop-base & loop-end instr ptrs
	u32* u32p;                  // uint32-arithmetic-pointer.
	i32* i32p;                  // int32-arithmetic-pointer.
	u32  u32reg[2];             // uint32-data-buffers.
	i32  i32reg[2];             // int32-data-buffers.
	int  cproc_retc = 0;

	// program execution begins here.
    nextop();

	// operation handling code.
    die:
		PRINT_DEBUG_OPCODE_INFO();

        free(pro->ram);
		free(pro);
        return cproc_retc;

    nop:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
        nextop();

    nspctr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;

		switch (OPERAND_VAL(u8))
		{
			case UINT_TYPE: 
				printf("ram[%u] = %u (uint)", OPERAND_VAL(u32), RAM_VAL(u32, OPERAND_VAL(u32)));
				break;

			case INT_TYPE: 
				printf("ram[%u] = %d (int)", OPERAND_VAL(u32), RAM_VAL(u32, OPERAND_VAL(u32)));
				break;

			default:
				printf("]unknown error..");
				goto die;
		}		

		ip += UINT_SIZE;
        nextop();

    nspctst: // FORGOT WHAT THIS DOES SO RIGHT NOW IT DISPLAYS STACK TOP
		PRINT_DEBUG_OPCODE_INFO();
		
		ip += OPCODE_SIZE;
		printf("\nwork-stack top: %u (uint)", STACK_TOP_VAL(u32));

        nextop();

    test_die:
		PRINT_DEBUG_OPCODE_INFO();
		goto die;

    call:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		++rp;
		rp = ip + ADDR_SIZE;
		ip = pro->ram + OPERAND_VAL(u32);
        nextop();

    ret:
		PRINT_DEBUG_OPCODE_INFO();

		ip = rp--;
        nextop();

    swtch:
		PRINT_DEBUG_OPCODE_INFO();
        printf("\nUNIMPLEMNTED OPCODE!");
        nextop();

    jmp:
		PRINT_DEBUG_OPCODE_INFO();
		ip += OPCODE_SIZE;
		ip = pro->ram + OPERAND_VAL(u32);
        nextop();

    je:
		PRINT_DEBUG_OPCODE_INFO();

		if (!memcmp(sp, sp + WS, WS))
			ip = pro->ram + OPERAND_VAL(u32);
		else
			ip += ADDR_SIZE;

		nextop();

    jn:
		PRINT_DEBUG_OPCODE_INFO();

		if (memcmp(sp, sp + WS, WS))
			ip = pro->ram + OPERAND_VAL(u32);
		else
			ip += ADDR_SIZE;

		nextop();

    jl:
		PRINT_DEBUG_OPCODE_INFO();

		if (STACK_TOP_VAL(u32) < STACK_SECOND_VAL(u32))
			ip = pro->ram + OPERAND_VAL(u32);
		else
			ip += ADDR_SIZE;

		nextop();

    jg:
		PRINT_DEBUG_OPCODE_INFO();

		if (STACK_TOP_VAL(u32) > STACK_SECOND_VAL(u32))
			ip = pro->ram + OPERAND_VAL(u32);
		else
			ip += ADDR_SIZE;

		nextop();

    jls:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;

		if (STACK_TOP_VAL(i32) < STACK_SECOND_VAL(i32))
			ip = pro->ram + OPERAND_VAL(u32);
		else
			ip += ADDR_SIZE;

		nextop();


    jgs:
		PRINT_DEBUG_OPCODE_INFO();

		if (STACK_TOP_VAL(i32) > STACK_SECOND_VAL(i32))
			ip = pro->ram + OPERAND_VAL(u32);
		else
			ip += ADDR_SIZE;

		nextop();

    loop:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		lc = OPERAND_VAL(u32);
		ip += UINT_SIZE;
		lb = pro->ram + OPERAND_VAL(u32);
		ip += ADDR_SIZE;
		le = pro->ram + OPERAND_VAL(u32);
		ip += ADDR_SIZE;

        nextop();

    lcont:
		PRINT_DEBUG_OPCODE_INFO();

		if (lc) {
			--lc;
			ip = lb;
		} else {
			ip = le;	
		}
		nextop();

    lbrk:
		PRINT_DEBUG_OPCODE_INFO();

		ip = le;
        nextop();

    psh:
		PRINT_DEBUG_OPCODE_INFO();
		printf("\n pushing uint value: %u", *((u32*) (ip + OPCODE_SIZE)));

		ip += OPCODE_SIZE;
		sp += WS;
		memcpy(sp, ip, WS);
		ip += UINT_SIZE;

		PRINT_DEBUG_STACK_TOP_UINT();
        nextop();

    pop:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp -= WS;
        nextop();

    pop2:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp -= DWS;
        nextop();

    popn:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp -= (WS * OPERAND_VAL(u32));
		ip += UINT_SIZE;
        nextop();

    pshfr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += WS;
		memcpy((void*) sp, (void*) pro->ram + OPERAND_VAL(u32), WS);
		ip += UINT_SIZE;
        nextop();

    poptr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		memcpy((void*) pro->ram + OPERAND_VAL(u32), (void*) sp, WS);
		sp -= WS;
		ip += UINT_SIZE;
        nextop();

    movtr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		memcpy(pro->ram + OPERAND_VAL(u32), sp, WS);
		ip += UINT_SIZE;
        nextop();

    stktr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		u32reg[0] = OPERAND_VAL(u32); // dst addr.
		ip += UINT_SIZE;
		memcpy((void*)((pro->ram) + (u32reg[0])), wstk + OPERAND_VAL(u32), WS);
		sp -= WS;
		ip += UINT_SIZE;
		nextop();

    cpyr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		u32reg[0] = OPERAND_VAL(u32); // dst addr.
		ip += UINT_SIZE;
		memcpy((void*)((pro->ram) + (u32reg[0])), (void*) pro->ram + OPERAND_VAL(u32), WS);
		sp -= WS;
		ip += UINT_SIZE;
		nextop();

    setr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		u32reg[0] = OPERAND_VAL(u32); // dst addr.
		ip += UINT_SIZE;
		memcpy((void*)((pro->ram) + (u32reg[0])), (void*) ip, WS);
		sp -= WS;
		ip += UINT_SIZE;
		nextop();

    pshfrr:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += WS;
		memcpy((void*) sp, (void*) pro->ram + RAM_VAL(u32, OPERAND_VAL(u32)), WS);
		ip += UINT_SIZE;
        nextop();

    pshfrs: // NOT IMPLEMENTED YET
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;

        nextop();

    inc:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		++STACK_TOP_VAL(u32);
        nextop();

    dec:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		--STACK_TOP_VAL(u32);
        nextop();

    add:
		PRINT_DEBUG_OPCODE_INFO();
		
		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) + STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
        nextop();

    sub:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) - STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    mul:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) * STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    div:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) / STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    mod:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) % STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    and:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) & STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    not:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		~(STACK_TOP_VAL(u32));

		//PRINT_DEBUG_RESULT_UINT();
		nextop();
    xor:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) ^ STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    or:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) | STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    lshft:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) >> STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    rshft:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += UINT_SIZE;
		STACK_TOP_VAL(u32) = STACK_SECOND_VAL(u32) << STACK_THIRD_VAL(u32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    lrot:
		PRINT_DEBUG_OPCODE_INFO();
		
		ip += OPCODE_SIZE;
		u32reg[0] = STACK_TOP_VAL(u32) << STACK_SECOND_VAL(u32);
		u32reg[1] = STACK_TOP_VAL(u32) >> (WS_BITS - STACK_SECOND_VAL(u32));
		sp += WS;
		STACK_TOP_VAL(u32) = u32reg[0] | u32reg[1];

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    rrot:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		u32reg[0] = STACK_TOP_VAL(u32) >> STACK_SECOND_VAL(u32);
		u32reg[1] = STACK_TOP_VAL(u32) << (WS_BITS - STACK_SECOND_VAL(u32));
		sp += WS;
		STACK_TOP_VAL(u32) = u32reg[0] | u32reg[1];

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    incs:
		PRINT_DEBUG_OPCODE_INFO();
	
		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    decs:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    adds:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    subs:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    muls:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    divs:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    mods:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    ands:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    nots:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    xors:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    ors:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) + STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    lshfts:
		PRINT_DEBUG_OPCODE_INFO();
		
		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) << STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    rshfts:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		sp += INT_SIZE;
		STACK_TOP_VAL(i32) = STACK_SECOND_VAL(i32) >> STACK_THIRD_VAL(i32);

		//PRINT_DEBUG_RESULT_UINT();
		nextop();

    lrots:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		i32reg[0] = STACK_TOP_VAL(i32) << STACK_SECOND_VAL(u32);
        i32reg[1] = STACK_TOP_VAL(i32) >> (WS_BITS - STACK_SECOND_VAL(u32));
		sp += WS;
		STACK_TOP_VAL(u32) = i32reg[0] | i32reg[1];
		nextop();

    rrots:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		i32reg[0] = STACK_TOP_VAL(i32) >> STACK_SECOND_VAL(i32);
		i32reg[1] = STACK_TOP_VAL(i32) << (WS_BITS - STACK_SECOND_VAL(i32));
		sp += WS;
		STACK_TOP_VAL(u32) = i32reg[0] | i32reg[1];

	brkp:
		PRINT_DEBUG_OPCODE_INFO();

		ip += OPCODE_SIZE;
		nextop();
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("\nfvm: no input files.");
		 	return 1;
	}

	return exec_file(argv[1], 1000, 500, 20);
}