#include "dvm.h"

dvmErrorObject::
dvmErrorObject(const char* _extra_msg,
	           u8          _old_errcode = 0)
	: old_errcode(_old_errcode)
{
	build_errmsg_str(_extra_msg);
}

dvmErrorObject::
~dvmErrorObject()
{
	free(errmsg);
}

void
dvmErrorObject::
build_errmsg_str(const char* _extra_msg = NULL)
{
	errmsg = (char*)malloc(ASMERR_MSG_BUFSIZE);

	if (!errmsg)
		throw AssemblerError("\nmalloc() failed in dvmErrorObject::build_errmsg_str() [errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);]", (u8)ParserError::ASM_ERR_BUF_ALLOC_ERR);

	char* ch = errmsg;

	if (_extra_msg) // optional
	{
		sprintf(errmsg, "\dvm error: %s", _extra_msg);
		ch += strlen(ch);
	}

	else // default
	{
		sprintf(errmsg, "\ndvm error");
		ch += strlen(ch);
	}
}

void
dvmErrorObject::
print_errmsg()
{
	printf("\n%s", errmsg);
}

void
CallStack::
modify_recur_limit(u32 new_recur_limit)
{
	// throw error if the new recur limit
	// lower than the current top index.
	if (curr_top_ndx > new_recur_limit)
		throw dvmErrorObject("modify_recur_limit", (u8) DvmError::INVALID_CALLSTACK_RECUR_MOD);

	stk = (u32*) realloc(stk, new_recur_limit);

	if (!stk)
		throw AssemblerError("\nmalloc() failed in CallStack::modify_recur_limit() [stk = (u32*) realloc(stk, new_recur_limit);]");

	recursion_limit = new_recur_limit;
}

void
CallStack::
print_all()
{
	if (!recur_level)
	{
		printf("\n call-stack is empty.");
		return;
	}

	for (u32 i = 0; i < (curr_top_ndx + 1); i++)
		printf("\nindex: %u :: return address: %u", i, stk[i]);
}

void
CallStack::
print_next()
{
	if (!recur_level)
	{
		printf("\n call-stack is empty.");
		return;
	}

	printf("\nnext return address: %u", stk[curr_top_ndx]);
}

CallStack::
CallStack(u32 _recursion_limit) :
	recursion_limit(_recursion_limit),
	recur_level(0),
	curr_top_ndx(0)
{
	stk = (u32*) malloc(_recursion_limit);

	if (!stk)
		throw AssemblerError("\nmalloc() failed in CallStack::CallStack() [stk = (u32*) malloc(_recursion_limit);]");

	top = stk;
}

CallStack::
~CallStack() { free(stk); }

void
CallStack::
push_addr(u32 addr)
{
    if (curr_top_ndx == recursion_limit)
		throw dvmErrorObject("push_addr, recursion limit reached", (u8)DvmError::CALLSTACK_EXCEEDED_RECUR_LIMIT);

	if (recur_level++)
		stk[++curr_top_ndx] = addr;
	else
		stk[curr_top_ndx] = addr;
}

u32
CallStack::
pop_addr()
{
	if (!recur_level)
		throw dvmErrorObject("pop_addr, empty stack pop attempt", (u8)DvmError::CALLSTACK_EMPTY_POP);

	recur_level--;
	return stk[curr_top_ndx--];
}

void
CallStack::
remove_addr(u32 index)
{
	if (!recur_level)
		throw dvmErrorObject("remove_addr, empty stack remove attempt", (u8)DvmError::CALLSTACK_EMPTY_POP);

	// check if we're removing the top address, if so
	// we can skip the actions below.
	if (index != curr_top_ndx)
	{
		// we're removing an addr from inside the stack,
		// all addrs above the addr to be removed will be
		// copied to one index below their current index.
		for (u32 ndx = index; ndx < curr_top_ndx; ndx++)
			stk[ndx] = stk[ndx + 1];
	}

	curr_top_ndx--;
	recur_level--;
}

void
CallStack::
reset()
{
	top          = stk;
	curr_top_ndx = 0;
	recur_level  = 0;
}

BreakVector::
BreakVector()
{
    // insert 0 at index 0 so index 0 cannot be used by any addresses.
    vec.push_back(0);
}

bool
BreakVector::
is_breakpoint(u32 addr)
// returns index of addr if present in vector otherwise returns 0.
{
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == addr)
            return i;
    }

    return 0;
}

u32
BreakVector::
new_breakpoint(u32 addr)
// returns addr that was inserted into vec otherwise return 0.
{
    // only append addr to vector if not already present.
	if (is_breakpoint(addr))
		throw dvmErrorObject("new_breakpoint, breakpoint already at that address", (u8)DvmError::DUPLICATE_BREAKPOINT_DEC);

	vec.push_back(addr);
    return addr;
}

// returns addr that was removed from vec otherwise return 0.
u32
BreakVector::
remove_breakpoint(u32 addr)
{
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == addr)
            vec.erase(vec.begin() + i);
        return i;
    }

    return 0;
}

// reverses execution of the last executed instruction-frame
// who's ptr has been passed as instr.
void
DebugVM::
undo_exec_iframe(IFrame* instr)
{
	if (instr->instr_num)
		curr_iframe = instr->last_iframe;
	else
		curr_iframe = iframe_vec->vec->at(0);

    curr_iframe_num = curr_iframe->instr_num;
	pc              = curr_iframe->addr;

	instr->un_exec_count++;
	
    // do whatever needs doing to reverse the action's of the instruction.
    switch (instr->opcode) {
			// all these instrs only modify pc & last_pc.so no actions are required.
			// because pc & last_pc's state are restored before this switch stmt.
		case die_op:     case test_die_op: case nop_op: case swtch_op: case nspctr_op:
		case nspctst_op: case jmp_op:      case je_op:  case jn_op:    case jl_op:
		case jg_op:      case jls_op:      case jgs_op:
			break;

		case call_op:
			popped_retaddrs.push_back(callstack->pop_addr());
			break;

		case ret_op:
			callstack->push_addr(popped_retaddrs.back());
			popped_retaddrs.pop_back();
			break;

		case loop_op:
			loop_counter = 0;
			break;

		case lcont_op:
			loop_counter++;
			break;

		case lbrk_op:
			loop_counter = instr->loop_count;
			break;

		case pop_op:
			workstack->push_uint(popped_uints.back());
			popped_uints.pop_back();
			break;

		case pop2_op:
			workstack->push_uint(popped_uints.back());
			popped_uints.pop_back();
			workstack->push_uint(popped_uints.back());
			popped_uints.pop_back();
			break;

		case popn_op:
			for (int i = 0; i < (instr->uint_opr1); i++)
				workstack->push_uint(popped_uints.back());
			popped_uints.pop_back();
			break;

		case poptr_op:
			workstack->push_uint(popped_uints.back());
			popped_uints.pop_back();
			ram->set_uint(curr_iframe->dst_addr, instr->prev_ram_uint_value);
			pc += UINT_SIZE;
			break;

		case movtr_op: case stktr_op: case cpyr_op: case setr_op:
			ram->set_uint(curr_iframe->dst_addr, instr->prev_ram_uint_value);
			pc += UINT_SIZE;
			break;

			// handle all instrs that push uint onto workstack.
		case pshfrr_op: case pshfrs_op: case psh_op:    case pshfr_op: case inc_op:
		case dec_op:    case add_op:    case sub_op:    case mul_op:   case div_op:
		case mod_op:    case and_op:    case not_op:    case xor_op:   case or_op:
		case lshft_op:  case rshft_op:  case lrot_op:   case rrot_op:
			unpushed_uints.push_back(workstack->pop_uint());
			break;

		case incs_op:   case decs_op:   case adds_op:   case subs_op:  case muls_op:
		case divs_op:   case mods_op:   case ands_op:   case nots_op:  case xors_op:
		case ors_op:    case lshfts_op: case rshfts_op: case lrots_op: case rrots_op:
			workstack->push_uint(instr->uint_result);
			break;

		default:
			throw InternalErrorException(DVM_UNEXEC_IFRAME_IERROR_STR);
    }
}

void
DebugVM::
build_inserted_instr()
{
	bool option_tbl[ASM_ARGV_OPT_COUNT] = { false };
	char* x = 0;
	char* y = 0;

	//Assembler* assembler = new Assembler(option_tbl, x, y);
	//IMForm*    imform    = assembler->imform;

	//for (;;)
	//{ get_user_instr_input_:
	//	assembler->parser->linebuf = get_user_instr_input();
	//	if (!(assembler->parser->linebuf)) continue;
	//	assembler->parser->tokenize_line();

	//	try
	//	{
	//		//assembler->first_stage_pass();
	//		//assembler->build_im_form();
	//		break;
	//	}
	//	
	//	catch (AssemblerError e) {
	//		assembler->parser->tokstream->vec->clear();
	//		printf("\ninstruction not written properly try again.");
	//	}
	//}

	//Token* tok = assembler->parser->tokstream->vec->at(0);

	//if (tok->type != OPCODE_TOK)
	//{
	//	// reset the assembler.
	//	delete assembler;
	//	assembler = new Assembler(option_tbl, x, y);
	//	imform = assembler->imform;

	//	printf("\ninstruction not written properly try again.");
	//	goto get_user_instr_input_;
	//}

	//if (get_user_confirmation("\nconfirm instruction insertion"))
	//{
	//	build_iframe_from_iblock(imform->vec->at(0));
	//	printf("\ninstruction inserted.");
	//}

	//if (get_user_confirmation("\ndo you want to write another instruction?"))
	//{
	//	// reset the assembler.
	//	delete assembler;
	//	assembler = new Assembler(option_tbl, x, y);
	//	imform    = assembler->imform;

	//	goto get_user_instr_input_;
	//}

	//delete assembler;
}

