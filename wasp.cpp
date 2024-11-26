#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>
#include <conio.h>

#include "infostrings.h"
#include "exceptions.h"
#include "workstack.h"
#include "opcode.h"
#include "iframe.h"
#include "ram.h"
#include "vm.h"

#define OP_RESULT      0  // index of result type in operand-typerecord array.
#define INSIDE_UINT   10
#define INSIDE_INT    11
#define INSIDE_UINT16 12
#define INSIDE_SYMTAB 13
#define PUSH          50
#define POP           60


// user input codes:
#define UINPUT_QUIT            0
#define UINPUT_EXEC_NEXT_OP    1
#define UINPUT_REVERSE_OP_EXEC 2
#define UINPUT_SHOW_NEXT_OP    3
#define UNINPUT_EXEC_ALL_OP    4

#define ESC_KEY_CODE 27

class CallStack {
    uint32_t* array;

    public:
        uint32_t next_ndx;

        CallStack()
        {
            array    = new uint32_t[RECUR_MAX];
            next_ndx = 0;
        }

        ~CallStack()
        {
            delete[] array;
        }

        void
        push_addr(uint32_t addr)
        {
            if (next_ndx != RECUR_MAX) {
                next_ndx++;
                array[next_ndx] = addr;
            }
        }

        uint32_t
        pop_addr()
        {
            if (next_ndx) {
                next_ndx--;
                return array[next_ndx + 1];
            }

            return 0;
        }
};

class BreakVector
{
    public:
        std::vector<uint32_t> vec;

        BreakVector()
        {
            // insert 0 at index 0 so index 0 cannot be used by any addresses.
            vec.push_back(0);
        }

        bool
        is_breakpoint(uint32_t addr)
        // returns index of addr if present in vector otherwise returns 0.
        {
            for (size_t i = 0; i < vec.size(); ++i) {
                if (vec[i] == addr)
                    return i;
            }

            return 0;
        }
        
        uint32_t
        new_breakpoint(uint32_t addr)
        // returns addr that was inserted into vec otherwise return 0.
        {
            // only append addr to vector if not already present.
            if (!is_breakpoint(addr)) {
                vec.push_back(addr);
                return addr;
            }

            return 0;
        }

        uint32_t
        remove_breakpoint(uint32_t addr)
        // returns addr that was removed from vec otherwise return 0.
        {
            for (size_t i = 0; i < vec.size(); ++i) {
                if (vec[i] == addr)
                    vec.erase(vec.begin() + i);
                    return i;
            }

            return 0;
        }
};

class DebugVM
{
    WorkStack*    workstack;
    CallStack*    callstack;
    BreakVector*  break_vec;
    IFrameVector* iframe_vec;
    IFrame*       iframe;        // current instruction-frame.
    Ram*          ram;           // tyson-ram.

    uint32_t      pc;            // program-counter addr value.
    uint8_t       opcode;        // current opcode.
    uint64_t      abs_addr;      // absolute-addr of pc.
    uint32_t      loop_counter;  // loop-counter.

    bool          breakpoint_hit;
    bool          exec_locked;
    bool          die_op_hit;
    bool          print_exec_info;

    // vectors used for storing values popped from workstack.
    std::vector<uint32_t> popped_uints;
    std::vector<uint32_t> popped_retaddrs;
    std::vector<int32_t>  popped_ints;
   
    // vectors used for storing values unpushed from workstack.
    std::vector<uint32_t> unpushed_uints;
    std::vector<uint32_t> unpushed_retaddrs;
    std::vector<int32_t>  unpushed_ints;

    void
    undo_exec_iframe(IFrame* instr)
    // reverses execution of the last executed instruction-frame
    // who's ptr has been passed as instr.
    {
        // set pc & last_pc to their values at the time of the
        // last instr. if iframe is the first instr of prog pc
        // & last_pc are set to the program-start-addr.
        if (iframe->instr_num)
            pc = instr->last_iframe->addr;
        else
            pc = ram->prog_start_addr;

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
                ram->set_uint(iframe->dst_addr, instr->prev_ram_uint_value);
                pc += UINT_SIZE;
                break;

            case movtr_op: case stktr_op: case cpyr_op: case setr_op:
                ram->set_uint(iframe->dst_addr, instr->prev_ram_uint_value);
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

        instr->un_exec_count++;
    }

