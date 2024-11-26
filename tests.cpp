// SEE BOTTOM OF FILE FOR INSTRUCTIONS TO RUNNING TESTS!

#include "tests.h"

#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

void
Parser_test()
{
	PRINT("PARSER TEST:");
    Parser* parser = new Parser();
    parser->parse_file("C:\\Users\\klopp\\furstd\\ptest.frt");
	parser->tokstream->print_tokvec();
    printf("\n\n %zu tokens created.\n\n", parser->tokstream->vec->size());
}

void SymbolTable_test()
{ // creates 5 symbols, adds them to a symbol table, prints it, serialises said table deletes it.
  // then recreates the symbol table from the serialised version, then prints it.
	PRINT("SYMBOL-TABLE TEST:");
	u8* x;
	Symbol* sym1;
	Symbol* sym2;
	Symbol* sym3;
	Symbol* sym4;
	Symbol* sym5;
	size_t tabsz = 0;
	char* id1 = (char*)malloc(5);
	char* id2 = (char*)malloc(7);
	char* id3 = (char*)malloc(8);
	char* id4 = (char*)malloc(7);
	char* id5 = (char*)malloc(8);
	strcpy(id1, "abcd");
	strcpy(id2, "abc456");
	strcpy(id3, "abc4568");
	strcpy(id4, "foufrf");
	strcpy(id5, "fivefrf");
	char* s1 = (char*)malloc(5);
	char* s2 = (char*)malloc(7);
	char* s3 = (char*)malloc(7);
	char* s4 = (char*)malloc(7);
	char* s5 = (char*)malloc(7);
	strcpy(s1, "aaaa");
	strcpy(s2, "aaaaaa");
	strcpy(s3, "aavdaa");
	strcpy(s4, "aavdxx");
	strcpy(s5, "aavdyy");
	SymbolTable* symtab = new SymbolTable(true);
	sym1 = symtab->create_new_symbol(MACRO_SYMBOL, id1);
	sym1->val_type = STRING_TYPE;
	sym1->val.strval = s1;
	sym2 = symtab->create_new_symbol(MACRO_SYMBOL, id2);
	sym2->val_type = STRING_TYPE;
	sym2->val.strval = s2;
	sym3 = symtab->create_new_symbol(MACRO_SYMBOL, id3);
	sym3->val_type = STRING_TYPE;
	sym3->val.strval = s3;

	sym4 = symtab->create_new_symbol(MACRO_SYMBOL, id4);
	sym4->val_type = STRING_TYPE;
	sym4->val.strval = s4;

	sym4 = symtab->create_new_symbol(MACRO_SYMBOL, id5);
	sym4->val_type = STRING_TYPE;
	sym4->val.strval = s5;

	tabsz = symtab->serial_size();
	x = (u8*)malloc(tabsz);
	print_newline();
	symtab->print();
	symtab->serialise(x);
	delete symtab;
	SymbolTable* symtab2 = new SymbolTable(x, true);
	print_newline();
	symtab2->print();
	free(x);
}

void
datestring_Test()
{
	PRINT("DATESTRING TEST:");

	u16 packed_date = get_packed_datestamp();
	char* decoded_datestr = decode_datestamp(packed_date);
	auto  now = std::chrono::system_clock::now();
	auto  now_time = std::chrono::system_clock::to_time_t(now);
	auto* local_time = std::localtime(&now_time);

	int day = local_time->tm_mday;
	int month = local_time->tm_mon + 1;
	int year = local_time->tm_year + 1900;
	year = year % 100;

	printf("\ncontrol date vars:\nday: %d, month: %d, year: %d", day, month, year);
	printf("\npacked value: %u", packed_date);
	printf("\ndecoded datestr: [%s]", decoded_datestr);
}

void
Assembler_test()
{
	PRINT("ASSEMBLER TEST:");
	const char* _in = "C:\\Users\\klopp\\furstd\\ptest.frt";
	const char* _out = "C:\\Users\\klopp\\furstd\\test.fbin";
	char in[100];
	strcpy(in, _in);
	char out[100];
	strcpy(out, _out);
	bool opttbl[ASM_ARGV_OPT_COUNT] = {false};
	opttbl[KEEP_SYMBOLS_OPT] = true;
	Assembler* Asm = new Assembler(opttbl, in, out);
	Asm->assemble_file();
	disbin(out);
}

