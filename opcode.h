#ifndef OPCODE_H
#define OPCODE_H

#include "vm.h"


#define OPCOUNT    61 

#define die_op      0
#define nop_op      1
#define nspctr_op   2
#define nspctst_op  3 // CURRENTLY ACTING AS PRINT STACK TOP AS UINT INSTR NO OPERAND INSTR.
#define test_die_op 4
#define call_op     5
#define ret_op      6
#define swtch_op    7
#define jmp_op      8
#define je_op       9
#define jn_op      10
#define jl_op      11
#define jg_op      12
#define jls_op     13
#define jgs_op     14
#define loop_op    15
#define lcont_op   16
#define lbrk_op    17
#define psh_op     18
#define pop_op     19
#define pop2_op    20
#define popn_op    21
#define pshfr_op   22
#define poptr_op   23
#define movtr_op   24
#define stktr_op   25
#define cpyr_op    26
#define setr_op    27
#define pshfrr_op  28
#define pshfrs_op  29
#define inc_op     30
#define dec_op     31
#define add_op     32
#define sub_op     33
#define mul_op     34
#define div_op     35
#define mod_op     36
#define incs_op    37
#define decs_op    38
#define adds_op    39
#define subs_op    40
#define muls_op    41
#define divs_op    42
#define mods_op    43
#define and_op     44
#define not_op     45
#define xor_op     46
#define or_op      47
#define lshft_op   48
#define rshft_op   49
#define lrot_op    50
#define rrot_op    51
#define ands_op    52
#define nots_op    53
#define xors_op    54
#define ors_op     55
#define lshfts_op  56
#define rshfts_op  57
#define lrots_op   58
#define rrots_op   59
#define brkp_op    60

#define build_optable()                   \
    void* optable[OPCOUNT] = {            \
        &&die,                            \
        &&nop,                            \
        &&nspctr,                         \
        &&nspctst,                        \
        &&test_die,                       \
        &&call,                           \
        &&ret,                            \
        &&swtch,                          \
        &&jmp,                            \
        &&je,                             \
        &&jn,                             \
        &&jl,                             \
        &&jg,                             \
        &&jls,                            \
        &&jgs,                            \
        &&loop,                           \
        &&lcont,                          \
        &&lbrk,                           \
        &&psh,                            \
        &&pop,                            \
        &&pop2,                           \
        &&popn,                           \
        &&pshfr,                          \
        &&poptr,                          \
        &&movtr,                          \
        &&stktr,                          \
        &&cpyr,                           \
        &&setr,                           \
        &&pshfrr,                         \
        &&pshfrs,                         \
        &&inc,                            \
        &&dec,                            \
        &&add,                            \
        &&sub,                            \
        &&mul,                            \
        &&div,                            \
        &&mod,                            \
        &&incs,                           \
        &&decs,                           \
        &&adds,                           \
        &&subs,                           \
        &&muls,                           \
        &&divs,                           \
        &&mods,                           \
        &&and,                            \
        &&not,                            \
        &&xor,                            \
        &&or,                             \
        &&lshft,                          \
        &&rshft,                          \
        &&lrot,                           \
        &&rrot,                           \
        &&ands,                           \
        &&nots,                           \
        &&xors,                           \
        &&ors,                            \
        &&lshfts,                         \
        &&rshfts,                         \
        &&lrots,                          \
        &&rrots,                          \
        &&brkp                            \
    }

#define is_opcode(_value)                      \
    ((_value > 0) || (_value < (OPCOUNT - 1))) \

