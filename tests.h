#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#ifndef TESTS_H
#define TESTS_H

#include "symtab.h"
#include "asm.h"

#define print_newline() printf("\n")
#define PRINT(STRING)   printf("\n%s", STRING)

#define ALL_TESTS           0
#define PARSER_TEST         1
#define SYMTAB_TEST         2
#define DATASTRING_TEST     3
#define ASSEMBLER_TEST      4
#define CHECK_FILEPATH_TEST 5


void Parser_test();
void SymbolTable_test();
void datestring_Test();
void Assembler_test();
void is_valid_filepath_Test();
//int  main();

#endif // TESTS.H