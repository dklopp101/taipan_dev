#ifndef OPCODE_H
#define OPCODE_H

#include "vm.h"

#define OPCOUNT 128

// process instrs.
#define die_op      0
#define nop_op      1
#define perr_op     2 // Print error message (perror)
#define brkp_op     3 // Breakpoint

// time / timing instrs.
// Time Retrieval
#define systime_op  4  // Retrieve current system time in seconds
#define htime_op    5  // Retrieve current time with nanosecond precision
#define getutc_op   6  // Get the current UTC time as a timestamp
#define getlocal_op 7  // Get the current local time as a timestamp

// Time Manipulation
#define timeadd_op  8  // Add a time offset (seconds or milliseconds) to a timestamp
#define timesub_op  9  // Subtract a time offset (seconds or milliseconds) from a timestamp
#define settime_op  10 // Modify specific components (e.g., hour, minute) of a timestamp

// Delays and Synchronization
#define delay_op    11 // Suspend execution for a specific duration (milliseconds)
#define waituntil_op 12 // Block execution until a specified timestamp is reached

// Performance Timing
#define startprof_op 13 // Start a high-precision performance timer
#define stopprof_op  14 // Stop the high-precision performance timer
#define elapsed_op   15 // Retrieve the elapsed time since the profiling timer was started

// Date and Calendar Utilities
#define getweekday_op 16 // Get the current day of the week (0-6, Sunday=0)
#define monthdays_op  17 // Calculate the number of days in a specific month/year
#define tstampstr_op  18 // Convert a timestamp to a formatted string (e.g., "YYYY-MM-DD HH:MM:SS")

// Advanced Time Operations
#define setepoch_op   19 // Define a custom epoch for time calculations
#define cmptime_op    20 // Compare two timestamps for ordering (less, equal, greater)
#define timediff_op   21 // Compute the time difference between two timestamps
#define normtime_op   22 // Normalize a timestamp (e.g., roll over excess seconds)

// File operations
#define fopn_op   23 // Open file (fopen)
#define fcls_op   24 // Close file (fclose)
#define frd_op    25 // Read file (fread)
#define fwr_op    26 // Write file (fwrite)
#define fsk_op    27 // Move file position (fseek)
#define ftel_op   28 // Get file position (ftell)
#define rwnd_op   29 // Reset file position (rewind)

// String and input/output operations
#define prntf_op    30 // Print formatted output to stdout (printf)
#define fprntf_op   31 // Print formatted output to a file (fprintf)
#define sprntf_op   32 // Print formatted output to a string (sprintf)
#define sputs_op    33 // Print string + newline to stdout (puts)
#define sgets_op    34 // Read string from stdin (gets, deprecated)
#define scnf_op     35 // Read formatted input from stdin (scanf)
#define sscnf_op    36 // Read formatted input from string (sscanf)
#define fgts_op     37 // Read string from file (fgets)
#define fpts_op     38 // Write string to file (fputs)

// Memory operations
#define mcpy_op    39 // Copy memory from source to destination (memcpy)
#define mmov_op    40 // Move memory safely (handles overlapping) (memmove)
#define scpy_op    41 // Copy a null-terminated string (strcpy)
#define sncy_op    42 // Copy up to n characters of a string (strncpy)
#define scat_op    43 // Concatenate strings (strcat)
#define snct_op    44 // Concatenate up to n characters of a string (strncat)
#define mcmp_op    45 // Compare two memory blocks (memcmp)
#define scmp_op    46 // Compare two null-terminated strings (strcmp)
#define sncm_op    47 // Compare up to n characters of strings (strncmp)
#define slen_op    48 // Get the length of a null-terminated string (strlen)
#define schr_op    49 // Find the first occurrence of a character (strchr)
#define srch_op    50 // Find the last occurrence of a character (strrchr)
#define sstr_op    51 // Find a substring within another string (strstr)
#define stok_op    52 // Tokenize a string using delimiters (strtok)
#define mset_op    53 // Set memory to a specific value (memset)
#define serr_op    54 // Get a string representation of an error code (strerror)
#define sdup_op    55 // Duplicate a string (strdup)
#define sspn_op    56 // Length of a prefix substring (strspn)
#define scspn_op   57 // Length of non-prefix substring (strcspn)
#define sfrm_op    58 // Transform string for locale comparison (strxfrm)