void
is_valid_filepath_Test()
{
#if defined(_WIN32) || defined(_WIN64)
	printf("\n testing on windows.");
#elif defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
	printf("\n testing on linux or mac");
#endif

	int result = 0;

	const char* valid_paths[] = {
		// Absolute paths on Windows
		"C:\\Program Files\\MyApp\\file.exe",
		"C:\\Users\\Public\\Documents\\..\\MyDocs\\report.docx",
		"C:\\MyFolder\\.hiddenfile",
		"D:\\folder with spaces\\file with spaces.txt",

		// Relative paths
		"..\\folder\\..\\anotherfolder\\file.dll",
		"relative\\path\\..\\to\\something\\..\\final.txt",

		// Edge cases with root and empty directories
		"\\file_at_root_level.log",
		"C:\\nested\\..\\rooted\\file.bat", // Contains '..' that goes to root level

		// Unix-style paths, sometimes seen in Windows compatibility layers or mixed environments
		"/usr/local/bin/script.sh",
		"/etc/config/../../file.conf",
		"/a/really/really/really/long/path/to/a/deeply/nested/file.java",

		// Invalid/edge case paths
		"relative\\path\\to\\nonexistent\\..\\file.txt",
		"..\\..\\..\\out_of_bounds_path\\file.ext",
		"C:\\reallylongfoldername_that_might_be_invalid\\subfolder\\file.txt",
		"/folder/with spaces/in/unix/path.png",
		"C:\\aux\\file.txt",                 // Reserved Windows device name 'aux'
		"C:\\com1\\file.txt",                // Reserved device name 'com1'

		// Excessively long paths
		"subfolder\\another_subfolder\\even_deeper_subfolder\\yet_another_level\\file.txt",
		"..\\..\\folder\\..\\..\\..\\file.txt",  // Relative traversal out of bounds

		// Empty and invalid drive names
		"Z:\\nonexistent_drive\\file.txt",    // Non-existent drive letter
		"C:\\folder\\..\\..\\..\\..\\file.txt",  // Excessive traversal that likely moves out of bounds
		NULL                          // End of list
	};

	const char* invalid_paths[] = {
		// Paths with disallowed characters on Windows
		"C:\\this_is_a_really_really_really_long_directory_name_that_exceeds_the_typical_maximum_path_length_limit\\", // no file.
		"C:\\folder|name\\file.txt",         // '|' character
		"C:\\folder<name>\\file.txt",        // '<' and '>' characters
		"C:\\folder\"name\\file.txt",        // '\"' character
		"C:\\folder:name\\file.txt",         // ':' character in a folder name
		"C:\\folder*name\\file.txt",         // '*' character
		"C:\\path\\with\\special!@#$%^&()_+=-chars\\file.ext",
		".\\relative\\path\\to\\file.exe",
		".\\just_a_relative_path",

		// Paths with invalid structure
		"C:folder_without_backslashes\\file.txt", // Missing backslash after drive
		"C:\\folder\\file?.txt",             // '?' character in file name
		"C:\\folder\\file.txt|other.txt",    // Pipe in the path
		"C::\\double_colon\\file.txt",       // Double colon after drive letter

		// Unix-style invalid paths
		"/invalid_path_with_<>_chars/file.txt",
		"/rooted/|file.png",                 // Unix paths with disallowed characters
		"/folder/with/question?mark/file.jpg",  // '?' in Unix path

		// Paths with improper use of traversal
		"D:file_without_backslash",           // Missing backslash after drive letter
		"C:\\",                     // Just a drive root
		"C:\\path_with_trailing_slash\\",
		NULL                                  // End of list
	};
	int u = 0;

	printf("\n these should all be valid (output 1)");
	for (int i = 0;; i++) {
		if (!valid_paths[i]) break;
		result = is_valid_filepath(valid_paths[i], "");
		printf("\n check_filepath(valid_paths[%d], "") = %d", i, result);
	}
	printf("\n\n");

	result = 0;
	printf("\n these should all be invalid (output 0)");
	for (int i = 0;; i++) {
		if (!invalid_paths[i]) break;
		result = is_valid_filepath(invalid_paths[i], "");
		printf("\n check_filepath(invalid_paths[%d], "") = %d", i, result);
	}
	printf("\ncheck_filepath() test complete.");
}

void
RUN_ALL_TESTS()
{
	PRINT("::: RUNNING ALL TESTS FROM tests.cpp!! ->>");
	print_newline();
	print_newline();
	Assembler_test();
	print_newline();
	Parser_test();
	print_newline();
	SymbolTable_test();
	print_newline();
	datestring_Test();
	print_newline();
	is_valid_filepath_Test();
	print_newline();
	print_newline();
}

/*
	TEST CHOICES:

	 * ALL_TESTS
	 * PARSER_TEST
	 * SYMTAB_TEST
	 * DATASTRING_TEST
	 * ASSEMBLER_TEST
	 * CHECK_FILEPATH_TEST

	TO RUN TESTS:
		ensure main() isn't commented out, set TEST_TO_RUN macro to one
		options presented above, then build and run this file.

*/

#define TEST_TO_RUN ASSEMBLER_TEST

//int main()
//{
//	PRINT("FURST HaLF-ASS TEST SUITE:::\n");
//	int test_choice = TEST_TO_RUN;
//
//	switch (test_choice) {
//		case ALL_TESTS:
//			RUN_ALL_TESTS();
//			return 0;
//
//		case PARSER_TEST:
//			Parser_test();
//			return 0;
//
//		case SYMTAB_TEST:
//			SymbolTable_test();
//			return 0;
//
//		case DATASTRING_TEST:
//			datestring_Test();
//			return 0;
//
//		case ASSEMBLER_TEST:
//			Assembler_test();
//			return 0;
// 
//		case CHECK_FILEPATH_TEST:
//			is_valid_filepath_Test();
//			return 0;
//	}
//}