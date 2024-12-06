#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#ifndef ASM_H
#define ASM_H

#include <unordered_map>
#include <chrono>


#include "symtab.h"

// .FBIN MEMORY-MAP

//   (0) : START_INSTR_ADDR        (uint32_t) 
//   (4) : MEMORY_SIZE             (uint32_t)   
//   (8) : PROG_SIZE               (uint32_t)
//  (12) : INSTR_COUNT             (uint32_t)
//  (16) : FIRST_INSTR_ADDR        (uint32_t)   
//  (20) : LAST_INSTR_ADDR         (uint32_t)   
//  (24) : FIRST_USER_BYTE_ADDR    (uint32_t) 
//  (28) : FLAGS                   (uint32_t)   
//  (32) : CREATION_DATE           (uint32_t)         

//  (36) : SYMTAB_SIZE             (uint32_t) ------ SYMBOL-TABLE_SEGMENT
//  (40) : SERIALISED SYMTAB       (Symbol() objects and their strings.)

// so in summary:
// 
// METADATA SEGMENT
// SYMTAB IF KEPT
// PROGRAM
// PROGRAM DATA


#define METADATA_SIZE                  36

#define MD_START_INSTR_ADDR_OFFSET      0
#define MD_MEMORY_SIZE_OFFSET           4
#define MD_PROG_SIZE_OFFSET             8
#define MD_INSTR_COUNT_OFFSET          12 
#define MD_FIRST_INSTR_ADDR_OFFSET     16
#define MD_LAST_INSTR_ADDR_OFFSET      20
#define MD_FIRST_USER_BYTE_ADDR_OFFSET 24
#define MD_FLAGS_OFFSET                28
#define MD_CREATION_DATE_OFFSET        32

// MD_SYMTAB_SIZE_OFFSET isn't technically part of the
// metadata but is part of the symbol-table segment.
#define SYMTAB_SIZE_OFFSET METADATA_SIZE
#define SYMTAB_DATA_OFFSET SYMTAB_SIZE_OFFSET

#define ASM_OUTPUT_COMPLETE_INFOSTR     "\n\nassembly complete.\n%zu bytes written to %s"

#define TOK_VALIDATE_ERR_BUFF_SIZE    300

#define TOK_VALIDATE_ERR_INFOSTR        "\nerror @ col %zu on line %zu : %s"
#define MACRODEF_MISSING_ID_INFOSTR     "\nidentifier required for macro definition."
#define INVALID_MACRO_VALUE_ERR_INFOSTR "\nmacro definition requires a valid value (ubyte, uint, int, real, hex, octal, char*, id)."
#define MISPLACED_COMMENT_ERR_INFOSTR   "\ncomment must come after a complete label/macro definition or instruction."
#define MISPLACED_BRKPNT_ERR_INFOSTR    "\nbreakpoint must be on it's own line."
#define MISPLACED_VALUE_TOK_ERR_INFO    "\nvalue tokens must be within an instruction or macro definition."
#define MISSING_UINT_ADDR_TOK_ERR_INFO  "\ninstruction requires address(unsigned integer) or label identifier as operand."
#define MISSING_UINT_VALUE_TOK_ERR_INFO "\ninstruction requires unsigned integer or label identifier as operand."
#define DATESTR_INFOSTR                 "%d / %d / %d"
#define ASM_COMPLETE_INFOSTR            "%zu out of %zu bytes written to %s\n\n"
#define OPR_SYM_ID_NONEXIST_ERR_INFOSTR "symbol operand non-existent, identifier: %s"
#define DUPLICATE_SYM_ID_ERR_INFOSTR    "symbol with identifier, %s, already declared."

#define DEFAULT_ASM_OUTPUT_PATH         "~/assembled_file.fbin"

#define DATESTR_MALLOC_LEN 15

// ARGV Assembler option stuff.
#define ASM_NO_INPUT_FILE_INFOSTR       "\nno input file."
#define ASM_INVALID_INPUT_PATH_INFOSTR  "\ninvalid input path."
#define ASM_INVALID_OPT_ARG_INFOSTR     "\ninvalid option argument."
#define ASM_OPT_TWICE_INFOSTR           "\noption given twice."
#define ASM_INVALID_OUTPUT_PATH_INFOSTR "\ninvalid output path."

#define ASM_MINIMUM_ARGC     2
#define ASM_ARGV_OPT_COUNT   8