// Control flow
#define nspctr_op  59 // Print stack top as unsigned integer
#define call_op    60 // Function call
#define ret_op     61 // Return from function
#define swtch_op   62 // Switch operation
#define jmp_op     63 // Jump operation
#define jz_op      64 // Jump if zero
#define jnz_op     65 // Jump if not zero
#define je_op      66 // Jump if equal
#define jn_op      67 // Jump if not equal
#define jl_op      68 // Jump if less
#define jg_op      69 // Jump if greater
#define jls_op     70 // Jump if less or equal
#define jgs_op     71 // Jump if greater or equal

// Stack operations
#define pshu_op    72 // Push unsigned integer to stack
#define popu_op    73 // Pop unsigned integer from stack
#define pop2u_op   74 // Pop 2 unsigned integers from stack
#define popnu_op   75 // Pop multiple unsigned integers from stack
#define pshfru_op  76 // Push floating-point register to stack
#define poptru_op  77 // Pop floating-point register from stack
#define movtru_op  78 // Move floating-point register
#define stktru_op  79 // Store floating-point register to stack
#define cpyru_op   80 // Copy floating-point register to another
#define setru_op   81 // Set floating-point register value

#define pshi_op    82 // Push signed integer to stack
#define pshfri_op  83 // Push floating-point register to stack
#define poptri_op  84 // Pop floating-point register from stack
#define movtri_op  85 // Move floating-point register
#define stktri_op  86 // Store floating-point register to stack
#define cpyri_op   87 // Copy floating-point register to another
#define setri_op   88 // Set floating-point register value

#define pshr_op    89 // Push unsigned integer to stack
#define pshfrr_op  90 // Push floating-point register to stack
#define poptrr_op  91 // Pop floating-point register from stack
#define movtrr_op  92 // Move floating-point register
#define stktrr_op  93 // Store floating-point register to stack
#define cpyrr_op   94 // Copy floating-point register to another
#define setrr_op   95 // Set floating-point register value

// Arithmetic operations
#define incu_op    96 // Increment unsigned integer
#define decu_op    97 // Decrement unsigned integer
#define addu_op    98 // Add unsigned integers
#define subu_op    99 // Subtract unsigned integers
#define mulu_op    100 // Multiply unsigned integers
#define divu_op    101 // Divide unsigned integers
#define modu_op    102 // Modulo unsigned integers

#define inci_op    103 // Increment signed integer
#define deci_op    104 // Decrement signed integer
#define addi_op    105 // Add signed integers
#define subi_op    106 // Subtract signed integers
#define muli_op    107 // Multiply signed integers
#define divi_op    108 // Divide signed integers
#define modi_op    109 // Modulo signed integers

#define addr_op    110 // Add signed integers
#define subr_op    111 // Subtract signed integers
#define mulr_op    112 // Multiply signed integers
#define divr_op    113 // Divide signed integers
#define modr_op    114 // Modulo signed integers

// Trigonometric and mathematical functions
#define sqrt_op    115 // Square root
#define ceil_op    116 // Round up to nearest integer
#define floor_op   117 // Round down to nearest integer
#define sin_op     118 // Sine (floating-point)
#define cos_op     119 // Cosine (floating-point)
#define tan_op     120 // Tangent (floating-point)

// Exponential and absolute value operations
#define powu_op    121 // Unsigned exponential power
#define pows_op    122 // Signed exponential power
#define powr_op    123 // Floating-point exponential power
#define absu_op    124 // Unsigned absolute value
#define abss_op    125 // Signed absolute value
#define absr_op    126 // Floating-point absolute value

// Floating-point specific operations
#define fabs_op    127 // Floating-point absolute value (