static
const char*
mnemonic_strings[] =
{
    "die",
    "nop",
    "nspctr",
    "nspctst",
    "test_die",
    "call",
    "ret",
    "swtch",
    "jmp",
    "je",
    "jn",
    "jl",
    "jg",
    "jls",
    "jgs",
    "loop",
    "lcont",
    "lbrk",
    "psh",
    "pop",
    "pop2",
    "popn",
    "pshfr",
    "poptr",
    "movtr",
    "stktr",
    "cpyr",
    "setr",
    "pshfrr",
    "pshfrs",
    "inc",
    "dec",
    "add",
    "sub",
    "mul",
    "div",
    "mod",
    "incs",
    "decs",
    "adds",
    "subs",
    "muls",
    "divs",
    "mods",
    "and",
    "not",
    "xor",
    "or",
    "lshft",
    "rshft",
    "lrot",
    "rrot",
    "ands",
    "nots",
    "xors",
    "ors",
    "lshfts",
    "rshfts",
    "lrots",
    "rrots",
    "brkp"
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
opsize_tbl[] =
{
     4,  // 'die'
     4,  // 'nop'
     8,  // 'nspct'
     4,  // 'nspctst'
     4,  // 'test_die'
     8,  // 'call'
     4,  // 'ret'
     4,  // 'swtch'
     8,  // 'jmp'
     8,  // 'je'
     8,  // 'jn'
     8,  // 'jl'
     8,  // 'jg'
     8,  // 'jls'
     8,  // 'jgs'
     16, // 'loop'
     4,  // 'lcont'
     4,  // 'lbrk'
     8,  // 'psh'
     4,  // 'pop'
     4,  // 'pop2'
     8,  // 'popn'
     8,  // 'pshfr'
     8,  // 'poptr'
     8,  // 'movtr'
     12,  // 'stktr'
     12,  // 'cpyr'
     12,  // 'setr'
     8,  // 'pshfrr'
     8,  // 'pshfrs'
     4,  // 'inc'
     4,  // 'dec'
     4,  // 'add'
     4,  // 'sub'
     4,  // 'mul'
     4,  // 'div'
     4,  // 'mod'
     4,  // 'incs'
     4,  // 'decs'
     4,  // 'adds'
     4,  // 'subs'
     4,  // 'muls'
     4,  // 'divs'
     4,  // 'mods'
     4,  // 'and'
     4,  // 'not'
     4,  // 'xor'
     4,  // 'or'
     4,  // 'lshft'
     4,  // 'rshft'
     4,  // 'lrot'
     4,  // 'rrot'
     4,  // 'ands'
     4,  // 'nots'
     4,  // 'xors'
     4,  // 'ors'
     4,  // 'lshfts'
     4,  // 'rshfts'
     4,  // 'lrots'
     4   // 'rrots'
 };

static
uint8_t
opargc_tbl[] =
{
     0,  // 'die'
     0,  // 'nop'
     1,  // 'nspct'
     0,  // 'nspctst'
     0,  // 'test_die'
     1,  // 'call'
     0,  // 'ret'
     0,  // 'swtch'
     1,  // 'jmp'
     1,  // 'je'
     1,  // 'jn'
     1,  // 'jl'
     1,  // 'jg'
     1,  // 'jls'
     1,  // 'jgs'
     3,  // 'loop'
     0,  // 'lcont'
     0,  // 'lbrk'
     1,  // 'psh'
     0,  // 'pop'
     0,  // 'pop2'
     1,  // 'popn'
     1,  // 'pshfr'
     1,  // 'poptr'
     1,  // 'movtr'
     2,  // 'stktr'
     2,  // 'cpyr'
     2,  // 'setr'
     1,  // 'pshfrr'
     1,  // 'pshfrs'
     0,  // 'inc'
     0,  // 'dec'
     0,  // 'add'
     0,  // 'sub'
     0,  // 'mul'
     0,  // 'div'
     0,  // 'mod'
     0,  // 'incs'
     0,  // 'decs'
     0,  // 'adds'
     0,  // 'subs'
     0,  // 'muls'
     0,  // 'divs'
     0,  // 'mods'
     0,  // 'and'
     0,  // 'not'
     0,  // 'xor'
     0,  // 'or'
     0,  // 'lshft'
     0,  // 'rshft'
     0,  // 'lrot'
     0,  // 'rrot'
     0,  // 'ands'
     0,  // 'nots'
     0,  // 'xors'
     0,  // 'ors'
     0,  // 'lshfts'
     0,  // 'rshfts'
     0,  // 'lrots'
     0   // 'rrots'
 };


#endif // OPCODE.H