#define NONE_OPT         0
#define SHOW_SOURCE_OPT  1
#define SHOW_PARSER_OPT  2
#define SHOW_SYMBOLS_OPT 3
#define KEEP_SYMBOLS_OPT 4
#define DEBUG_MODE_OPT   5
#define SHOW_DASM_OPT    6
#define SHOW_BIN_OPT     7

#define PATH_MAX 300

#define ASM_OPERATOR_MAX 200
#define ASM_VALUE_MAX    200

// input/output file extensions.
#define INPUT_FILE_EXTENSION  ".frt"
#define OUTPUT_FILE_EXTENSION ".fbin"

// Node() object type codes.
#define OPNODE  80
#define VALNODE 81

// maximum number of operators in any expression.
#define EXPR_OPERATOR_MAX 15

// defines the maximum number of operands any instruction can have.
// guaranteed no instructions have more operands than this.
#define INSTR_OPR_MAX 4

// defines the max number of references a macro symbol can have.
// sym x = 1, y = x, u = y and so on. so u would have a reference
// level of 2 because 2 symbols must be looked up to loop up
// the final value of u.
#define MACRO_REFERENCE_MAX 10

// action-codes used by EvalStack::eval_opstack(ShuntingYardStack* eval_state, u8 action);
#define EVAL_CONTAINER 50
#define EVAL_TOP_OP    51
#define EVAL_ALL       53

// short-form argv option strings.
static
const
char*
opttbl_sf_str[ASM_ARGV_OPT_COUNT] =
{
	"none",
	"-ss",
	"-sp",
	"-ssb",
	"-ks",
	"-dbg",
	"-sdsm",
	"-sb",
};

enum class 
AsmErrorCode
{
	GENERAL_ERR = 1,
	INVALID_TOKEN,
	INPUT_FOPEN,
	OUTPUT_FOPEN,
	PROG_BYTESTREAM_MALLOC,
	TOK_VALIDATE_ERR_BUFF_MALLOC,
	SYM_DESERIALISE_ID_MALLOC,
	SYM_DESERIALISE_STRVAL_MALLOC,
	DATESTR_MALLOC,
	NONEXISTENT_SYMBOL,
	SYM_ID_INUSE,
	CONSTRUCTOR_MALLOC,
	IMFORM_CONSTR_NEW_ERR,
	INSTR_BLOCK_CONSTR_NEW_ERR,
	ASMOBJ_CONSTR_NEW_ERR,
	NODE_NEW_ERR,
	BUILD_IM_NEW_ERR,
};

struct OperandResolver;

struct
Node
{
	OperandResolver* resolver;

	u8     node_type;  // VALNODE or OPNODE.
	u8     op_prec;
	u8     op_asso;    // LEFT or RIGHT.
	u8     val_type;
	u8     stack_num; // number within it's stack.
	Value  val;        // datatype is parent_tok's val_type.
	Token* parent_tok;

	Node(OperandResolver* _resolver,
		 Token*           _owner_tok,
		 u8               _node_type,
		 u8               _stack_num,
		 u8               _val_type = VOID_TYPE);

	void print();
};


struct
MetadataTable
{
	u32 start_instr_addr;
	u32 mem_size;
	u32 prog_size;
	u32 instr_count;
	u32 first_instr_addr;
	u32 last_instr_addr;
	u32 first_user_byte_addr;
	u32 flags;
	u32 creation_date;
	u32 symtab_size;

	void serialise(u8* _ram_base);
	void print();
};

// these instruction block objects make up the intermediate form
struct
InstrBlock
{
	size_t  instr_num;
	u32     ram_addr;
	u8      opcode;
	u8      val_type[INSTR_OPR_MAX];
	Value** val;
	Value** pos_offset;
	Value** neg_offset;

	InstrBlock(u8     _opcode,
		       size_t _instr_num);

	~InstrBlock();

	void print();
	void serialise(u8* buff_);

	void apply_offset(const u8 _datatype,
		              const u8 _oprnum);

	void write_operand(const u8 datatype,
		               const u8 oprnum,
		                    u8* buff_);

	InstrBlock* make_clone();
};


