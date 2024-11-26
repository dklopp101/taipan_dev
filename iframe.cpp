#include "iframe.h"

#define RESULT              0  // index of result type in operand-typerecord array.
#define INSIDE_UINT_CODE   10
#define INSIDE_INT_CODE    11
#define INSIDE_UINT16_CODE 12
#define INSIDE_SYMTAB_CODE 13
#define PUSH               50
#define POP                60

/*
* TODO: implement code to produce src_code_string.
*/

IFrame::
IFrame(u8 _opcode,
	   u32 _addr,
	   u32 _abs_addr)
:
    opcode(_opcode),     addr(_addr),
    uint_arg_left(0),    uint_arg_right(0),
    uint_result(0),      info_str(nullptr),
    int_arg_left(0),     int_arg_right(0),
    int_result(0),       un_exec_count(0),
    uint_opr1(0),        uint_opr2(0),
    uint_opr3(0),        int_opr1(0),
    int_opr2(0),         int_opr3(0),
    opr_count(0),        opr_typerec{ 0, 0, 0, 0 },
	src_addr(0),         result_produced(false),
    dst_addr(0),         interm_src_addr(0),
    bool_result(false),  zero_div(false),
    loop_count(0),       abs_addr(_abs_addr),
    parent_iframe(NULL), last_iframe(NULL),
    next_iframe(NULL),   instr_num(0),
	exec_count(0),       prev_ram_uint_value(0),
	ignored(false),      prev_ram_int_value(0),
	is_inserted(false) {}

IFrame::
~IFrame() { free(info_str); }

void
IFrame::
build_infostr()
{
    info_str   = (char*) malloc(IFRAME_INFOSTR_MAXLEN);
	char* cursor = info_str;

	// PUT info_str MALLOC ERR CHECKING CODE HERE!

    // add base-level identifying instr properties to instr-infostr.
    sprintf(cursor, IFRAME_BASEID_INFOSTR, opcode, mnemonic_strings[opcode], addr, abs_addr, instr_num, exec_count, un_exec_count);
    cursor += strlen(cursor);

    // check if instr has any operands if so update infostr with their info.
    switch (opr_count)
	{
		case 0: break;

		case 1:
			switch (opr_typerec[OPERAND1])
			{
				case UINT32_CODE:
					sprintf(cursor, UINT_OPR1_INFOSTR, uint_opr1);
					break;

				case INT32_CODE:
					sprintf(cursor, INT_OPR1_INFOSTR, int_opr1);
					break;
			}

			cursor += strlen(cursor);
			break;

		case 2:
			switch (opr_typerec[OPERAND1])
			{
				case UINT32_CODE:
					sprintf(cursor, UINT_OPR1_INFOSTR, uint_opr1);
					break;

				case INT32_CODE:
					sprintf(cursor, INT_OPR1_INFOSTR, int_opr1);
					break;
			}

			cursor += strlen(cursor);

			switch (opr_typerec[OPERAND2])
			{
				case UINT32_CODE:
					sprintf(cursor, UINT_OPR2_INFOSTR, uint_opr2);
					break;

				case INT32_CODE:
					sprintf(cursor, INT_OPR2_INFOSTR, int_opr2);
					break;
			}

			cursor += strlen(cursor);
			break;

		case 3:
			switch (opr_typerec[OPERAND1])
			{
				case UINT32_CODE:
					sprintf(cursor, UINT_OPR1_INFOSTR, uint_opr1);
					break;

				case INT32_CODE:
					sprintf(cursor, INT_OPR1_INFOSTR, int_opr1);
					break;
			}

			cursor += strlen(cursor);

			switch (opr_typerec[OPERAND2])
			{
				case UINT32_CODE:
					sprintf(cursor, UINT_OPR2_INFOSTR, uint_opr2);
					break;

				case INT32_CODE:
					sprintf(cursor, INT_OPR2_INFOSTR, int_opr2);
					break;
			}

			cursor += strlen(cursor);

			switch (opr_typerec[OPERAND3])
			{
				case UINT32_CODE:
					sprintf(cursor, UINT_OPR3_INFOSTR, uint_opr3);
					break;

				case INT32_CODE:
					sprintf(cursor, INT_OPR3_INFOSTR, int_opr3);
					break;
			}

			cursor += strlen(cursor);
			break;
    }

    // check if instr produces a result if so add it to infostr.
    if (result_produced)
	{
        switch (opr_typerec[RESULT])
		{
			case UINT32_CODE:
				sprintf(cursor, UINT_RESULT_INFOSTR, uint_result);
				break;

			case INT32_CODE:
				sprintf(cursor, INT_RESULT_INFOSTR, int_result);
				break;

			case BOOL_CODE:
				sprintf(cursor, BOOL_RESULT_INFOSTR, bool_strings[bool_result]);
				break;
        }
    }

    // check if any form of divison-by-zero occurs within instr.
    // if so add to the infostr and end this function.
    if (zero_div)
        sprintf(cursor, ZERO_DIV_STR);

	// tack on some newlines to the infostr.
	//sprintf(cursor, "\n\n");

	//printf("\nDEBUG: \n%s\n", info_str); // debug print
}