void
DebugVM::
build_iframe_from_iblock(InstrBlock* instr)
{
	curr_iframe = new IFrame(instr->opcode, pc, abs_addr);
	curr_iframe->is_inserted = true;

	switch (instr->opcode) {
	case nspctr_op:   build_nspctr_op_iframe_INSERTED(instr);   break;
	case call_op:     build_call_op_iframe_INSERTED(instr);     break;
	case jmp_op:      build_jmp_op_iframe_INSERTED(instr);      break;
	case je_op:       build_je_op_iframe_INSERTED(instr);       break;
	case jn_op:       build_jn_op_iframe_INSERTED(instr);       break;
	case jl_op:       build_jl_op_iframe_INSERTED(instr);       break;
	case jg_op:       build_jg_op_iframe_INSERTED(instr);       break;
	case jls_op:      build_jls_op_iframe_INSERTED(instr);      break;
	case jgs_op:      build_jgs_op_iframe_INSERTED(instr);      break;
	case loop_op:     build_loop_op_iframe_INSERTED(instr);     break;
	case lcont_op:    build_lcont_op_iframe_INSERTED(instr);    break;
	case psh_op:      build_psh_op_iframe_INSERTED(instr);      break;
	case popn_op:     build_popn_op_iframe_INSERTED(instr);     break;
	case pshfr_op:    build_pshfr_op_iframe_INSERTED(instr);    break;
	case movtr_op:    build_movtr_op_iframe_INSERTED(instr);    break;
	case stktr_op:    build_stktr_op_iframe_INSERTED(instr);    break;
	case cpyr_op:     build_cpyr_op_iframe_INSERTED(instr);     break;
	case setr_op:     build_setr_op_iframe_INSERTED(instr);     break;
	case pshfrr_op:   build_pshfrr_op_iframe_INSERTED(instr);   break;
	case pshfrs_op:   build_pshfrs_op_iframe_INSERTED(instr);   break;
	case inc_op:      build_inc_op_iframe_INSERTED(instr);      break;
	case dec_op:      build_dec_op_iframe_INSERTED(instr);      break;
	case add_op:      build_add_op_iframe_INSERTED(instr);      break;
	case sub_op:      build_sub_op_iframe_INSERTED(instr);      break;
	case mul_op:      build_mul_op_iframe_INSERTED(instr);      break;
	case div_op:      build_div_op_iframe_INSERTED(instr);      break;
	case mod_op:      build_mod_op_iframe_INSERTED(instr);      break;
	case incs_op:     build_incs_op_iframe_INSERTED(instr);     break;
	case decs_op:     build_decs_op_iframe_INSERTED(instr);     break;
	case adds_op:     build_adds_op_iframe_INSERTED(instr);     break;
	case subs_op:     build_subs_op_iframe_INSERTED(instr);     break;
	case muls_op:     build_muls_op_iframe_INSERTED(instr);     break;
	case divs_op:     build_divs_op_iframe_INSERTED(instr);     break;
	case mods_op:     build_mods_op_iframe_INSERTED(instr);     break;
	case and_op:      build_and_op_iframe_INSERTED(instr);      break;
	case not_op:      build_not_op_iframe_INSERTED(instr);      break;
	case xor_op:      build_xor_op_iframe_INSERTED(instr);      break;
	case or_op:       build_or_op_iframe_INSERTED(instr);       break;
	case lshft_op:    build_lshft_op_iframe_INSERTED(instr);    break;
	case rshft_op:    build_rshft_op_iframe_INSERTED(instr);    break;
	case lrot_op:     build_lrot_op_iframe_INSERTED(instr);     break;
	case rrot_op:     build_rrot_op_iframe_INSERTED(instr);     break;
	case ands_op:     build_ands_op_iframe_INSERTED(instr);     break;
	case nots_op:     build_nots_op_iframe_INSERTED(instr);     break;
	case xors_op:     build_xors_op_iframe_INSERTED(instr);     break;
	case ors_op:      build_ors_op_iframe_INSERTED(instr);      break;
	case lshfts_op:   build_lshfts_op_iframe_INSERTED(instr);   break;
	case rshfts_op:   build_rshfts_op_iframe_INSERTED(instr);   break;
	case lrots_op:    build_lrots_op_iframe_INSERTED(instr);    break;
	case rrots_op:    build_rrots_op_iframe_INSERTED(instr);    break;

	case ret_op: case swtch_op:    case nop_op:
	case pop_op: case pop2_op:     case lbrk_op:
	case die_op: case test_die_op: case nspctst_op:              break;

	default:
		throw InternalErrorException(DVM_BUILD_IFRAME_IERROR_STR);
	}
}

// at the point of this function being called pc is
// still set to the opcode addr of instr.
void
DebugVM::
exec_iframe(IFrame* instr)
{
	DEBUG_currently_in_function("DebugVM::exec_iframe()");

	printf("\n\nexecuting ram[%u] %s instr #%u", instr->addr, mnemonic_strings[instr->opcode], instr->instr_num);

	if (!(instr->is_inserted))
		pc += opsize_tbl[instr->opcode];

    switch (instr->opcode) {
		case die_op: case test_die_op:
			program_end_reached = true;
			break;

		case nop_op: case swtch_op:
			break;

		case nspctr_op:
			printf(NSPCTR_INFOSTR, instr->src_addr, instr->uint_result);
			break;

		case nspctst_op:
			printf(NSPCTST_INFOSTR, instr->src_addr, instr->uint_result);
			break;

		case call_op:
			// pc is already set to the addr of the next instr.
			// so we can just push it straight onto the callstack.
			callstack->push_addr(pc);
			pc = instr->uint_opr1;
			break;

		case ret_op:
			pc = callstack->pop_addr();
			break;

		case jmp_op:
			pc = instr->uint_opr1;
			break;

		case je_op: case jn_op:  case jl_op:
		case jg_op: case jls_op: case jgs_op:
			if (instr->bool_result)
				pc = instr->uint_opr1;

			break;

		case loop_op:
			loop_counter = instr->uint_opr1;
			pc = instr->uint_opr2;
			break;

		case lcont_op:
			if (loop_counter--)
				pc = instr->parent_iframe->uint_opr2;
			else
				pc = instr->parent_iframe->uint_opr3;
			break;

		case lbrk_op:
			instr->loop_count = loop_counter;
			loop_counter = 0;
			pc = instr->parent_iframe->uint_opr3;
			break;

		case psh_op:
			workstack->push_uint(instr->uint_opr1);
			break;

		case pshfr_op:
		case pshfrr_op:
		case pshfrs_op:
			workstack->push_uint(ram->get_uint(instr->src_addr));
			break;

		case pop_op:
			popped_uints.push_back(workstack->pop_uint());
			break;

		case pop2_op:
			popped_uints.push_back(workstack->pop_uint());
			popped_uints.push_back(workstack->pop_uint());
			break;

		case popn_op:
			for (int i = 0; i < (instr->uint_opr1); i++)
				popped_uints.push_back(workstack->pop_uint());
			break;

		case poptr_op:
			popped_uints.push_back(workstack->pop_uint());
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, popped_uints.back());
			break;

		case movtr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, workstack->read_top_uint());
			break;

		case stktr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, workstack->read_uint(instr->src_addr));
			break;

		case cpyr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, ram->get_uint(instr->src_addr));
			break;

		case setr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, instr->uint_opr1);
			break;

		case inc_op:    case dec_op:   case add_op:    case sub_op:   case mul_op:
		case div_op:    case mod_op:   case and_op:    case not_op:   case xor_op:
		case or_op:     case lshft_op: case rshft_op:  case lrot_op:  case rrot_op:
			workstack->push_uint(instr->uint_result);
			break;

		case incs_op:   case decs_op:   case adds_op:   case subs_op:  case muls_op:
		case divs_op:   case mods_op:   case ands_op:   case nots_op:  case xors_op:
		case ors_op:    case lshfts_op: case rshfts_op: case lrots_op: case rrots_op:
			workstack->push_int(instr->int_result);
			break;

		default:
			throw InternalErrorException(DVM_EXEC_IFRAME_IERROR_STR);
    }

    curr_iframe->exec_count++;
	last_iframe = curr_iframe;
}

// main debugger execution loop.
void
DebugVM::
mainloop_handle()
{
	DEBUG_currently_in_function("DebugVM::mainloop_handle()");

	restart_mainloop_handle:
	DEBUG_currently_in_function("DebugVM::mainloop_handle() at restart_mainloop_handle label");

	u8 _user_input = 0;

	curr_iframe = iframe_vec->vec->at(0);
	pc          = ram->prog_start_addr;

	print_main_menu();

	for (;;)
	{ mainloop:
		exec_handle();
		curr_iframe_num++;

		if (program_end_reached) break;
		if (shutdown_triggered)  return;

		last_iframe = curr_iframe->last_iframe;
		curr_iframe = curr_iframe->next_iframe;
		next_iframe = curr_iframe->next_iframe;
	}

	// execution reaching here means shutdown has been triggered.
	// here we will show the user the shutdown menu which basically is
	// a main menu but only with options that make sense
	// (no exec-next-instr option) ect.
	printf("\nend of program reached.");

	print_prog_complete_menu_str_arr();

	get_user_input_shutdown_menu:
		_user_input = get_main_menu_input();
		Sleep(USER_INPUT_RESPONSE_TIME);

	switch (_user_input)
	{
		case UI_RESET_DEBUGGER:
			reset_state();
			printf("\ndebug session has been reset");
			goto restart_mainloop_handle;
		
		case UI_HELP:
			printf("\nsorry, help function has not been implemented yet.");
			goto get_user_input_shutdown_menu;

		case UI_INVALID:
			printf("\nsorry, user input was not valid.");
			Sleep(INVALID_USER_INPUT_TEXT_STAY_TIME);
			clear_console_line();
			goto get_user_input_shutdown_menu;

		case UI_QUIT:
			shutdown_triggered = true;
			printf("\nshutting down debugger.");
			return;

		case UI_UNDO_PREV_INSTR:
			program_end_reached = false;
			undo_exec_iframe(curr_iframe);
			break;

		case UI_SHOW_LAST_INSTR:
			if (!curr_iframe_num)
				printf("\nalready at first instruction.");
			else
				printf(last_iframe->info_str);
			break;

		case UI_RAM_MENU:
			ram_menu();
			goto get_user_input_shutdown_menu;

		case UI_WSTACK_MENU:
			wstk_menu();
			goto get_user_input_shutdown_menu;

		case UI_CSTACK_MENU:
			cstk_menu();
			goto get_user_input_shutdown_menu;

		case UI_BP_MENU:
			printf("\nsorry, this function has not been implemented yet.");
			goto get_user_input_shutdown_menu;

		case UI_DISPLAY_IFRAMES:
			DEBUG_input_code_received_from_get_main_menu_input("(UI_DISPLAY_IFRAMES)");

			printf("\ninstruction frame vector:");
			display_iframes();
			goto get_user_input_shutdown_menu;

		// handle all input corresponding to actions that make
		// no sense after the last instr has been executed.
		case UI_MAIN_MENU:
		case UI_EXEC_UNTIL_END:
		case UI_SHOW_NEXT_INSTR:
		case UI_EXEC_CURR_INSTR:
		case UI_EXEC_UNTIL_BP:
			printf("\nsorry, user input was not valid.");
			Sleep(INVALID_USER_INPUT_TEXT_STAY_TIME);
			clear_console_line();
			goto get_user_input_shutdown_menu;

		default:
			printf("\nUSER-INPUT PROCESSING FAILURE!");
			//user_prompt("\npress any key to go to shutdown menu.");
			goto get_user_input_shutdown_menu;
	}
}

// iterates through the entire program building iframes and executing them, fully building them.
void
DebugVM::
exec_all_IFBF()
{
	pc = ram->prog_start_addr;

	for (;;)
	{
		opcode   = ram->get_ubyte(pc);
		abs_addr = ram->get_abs_addr(pc);

		build_iframe();
		exec_iframe_IFBF(curr_iframe);

		if (program_end_reached) break;
		if (shutdown_triggered)  return;
	}
}