/*
	IMFORM STRUCT:
		intermediate-form of the furst processses.
		intermediate-form consists of a symbol-table,
		metadata-table & vector of InstrBlock objects
		which are the atomic imform representation
		program instructions.

		the imform makes it simple to work with processes
		in all forms. when .frt text is assembled it is
		tokenised then that stream converted into an
		imform. this imform object has a symbol-table,
		metadata-table and vector of instrs.

		the main utility of the imform object is for changing
		processes from one form to another. the imform of a
		process can be created from .frt text or from a bytestream.
		Likewise the imform can be converted into a bytestream,
		which could then be written to a file creating an executable
		.fbin file of the process.

	CONCEPT:
		once a furst process has been converted into an imform
		all the imform tools are available to be used on it.

		a .frt input text file is first tokenised by the parser
		which produces a token-stream form of the process.

		the assembler faciliated that initial step, the next step
		is a pass over the token-stream looking for major errors,
		calculating the process' memory size and building
		the symbol-table.

		next the assembler does the big job of converting the token-
		stream in an imform object. once the imform is created then
		the imform funcions are used to complete the remaining assembly
		steps.

	GRITTY:
		the InstrBlock() object contains all info required to fully
		represent an instruction. an imform is a vector of these objects,
		a metadata object and a symtab object.

		this means that a process in imform can be easily modified,
		translated between forms. they could even be interpreted as
		instructions and executed within a vm. they may be incorporated
		into the DVM in future.
*/

struct SymbolTable;

struct
IMForm
{
	Assembler* Asm;
	std::vector<InstrBlock*>* instr_vec;

	MetadataTable*  metadata;
	SymbolTable*    symtab;
	moduleFileData* in_fdata;

	char*  input_path;
	char*  output_path;

	u8*    prog_bytestream;
	size_t bytestream_size; // size of prog_bytestream in bytes.

	bool   symbols_kept;
	bool   ibvec_changed; // if this is true then the current prog_bytestream and stuff are old and need redoing.

	// flags telling if a section has been serialised/
	// written to prog_bytestream. once all required
	// sections have been serialised writing to file 
	// can commence.
	bool mdata_byte_serialised;
	bool symtab_byte_serialised;
	bool instrs_byte_serialised;

	// flags telling if a section has been written to
	// a target output file.
	bool imform_file_written;

	IMForm();
	~IMForm();

	void build_infdata(char* _path);

	void register_fileid(char* _id);

	void print();
	u32  get_ram_addr(u8* ram_base, u8* cptr);
	void print_metadata_bytestream(u8* bytestream);
	void print_program_bytestream(u8* bytestream);
	void print_bytestream(u8* bytestream);

	InstrBlock* new_instr(u8 _opcode);

	// these functions convert instruction-blocks, symtab and metadata objects
	// into their byte-form representations, serialising them into bytestreams
	// this bytestream is the .fbin memory map.
	void write_prog_to_bytes(u8* buffer);

	// writes instr_vec to a bytestream(buffer argument)
	// sets the instrs_byte_serialised imform-flag.
	void write_instrs_to_bytes(u8* buffer);

	// writes the symtab to the buffer to a bytestream(buffer argument)
	// sets the symtab_byte_serialised imform-flag.
	void write_symtab_to_bytes(u8* buffer);

	// writes the metadata to the buffer to a bytestream(buffer argument)
	// sets the symtab_byte_serialised imform-flag.
	void write_metadata_to_bytes(u8* buffer);

	// once all the above functions have ran and
	// all the required flags have been set then
	// this function can be ran on a passed path
	// writing the serialised bytes to the file.
	 
	void write_bytes_to_file(char* output_path_,
		                     bool  print_info);

	// these functions de-serialise programs in memory-form,
	// converting them into instruction-blocks within an IMForm() object.
	// doing the opposite of the write-to-bytes functions.
	void read_metadata_from_bytes(u8* _buf);
	void read_symtab_from_bytes(u8* _buf); // IMPLEMENT ME
	void read_program_from_bytes(u8* _buf);

	// use this function as a one stop solution for
	// reading a .fbin file and converting it into an
	// imform() object.
	void read_prog_from_file(char* input_path_);

	void interpret_flags(u8* flagbytes);

	// function for merging another imform into this one.
	void merge_imform(ImportCard* import_card);
	 
	IMForm* make_clone();
};

struct
ImportCard
{
	size_t  insertion_index; // index of parent module's instr-vector to import imform at.
	size_t  insertion_addr;
	size_t  curr_next_addr;
	size_t  curr_progsize; // prog-size of importer imform at time of import call.
	IMForm* imform;

	ImportCard(size_t  _insertion_index,
		       size_t  _insertion_addr,
		       size_t  _curr_progsize);
};

