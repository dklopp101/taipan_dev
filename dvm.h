#ifndef DVM_H
#define DVM_H

#include <iostream>
#include <fstream>
#include <string>

#include <conio.h>

#include "infostrings.h"
#include "exceptions.h"
#include "user_input.h"
#include "workstack.h"
#include "opcode.h"
#include "iframe.h"
#include "ram.h"
#include "asm.h"
#include "vm.h"

#define DVM_DEFAULT_RAM_SIZE    1000
#define DVM_DEFAULT_STACK_SIZE  1000
#define DVM_DEFAULT_RECUR_LIMIT  100

#define DVM_INPUT_FILE_EXTENSION ".fbin"
#define DVM_OPT_TBL_SIZE 6

#define RAM_SIZE_ARG    1
#define STACK_SIZE_ARG  2
#define RECUR_LIMIT_ARG 3
#define EXEC_LOCKED_ARG 4

static
const char*
dvm_opt_tbl[DVM_OPT_TBL_SIZE] =
{
	"none",
	"ram_size=",
	"stack_size=",
	"recur_limit=",
	"exec_locked=",
	NULL
};

struct DVM_ArgTbl;

// this macro below controls whether the internal debugging messages 
// are displayed. leaving it defined enables debugging messages,
// commenting it out removes them.
//#define ENABLE_INTERNAL_DEBUG MESSAGES

#ifdef ENABLE_INTERNAL_DEBUG MESSAGES
	// INTERNAL DEBUGGING MACROS.
	#define DEBUG_currently_in_function(MSG__) \
		printf("\n\nDEBUG MSG: currently in %s function.", (MSG__))

	// INTERNAL DEBUGGING USER-INPUT / INPUT-CODES PRINTF MACROS.
	#define DEBUG_user_input_received_from_get_main_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: user input given to get_main_menu_input() :: key: %s", (MSG__))

	#define DEBUG_input_code_received_from_get_main_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: input-code received from get_main_menu_input() :: CODE: %s", (MSG__))

	#define DEBUG_user_input_received_from_get_bp_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: user input given to get_bp_menu_input() :: key: %s", (MSG__))

	#define DEBUG_input_code_received_from_get_bp_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: input-code received from get_bp_menu_input() :: CODE: %s", (MSG__))

	#define DEBUG_user_input_received_from_get_ram_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: user input given to get_ram_menu_input() :: key: %s", (MSG__))

	#define DEBUG_input_code_received_from_get_ram_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: input-code received from get_ram_menu_input() :: CODE: %s", (MSG__))

	#define DEBUG_user_input_received_from_get_wstk_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: user input given to get_wstk_menu_input() :: key: %s", (MSG__))

	#define DEBUG_input_code_received_from_get_wstk_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: input-code received from get_wstk_menu_input() :: CODE: %s", (MSG__))

	#define DEBUG_user_input_received_from_get_cstk_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: user input given to get_cstk_menu_input() :: key: %s", (MSG__))

	#define DEBUG_input_code_received_from_get_cstk_menu_input(MSG__) \
		printf("\n\nDEBUG MSG: input-code received from get_cstk_menu_input() :: CODE: %s", (MSG__))

	#define DEBUG_user_input_received_from_get_user_confirmation(MSG__) \
		printf("\n\nDEBUG MSG: input-code received from get_user_confirmation() :: %s", (MSG__))

#else
	#define DEBUG_currently_in_function(MSG__) ;
	#define DEBUG_user_input_received_from_get_main_menu_input(MSG__) ;
	#define DEBUG_input_code_received_from_get_main_menu_input(MSG__) ;
	#define DEBUG_user_input_received_from_get_bp_menu_input(MSG__) ;
	#define DEBUG_input_code_received_from_get_bp_menu_input(MSG__) ;
	#define DEBUG_user_input_received_from_get_ram_menu_input(MSG__) ;
	#define DEBUG_input_code_received_from_get_ram_menu_input(MSG__) ;
	#define DEBUG_user_input_received_from_get_wstk_menu_input(MSG__) ;
	#define DEBUG_input_code_received_from_get_wstk_menu_input(MSG__) ;
	#define DEBUG_user_input_received_from_get_cstk_menu_input(MSG__) ;
	#define DEBUG_input_code_received_from_get_cstk_menu_input(MSG__) ;
	#define DEBUG_user_input_received_from_get_user_confirmation(MSG__) ;
#endif

#define RESULT      0  // index of result type in operand-typerecord array.

#define ESC_KEY_CODE 27 // POSSIBLY NOT NEEDED?!?1

struct
dvmErrorObject : public std::exception
{
	char* errmsg; // msg to print to the user.
	u8    old_errcode;
	dvmErrorObject(const char* _extra_msg, u8 _old_errcode);

	~dvmErrorObject();

	void build_errmsg_str(const char* _extra_msg);
	void print_errmsg();
};