// iframe builder function.
// executes all iframe silently, without running any instr's printfs.
void
DebugVM::
exec_iframe_IFBF(IFrame* instr)
{
	// DEBUG PRINTF BELOW
	//printf("\nexec_iframe_IFBF: executing ram[%u] %s instr #%u", instr->addr, mnemonic_strings[instr->opcode], instr->instr_num);

	pc += opsize_tbl[instr->opcode];

	switch (instr->opcode) {
		case die_op: case test_die_op:
			program_end_reached = true;
			break;

		case nop_op: case swtch_op: case nspctr_op: case nspctst_op:
			break;

		case call_op:
			// pc is already set to the addr of the next instr.
			// so we can just push it straight onto the callstack.
			callstack->push_addr(pc);
			pc = instr->uint_opr1;
			break;

		case ret_op:
			pc = callstack->pop_addr();
			break;

		case jmp_op:
			pc = instr->uint_opr1;
			break;

		case je_op: case jn_op:  case jl_op:
		case jg_op: case jls_op: case jgs_op:
			if (instr->bool_result)
				pc = instr->uint_opr1;
			break;

		case loop_op:
			loop_counter = instr->uint_opr1;
			pc = instr->uint_opr2;
			break;

		case lcont_op:
			if (loop_counter--)
				pc = instr->parent_iframe->uint_opr2;
			else
				pc = instr->parent_iframe->uint_opr3;
			break;

		case lbrk_op:
			instr->loop_count = loop_counter;
			loop_counter = 0;
			pc = instr->parent_iframe->uint_opr3;
			break;

		case psh_op:
			workstack->push_uint(instr->uint_opr1);
			break;

		case pshfr_op:
		case pshfrr_op:
		case pshfrs_op:
			workstack->push_uint(ram->get_uint(instr->src_addr));
			break;

		case pop_op:
			popped_uints.push_back(workstack->pop_uint());
			break;

		case pop2_op:
			popped_uints.push_back(workstack->pop_uint());
			popped_uints.push_back(workstack->pop_uint());
			break;

		case popn_op:
			for (int i = 0; i < (instr->uint_opr1); i++)
				popped_uints.push_back(workstack->pop_uint());
			break;

		case poptr_op:
			popped_uints.push_back(workstack->pop_uint());
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, popped_uints.back());
			break;

		case movtr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, workstack->read_top_uint());
			break;

		case stktr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, workstack->read_uint(instr->src_addr));
			break;

		case cpyr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, ram->get_uint(instr->src_addr));
			break;

		case setr_op:
			instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
			ram->set_uint(instr->dst_addr, instr->uint_opr1);
			break;

		case inc_op:    case dec_op:   case add_op:    case sub_op:   case mul_op:
		case div_op:    case mod_op:   case and_op:    case not_op:   case xor_op:
		case or_op:     case lshft_op: case rshft_op:  case lrot_op:  case rrot_op:
			workstack->push_uint(instr->uint_result);
			break;

		case incs_op:   case decs_op:   case adds_op:   case subs_op:  case muls_op:
		case divs_op:   case mods_op:   case ands_op:   case nots_op:  case xors_op:
		case ors_op:    case lshfts_op: case rshfts_op: case lrots_op: case rrots_op:
			workstack->push_int(instr->int_result);
			break;

		default:
			throw InternalErrorException(DVM_EXEC_IFRAME_IERROR_STR);
	}

	curr_iframe->exec_count++;
	last_iframe = curr_iframe;
}

// execution-handle, controls the execution of program, basically controls the locking mechanism inbetween
// each instruction. runs the user-input functions inbetween the opcode's execution.
void
DebugVM::
exec_handle()
{
	DEBUG_currently_in_function("DebugVM::exec_handle()");

	u16     show_iframe_pos  = curr_iframe_num;
	u8      user_input       = 0;
	IFrame* ifptr            = NULL; // convinience pointer to shorten some lines.

	// check if the curr_iframe is an inserted iframe if so disable
	// ram's datatype checking while we get the opcode them reenable.
	if (curr_iframe->is_inserted)
		opcode = *((ram->base) + pc);
	else
		opcode = ram->get_ubyte(pc);

	abs_addr = ram->get_abs_addr(pc);

	// if execution isn't locked program will freely execute until it's end.
	if (exec_locked)
	{
		// if in breakpoint mode program will execute until the next
		// breakpoint. stopping *at* the breakpoint instr and
		// prompting user input, the instr is not executed.
		if (in_bp_mode)
		{
			// if pc *isnt* a breakpoint then execute the current instr.
			// otherwise enter the user-input loop.
			if (!break_vec->is_breakpoint(pc))
				goto execute_iframe;
		}

	get_user_input:
		DEBUG_currently_in_function("DebugVM::exec_handle() at get_user_input label");

		user_input = get_main_menu_input();
		Sleep(USER_INPUT_RESPONSE_TIME);

		switch (user_input)
		{
			case UI_MAIN_MENU:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_MAIN_MENU)");

				print_main_menu();
				goto get_user_input;

			case UI_HELP:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_HELP)");

				printf("\nsorry, help function has not been implemented yet.");
				goto get_user_input;

			case UI_INVALID:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_INVALID)");

				printf("\nsorry, user input was not valid.");
				Sleep(INVALID_USER_INPUT_TEXT_STAY_TIME);
				clear_console_line();
				goto get_user_input;

			case UI_QUIT:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_QUIT)");

				shutdown_triggered = true;
				printf("\nshutting down debugger.");
				break;

			case UI_EXEC_CURR_INSTR:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_EXEC_CURR_INSTR)");

			execute_iframe:
				exec_iframe(curr_iframe);
				break;

			case UI_UNDO_PREV_INSTR:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_UNDO_PREV_INSTR)");

				if (!curr_iframe_num)
					printf("\ncurrently at first instruction.");
				else
					undo_exec_iframe(curr_iframe->last_iframe);
				goto get_user_input;

			case UI_EXEC_UNTIL_BP:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_EXEC_UNTIL_BP)");

				in_bp_mode = true;
				break;

			case UI_EXEC_UNTIL_END:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_EXEC_UNTIL_END)");

				exec_locked = false;
				break;

			case UI_SHOW_IMD_NEXT_INSTR:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_SHOW_IMD_NEXT_INSTR)");

				if (show_iframe_pos == (iframe_vec->vec->size() - 1))
					printf("\ncurrently at last instruction, cannot show next instruction.");
				else
					printf("\n%s", curr_iframe->info_str);
				goto get_user_input;

			case UI_SHOW_IMD_LAST_INSTR:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_SHOW_IMD_LAST_INSTR)");

				if (!curr_iframe_num)
					printf("\ncurrently at first instruction, cannot show last instruction.");
				else
					printf("\n%s", last_iframe->info_str);
				goto get_user_input;

			case UI_SHOW_NEXT_INSTR:
				DEBUG_input_code_received_from_get_main_menu_input("(UI_SHOW_NEXT_INSTR)");

				if (show_iframe_pos == (iframe_vec->vec->size() - 1))
				{
					printf("\ncurrently at last instruction, cannot show next instruction.");
				}	

				else
				{
					show_iframe_pos++;
					printf("\n%s", iframe_vec->vec->at(show_iframe_pos)->info_str);
				}

				goto get_user_input;

				case UI_SHOW_LAST_INSTR:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_SHOW_LAST_INSTR)");

					if (!curr_iframe_num)
					{
						printf("\ncurrently at first instruction, cannot show last instruction.");
					}

					else
					{
						show_iframe_pos--;
						printf("\n%s", iframe_vec->vec->at(show_iframe_pos)->info_str);
					}
					goto get_user_input;

				case UI_SHOW_CURR_INSTR:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_SHOW_CURR_INSTR)");

					show_iframe_pos = curr_iframe_num;
					printf("\n%s", iframe_vec->vec->at(show_iframe_pos)->info_str);
					goto get_user_input;

				case UI_RESET_DEBUGGER:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_RESET_DEBUGGER)");

					reset_state();
					printf("\ndebug session has been reset");
					goto get_user_input;

				case UI_RAM_MENU:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_RAM_MENU)");

					ram_menu();
					goto get_user_input;

				case UI_WSTACK_MENU:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_WSTACK_MENU)");

					wstk_menu();
					goto get_user_input;

				case UI_CSTACK_MENU:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_CSTACK_MENU)");

					cstk_menu();
					goto get_user_input;

				case UI_BP_MENU:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_BP_MENU)");

					bp_menu();
					goto get_user_input;

				case UI_INSERT_INSTR:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_INSERT_INSTR)");

					build_inserted_instr();
					goto get_user_input;

				case UI_DISPLAY_IFRAMES:
					DEBUG_input_code_received_from_get_main_menu_input("(UI_DISPLAY_IFRAMES)");

					printf("\ninstruction frame vector:");
					display_iframes();
					goto get_user_input;

				default:
					printf("\nUSER-INPUT PROCESSING FAILURE!");
					goto get_user_input;
		}
	}
}

void
DebugVM::
display_iframes()
{
	u32 inserted_iframe_count = 0;
	iframe_vec->print();
	newline();

	// count inserted iframes.
	inserted_iframe_count = 0;
	for (u32 i = 0; i < iframe_vec->vec->size(); i++)
	{
		if (iframe_vec->vec->at(i)->is_inserted)
			inserted_iframe_count++;
	}

	printf("\n%zu instruction-frames.", iframe_vec->vec->size());

	if (inserted_iframe_count)
	{
		printf("\n%zu program instructions.", iframe_vec->vec->size() - inserted_iframe_count);
		printf("\n%u debugger inserted instructions.", inserted_iframe_count);
	}

	newline();
}

void
DebugVM::
reset_state()
{
	DEBUG_currently_in_function("DebugVM::reset_state()");

	program_end_reached = false;
	shutdown_triggered  = false;
	exec_locked         = exec_locked_argv;
	in_bp_mode          = in_bp_mode_argv;
	curr_iframe         = iframe_vec->vec->at(0);
	curr_iframe_num     = 0;

	callstack->reset();
	workstack->reset();
	ram->reset_all();
}