#define build_optable() \
    void* optable[128] = { \
        &&op_die, \
        &&op_nop, \
        &&op_perr, \
        &&op_brkp, \
        &&op_systime, \
        &&op_htime, \
        &&op_getutc, \
        &&op_getlocal, \
        &&op_timeadd, \
        &&op_timesub, \
        &&op_settime, \
        &&op_delay, \
        &&op_waituntil, \
        &&op_startprof, \
        &&op_stopprof, \
        &&op_elapsed, \
        &&op_getweekday, \
        &&op_monthdays, \
        &&op_tstampstr, \
        &&op_setepoch, \
        &&op_cmptime, \
        &&op_timediff, \
        &&op_normtime, \
        &&op_fopn, \
        &&op_fcls, \
        &&op_frd, \
        &&op_fwr, \
        &&op_fsk, \
        &&op_ftel, \
        &&op_rwnd, \
        &&op_prntf, \
        &&op_fprntf, \
        &&op_sprntf, \
        &&op_sputs, \
        &&op_sgets, \
        &&op_scnf, \
        &&op_sscnf, \
        &&op_fgts, \
        &&op_fpts, \
        &&op_mcpy, \
        &&op_mmov, \
        &&op_scpy, \
        &&op_sncy, \
        &&op_scat, \
        &&op_snct, \
        &&op_mcmp, \
        &&op_scmp, \
        &&op_sncm, \
        &&op_slen, \
        &&op_schr, \
        &&op_srch, \
        &&op_sstr, \
        &&op_stok, \
        &&op_mset, \
        &&op_serr, \
        &&op_sdup, \
        &&op_sspn, \
        &&op_scspn, \
        &&op_sfrm, \
        &&op_nspctr, \
        &&op_call, \
        &&op_ret, \
        &&op_swtch, \
        &&op_jmp, \
        &&op_jz, \
        &&op_jnz, \
        &&op_je, \
        &&op_jn, \
        &&op_jl, \
        &&op_jg, \
        &&op_jls, \
        &&op_jgs, \
        &&op_pshu, \
        &&op_popu, \
        &&op_pop2u, \
        &&op_popnu, \
        &&op_pshfru, \
        &&op_poptru, \
        &&op_movtru, \
        &&op_stktru, \
        &&op_cpyru, \
        &&op_setru, \
        &&op_pshi, \
        &&op_pshfri, \
        &&op_poptri, \
        &&op_movtri, \
        &&op_stktri, \
        &&op_cpyri, \
        &&op_setri, \
        &&op_pshr, \
        &&op_pshfrr, \
        &&op_poptrr, \
        &&op_movtrr, \
        &&op_stktrr, \
        &&op_cpyrr, \
        &&op_setrr, \
        &&op_incu, \
        &&op_decu, \
        &&op_addu, \
        &&op_subu, \
        &&op_mulu, \
        &&op_divu, \
        &&op_modu, \
        &&op_inci, \
        &&op_deci, \
        &&op_addi, \
        &&op_subi, \
        &&op_muli, \
        &&op_divi, \
        &&op_modi, \
        &&op_addr, \
        &&op_subr, \
        &&op_mulr, \
        &&op_divr, \
        &&op_modr, \
        &&op_sqrt, \
        &&op_ceil, \
        &&op_floor, \
        &&op_sin, \
        &&op_cos, \
        &&op_tan, \
        &&op_powu, \
        &&op_pows, \
        &&op_powr, \
        &&op_absu, \
        &&op_abss, \
        &&op_absr, \
        &&op_fabs \
    };


#define is_opcode(_value)                      \
    ((_value > 0) || (_value < (OPCOUNT - 1))) \

