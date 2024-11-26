#ifndef INFOSTRINGS_H
#define INFOSTRINGS_H

#include "vm.h"

#define IFRAME_INFOSTR_MAXLEN 300
#define UINPUT_BUFF_LEN     10

#define IFRAME_BASEID_INFOSTR "\nopcode: %u (%s) :: tyson-ram addr: %u :: machine-ram addr: %u :: instr-num: %u :: execution-count: %u :: unexecution-count: %u"

#define UINT_OPR1_INFOSTR            "\noperand1: (uint32) %u\0"
#define UINT_OPR2_INFOSTR            "\noperand2: (uint32) %u\0"
#define UINT_OPR3_INFOSTR            "\noperand3: (uint32) %u\0"
#define INT_OPR1_INFOSTR             "\noperand1: (int32) %d\0"
#define INT_OPR2_INFOSTR             "\noperand2: (int32) %d\0"
#define INT_OPR3_INFOSTR             "\noperand3: (int32) %d\0"

#define ZERO_DIV_STR                 "\n    division-by-zero occurs in this instruction."
#define UINT_RESULT_INFOSTR          "\noperation result: (uint32) %u"
#define INT_RESULT_INFOSTR           "\noperation result: (int32) %d"
#define BOOL_RESULT_INFOSTR          "\noperation result: (bool) %s"

#define UINT_RAM_VALUE_INFOSTR       "\nram[%u] = %u\0"
#define INT_RAM_VALUE_INFOSTR        "\nram[%u] = %d\0"
#define BYTE_RAM_VALUE_INFOSTR       "\nram[%u] = %u\0"

#define UINT_STK_VALUE_INFOSTR       "\nstack[%u] = %u (uint)"
#define INT_STK_VALUE_INFOSTR        "\nstack[%u] = %d (int)"
#define BYTE_STK_VALUE_INFOSTR       "\nstack[%u] = %u (ubyte)"
#define UINT_STKTOP_VALUE_INFOSTR    "\nstack[top] = %u (uint)"
#define INT_STKTOP_VALUE_INFOSTR     "\nstack[top] = %d (int)"
#define BYTE_STKTOP_VALUE_INFOSTR    "\nstack[top] = %u (ubyte)"

#define NSPCTR_INFOSTR               UINT_RAM_VALUE_INFOSTR
#define NSPCTRS_INFOSTR              INT_RAM_VALUE_INFOSTR
#define NSPCTST_INFOSTR              UINT_STK_VALUE_INFOSTR
#define NSPCTRSTS_INFOSTR            INT_STK_VALUE_INFOSTR

#define DVM_BUILD_IFRAME_IERROR_STR  "DebugVM::build_iframe(), invalid opcode."
#define DVM_EXEC_IFRAME_IERROR_STR   "DebugVM::exec_iframe(), invalid opcode."
#define DVM_UNEXEC_IFRAME_IERROR_STR "DebugVM::un_exec_iframe(), invalid opcode."

#define FVM_INTERNAL_ERROR_MSG "fvm: encountered an internal error, shutting down."


#endif // INFOSTRINGS.H