void
DebugVM::
bp_menu()
{
	DEBUG_currently_in_function("DebugVM::bp_menu()");

	u8 user_input = 0;

	print_bp_menu();

get_user_input_bp_menu:
	DEBUG_currently_in_function("DebugVM::bp_menu() at get_user_input_bp_menu label");

	user_input = get_bp_menu_input();
	Sleep(USER_INPUT_RESPONSE_TIME);

	switch (user_input)
	{
		case UI_MAIN_MENU:
			DEBUG_input_code_received_from_get_bp_menu_input("(UI_MAIN_MENU)");

			print_main_menu();
			break;

		case UI_INVALID:
			DEBUG_input_code_received_from_get_bp_menu_input("(UI_INVALID)");

			printf("\nsorry, user input was not valid.");
			Sleep(INVALID_USER_INPUT_TEXT_STAY_TIME);
			clear_console_line();
			goto get_user_input_bp_menu;

		case UI_QUIT:
			DEBUG_input_code_received_from_get_bp_menu_input("(UI_QUIT)");

			shutdown_triggered = true;
			break;

		case BP_MENU_HELP:
			DEBUG_input_code_received_from_get_bp_menu_input("(BP_MENU_HELP)");

			goto get_user_input_bp_menu;

		case BP_MENU_SET_HERE:
			DEBUG_input_code_received_from_get_bp_menu_input("(BP_MENU_SET_HERE)");

			break;

		case BP_MENU_SET_ADDR:
			DEBUG_input_code_received_from_get_bp_menu_input("(BP_MENU_SET_ADDR)");

			break;

		case BP_MENU_DISPLAY_ALL:
			DEBUG_input_code_received_from_get_bp_menu_input("(BP_MENU_DISPLAY_ALL)");

			break;

		case BP_MENU_SEARCH:
			DEBUG_input_code_received_from_get_bp_menu_input("(BP_MENU_SEARCH)");

			break;

		default:
			printf("\n bp user-input-procrssing err"); // DEBUG
			printf("\nUSER-INPUT PROCESSING FAILURE!");
			//user_prompt("\npress any key to go to main menu.");
			goto get_user_input_bp_menu;
	}
}

void
DebugVM::
ram_menu()
{
	DEBUG_currently_in_function("DebugVM::ram_menu()");

	u8   user_input = 0;
	u32  uintval, uintval2, uintval3;
	i32  intval;
	bool action_confirmed;

	get_user_input_ram_menu_SHOW_MENU:
	print_ram_menu();

	get_user_input_ram_menu:
	DEBUG_currently_in_function("DebugVM::ram_menu() at get_user_input_ram_menu label");

	user_input = get_ram_menu_input();
	Sleep(USER_INPUT_RESPONSE_TIME);

	switch (user_input)
	{
		case UI_MAIN_MENU:
			DEBUG_input_code_received_from_get_ram_menu_input("(UI_MAIN_MENU)");
			print_main_menu();
			break;

		case UI_INVALID:
			DEBUG_input_code_received_from_get_ram_menu_input("(UI_INVALID)");
			printf("\nsorry, user input was not valid.");
			Sleep(INVALID_USER_INPUT_TEXT_STAY_TIME);
			clear_console_line();
			goto get_user_input_ram_menu_SHOW_MENU;

		case UI_QUIT:
			DEBUG_input_code_received_from_get_ram_menu_input("(UI_QUIT)");
			shutdown_triggered = true;
			break;

		case RAM_MENU_HELP:
			DEBUG_input_code_received_from_get_ram_menu_input("(RAM_MENU_HELP)");
			goto get_user_input_ram_menu;

		case RAM_DISPLAY_TR_ALL:
			DEBUG_input_code_received_from_get_ram_menu_input("(WSTK_DISPLAY_TR_ALL)");

			printf("\ndisplay entire workstack type-record:");
			workstack->print_typerecord(0, workstack->top_addr);
			goto get_user_input_ram_menu_SHOW_MENU;

		case RAM_DISPLAY_TR_ADDR:
			DEBUG_input_code_received_from_get_ram_menu_input("(WSTK_DISPLAY_TR_PART)");

			printf("\ndisplay one address' workstack type-record:");
			uintval = get_u32_user_input("\nenter work-stack address(in decimal): ", "\ninvalid address, try again");
			workstack->print_typerecord(uintval, uintval + 1);
			goto get_user_input_ram_menu_SHOW_MENU;

		case RAM_DISPLAY_ALL:
			DEBUG_input_code_received_from_get_ram_menu_input("(WSTK_DISPLAY_ALL)");

			printf("\nentire work-stack:");
			workstack->print_values(0, ram->memory_size);
			goto get_user_input_ram_menu_SHOW_MENU;

		case RAM_DISPLAY_ADDR:
			DEBUG_input_code_received_from_get_ram_menu_input("(RAM_DISPLAY_ADDR)");

			printf("\ndisplay ram address:");
			uintval = get_u32_user_input("\nenter ram address(in decimal): ", "\ninvalid address, try again");
			ram->print_addr(uintval);
			goto get_user_input_ram_menu_SHOW_MENU;

		case RAM_SET_WATCH:
			DEBUG_input_code_received_from_get_ram_menu_input("(RAM_SET_WATCH)");

			printf("\n set ram watch:");
			uintval = get_u32_user_input("\nenter ram address(in decimal): ", "\ninvalid address, try again");
			ram->watch_addr(uintval);
			goto get_user_input_ram_menu_SHOW_MENU;

		case RAM_MODIFY:
			DEBUG_input_code_received_from_get_ram_menu_input("(RAM_MODIFY)");

			printf("\nmodify ram:");
			uintval = get_u32_user_input("\nenter ram address(in decimal): ", "\ninvalid address, try again");

		choose_datatype:
			printf("\n\tuint (1)\n\tint (2)");
			uintval2 = get_u32_user_input("\nchoose datatype: ", "\ninvalid datatype code, try again");

			switch (uintval2)
			{
				case UINT_TYPE:
					uintval3 = get_u32_user_input("\nenter uint value(in decimal): ", "\ninvalid value, try again");
					break;

				case INT_TYPE:
					intval = get_i32_user_input("\nenter int value(in decimal): ", "\ninvalid value, try again");
					break;

				default:
					printf("\ninvalid datatype code, try again");
					goto choose_datatype;
			}

			action_confirmed = get_user_confirmation("\nmodifying ram memory");

			if (!action_confirmed)
				goto get_user_input_ram_menu_SHOW_MENU;

			switch (uintval2)
			{
				case UINT_TYPE:
					ram->set_uint(uintval, uintval3);
					printf("\nset ram address, %u to uint value, %u", uintval, uintval3);
					break;

				case INT_TYPE:
					ram->set_int(uintval, intval);
					printf("\nset ram address, %u to int value, %d", uintval, intval);
					break;
			}

			goto get_user_input_ram_menu_SHOW_MENU;

		case RAM_DISPLAY_SIZE:
			DEBUG_input_code_received_from_get_ram_menu_input("(RAM_DISPLAY_SIZE)");

			printf("\nram memory size: %u", ram->memory_size);
			goto get_user_input_ram_menu_SHOW_MENU;

		case RAM_MODIFY_SIZE:
			DEBUG_input_code_received_from_get_ram_menu_input("(RAM_MODIFY_SIZE)");

			printf("\nmodifying ram size: ");
			uintval = get_u32_user_input("\nenter ram address(in decimal): ", "\ninvalid address, try again");

			action_confirmed = get_user_confirmation("\nmodifying ram size");

			if (!action_confirmed)
				goto get_user_input_ram_menu_SHOW_MENU;

			ram->modify_size(uintval);
			printf("\nram size modified");
			goto get_user_input_ram_menu_SHOW_MENU;

		default:
			printf("\n ram user-input-procrssing err"); // DEBUG
			printf("\nUSER-INPUT PROCESSING FAILURE!");
			//user_prompt("\npress any key to go to main menu.");
			goto get_user_input_ram_menu;
	}
}

void
DebugVM::
wstk_menu()
{
	DEBUG_currently_in_function("DebugVM::wstk_menu()");

	u8   user_input = 0;
	u32  uintval, uintval2, uintval3;
	i32  intval;
	bool action_confirmed;

get_user_input_wstk_menu_SHOW_MENU:
	print_wstk_menu();

get_user_input_wstk_menu:
	DEBUG_currently_in_function("DebugVM::wstk_menu() at get_user_input_wstk_menu label");

	user_input = get_wstk_menu_input();
	Sleep(USER_INPUT_RESPONSE_TIME);

	switch (user_input)
	{
		case UI_MAIN_MENU:
			DEBUG_input_code_received_from_get_wstk_menu_input("(UI_MAIN_MENU)");

			print_main_menu();
			break;

		case UI_INVALID:
			DEBUG_input_code_received_from_get_wstk_menu_input("(UI_INVALID)");

			printf("\nsorry, user input was not valid.");
			Sleep(INVALID_USER_INPUT_TEXT_STAY_TIME);
			clear_console_line();
			goto get_user_input_wstk_menu;

		case UI_QUIT:
			DEBUG_input_code_received_from_get_wstk_menu_input("(UI_QUIT)");

			shutdown_triggered = true;
			break;

		case WSTK_MENU_HELP:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_MENU_HELP)");

			goto get_user_input_wstk_menu;

		case WSTK_DISPLAY_TR_ALL:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_DISPLAY_TR_ALL)");

			if (!workstack->obj_count)
			{
				printf("\nwork-stack is empty.");
				goto get_user_input_wstk_menu_SHOW_MENU;
			}

			printf("\ndisplay entire workstack type-record:");
			workstack->print_typerecord(0, workstack->top_addr);
			goto get_user_input_wstk_menu_SHOW_MENU;

		case WSTK_DISPLAY_TR_ADDR:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_DISPLAY_TR_PART)");

			if (!workstack->obj_count)
			{
				printf("\nwork-stack is empty.");
				goto get_user_input_wstk_menu_SHOW_MENU;
			}
			
			printf("\ndisplay one address' workstack type-record:");
			uintval = get_u32_user_input("\nenter work-stack address(in decimal): ", "\ninvalid address, try again");
			workstack->print_typerecord(uintval, uintval + 1);
			goto get_user_input_wstk_menu_SHOW_MENU;

		case WSTK_DISPLAY_ALL:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_DISPLAY_ALL)");

			if (!workstack->obj_count)
			{
				printf("\nwork-stack is empty.");
				goto get_user_input_wstk_menu_SHOW_MENU;
			}

			printf("\nentire work-stack:");
			workstack->print_values(0, workstack->top_addr);
			goto get_user_input_wstk_menu_SHOW_MENU;

		case WSTK_DISPLAY_ADDR:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_DISPLAY_ADDR)");

			if (!workstack->obj_count)
			{
				printf("\nwork-stack is empty.");
				goto get_user_input_wstk_menu_SHOW_MENU;
			}

			printf("\ndisplay work-stack address:");
			uintval = get_u32_user_input("\nenter work-stack address(in decimal): ", "\ninvalid address, try again");
			workstack->print_addr(uintval);
			goto get_user_input_wstk_menu_SHOW_MENU;

		case WSTK_SET_WATCH:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_SET_WATCH)");

			printf("\n set work-stack watch:");
			uintval = get_u32_user_input("\nenter work-stack address(in decimal): ", "\ninvalid address, try again");
			workstack->watch_addr(uintval);
			goto get_user_input_wstk_menu_SHOW_MENU;

		case WSTK_MODIFY:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_MODIFY)");

			printf("\nmodify work-stack:");
			uintval = get_u32_user_input("\nenter work-stack address(in decimal): ", "\ninvalid address, try again");
			
			choose_datatype:
			printf("\n\tuint (1)\n\tint (2)");
			uintval2 = get_u32_user_input("\nchoose datatype: ", "\ninvalid datatype code, try again");

			switch (uintval2)
			{
				case UINT_TYPE:
					uintval3 = get_u32_user_input("\nenter uint value(in decimal): ", "\ninvalid value, try again");
					break;

				case INT_TYPE:
					intval = get_i32_user_input("\nenter int value(in decimal): ", "\ninvalid value, try again");
					break;

				default:
					printf("\ninvalid datatype code, try again");
					goto choose_datatype;
			}

			action_confirmed = get_user_confirmation("\nmodifying work-stack memory");

			if (!action_confirmed)
				goto get_user_input_wstk_menu_SHOW_MENU;

			switch (uintval2)
			{
				case UINT_TYPE:
					workstack->set_uint(uintval, uintval3);
					printf("\nset work-stack address, %u to uint value, %u", uintval, uintval3);
					break;

				case INT_TYPE:
					workstack->set_int(uintval, intval);
					printf("\nset work-stack address, %u to int value, %d", uintval, intval);
					break;
			}	

			goto get_user_input_wstk_menu_SHOW_MENU;

		case WSTK_PUSH:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_PUSH)");

			printf("\npush value onto work-stack:");

			choose_datatype_wstk_push:
			printf("\n\tuint (1)\n\tint (2)");
			uintval2 = get_u32_user_input("\nchoose datatype: ", "\ninvalid datatype code, try again");

			switch (uintval2)
			{
				case UINT_TYPE:
					uintval3 = get_u32_user_input("\nenter uint value(in decimal): ", "\ninvalid value, try again");
					break;

				case INT_TYPE:
					intval = get_i32_user_input("\nenter int value(in decimal): ", "\ninvalid value, try again");
					break;

				default:
					printf("\ninvalid datatype code, try again");
					goto choose_datatype_wstk_push;
			}

			action_confirmed = get_user_confirmation("\npushing value onto work-stack");

			if (!action_confirmed)
				goto get_user_input_wstk_menu_SHOW_MENU;

			switch (uintval2)
			{
				case UINT_TYPE:
					workstack->push_uint(uintval3);
					printf("\n%u pushed onto work-stack.", uintval3);
					break;

				case INT_TYPE:
					workstack->push_int(intval);
					printf("\n%d pushed onto work-stack.", intval);
					break;
			}

			goto get_user_input_wstk_menu_SHOW_MENU;

		case WSTK_POP:
			DEBUG_input_code_received_from_get_wstk_menu_input("(WSTK_POP)");

			printf("\npop value off work-stack:");

			if (!workstack->obj_count)
			{
				printf("\nwork-stack is empty.");
				goto get_user_input_wstk_menu_SHOW_MENU;
			}

			action_confirmed = get_user_confirmation("\npopping value off top of work-stack");

			if (!action_confirmed)
				goto get_user_input_wstk_menu_SHOW_MENU;

			workstack->remove(1);
			printf("\npopped value off top of work-stack");
			goto get_user_input_wstk_menu_SHOW_MENU;

		default:
			printf("\n wstk user-input-procrssing err"); // DEBUG
			printf("\nUSER-INPUT PROCESSING FAILURE!");
			//user_prompt("\npress any key to go to main menu.");
			goto get_user_input_wstk_menu;
	}
}