    void
    exec_iframe(IFrame* instr)
    {
        pc++;

        switch (instr->opcode) {
            case die_op:
                die_op_hit = true;
                break;

            case nop_op: case swtch_op:
                break;

            case nspctr_op:
                printf(NSPCTR_INFOSTR, instr->src_addr, instr->uint_result);
                break;

            case nspctst_op:
                printf(NSPCTST_INFOSTR, instr->src_addr, instr->uint_result);
                break;

            case test_die_op:
                dvm_shutdown();

            case call_op:
                callstack->push_addr(pc + UINT_SIZE);
                pc = instr->uint_opr1;
                break;

            case ret_op:
                pc = callstack->pop_addr();
                break;

            case jmp_op:
                pc = instr->uint_opr1;
                break;

            case je_op: case jn_op: case jl_op: case jg_op: case jls_op: case jgs_op:
                if (instr->bool_result)
                    pc = instr->uint_opr1;
                else
                    pc = UINT_SIZE;
                break;

            case loop_op:
                loop_counter = instr->uint_opr1;
                pc += (UINT_SIZE * 3);
                break;

            case lcont_op:
                if (loop_counter) {
                    loop_counter--;
                    pc = instr->parent_iframe->uint_opr2;
                } else {
                    pc = instr->parent_iframe->uint_opr3;
                }
                break;

            case lbrk_op:
                instr->loop_count = loop_counter;
                loop_counter = 0;
                pc = instr->parent_iframe->uint_opr3;
                break;

            case psh_op:
                workstack->push_uint(instr->uint_opr1);
                pc += UINT_SIZE;
                break;

            case pshfr_op: case pshfrr_op: case pshfrs_op:
                workstack->push_uint(ram->get_uint(instr->src_addr));
                pc += UINT_SIZE;
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
                pc += UINT_SIZE;
                break;

            case movtr_op:
                instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
                ram->set_uint(instr->dst_addr, workstack->read_top_uint());
                pc += UINT_SIZE;
                break;

            case stktr_op:
                instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
                ram->set_uint(instr->dst_addr, workstack->read_uint(instr->src_addr));
                pc += UINT_SIZE;
                break;

            case cpyr_op:
                instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
                ram->set_uint(instr->dst_addr, ram->get_uint(instr->src_addr));
                pc += UINT_SIZE;
                break;

            case setr_op:
                instr->prev_ram_uint_value = ram->get_uint(instr->dst_addr);
                ram->set_uint(instr->dst_addr, instr->uint_opr1);
                pc += UINT_SIZE;
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

            default: throw InternalErrorException(DVM_EXEC_IFRAME_IERROR_STR);
        }

        iframe->exec_count++;
    }

    // main debugger execution loop.
    void
    run_main_loop()
    {
        pc = ram->prog_start_addr;

        do {
            exec_handle();
        } while (!die_op_hit);
    }

    // execution-handle, controls the execution of program, basically controls the locking mechanism inbetween
    // each instruction. runs the user-input functions inbetween the opcode's execution.
    void
    exec_handle()
    {
        opcode = ram->get_ubyte(pc);
        abs_addr = ram->get_abs_addr(pc);

        build_iframe();

        if (exec_locked) {
            switch (get_user_input()) {
                case UINPUT_QUIT:
                    dvm_shutdown();

                case UINPUT_SHOW_NEXT_OP:
                    printf(iframe->info_str);
                    return;

                case UINPUT_EXEC_NEXT_OP:
                    exec_iframe(iframe);
                    return;

                case UINPUT_REVERSE_OP_EXEC:
                    undo_exec_iframe(iframe);
                    return;

                case UNINPUT_EXEC_ALL_OP:
                    exec_locked = false;
                    return;
            }
        }

        // execution isn't locked which allows program to execute until finish.
        exec_iframe(iframe);
    }