/*
	Assembler is a cooporative process performed by many objects.
		* assembler
		* parser
		* symbol-table
		* imform
	
	simply, the asm's job is to be given a path to an .frt file and
	some option specifications and convert this into en .fbin file.

	the assembler uses the parser to produce a token-stream. the asm
	will then produce a symbol-table and calculate the memory-size
	of the process(in token-stream), next the bytestream memory is
	allocated.

	next is the larger task of converting the token-stream & symtab
	into a complete imform. once this conversion is complete the
	process can now use all of the imform tools so the remaining
	parts of the assembly such as writing to a file ect are
	acheived via calling the imform functions.

	the only official forms of furst process are:

	    * .fbin text file.
		* imform object
		* binary form.
		* .fbin binary file.
		* 
	the latter 2 

	*/
struct
Assembler
{
	OperandResolver* resolver;
	IMForm*      imform;
	SymbolTable* symtab;
	Parser*      parser;
	size_t       bytestream_size; // prog size in bytes, calculated during token validation.
	size_t       prog_size;
	size_t       instr_count;
	Token*       currtok;
	size_t       toknum;
	const char*  currline;
	char*        input_path;
	char*        output_path;
	FILE*        input_file;
	FILE*        output_file;
	u8*          prog_bytestream;
	u8*          bufptr;
	Value        oprval;
	bool         option_tbl[ASM_ARGV_OPT_COUNT];
	u32          next_byte_addr;
	int          main_retval;
	char*        in_fileid; // pretty much the identifier of the file.

	std::vector<ImportCard*>* import_list;

	// build all import imform objects.
	void merge_import_symbols();
	void import_module();
	void first_stage_pass();
	void merge_import_list();
	void build_metadata_segment();
	void write_prog_bytestream();
	u32  build_flags_u32();
	void count_uint_operand();
	void build_im_form();
	void assemble_file();
	void alloc_bytestream();
	void reset_all(bool reset_symtab);
	void build_imform_metadata();

	// function used for throwing errors found within the input text.
	void throw_error_here(Token*       token,
             			  const char*  errmsg);

	Assembler(bool*  _option_tbl,
		      char*  _input_path,
		      char*  _output_path,
		      size_t _prog_size = 0,
		      size_t _next_byte_addr = 0);

	~Assembler();
};


// object that resolves an instruction operand from
// a token-stream using shunting-yard.
struct
OperandResolver // shunting yard algoritm stack for evaluating operands.
{
	// owner assembler object.
	Assembler* asmobj;

	// operator / value stacks & their top pointers.
	Node*  opstk[EXPR_OPERATOR_MAX];
	Node*  valstk[EXPR_OPERATOR_MAX];
	Node** opstk_top  = opstk;
	Node** valstk_top = valstk;

	// op/val object on stack counters.
	u8 op_count  = 0;
	u8 val_count = 0;

	// flags.
	bool inside_offset = false; // inside a memory offset eg: array[offset]
	bool inside_paren  = false; // inside a parenthesised expression or subexpression.

	OperandResolver(Assembler* asm_owner_obj);
	~OperandResolver();

	// instr block and the operand number of the required operand to resolve.
	// OperandResolver() object has access to the asm object's parser so this
	// function inspects the tokstream until the required operand is resolved
	// or an error is found. if a value is resolved it's set within the instr
	// object and asm obj's toknum is  set to the first token after the operand tokens.
	void resolve_uint_operand(InstrBlock* instr,
		                      const u8    argnum);

	// evaluates the top operator on the operator-stack.
	// all operator evaluating is done through this function.
	void eval_opstack_top();

	// evaluates top operator on opstack until the top operator
	// matches the toktype(operator) code.
	bool eval_opstack_until(u8 toktype);

	// operator & value pushing/popping from their respective stacks must
	// only be done via these functions below. this is to centralise and
	// control the node alloc and dealloc.
	void push_operator(Token* token);
	void push_value(Token* token);

	void push_value(Value* val,
		            u8     datatype);

	// these functions pop, delete(free) & discard the top node of it's stack.
	void popdel_value();
	void popdel_operator();

	// these functions pop and return a pointer to the popped node. caller is
	// now responsible the sole owner of the node object.
	Node* pop_value();
	Node* pop_operator();

	// call this function once operand has been found.
	void reset();
	void print();
};

u8    get_arg_option(const char* string);
char* decode_datestamp(u32 datestamp);
u32   get_packed_datestamp();


void print_option_tbl(int   argc,
					  char* argv[],
					  bool* option_tbl,
					  bool* option_tbl_record);

int  Assembler_main(int    argc, char* argv[]);



int main(int argc, char* argv[]);

//
// OG UTILITY FUNCTIONS.
//
void  disbin(const char* input_path);
void  printbin(const char* input_path);
void  print_prog_bytestream(u8*    bytestream,
	                       size_t size);



#endif // ASM.H