void
DebugVM::
cstk_menu()
{
	DEBUG_currently_in_function("DebugVM::cstk_menu()");

	bool action_confirmed;
	u8   user_input = 0;
	u32  uintval    = 0;

get_user_input_cstk_menu_SHOW_MENU:
	print_cstk_menu();

get_user_input_cstk_menu:
	DEBUG_currently_in_function("DebugVM::cstk_menu() at get_user_input_cstk_menu label");

	user_input = get_cstk_menu_input();
	Sleep(USER_INPUT_RESPONSE_TIME);

	switch (user_input)
	{
		case UI_MAIN_MENU:
			DEBUG_input_code_received_from_get_cstk_menu_input("(UI_MAIN_MENU)");

			print_main_menu();
			break;

		case UI_INVALID:
			DEBUG_input_code_received_from_get_cstk_menu_input("(UI_INVALID)");

			printf("\nsorry, user input was not valid.");
			Sleep(INVALID_USER_INPUT_TEXT_STAY_TIME);
			clear_console_line();
			goto get_user_input_cstk_menu;

		case UI_QUIT:
			DEBUG_input_code_received_from_get_cstk_menu_input("(UI_QUIT)");

			shutdown_triggered = true;
			break;

		case CSTK_MENU_HELP:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_MENU_HELP)");

			goto get_user_input_cstk_menu_SHOW_MENU;

		case CSTK_DISPLAY_ALL:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_DISPLAY_ALL)");

			printf("\nentire call-stack:");
			callstack->print_all();
			goto get_user_input_cstk_menu_SHOW_MENU;

		case CSTK_DISPLAY_NEXT:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_DISPLAY_NEXT)");

			printf("\nnext return address in call-stack:");
			callstack->print_next();
			goto get_user_input_cstk_menu_SHOW_MENU;

		case CSTK_MODIFY_RECUR:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_MODIFY_RECUR)");

			for (;;)
			{
				printf("\nmodify call-stack recursion limit\n\tcurrent recursion limit: %u", callstack->recursion_limit);
				uintval = get_u32_user_input("\nenter new recursion limit: ", "\ninvalid recursion limit, try again");
				action_confirmed = get_user_confirmation("\nmodifying recursion limit");

				if (!action_confirmed)
					goto get_user_input_cstk_menu_SHOW_MENU;

				try {
					callstack->modify_recur_limit(uintval);
					printf("\ncall-stack recursion limit has been modified.");
					break;
				} catch (DvmError e) {
					printf("\n%u is lower than the current recursion level, %u, enter a higher recursion limit", uintval, callstack->recur_level);
					continue;
				}
			}

			goto get_user_input_cstk_menu_SHOW_MENU;

		case CSTK_PUSH_ADDR:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_PUSH_ADDR)");

			printf("\npush a return address onto call-stack");
			uintval = get_u32_user_input("\nenter address(in decimal): ", "\ninvalid address, try again");
			action_confirmed = get_user_confirmation("\npushing address onto call-stack");

			if (!action_confirmed)
				goto get_user_input_cstk_menu_SHOW_MENU;

			callstack->push_addr(uintval);
			printf("\naddress has been pushed onto call-stack");
			goto get_user_input_cstk_menu_SHOW_MENU;

		case CSTK_POP_ADDR:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_POP_ADDR)");

			printf("\npop top return address off call-stack");

			if (!(callstack->recur_level))
			{
				printf("\nrecursion level is 0, no address on call-stack currently.");
				goto get_user_input_cstk_menu_SHOW_MENU;
			}

			action_confirmed = get_user_confirmation("\npopping address off call-stack");

			if (!action_confirmed)
				goto get_user_input_cstk_menu_SHOW_MENU;

			uintval = callstack->pop_addr();
			printf("\naddress has been popped off call-stack");
			goto get_user_input_cstk_menu_SHOW_MENU;


		case CSTK_REMOVE_ADDR:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_REMOVE_ADDR)");

			printf("\nremove a return address from call-stack");

			if (!(callstack->recur_level))
			{
				printf("\nrecursion level is 0, no address on call-stack currently.");
				goto get_user_input_cstk_menu_SHOW_MENU;
			}

			uintval = get_u32_user_input("\nenter index(in decimal): ", "\ninvalid index, try again");
			action_confirmed = get_user_confirmation("\nremoving address from call-stack");

			if (!action_confirmed)
				goto get_user_input_cstk_menu_SHOW_MENU;

			callstack->remove_addr(uintval);
			printf("\naddress has been removed from call-stack");
			goto get_user_input_cstk_menu_SHOW_MENU;

		case CSTK_CLEAR_ALL:
			DEBUG_input_code_received_from_get_cstk_menu_input("(CSTK_CLEAR_ALL)");

			action_confirmed = get_user_confirmation("\nclear entire call-stack");

			if (!action_confirmed)
				goto get_user_input_cstk_menu_SHOW_MENU;

			callstack->reset();
			printf("\ncall-stack has been cleared");
			goto get_user_input_cstk_menu_SHOW_MENU;

		default:
			printf("\n cstk user-input-procrssing err"); // DEBUG
			printf("\nUSER-INPUT PROCESSING FAILURE!");
			goto get_user_input_cstk_menu;
	}
}

void
DebugVM::
build_iframe()
{
    curr_iframe = new IFrame(opcode, pc, abs_addr);

    switch (opcode) {
		case nspctr_op:   build_nspctr_op_iframe();   break;
		case call_op:     build_call_op_iframe();     break;
		case jmp_op:      build_jmp_op_iframe();      break;
		case je_op:       build_je_op_iframe();       break;
		case jn_op:       build_jn_op_iframe();       break;
		case jl_op:       build_jl_op_iframe();       break;
		case jg_op:       build_jg_op_iframe();       break;
		case jls_op:      build_jls_op_iframe();      break;
		case jgs_op:      build_jgs_op_iframe();      break;
		case loop_op:     build_loop_op_iframe();     break;
		case lcont_op:    build_lcont_op_iframe();    break;
		case psh_op:      build_psh_op_iframe();      break;
		case popn_op:     build_popn_op_iframe();     break;
		case pshfr_op:    build_pshfr_op_iframe();    break;
		case movtr_op:    build_movtr_op_iframe();    break;
		case stktr_op:    build_stktr_op_iframe();    break;
		case cpyr_op:     build_cpyr_op_iframe();     break;
		case setr_op:     build_setr_op_iframe();     break;
		case pshfrr_op:   build_pshfrr_op_iframe();   break;
		case pshfrs_op:   build_pshfrs_op_iframe();   break;
		case inc_op:      build_inc_op_iframe();      break;
		case dec_op:      build_dec_op_iframe();      break;
		case add_op:      build_add_op_iframe();      break;
		case sub_op:      build_sub_op_iframe();      break;
		case mul_op:      build_mul_op_iframe();      break;
		case div_op:      build_div_op_iframe();      break;
		case mod_op:      build_mod_op_iframe();      break;
		case incs_op:     build_incs_op_iframe();     break;
		case decs_op:     build_decs_op_iframe();     break;
		case adds_op:     build_adds_op_iframe();     break;
		case subs_op:     build_subs_op_iframe();     break;
		case muls_op:     build_muls_op_iframe();     break;
		case divs_op:     build_divs_op_iframe();     break;
		case mods_op:     build_mods_op_iframe();     break;
		case and_op:      build_and_op_iframe();      break;
		case not_op:      build_not_op_iframe();      break;
		case xor_op:      build_xor_op_iframe();      break;
		case or_op:       build_or_op_iframe();       break;
		case lshft_op:    build_lshft_op_iframe();    break;
		case rshft_op:    build_rshft_op_iframe();    break;
		case lrot_op:     build_lrot_op_iframe();     break;
		case rrot_op:     build_rrot_op_iframe();     break;
		case ands_op:     build_ands_op_iframe();     break;
		case nots_op:     build_nots_op_iframe();     break;
		case xors_op:     build_xors_op_iframe();     break;
		case ors_op:      build_ors_op_iframe();      break;
		case lshfts_op:   build_lshfts_op_iframe();   break;
		case rshfts_op:   build_rshfts_op_iframe();   break;
		case lrots_op:    build_lrots_op_iframe();    break;
		case rrots_op:    build_rrots_op_iframe();    break;

		case ret_op: case swtch_op:    case nop_op:
		case pop_op: case pop2_op:     case lbrk_op:
		case die_op: case test_die_op: case nspctst_op: break;

		default:
			throw InternalErrorException(DVM_BUILD_IFRAME_IERROR_STR);
    }

	iframe_vec->append_iframe(curr_iframe);
	curr_iframe->build_infostr();
}