static
const char*
mnemonic_strings[] =
{
		"die", \
		"nop", \
		"perr", \
		"brkp", \
		"systime", \
		"htime", \
		"getutc", \
		"getlocal", \
		"timeadd", \
		"timesub", \
		"settime", \
		"delay", \
		"waituntil", \
		"startprof", \
		"stopprof", \
		"elapsed", \
		"getweekday", \
		"monthdays", \
		"tstampstr", \
		"setepoch", \
		"cmptime", \
		"timediff", \
		"normtime", \
		"fopn", \
		"fcls", \
		"frd", \
		"fwr", \
		"fsk", \
		"ftel", \
		"rwnd", \
		"prntf", \
		"fprntf", \
		"sprntf", \
		"sputs", \
		"sgets", \
		"scnf", \
		"sscnf", \
		"fgts", \
		"fpts", \
		"mcpy", \
		"mmov", \
		"scpy", \
		"sncy", \
		"scat", \
		"snct", \
		"mcmp", \
		"scmp", \
		"sncm", \
		"slen", \
		"schr", \
		"srch", \
		"sstr", \
		"stok", \
		"mset", \
		"serr", \
		"sdup", \
		"sspn", \
		"scspn", \
		"sfrm", \
		"nspctr", \
		"call", \
		"ret", \
		"swtch", \
		"jmp", \
		"jz", \
		"jnz", \
		"je", \
		"jn", \
		"jl", \
		"jg", \
		"jls", \
		"jgs", \
		"pshu", \
		"popu", \
		"pop2u", \
		"popnu", \
		"pshfru", \
		"poptru", \
		"movtru", \
		"stktru", \
		"cpyru", \
		"setru", \
		"pshi", \
		"pshfri", \
		"poptri", \
		"movtri", \
		"stktri", \
		"cpyri", \
		"setri", \
		"pshr", \
		"pshfrr", \
		"poptrr", \
		"movtrr", \
		"stktrr", \
		"cpyrr", \
		"setrr", \
		"incu", \
		"decu", \
		"addu", \
		"subu", \
		"mulu", \
		"divu", \
		"modu", \
		"inci", \
		"deci", \
		"addi", \
		"subi", \
		"muli", \
		"divi", \
		"modi", \
		"addr", \
		"subr", \
		"mulr", \
		"divr", \
		"modr", \
		"sqrt", \
		"ceil", \
		"floor", \
		"sin", \
		"cos", \
		"tan", \
		"powu", \
		"pows", \
		"powr", \
		"absu", \
		"abss", \
		"absr", \
		"fabs" \
};

static
int
is_op_mnemonic(const char* test_string)
{
    for (uint8_t i = 0; i < OPCOUNT; i++)
	{
        if (strcmp(mnemonic_strings[i], test_string) == 0)
            return 1;
    }
    return 0;
}

static
uint8_t
get_mnemonic_opcode(const char* mnemonic)
{
    for (uint8_t i = 0; i < OPCOUNT; i++)
	{
        if (strcmp(mnemonic_strings[i], mnemonic) == 0)
            return i;
    }
    return 255;
}

static
uint8_t
opsize_tbl[] = // 61
{
	4, 4, 8, 4,
	8, 8, 8, 8,
	12, 12, 12, 8,
	8, 8, 4, 4,

	8, 8, 8, 12,
	8, 12, 12, 8,
	12, 8, 16, 16,
	12, 8, 8, 12,

	16, 16, 8, 8,
	12, 16, 12, 12,
	16, 16, 12, 16,
	12, 16, 16, 12,

	16, 8, 12, 12,
	12, 12, 16, 8,
	8, 12, 12, 8,  // 60
	8, 4, 4, 4,

	8, 8, 8, 8,
	8, 8, 8, 8,
	8, 8, 4, 4,
	8, 8, 8, 8,

	8, 12, 12, 8,
	12, 12, 8, 8,

	8, 8, 8, 12, 12,
	4, 4, 4, 4,
	4, 4, 4, 4,
	4, 4, 4, 4,
	4, 4, 4, 4,
	4, 4, 4, 4,
	4, 4, 4, 4,
	4, 4, 4, 4,
	4, 4, 4, 4
};

static
uint8_t
opargc_tbl[] =
{
	0, 0, 1, 0,
	1, 1, 1, 1,
	2, 2, 2, 1,
	1, 1, 0, 0,

	1, 1, 1, 2,
	1, 2, 2, 1,
	2, 1, 3, 3,
	2, 1, 1, 2,

	3, 3, 1, 1,
	2, 3, 2, 2,
	3, 3, 2, 3,
	2, 3, 3, 2,

	3, 1, 2, 2, // 52 i
	2, 2, 3, 1,
	1, 2, 2, 1,
	0, 1, 0, 0,

	1, 1, 1, 1,
	1, 1, 1, 1,
	1, 1, 0, 0,
	1, 1, 1, 1,

	1, 2, 2, 1,
	2, 2, 1, 1,
	1, 1, 1, 2, 2,

	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0
};

#endif // OPCODE.H