enum class DvmError
{
	GENERAL_ERR = 1,
	CALLSTACK_EXCEEDED_RECUR_LIMIT,
	CALLSTACK_EMPTY_POP,
	DUPLICATE_BREAKPOINT_DEC,
	INVALID_BREAKPOINT_ACCESS,
	INVALID_CALLSTACK_RECUR_MOD
};

class CallStack
{
    u32* stk;

	public:
		u32* top;
		u32  curr_top_ndx;
		u32  recur_level;
		u32  recursion_limit;

		CallStack(u32 _recursion_limit);
		~CallStack();
		void push_addr(u32 addr);
		u32  pop_addr();
		void remove_addr(u32 index);
		void reset();
		void print_all();
		void print_next();
		void modify_recur_limit(u32 new_recur_limit);
};

struct BreakVector
{
	std::vector<u32> vec;

	BreakVector();
	bool is_breakpoint(u32 addr);
	u32 new_breakpoint(u32 addr);
	u32 remove_breakpoint(u32 addr);
};

struct
DebugVM
{
	DVM_ArgTbl*   argtbl;
    WorkStack*    workstack;
    CallStack*    callstack;
    BreakVector*  break_vec;
    IFrameVector* iframe_vec;

    IFrame* last_iframe;
    IFrame* curr_iframe;
    IFrame* next_iframe;

    Ram* ram;

    u32 pc;
    u8  opcode;
    u64 abs_addr;
    u32 loop_counter;
	u32 curr_iframe_num;

	bool in_bp_mode_argv; // option set via argv
	bool exec_locked_argv;        // option set via argv

	bool in_bp_mode;
    bool exec_locked;
	bool breakpoint_hit;
    bool program_end_reached;
	bool shutdown_triggered;
    bool print_exec_info;

    std::vector<u32>  popped_uints;
    std::vector<u32>  popped_retaddrs;
    std::vector<i32>  popped_ints;
    std::vector<u32>  unpushed_uints;
    std::vector<u32>  unpushed_retaddrs;
    std::vector<i32>  unpushed_ints;

    void undo_exec_iframe(IFrame* instr);
    void exec_iframe(IFrame* instr);

	DebugVM(DVM_ArgTbl* argtbl_,
		    Ram* ram_,
		    IFrameVector* iframe_vec_,
		    u32 callstack_recur_limit,
		    u8 exec_locked_,
		    u8 in_bp_mode_);

	~DebugVM();

	void build_iframe_from_iblock(InstrBlock* instr);

    void mainloop_handle();
    void exec_handle();
	void start_debug(const char* path);
	void loadprog(const char* path);
	void reset_state();

	// iframe-builder functions.
	void exec_all_IFBF();
	void exec_iframe_IFBF(IFrame* instr);

	void bp_menu();
	void ram_menu();
	void wstk_menu();
	void cstk_menu();

	void display_iframes();

	// individual iframe builder functions.
	void build_iframe();
    void build_nspctr_op_iframe();
    void build_nspctrs_op_iframe();
    void build_nspctst_op_iframe();
    void build_nspctsts_op_iframe();
    void build_call_op_iframe();
    void build_jmp_op_iframe();
    void build_je_op_iframe();
    void build_jn_op_iframe();
    void build_jl_op_iframe();
    void build_jg_op_iframe();
    void build_jls_op_iframe();
    void build_jgs_op_iframe();
    void build_loop_op_iframe();
    void build_lcont_op_iframe();
    void build_psh_op_iframe();
    void build_popn_op_iframe();
    void build_pshfr_op_iframe();
    void build_poptr_op_iframe();
    void build_movtr_op_iframe();
    void build_stktr_op_iframe();
    void build_cpyr_op_iframe();
    void build_setr_op_iframe();
    void build_pshfrr_op_iframe();
    void build_pshfrs_op_iframe();
    void build_inc_op_iframe();
    void build_dec_op_iframe();
    void build_add_op_iframe();
    void build_sub_op_iframe();
    void build_mul_op_iframe();
    void build_div_op_iframe();
    void build_mod_op_iframe();
    void build_incs_op_iframe();
    void build_decs_op_iframe();
    void build_adds_op_iframe();
    void build_subs_op_iframe();
    void build_muls_op_iframe();
    void build_divs_op_iframe();
    void build_mods_op_iframe();
    void build_and_op_iframe();
    void build_not_op_iframe();
    void build_xor_op_iframe();
    void build_or_op_iframe();
    void build_lshft_op_iframe();
    void build_rshft_op_iframe();
    void build_lrot_op_iframe();
    void build_rrot_op_iframe();
    void build_ands_op_iframe();
    void build_nots_op_iframe();
    void build_xors_op_iframe();
    void build_ors_op_iframe();
    void build_lshfts_op_iframe();
    void build_rshfts_op_iframe();
    void build_lrots_op_iframe();
    void build_rrots_op_iframe();

	void build_inserted_instr();
	void build_nspctr_op_iframe_INSERTED(InstrBlock* instr);
	void build_nspctrs_op_iframe_INSERTED(InstrBlock* instr);
	void build_nspctst_op_iframe_INSERTED(InstrBlock* instr);
	void build_nspctsts_op_iframe_INSERTED(InstrBlock* instr);
	void build_call_op_iframe_INSERTED(InstrBlock* instr);
	void build_jmp_op_iframe_INSERTED(InstrBlock* instr);
	void build_je_op_iframe_INSERTED(InstrBlock* instr);
	void build_jn_op_iframe_INSERTED(InstrBlock* instr);
	void build_jl_op_iframe_INSERTED(InstrBlock* instr);
	void build_jg_op_iframe_INSERTED(InstrBlock* instr);
	void build_jls_op_iframe_INSERTED(InstrBlock* instr);
	void build_jgs_op_iframe_INSERTED(InstrBlock* instr);
	void build_loop_op_iframe_INSERTED(InstrBlock* instr);
	void build_lcont_op_iframe_INSERTED(InstrBlock* instr);
	void build_psh_op_iframe_INSERTED(InstrBlock* instr);
	void build_popn_op_iframe_INSERTED(InstrBlock* instr);
	void build_pshfr_op_iframe_INSERTED(InstrBlock* instr);
	void build_poptr_op_iframe_INSERTED(InstrBlock* instr);
	void build_movtr_op_iframe_INSERTED(InstrBlock* instr);
	void build_stktr_op_iframe_INSERTED(InstrBlock* instr);
	void build_cpyr_op_iframe_INSERTED(InstrBlock* instr);
	void build_setr_op_iframe_INSERTED(InstrBlock* instr);
	void build_pshfrr_op_iframe_INSERTED(InstrBlock* instr);
	void build_pshfrs_op_iframe_INSERTED(InstrBlock* instr);
	void build_inc_op_iframe_INSERTED(InstrBlock* instr);
	void build_dec_op_iframe_INSERTED(InstrBlock* instr);
	void build_add_op_iframe_INSERTED(InstrBlock* instr);
	void build_sub_op_iframe_INSERTED(InstrBlock* instr);
	void build_mul_op_iframe_INSERTED(InstrBlock* instr);
	void build_div_op_iframe_INSERTED(InstrBlock* instr);
	void build_mod_op_iframe_INSERTED(InstrBlock* instr);
	void build_incs_op_iframe_INSERTED(InstrBlock* instr);
	void build_decs_op_iframe_INSERTED(InstrBlock* instr);
	void build_adds_op_iframe_INSERTED(InstrBlock* instr);
	void build_subs_op_iframe_INSERTED(InstrBlock* instr);
	void build_muls_op_iframe_INSERTED(InstrBlock* instr);
	void build_divs_op_iframe_INSERTED(InstrBlock* instr);
	void build_mods_op_iframe_INSERTED(InstrBlock* instr);
	void build_and_op_iframe_INSERTED(InstrBlock* instr);
	void build_not_op_iframe_INSERTED(InstrBlock* instr);
	void build_xor_op_iframe_INSERTED(InstrBlock* instr);
	void build_or_op_iframe_INSERTED(InstrBlock* instr);
	void build_lshft_op_iframe_INSERTED(InstrBlock* instr);
	void build_rshft_op_iframe_INSERTED(InstrBlock* instr);
	void build_lrot_op_iframe_INSERTED(InstrBlock* instr);
	void build_rrot_op_iframe_INSERTED(InstrBlock* instr);
	void build_ands_op_iframe_INSERTED(InstrBlock* instr);
	void build_nots_op_iframe_INSERTED(InstrBlock* instr);
	void build_xors_op_iframe_INSERTED(InstrBlock* instr);
	void build_ors_op_iframe_INSERTED(InstrBlock* instr);
	void build_lshfts_op_iframe_INSERTED(InstrBlock* instr);
	void build_rshfts_op_iframe_INSERTED(InstrBlock* instr);
	void build_lrots_op_iframe_INSERTED(InstrBlock* instr);
	void build_rrots_op_iframe_INSERTED(InstrBlock* instr);
};

struct DVM_ArgTbl
{
	IFrameVector* iframe_vec;

	const char* input_path;
	u32   ram_size;
	u32   workstack_size;
	u32   callstack_recur_limit;
	bool  exec_locked;
	bool  in_bp_mode;

	DVM_ArgTbl(IFrameVector* _iframe_vec,
		        const char* _input_path,
				u32   _ram_size,
				u32   _workstack_size,
				u32   _callstack_recur_limit,
				bool  _exec_locked,
				bool  _in_bp_mode) 
	:
		input_path(_input_path),
		ram_size(_ram_size),
		workstack_size(_workstack_size),
		callstack_recur_limit(_callstack_recur_limit),
		exec_locked(_exec_locked),
		in_bp_mode(_in_bp_mode),
		iframe_vec(_iframe_vec) {}
};

u8 get_dvm_arg_option(const char* string);

void print_DVM_ArgTbl(DVM_ArgTbl* argtbl);

// dvm entry point function.
int dvm_main(int argc, char* argv[]);
int main(int argc, char* argv[]);

// test functions.
void non_threaded_dvm_TEST();
void threaded_dvm_TEST();

#endif