void
DebugVM::
build_nspctr_op_iframe()
{
    curr_iframe->uint_opr1   = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->src_addr    = curr_iframe->uint_opr1;
    curr_iframe->uint_result = ram->get_uint(curr_iframe->src_addr);
    curr_iframe->opr_count   = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = UINT32_CODE;
}

void
DebugVM::
build_nspctrs_op_iframe()
{
    curr_iframe->uint_opr1  = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->src_addr   = curr_iframe->uint_opr1;
    curr_iframe->int_result = ram->get_int(curr_iframe->src_addr);
    curr_iframe->opr_count  = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = INT32_CODE;
}

void
DebugVM::
build_nspctst_op_iframe()
{
}

void
DebugVM::
build_nspctsts_op_iframe()
{
    curr_iframe->uint_opr1  = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->src_addr   = curr_iframe->uint_opr1;
    curr_iframe->int_result = workstack->get_int(curr_iframe->src_addr);
    curr_iframe->opr_count  = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = INT32_CODE;
}

void
DebugVM::
build_call_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_jmp_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_je_op_iframe()
{
    curr_iframe->uint_opr1       = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->bool_result = curr_iframe->uint_arg_left == curr_iframe->uint_arg_right;
    curr_iframe->opr_count   = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = BOOL_CODE;
}

void
DebugVM::
build_jn_op_iframe()
{
    curr_iframe->uint_opr1       = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->bool_result = curr_iframe->uint_arg_left != curr_iframe->uint_arg_right;
    curr_iframe->opr_count   = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = BOOL_CODE;
}

void
DebugVM::
build_jl_op_iframe()
{
    curr_iframe->uint_opr1       = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->bool_result = curr_iframe->uint_arg_left < curr_iframe->uint_arg_right;
    curr_iframe->opr_count   = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = BOOL_CODE;
}

void
DebugVM::
build_jg_op_iframe()
{
    curr_iframe->uint_opr1       = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->bool_result = curr_iframe->uint_arg_left > curr_iframe->uint_arg_right;
    curr_iframe->opr_count   = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = BOOL_CODE;
}

void
DebugVM::
build_jls_op_iframe()
{
    curr_iframe->uint_opr1       = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->bool_result = curr_iframe->int_arg_left < curr_iframe->int_arg_right;
    curr_iframe->opr_count   = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = BOOL_CODE;
}

void
DebugVM::
build_jgs_op_iframe()
{
    curr_iframe->uint_opr1       = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->bool_result = curr_iframe->int_arg_left > curr_iframe->int_arg_right;
    curr_iframe->opr_count   = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[RESULT]   = BOOL_CODE;
}

void
DebugVM::
build_loop_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->uint_opr2 = ram->get_uint(pc + OPCODE_SIZE + UINT_SIZE);
    curr_iframe->uint_opr3 = ram->get_uint(pc + OPCODE_SIZE + UINT_SIZE + UINT_SIZE);
    curr_iframe->opr_count = 3;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
    curr_iframe->opr_typerec[OPERAND3] = UINT32_CODE;
}

void
DebugVM::
build_lcont_op_iframe()
{
    curr_iframe->loop_count = loop_counter;
}

void
DebugVM::
build_psh_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_popn_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_pshfr_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->src_addr  = curr_iframe->uint_opr1;
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_poptr_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->dst_addr  = curr_iframe->uint_opr1;
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_movtr_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->dst_addr  = curr_iframe->uint_opr1;
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_stktr_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->dst_addr  = curr_iframe->uint_opr1;
    curr_iframe->uint_opr2 = ram->get_uint(pc + OPCODE_SIZE + UINT_SIZE);
    curr_iframe->src_addr  = curr_iframe->uint_opr2;
    curr_iframe->opr_count = 2;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
}

void
DebugVM::
build_cpyr_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->src_addr  = curr_iframe->uint_opr1;
    curr_iframe->uint_opr2 = ram->get_uint(pc + OPCODE_SIZE + UINT_SIZE);
    curr_iframe->dst_addr  = curr_iframe->uint_opr2;
    curr_iframe->opr_count = 2;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
}

void
DebugVM::
build_setr_op_iframe()
{
    curr_iframe->uint_opr1 = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->dst_addr  = curr_iframe->uint_opr1;
    curr_iframe->uint_opr2 = ram->get_uint(pc + OPCODE_SIZE + UINT_SIZE);
    curr_iframe->opr_count = 2;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
    curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
}

void
DebugVM::
build_pshfrr_op_iframe()
{
    curr_iframe->uint_opr1       = ram->get_uint(pc + OPCODE_SIZE);
    curr_iframe->interm_src_addr = curr_iframe->uint_opr1;
    curr_iframe->src_addr  = ram->get_uint(curr_iframe->interm_src_addr);
    curr_iframe->opr_count = 1;
    curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_pshfrs_op_iframe()
{
    curr_iframe->interm_src_addr = workstack->read_top_uint();
    curr_iframe->src_addr = ram->get_uint(curr_iframe->interm_src_addr);
}

void
DebugVM::
build_inc_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result     = curr_iframe->uint_arg_left + 1;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_dec_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result     = curr_iframe->uint_arg_left - 1;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_add_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left + curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_sub_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left - curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_mul_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left * curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_div_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;

    if ((!(curr_iframe->uint_arg_left)) || (!(curr_iframe->uint_arg_right))) {
        curr_iframe->zero_div = true;
        curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
    } else {
        curr_iframe->uint_result = curr_iframe->uint_arg_left / curr_iframe->uint_arg_right;
    }
}

void
DebugVM::
build_mod_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;

    if (!(curr_iframe->uint_arg_right)) {
        curr_iframe->zero_div = true;
        curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
    }
    else {
        curr_iframe->uint_result = curr_iframe->uint_arg_left % curr_iframe->uint_arg_right;
    }
}

void
DebugVM::
build_incs_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result      = curr_iframe->int_arg_left + 1;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_decs_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result      = curr_iframe->int_arg_left - 1;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_adds_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result = curr_iframe->int_arg_left + curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_subs_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result = curr_iframe->int_arg_left - curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_muls_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result = curr_iframe->int_arg_left * curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_divs_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;

    if ((!(curr_iframe->int_arg_left)) || (!(curr_iframe->int_arg_right))) {
        curr_iframe->zero_div = true;
        curr_iframe->opr_typerec[RESULT] = INT32_CODE;
    }
    else {
        curr_iframe->int_result = curr_iframe->int_arg_left / curr_iframe->int_arg_right;
    }
}

void
DebugVM::
build_mods_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;

    if (!(curr_iframe->int_arg_right)) {
        curr_iframe->zero_div = true;
        curr_iframe->opr_typerec[RESULT] = INT32_CODE;
    }
    else {
        curr_iframe->int_result = curr_iframe->int_arg_left % curr_iframe->int_arg_right;
    }
}


