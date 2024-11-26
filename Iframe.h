#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#ifndef IFRAME_H
#define IFRAME_H

#include "infostrings.h"
#include "exceptions.h"
#include "workstack.h"
#include "opcode.h"
#include "ram.h"
#include "vm.h"

#include <stdint.h>
#include <vector>

// macros used by Iframe()'s opr_typerecord[]
// used like this:
// opr_typerecord[RESULT] = datatype record of any result produced by iframe.
// opr_typerecord[OPERAND1]
// opr_typerecord[OPERAND2]
// opr_typerecord[OPERAND3]

#define RESULT   0
#define OPERAND1 1
#define OPERAND2 2
#define OPERAND3 3

class IFrame {
	public:
		u8 opcode;
		u32 addr;
		u32 abs_addr;
		u32 instr_num;
		char* info_str;
		IFrame* last_iframe;
		IFrame* next_iframe;
		IFrame* parent_iframe;
		u32 loop_count;
		u32 exec_count;
		u32 un_exec_count;
		u32 uint_arg_left;
		u32 uint_arg_right;
		u32 uint_result;
		i32 int_arg_left;
		i32 int_arg_right;
		i32 int_result;
		bool bool_result;
		bool zero_div;
		bool result_produced;
		bool ignored;
		u8 opr_count;
		u8 opr_typerec[4];
		u32 uint_opr1;
		u32 uint_opr2;
		u32 uint_opr3;
		u32 int_opr1;
		u32 int_opr2;
		u32 int_opr3;
		u32 src_addr;
		u32 dst_addr;
		u32 interm_src_addr;
		u32 prev_ram_uint_value;
		i32 prev_ram_int_value;
		
		bool is_inserted;

		IFrame(u8 _opcode, u32 _addr, u32 _abs_addr);
		~IFrame();
		void build_infostr();
};

struct IFrameVector
{

	std::vector<IFrame*>* vec;

	IFrameVector();
	~IFrameVector();
	void append_iframe(IFrame* iframe);
	void insert_iframe(IFrame* iframe, size_t index);
	IFrame* get_iframe(u32 addr);
	void print_iframe(u32 instr_num);
	void print();
};

#endif // IFRAME_H