    uint8_t // prompts user the next action, sets user-input-vars based on said input.
    get_user_input()
    {
        char uinput[UINPUT_BUFF_LEN];

        for (;;) {
            memset(uinput, '\0', sizeof(uinput));
            printf(UINPUT_PROMPT);
            fgets(uinput, sizeof(uinput), stdin);

            if ((strlen(uinput) - 1) == 1) {
                switch (*uinput) {
                case 'q':
                case 'Q':
                    return UINPUT_QUIT;

                case 'n':
                case 'N':
                    return UINPUT_EXEC_NEXT_OP;

                case 's':
                case 'S':
                    return UINPUT_SHOW_NEXT_OP;

                default:
                    // process any non alphanumeric input from user.
                    if (_kbhit()) {
                        switch (_getch()) {
                            case ESC_KEY_CODE:
                                return UINPUT_QUIT;

                            default:
                                break;
                        }
                    }

                    // no valid user input received.
                    printf(UINPUT_INVALID);
                    continue;
                }
            }
        }
    }

    void
    dvm_shutdown()
    {
        exit(0);
    }

    // checks the breakpoints-datastructure to see if the next instr aligns with a breakpoint.
    // if there is a breakpoint, breakpoint_hit flag is set to true.
    void
    check_for_breakpoint()
    {

    }

    void
    build_iframe()
    {
        iframe = new IFrame(opcode, pc, abs_addr);

        switch (opcode) {
            case die_op:      build_die_op_iframe();      break;
            case nspctr_op:   build_nspctr_op_iframe();   break;
            case nspctst_op:  build_nspctst_op_iframe();  break;
            case test_die_op: build_test_die_op_iframe(); break;
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

            case ret_op: case swtch_op: case nop_op:
            case pop_op: case pop2_op:  case lbrk_op:     break;

            default: throw InternalErrorException(DVM_BUILD_IFRAME_IERROR_STR);
           }

        iframe_vec->append_iframe(iframe);
    }

    void
    build_die_op_iframe()
    {
        die_op_hit = true;
    }
    
    void
    build_nspctr_op_iframe()
    {
        iframe->uint_opr1              = ram->get_uint(pc);
        iframe->src_addr               = iframe->uint_opr1;
        iframe->uint_result            = ram->get_uint(iframe->src_addr);
        iframe->opr_count              = 1;
        iframe->opr_typerec[1]         = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }
    
    void
    build_nspctrs_op_iframe()
    {
        iframe->uint_opr1              = ram->get_uint(pc);
        iframe->src_addr               = iframe->uint_opr1;
        iframe->int_result             = ram->get_int(iframe->src_addr);
        iframe->opr_count              = 1;
        iframe->opr_typerec[1]         = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void
    build_nspctst_op_iframe()
    {
        iframe->uint_opr1              = ram->get_uint(pc);
        iframe->src_addr               = iframe->uint_opr1;
        iframe->uint_result            = workstack->get_uint(iframe->src_addr);
        iframe->opr_count              = 1;
        iframe->opr_typerec[1]         = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }

    void
    build_nspctsts_op_iframe()
    {
        iframe->uint_opr1              = ram->get_uint(pc);
        iframe->src_addr               = iframe->uint_opr1;
        iframe->int_result             = workstack->get_int(iframe->src_addr);
        iframe->opr_count              = 1;
        iframe->opr_typerec[1]         = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void
    build_test_die_op_iframe()
    {
        die_op_hit = true;
    }
    
    void
    build_call_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }
    
    void
    build_jmp_op_iframe()
    {
        iframe->uint_opr1      = ram->get_uint(pc);
        iframe->opr_count      = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }
    
    void
    build_je_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->bool_result = iframe->uint_arg_left == iframe->uint_arg_right;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = BOOL_CODE;
    }
    