void
DebugVM::
build_and_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left & curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_not_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result     = ~(curr_iframe->uint_arg_left);
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_xor_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left ^ curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_or_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left | curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_lshft_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left << curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_rshft_op_iframe()
{
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left >> curr_iframe->uint_arg_right;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_lrot_op_iframe()
{
    u32 interm_result = 0;
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left << curr_iframe->uint_arg_right;
    interm_result = curr_iframe->uint_arg_left >> (WS_BITS - (curr_iframe->uint_arg_right));
    curr_iframe->uint_result = curr_iframe->uint_result | interm_result;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_rrot_op_iframe()
{
    u32 interm_result = 0;
    curr_iframe->uint_arg_left   = workstack->read_top_uint();
    curr_iframe->uint_arg_right  = workstack->read_sectop_uint();
    curr_iframe->result_produced = true;
    curr_iframe->uint_result = curr_iframe->uint_arg_left >> curr_iframe->uint_arg_right;
    interm_result = curr_iframe->uint_arg_left << (WS_BITS - (curr_iframe->uint_arg_right));
    curr_iframe->uint_result = curr_iframe->uint_result | interm_result;
    curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_ands_op_iframe()
{
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result = curr_iframe->int_arg_left & curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_nots_op_iframe()
{
    curr_iframe->int_arg_left        = workstack->read_top_int();
    curr_iframe->result_produced     = true;
    curr_iframe->int_result          = ~(curr_iframe->int_arg_left);
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_xors_op_iframe()
{
    curr_iframe->int_arg_left        = workstack->read_top_int();
    curr_iframe->int_arg_right       = workstack->read_sectop_int();
    curr_iframe->result_produced     = true;
    curr_iframe->int_result          = curr_iframe->int_arg_left ^ curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_ors_op_iframe()
{
    curr_iframe->int_arg_left        = workstack->read_top_int();
    curr_iframe->int_arg_right       = workstack->read_sectop_int();
    curr_iframe->result_produced     = true;
    curr_iframe->int_result          = curr_iframe->int_arg_left | curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_lshfts_op_iframe()
{
    curr_iframe->int_arg_left        = workstack->read_top_int();
    curr_iframe->int_arg_right       = workstack->read_sectop_int();
    curr_iframe->result_produced     = true;
    curr_iframe->int_result          = curr_iframe->int_arg_left << curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_rshfts_op_iframe()
{
    curr_iframe->int_arg_left        = workstack->read_top_int();
    curr_iframe->int_arg_right       = workstack->read_sectop_int();
    curr_iframe->result_produced     = true;
    curr_iframe->int_result          = curr_iframe->int_arg_left >> curr_iframe->int_arg_right;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_lrots_op_iframe()
{
    i32 interm_result = 0;
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result = curr_iframe->int_arg_left << curr_iframe->int_arg_right;
    interm_result      = curr_iframe->int_arg_left >> (WS_BITS - (curr_iframe->int_arg_right));
    curr_iframe->int_result = curr_iframe->int_result | interm_result;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_rrots_op_iframe()
{
    i32 interm_result = 0;
    curr_iframe->int_arg_left    = workstack->read_top_int();
    curr_iframe->int_arg_right   = workstack->read_sectop_int();
    curr_iframe->result_produced = true;
    curr_iframe->int_result = curr_iframe->int_arg_left >> curr_iframe->int_arg_right;
    interm_result      = curr_iframe->int_arg_left << (WS_BITS - (curr_iframe->int_arg_right));
    curr_iframe->int_result = curr_iframe->int_result | interm_result;
    curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

// --------------------------

void
DebugVM::
build_nspctr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->src_addr = curr_iframe->uint_opr1;
	curr_iframe->uint_result = ram->get_uint(curr_iframe->src_addr);
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_nspctrs_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->src_addr = curr_iframe->uint_opr1;
	curr_iframe->int_result = ram->get_int(curr_iframe->src_addr);
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_nspctst_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->src_addr = curr_iframe->uint_opr1;
	curr_iframe->uint_result = workstack->get_uint(curr_iframe->src_addr);
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_nspctsts_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->src_addr = curr_iframe->uint_opr1;
	curr_iframe->int_result = workstack->get_int(curr_iframe->src_addr);
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_call_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_jmp_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_je_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->bool_result = curr_iframe->uint_arg_left == curr_iframe->uint_arg_right;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = BOOL_CODE;
}

void
DebugVM::
build_jn_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->bool_result = curr_iframe->uint_arg_left != curr_iframe->uint_arg_right;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = BOOL_CODE;
}

void
DebugVM::
build_jl_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->bool_result = curr_iframe->uint_arg_left < curr_iframe->uint_arg_right;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = BOOL_CODE;
}

void
DebugVM::
build_jg_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->bool_result = curr_iframe->uint_arg_left > curr_iframe->uint_arg_right;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = BOOL_CODE;
}

void
DebugVM::
build_jls_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->bool_result = curr_iframe->int_arg_left < curr_iframe->int_arg_right;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = BOOL_CODE;
}

void
DebugVM::
build_jgs_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->bool_result = curr_iframe->int_arg_left > curr_iframe->int_arg_right;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[RESULT] = BOOL_CODE;
}

void
DebugVM::
build_loop_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->uint_opr2 = instr->val[2]->uintval;
	curr_iframe->uint_opr3 = instr->val[3]->uintval;
	curr_iframe->opr_count = 3;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
	curr_iframe->opr_typerec[OPERAND3] = UINT32_CODE;
}

void
DebugVM::
build_lcont_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->loop_count = loop_counter;
}

void
DebugVM::
build_psh_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_popn_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_pshfr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->src_addr = curr_iframe->uint_opr1;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_poptr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->dst_addr = curr_iframe->uint_opr1;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_movtr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->dst_addr = curr_iframe->uint_opr1;
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_stktr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->dst_addr = curr_iframe->uint_opr1;
	curr_iframe->uint_opr2 = instr->val[2]->uintval;
	curr_iframe->src_addr = curr_iframe->uint_opr2;
	curr_iframe->opr_count = 2;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
}

void
DebugVM::
build_cpyr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->src_addr = curr_iframe->uint_opr1;
	curr_iframe->uint_opr2 = instr->val[2]->uintval;
	curr_iframe->dst_addr = curr_iframe->uint_opr2;
	curr_iframe->opr_count = 2;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
}

void
DebugVM::
build_setr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->dst_addr = curr_iframe->uint_opr1;
	curr_iframe->uint_opr2 = instr->val[2]->uintval;
	curr_iframe->opr_count = 2;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
	curr_iframe->opr_typerec[OPERAND2] = UINT32_CODE;
}

void
DebugVM::
build_pshfrr_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_opr1 = instr->val[1]->uintval;
	curr_iframe->interm_src_addr = curr_iframe->uint_opr1;
	curr_iframe->src_addr = ram->get_uint(curr_iframe->interm_src_addr);
	curr_iframe->opr_count = 1;
	curr_iframe->opr_typerec[OPERAND1] = UINT32_CODE;
}

void
DebugVM::
build_pshfrs_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->interm_src_addr = workstack->read_top_uint();
	curr_iframe->src_addr = ram->get_uint(curr_iframe->interm_src_addr);
}

void
DebugVM::
build_inc_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left + 1;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_dec_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left - 1;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_add_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left + curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_sub_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left - curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_mul_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left * curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_div_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;

	if ((!(curr_iframe->uint_arg_left)) || (!(curr_iframe->uint_arg_right))) {
		curr_iframe->zero_div = true;
		curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
	}
	else {
		curr_iframe->uint_result = curr_iframe->uint_arg_left / curr_iframe->uint_arg_right;
	}
}

void
DebugVM::
build_mod_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;

	if (!(curr_iframe->uint_arg_right)) {
		curr_iframe->zero_div = true;
		curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
	}
	else {
		curr_iframe->uint_result = curr_iframe->uint_arg_left % curr_iframe->uint_arg_right;
	}
}

void
DebugVM::
build_incs_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left + 1;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_decs_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left - 1;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_adds_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left + curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_subs_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left - curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_muls_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left * curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_divs_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;

	if ((!(curr_iframe->int_arg_left)) || (!(curr_iframe->int_arg_right))) {
		curr_iframe->zero_div = true;
		curr_iframe->opr_typerec[RESULT] = INT32_CODE;
	}
	else {
		curr_iframe->int_result = curr_iframe->int_arg_left / curr_iframe->int_arg_right;
	}
}

void
DebugVM::
build_mods_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;

	if (!(curr_iframe->int_arg_right)) {
		curr_iframe->zero_div = true;
		curr_iframe->opr_typerec[RESULT] = INT32_CODE;
	}
	else {
		curr_iframe->int_result = curr_iframe->int_arg_left % curr_iframe->int_arg_right;
	}
}