IFrameVector::
IFrameVector()
{
	vec = new std::vector<IFrame*>();
}

IFrameVector::
~IFrameVector()
{
    for (size_t i = 0; i < vec->size(); ++i)
        delete vec->at(i);

	delete vec;
}

void
IFrameVector::
append_iframe(IFrame* iframe)
{
    if (vec->size())
	{
        iframe->last_iframe = vec->back();
        vec->back()->next_iframe = iframe;
    }

    iframe->instr_num = vec->size();

	// set iframe parent-instrs if required.
    switch (iframe->opcode)
	{
		case lcont_op:
		case lbrk_op:
			for (size_t i = vec->size(); i > 0; --i)
			{
				if (vec->at(i)->opcode == loop_op)
					iframe->parent_iframe = vec->at(i);
			}
    }

    vec->push_back(iframe);
}

void
IFrameVector::
insert_iframe(IFrame* iframe, size_t index)
{
	if (vec->size())
	{
		iframe->last_iframe = vec->back();
		vec->back()->next_iframe = iframe;
	}

	iframe->instr_num = index;

	// set iframe parent-instrs if required.
	switch (iframe->opcode)
	{
		case lcont_op:
		case lbrk_op:
			for (size_t i = vec->size(); i > 0; --i)
			{
				if (vec->at(i)->opcode == loop_op)
					iframe->parent_iframe = vec->at(i);
			}
	}
	
	// here we need to set the next_iframe var of the iframe that will preceed
	// the inserted iframe so it will point to the new inserted iframe. then
	// the new inserted iframes next_iframe var needs to point to the iframe currently
	// at the index where the new iframe is going to go, then that same iframe's 
	// last_iframe var needs to be set to the newly inserted iframe.

	// check if there's an iframe preceeding where the new iframe will go.
	if (index)
	{
		vec->at(index - 1)->next_iframe = iframe;
		iframe->last_iframe = vec->at(index - 1);
	}

	// check if there's an iframe after where the new iframe will be inserted.
	if (index != (vec->size() - 1))
	{
		// set next_iframe of new iframe to the iframe that will
		// proceed it to the new iframe.
		iframe->next_iframe = vec->at(index);

		// set the last_iframe of iframe that will proceed
		// the new iframe to the new iframe.
		vec->at(index)->last_iframe = iframe;
	}

	// insert the iframe.
	vec->insert(vec->begin() + index, iframe);

	// update all the iframe-num vars of iframes that proceed the inserted iframe.
	for (size_t i = index + 1; i < vec->size(); ++i)
	{
		(vec->at(i)->instr_num)++;
		vec->at(i)->build_infostr();
	}
}

IFrame*
IFrameVector::
get_iframe(u32 addr)
{
    for (size_t i = 0; i < vec->size(); ++i)
	{
        if (vec->at(i)->addr == addr)
            return vec->at(i);
    }

    return nullptr;
}

void
IFrameVector::
print_iframe(u32 instr_num)
{
	for (size_t i = 0; i < vec->size(); ++i)
	{
		if (vec->at(i)->instr_num == instr_num)
			printf("\n%s", vec->at(i)->info_str);
	}
}

void
IFrameVector::
print()
{
	for (size_t i = 0; i < vec->size(); ++i)
		printf("\n%s", vec->at(i)->info_str);
}