    void
    build_jn_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->bool_result = iframe->uint_arg_left != iframe->uint_arg_right;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = BOOL_CODE;
    }
    
    void
    build_jl_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->bool_result = iframe->uint_arg_left < iframe->uint_arg_right;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = BOOL_CODE;
    }

    void
    build_jg_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->bool_result = iframe->uint_arg_left > iframe->uint_arg_right;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = BOOL_CODE;
    }
    
    void
    build_jls_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->bool_result = iframe->int_arg_left < iframe->int_arg_right;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = BOOL_CODE;
    }

    void
    build_jgs_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->bool_result = iframe->int_arg_left > iframe->int_arg_right;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[OP_RESULT] = BOOL_CODE;
    }

    void
    build_loop_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->uint_opr2 = ram->get_uint(pc + UINT_SIZE);
        iframe->uint_opr3 = ram->get_uint(pc + UINT_SIZE + UINT_SIZE);
        iframe->opr_count = 3;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[2] = UINT32_CODE;
        iframe->opr_typerec[3] = UINT32_CODE;
    }
    
    void
    build_lcont_op_iframe()
    {
        iframe->loop_count = loop_counter;
    }

    void
    build_psh_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }

    void
    build_popn_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }

    void
    build_pshfr_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->src_addr = iframe->uint_opr1;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }
    
    void
    build_poptr_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->dst_addr = iframe->uint_opr1;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }
    
    void
    build_movtr_op_iframe()
   {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->dst_addr = iframe->uint_opr1;
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }
    
    void
    build_stktr_op_iframe()
   {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->dst_addr = iframe->uint_opr1;
        iframe->uint_opr2 = ram->get_uint(pc + UINT_SIZE);
        iframe->src_addr = iframe->uint_opr2;
        iframe->opr_count = 2;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[2] = UINT32_CODE;
    }
    
    void
    build_cpyr_op_iframe()
   {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->src_addr = iframe->uint_opr1;
        iframe->uint_opr2 = ram->get_uint(pc + UINT_SIZE);
        iframe->dst_addr = iframe->uint_opr2;
        iframe->opr_count = 2;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[2] = UINT32_CODE;
    }
    
    void
    build_setr_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->dst_addr = iframe->uint_opr1;
        iframe->uint_opr2 = ram->get_uint(pc + UINT_SIZE);
        iframe->opr_count = 2;
        iframe->opr_typerec[1] = UINT32_CODE;
        iframe->opr_typerec[2] = UINT32_CODE;
    }
    
    void
    build_pshfrr_op_iframe()
    {
        iframe->uint_opr1 = ram->get_uint(pc);
        iframe->interm_src_addr = iframe->uint_opr1;
        iframe->src_addr = ram->get_uint(iframe->interm_src_addr);
        iframe->opr_count = 1;
        iframe->opr_typerec[1] = UINT32_CODE;
    }
    
    void
    build_pshfrs_op_iframe()
    {
        iframe->interm_src_addr = workstack->read_top_uint();
        iframe->src_addr = ram->get_uint(iframe->interm_src_addr);
    }
    
    void
    build_inc_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left + 1;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }
    
    void
    build_dec_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left - 1;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        
    
    void
    build_add_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left + iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        
    
    void
    build_sub_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left - iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        

    void
    build_mul_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left * iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        
    
    void
    build_div_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;

        if ((!(iframe->uint_arg_left)) || (!(iframe->uint_arg_right))) {
            iframe->zero_div = true;
            iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
        } else {
            iframe->uint_result = iframe->uint_arg_left / iframe->uint_arg_right;
        }
    }        
    
    void
    build_mod_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;

        if (!(iframe->uint_arg_right)) {
            iframe->zero_div = true;
            iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
        } else {
            iframe->uint_result = iframe->uint_arg_left % iframe->uint_arg_right;
        }
    }
    
    void
    build_incs_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left + 1;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void
    build_decs_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left - 1;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void
    build_adds_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left + iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void
    build_subs_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left - iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void
    build_muls_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left * iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void
    build_divs_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;

        if ((!(iframe->int_arg_left)) || (!(iframe->int_arg_right))) {
            iframe->zero_div = true;
            iframe->opr_typerec[OP_RESULT] = INT32_CODE;
        } else {
            iframe->int_result = iframe->int_arg_left / iframe->int_arg_right;
        }
    }

    void
    build_mods_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;

        if (!(iframe->int_arg_right)) {
            iframe->zero_div = true;
            iframe->opr_typerec[OP_RESULT] = INT32_CODE;
        } else {
            iframe->int_result = iframe->int_arg_left % iframe->int_arg_right;
        }
    }

    
    void build_and_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left & iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }      
      
    void build_not_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->result_produced = true;
        iframe->uint_result = ~(iframe->uint_arg_left);
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }
    
    void build_xor_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left ^ iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        
    
    void build_or_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left | iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        
    
    void build_lshft_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left << iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        
    
    void build_rshft_op_iframe()
    {
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left >> iframe->uint_arg_right;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }   
    
    void build_lrot_op_iframe()
    {
        uint32_t interm_result = 0;
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left << iframe->uint_arg_right;
        interm_result = iframe->uint_arg_left >> (WS_BITS - (iframe->uint_arg_right));
        iframe->uint_result = iframe->uint_result | interm_result;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }        
    
    void build_rrot_op_iframe()
    {
        uint32_t interm_result = 0;
        iframe->uint_arg_left = workstack->read_top_uint();
        iframe->uint_arg_right = workstack->read_sectop_uint();
        iframe->result_produced = true;
        iframe->uint_result = iframe->uint_arg_left >> iframe->uint_arg_right;
        interm_result = iframe->uint_arg_left << (WS_BITS - (iframe->uint_arg_right));
        iframe->uint_result = iframe->uint_result | interm_result;
        iframe->opr_typerec[OP_RESULT] = UINT32_CODE;
    }     
    
    void build_ands_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left & iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void build_nots_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->result_produced = true;
        iframe->int_result = ~(iframe->int_arg_left);
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void build_xors_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left ^ iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void build_ors_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left | iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void build_lshfts_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left << iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void build_rshfts_op_iframe()
    {
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left >> iframe->int_arg_right;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void build_lrots_op_iframe()
    {
        int32_t interm_result = 0;
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left << iframe->int_arg_right;
        interm_result = iframe->int_arg_left >> (WS_BITS - (iframe->int_arg_right));
        iframe->int_result = iframe->int_result | interm_result;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }

    void build_rrots_op_iframe()
    {
        int32_t interm_result = 0;
        iframe->int_arg_left = workstack->read_top_int();
        iframe->int_arg_right = workstack->read_sectop_int();
        iframe->result_produced = true;
        iframe->int_result = iframe->int_arg_left >> iframe->int_arg_right;
        interm_result = iframe->int_arg_left << (WS_BITS - (iframe->int_arg_right));
        iframe->int_result = iframe->int_result | interm_result;
        iframe->opr_typerec[OP_RESULT] = INT32_CODE;
    }
    
    public:
        DebugVM(Ram* ram_) :
            workstack(new WorkStack(STACK_SIZE, malloc(STACK_SIZE))),
            break_vec(new BreakVector()), iframe_vec(new IFrameVector()),
            callstack(new CallStack()),   breakpoint_hit(false),
            exec_locked(true),            ram(ram_),
            opcode(0),                    pc(0),
            abs_addr(0),                  loop_counter(0),
            iframe(nullptr),              print_exec_info(true) {}

        ~DebugVM()
        {
            delete iframe_vec;
            delete break_vec;
            delete workstack;
            delete callstack;
        }

        void
        start_debug()
        {
            run_main_loop();
        }

        void
        loadprog(const char* path)
        {
            ram->loadin_program(path);
        }
};

void
do_test()
{
    printf("\nprog start");
    void* master = init_ram(RAMSIZE, TRUE);
    Ram* ram     = new Ram(RAMSIZE, master);
    DebugVM* dvm = new DebugVM(ram);
    dvm->loadprog("vmt.fbin");
    dvm->start_debug();
    delete dvm;
    delete ram;
}

void
do_test2()
{

    Ram* ram = new Ram(500, malloc(500));
    ram->validate_addr(600);
    delete ram;

}


int main() {
    do_test();
    
    return 0;
}