void
DebugVM::
build_and_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left & curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_not_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = ~(curr_iframe->uint_arg_left);
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_xor_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left ^ curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_or_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left | curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_lshft_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left << curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_rshft_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left >> curr_iframe->uint_arg_right;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_lrot_op_iframe_INSERTED(InstrBlock* instr)
{
	u32 interm_result = 0;
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left << curr_iframe->uint_arg_right;
	interm_result = curr_iframe->uint_arg_left >> (WS_BITS - (curr_iframe->uint_arg_right));
	curr_iframe->uint_result = curr_iframe->uint_result | interm_result;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_rrot_op_iframe_INSERTED(InstrBlock* instr)
{
	u32 interm_result = 0;
	curr_iframe->uint_arg_left = workstack->read_top_uint();
	curr_iframe->uint_arg_right = workstack->read_sectop_uint();
	curr_iframe->result_produced = true;
	curr_iframe->uint_result = curr_iframe->uint_arg_left >> curr_iframe->uint_arg_right;
	interm_result = curr_iframe->uint_arg_left << (WS_BITS - (curr_iframe->uint_arg_right));
	curr_iframe->uint_result = curr_iframe->uint_result | interm_result;
	curr_iframe->opr_typerec[RESULT] = UINT32_CODE;
}

void
DebugVM::
build_ands_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left & curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_nots_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = ~(curr_iframe->int_arg_left);
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_xors_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left ^ curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_ors_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left | curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_lshfts_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left << curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_rshfts_op_iframe_INSERTED(InstrBlock* instr)
{
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left >> curr_iframe->int_arg_right;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_lrots_op_iframe_INSERTED(InstrBlock* instr)
{
	i32 interm_result = 0;
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left << curr_iframe->int_arg_right;
	interm_result = curr_iframe->int_arg_left >> (WS_BITS - (curr_iframe->int_arg_right));
	curr_iframe->int_result = curr_iframe->int_result | interm_result;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

void
DebugVM::
build_rrots_op_iframe_INSERTED(InstrBlock* instr)
{
	i32 interm_result = 0;
	curr_iframe->int_arg_left = workstack->read_top_int();
	curr_iframe->int_arg_right = workstack->read_sectop_int();
	curr_iframe->result_produced = true;
	curr_iframe->int_result = curr_iframe->int_arg_left >> curr_iframe->int_arg_right;
	interm_result = curr_iframe->int_arg_left << (WS_BITS - (curr_iframe->int_arg_right));
	curr_iframe->int_result = curr_iframe->int_result | interm_result;
	curr_iframe->opr_typerec[RESULT] = INT32_CODE;
}

DebugVM::
DebugVM(DVM_ArgTbl* argtbl_,
		Ram* ram_,
	    IFrameVector* iframe_vec_,
	    u32 callstack_recur_limit,
	    u8 exec_locked_,
	    u8 in_bp_mode_)
:
		iframe_vec(iframe_vec_),     breakpoint_hit(false),
		exec_locked(exec_locked_),   ram(ram_), opcode(0), pc(0),
		abs_addr(0),                 loop_counter(0),
	    curr_iframe(0),              print_exec_info(true), 
	    program_end_reached(false),  in_bp_mode(in_bp_mode_),
		shutdown_triggered(false),   last_iframe(nullptr),
	    in_bp_mode_argv(),           exec_locked_argv(exec_locked_),
		curr_iframe_num(0),          next_iframe(NULL),
	    argtbl(argtbl_)
{
	workstack = new WorkStack(argtbl_->ram_size, (u8*) malloc(argtbl_->ram_size));
	callstack = new CallStack(argtbl_->callstack_recur_limit);
	break_vec = new BreakVector();
}

DebugVM::
~DebugVM()
{
    delete break_vec;
    delete workstack;
    delete callstack;
}

void
DebugVM::
start_debug(const char* path)
{
	loadprog(path);
    mainloop_handle();
	ExitProcess(0);
}

void
DebugVM::
loadprog(const char* path)
{
    ram->loadin_program(path);
}

// Iframe builder thread.
DWORD WINAPI
IFrame_builder_thread(LPVOID lpParam)
{
	DVM_ArgTbl* _args = static_cast<DVM_ArgTbl*>(lpParam);
	
	u8* ifb_ram_mem = (u8*) malloc(_args->ram_size);
	// CHECK FOR MALLOC FAIL HERE.

	Ram* ram     = new Ram(_args->ram_size, ifb_ram_mem);
	ram->loadin_program(_args->input_path);

	DebugVM* dvm = new DebugVM(_args,
		                        ram,
							   _args->iframe_vec,
							   _args->callstack_recur_limit,
							   _args->exec_locked,
							   _args->in_bp_mode);

	// run dvm to execute program simply to build all iframes.
	dvm->exec_all_IFBF();

	// _args->iframe_vec is now populated with program's iframes.
	// now we can free up the Ram() & DebugVM() objects used.
	delete ram;
	delete dvm;

	return 0; // just so thing compiles CHZNGE THIS!
}

// user-controlled main thread.
DWORD WINAPI
DVM_user_thread(LPVOID lpParam)
{
	DVM_ArgTbl* _args = static_cast<DVM_ArgTbl*>(lpParam);

	Ram*     ram = new Ram(_args->ram_size, alloc_ram_memory(_args->ram_size));
	ram->loadin_program(_args->input_path);

	DebugVM* dvm = new DebugVM(_args,
		                       ram,
							   _args->iframe_vec,
							   _args->callstack_recur_limit,
							   _args->exec_locked,
							   _args->in_bp_mode);

	dvm->start_debug(_args->input_path);

	return 0;
}

// central function that is responsible for controlling everyting
// required to use the dvm. simply call this function passing a
// valid DVM_Arg_Tbl object to it.
void
main_dvm_handle(DVM_ArgTbl* _args)
{
	hide_console_cursor();

	// set up threads.
	// execution will have two threads, one thread goes ahead and executes the entire
	// program populating iframe_vec with fully built iframes. the other thread is
	// the actual user controlled debugger, this should work fine because the builder
	// thread will be very far ahead of the user thread.

	HANDLE user_thread,    ifb_thread;
	DWORD  user_thread_ID, ifb_thread_ID;

	// initialise the user thread.
	user_thread = CreateThread(
		NULL,                  // Default security attributes
		0,                     // Default stack size
		DVM_user_thread,       // Function to be executed
		_args,                 // Parameter to pass to the function
		0,                     // Default creation flags
		&user_thread_ID        // Pointer to store the thread ID
	);

	if (user_thread == NULL)
	{
		printf("Failed to create thread 1.\n");
		return;
	}

	// initialise the iframe-builder thread.
	ifb_thread = CreateThread(
		NULL,
		0,
		IFrame_builder_thread,
		_args,
		0,
		&ifb_thread_ID
	);

	if (ifb_thread == NULL)
	{
		printf("Failed to create iframe-builder thread.\n");
		CloseHandle(user_thread);  // clean up already set up user thread.
		return;
	}

	// Get the number of logical processors
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	DWORD numCores = sysInfo.dwNumberOfProcessors;

	// Set thread affinity based on the number of cores
	if (numCores >= 2)
	{
		// Set separate affinity for each thread
		SetThreadAffinityMask(user_thread, 1 << 0);  // Bind user_thread to core 0
		SetThreadAffinityMask(ifb_thread, 1 << 1);   // Bind ifb_thread to core 1
	}

	else
	{
		// Both threads can share the only available core
		SetThreadAffinityMask(user_thread, 1 << 0);  // Bind user_thread to core 0
		SetThreadAffinityMask(ifb_thread, 1 << 0);   // Bind ifb_thread to core 0
	}

	// Wait for both threads to finish
	WaitForSingleObject(user_thread, INFINITE);
	WaitForSingleObject(ifb_thread, INFINITE);

	// Close thread handles
	CloseHandle(ifb_thread);
	CloseHandle(user_thread);
}

//void
//non_threaded_dvm_TEST()
//{
//	u32 recursion_limit = RECUR_MAX;
//	u32 ram_size = 2000;
//	const char* input_path = "C:/Users/klopp/furst_built/bella.fbin";
//
//	Ram* ram = new Ram(ram_size, alloc_ram_memory(ram_size));
//	IFrameVector* ifvec = new IFrameVector();
//	DebugVM* dvm = new DebugVM(ram, ifvec, 5000, false, false);
//
//	ram->loadin_program(input_path);
//	dvm->exec_all_IFBF();
//
//	delete ram;
//	delete dvm;
//
//	ram = new Ram(ram_size, alloc_ram_memory(ram_size));
//	dvm = new DebugVM(ram, ifvec, 500, true, false);
//
//	dvm->start_debug(input_path);
//
//	newlines(2);
//}

void
threaded_dvm_TEST()
{
	u32 ram_size = 2000;
	u32 wstk_size = 1000;
	u32 cstk_recur_limit = 100;
	const char* input_path = "C:/Users/klopp/furst_built/bella.fbin";

	IFrameVector* ifvec = new IFrameVector();

	DVM_ArgTbl* arg_tbl = new DVM_ArgTbl(ifvec,
										 input_path,
									     ram_size,
										 wstk_size,
										 cstk_recur_limit,
										 true,
										 false);

	main_dvm_handle(arg_tbl);

	delete ifvec;
	delete arg_tbl;

	newlines(2);
}

DVM_ArgTbl* 
build_dvm_argtbl(int argc, char* argv[])
{
	bool option_tbl_record[DVM_OPT_TBL_SIZE] = { false };
	u8   args_processed = 0;
	u8   opt;

	u32  ram_size    = DVM_DEFAULT_RAM_SIZE;
	u32  stack_size  = DVM_DEFAULT_STACK_SIZE;
	u32  recur_limit = DVM_DEFAULT_RECUR_LIMIT;
	bool exec_locked = true;

	// check argv for a valid input-file path.
	if (argc < 2)
	{
		printf("\ndvm: no input file.");
		return 0;
	}

	const char* input_path = argv[1];

	// check input path is valid. will check if it exists when it is opened.
	if (!is_valid_filepath(argv[1], DVM_INPUT_FILE_EXTENSION))
	{
		printf("\ndvm: invalid input-file path.");
		return 0;
	}

	// check if there any options given.
	if (argc > 2)
	{
		// arg reader loop.
		for (u8 argi = 2; argi < argc; argi++)
		{
			// check what option was given.
			opt = get_dvm_arg_option(argv[argi]);
			if (!opt)
			{
				printf("\ndvm: invalid option given, %s", argv[argi]);
				return 0;
			}

			// check if option has already been given.
			if (option_tbl_record[opt]) {
				printf("\ndvm: invalid option given twice, %s", argv[argi]);
				return 0;
			}

			else
			{
				option_tbl_record[opt] = true;

				switch (opt)
				{
				case RAM_SIZE_ARG:
					// check if the size value was given after the option.
					// eg: ram_size= 5000
					if (++argi > argc)
					{
						printf("\ndvm: no ram size given.");
						return 0;
					}

					// here argv[argi] should be the value "5000".
					// attempt to convert it to an integer and assign to ram_size.
					ram_size = atoi(argv[argi]);

					// check if argv[argi] was a valid integer.
					if (!ram_size)
					{
						printf("\ndvm: invalid ram size.");
						return 0;
					}

					break;

				case STACK_SIZE_ARG:
					if (++argi > argc)
					{
						printf("\ndvm: no ram stack given.");
						return 0;
					}

					stack_size = atoi(argv[argi]);

					if (!stack_size)
					{
						printf("\ndvm: invalid stack size.");
						return 0;
					}

					break;

				case RECUR_LIMIT_ARG:
					if (++argi > argc)
					{
						printf("\ndvm: no recursion limit given.");
						return 0;
					}

					recur_limit = atoi(argv[argi]);

					if (!recur_limit)
					{
						printf("\ndvm: invalid recursion limit.");
						return 0;
					}

					break;

				case EXEC_LOCKED_ARG:
					if (++argi > argc)
					{
						printf("\ndvm: no choice given for exec_locked option.");
						return 0;
					}

					// here argv[argi] should be either "true" or "false".
					if (strcmp(argv[argi], "true") == 0)
					{
						exec_locked = true;
						break;
					}

					if (strcmp(argv[argi], "false") == 0)
					{
						exec_locked = false;
						break;
					}

					if (!recur_limit)
					{
						printf("\ndvm: no choice given for exec_locked option.");
						return 0;
					}
				}
			}
		}
	}

	return new DVM_ArgTbl(new IFrameVector(),
						  input_path,
					      ram_size,
						  stack_size,
						  recur_limit,
						  exec_locked,
						  false);
}

u8
get_dvm_arg_option(const char* string)
{
	for (u8 i = 0; i < DVM_OPT_TBL_SIZE; i++)
	{
		if (strcmp(string, dvm_opt_tbl[i]) == 0)
			return i;
	}

	return 0;
}

void
print_DVM_ArgTbl(DVM_ArgTbl* argtbl)
{
	newlines(2);
	printf("\nDVM arg-tbl:");
	printf("\ninput_path: [%s]", argtbl->input_path);
	printf("\nram_size: [%u]", argtbl->ram_size);
	printf("\nworkstack_size: [%u]", argtbl->workstack_size);
	printf("\ncallstack_recur_limit: [%u]", argtbl->callstack_recur_limit);
	printf("\nexec_locked: [%s]", bool_strings[argtbl->exec_locked]);
	printf("\nin_bp_mode: [%s]", bool_strings[argtbl->in_bp_mode]);
	newlines(2);
}

//void
//non_threaded_dvm_run(DVM_ArgTbl* _args)
//{
//	u32 recursion_limit = RECUR_MAX;
//	u32 ram_size = 2000;
//	const char* input_path = "C:/Users/klopp/furst_built/bella.fbin";
//
//	Ram* ram = new Ram(ram_size, alloc_ram_memory(ram_size));
//	IFrameVector* ifvec = new IFrameVector();
//	DebugVM* dvm = new DebugVM(ram, ifvec, 5000, false, false);
//
//	ram->loadin_program(input_path);
//	dvm->exec_all_IFBF();
//
//	delete ram;
//	delete dvm;
//
//	ram = new Ram(ram_size, alloc_ram_memory(ram_size));
//	dvm = new DebugVM(ram, ifvec, 500, true, false);
//
//	dvm->start_debug(input_path);
//
//	newlines(2);
//}

int
dvm_main(int argc, char* argv[])
{
	DVM_ArgTbl* arg_tbl = build_dvm_argtbl(argc, argv);
	main_dvm_handle(arg_tbl);
	delete arg_tbl->iframe_vec;
	return 0;
}

int
dvm_main_NOTHREAD(int argc, char* argv[])
{
	DVM_ArgTbl* arg_tbl = build_dvm_argtbl(argc, argv);
	u8* ifb_ram_mem = (u8*)malloc(arg_tbl->ram_size);
	// CHECK FOR MALLOC FAIL HERE.


	Ram* ram = new Ram(arg_tbl->ram_size, ifb_ram_mem);
	ram->loadin_program(arg_tbl->input_path);

	DebugVM* dvm = new DebugVM(arg_tbl,
		                       ram,
							   arg_tbl->iframe_vec,
							   arg_tbl->callstack_recur_limit,
							   arg_tbl->exec_locked,
							   arg_tbl->in_bp_mode);

	// run dvm to execute program simply to build all iframes.
	dvm->exec_all_IFBF();

	// _args->iframe_vec is now populated with program's iframes.
	// now we can free up the Ram() & DebugVM() objects used.
	delete ram;
	delete dvm;

	ram = new Ram(arg_tbl->ram_size, alloc_ram_memory(arg_tbl->ram_size));
	dvm = new DebugVM(arg_tbl,
		              ram,
                      arg_tbl->iframe_vec,
				      arg_tbl->callstack_recur_limit,
					  arg_tbl->exec_locked,
			          arg_tbl->in_bp_mode);

	dvm->start_debug(arg_tbl->input_path);

	newlines(2);

	return 0;
}

int
main(int argc, char* argv[])
{
	hide_console_cursor();
	return dvm_main_NOTHREAD(argc, argv);
}