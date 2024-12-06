#ifndef PARSER_H
#define PARSER_H

#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#include "vm.h"
#include "opcode.h"

//
// Assembly Error Object:
// NOTE: all memory errors are handled by objects individual memory error handlers.
// 
// Whether an error happens within the Parser() object or 
// the Assembler() object, or any other stage object in future,
// all errors are simply an instance of this asmErrorObject class.
// this object holds all required information to notify the
// user of the error and for the assembler itself to die quietly.
//
// Any class itself who uses this error system must have it's own
// function similar to this:
// void
// throw_error_here(Token* token, char* errmsg)
// {
// 	     throw asmErrorObject(token, LOOKUP_LINE(token->line), errmsg);
// }
// 
// this function will create a uniform way for all objects using the
// error system to trigger an error, simply pass the token and a message
// and the caller function will do the actual error throwing.
//
// once the error becomes an exception, when said exception is caught
// it's message will be printed to the console and program terminated.
// resulting in a nice useful error message.
struct
inputTextError : public std::exception
{
	char* errmsg; // msg to print to the user.
	char* line;   // pointer to line error is on.
	u32   linenum;
	u32   posnum;

	inputTextError(u32    _linenum,
				   u32    _posnum,
				   char*  _line,
				   const char*  _extra_msg);

	~inputTextError();

	void build_errmsg_str(const char* _extra_msg);
	void print_errmsg();
	void runtime_end_err_handler();
};

struct
SystematicError : public std::exception
{
	char* errmsg; // msg to print to the user.

	SystematicError(char* _extra_msg,
		            bool  free_extra_msg);

	~SystematicError();

	void build_errmsg_str(char* _extra_msg);
	void print_errmsg();
	void runtime_end_err_handler();
};

enum class
ParserError
{
    TOKBUF_MALLOC_ERR = 1,
    TOK_LINESTR_MALLOC_ERR,
    TOKBUF_MALLOC_ERR3,
    LINEBUFFER_MALLOC_ERR,
    LINEBUFFER_REALLOC_ERR,
    INVALID_TOK_MALLOC_ERR,
    TOK_INFOSTR_MALLOC_ERR,
    TOK_INFOSTR_REALLOC_ERR,
    LINEBUF_FGETC_ERR,
    INVALID_TOK_ERR,
    INPUT_PATH_MALLOC_ERR,
    INPUT_FILE_FOPEN_ERR,
    TOKVEC_NEW_ERR,
    TOKVEC_CREATE_TOK_ERR,
    MAIN_LABEL_MISSING_ERR,
    PARSER_ERR,
	ASM_ERR_BUF_ALLOC_ERR
};

// error print-strings.
#define INVALID_TOK_ERR_PRINTSTR              "\ninvalid token error:\n%s"
#define INVALID_TOK_MALLOC_ERR_PRINTSTR       "\ninvalid_tok_infostr = malloc(), failed in Parser::handle_invalid_tok()"
#define LINEBUFFER_MALLOC_ERR_PRINTSTR        "\nbuffer = malloc(), failed in Parser::readline()"
#define LINEBUFFER_REALLOC_ERR_PRINTSTR       "\nbuffer = realloc(), failed in Parser::readline()"
#define LINEBUF_FGETC_ERR_PRINTSTR            "\ncurrline = fgetc(), failed in Parser::readline()"
#define TOK_INFOSTR_MALLOC_ERR_PRINTSTR       "\ninfostr = malloc(), failed in Token::build_infostr()"
#define TOK_INFOSTR_REALLOC_ERR_PRINTSTR      "\ninfostr = realloc(), failed in Token::build_infostr()"
#define TOK_LINESTR_MALLOC_ERR_PRINTSTR       "\ntok->linestr = malloc(), failed in Parser::tokenize_line()"
#define TOKBUF_MALLOC_ERR_PRINTSTR            "tokbuf = malloc(), failed in Parser::tokenize_line()"
#define TOKVEC_NEW_ERR_PRINTSTR               "tokvec = new TokVec(), failed in Parser::Parser()"
#define TOKVEC_CREATE_TOK_ERR_PRINTSTR        "vec.push_back(new Token()), failed in TokVec::create()"

// token info-strings.
#define INVALID_TOK_INFOSTR  "\ninvalid token @ col: %u line: %u\n"
#define TOK_INFOSTR_BASE     "\ntoknum: %d <%s> @ col: %u line: %u"
#define TOK_LINESTR_INFOSTR  "\nsource line:\n[%s]"
#define TOK_OPCODE_INFOSTR   "\nopcode: %u (%s)"
#define TOK_HEXVAL_INFOSTR   "\nhex-value: %s"
#define TOK_OCTVAL_INFOSTR   "\noct-value: %s"
#define TOK_UINTVAL_INFOSTR  "\nuint-value: %s"
#define TOK_INTVAL_INFOSTR   "\nint-value: %s"
#define TOK_REALVAL_INFOSTR  "\nreal-value: %s"
#define TOK_UREALVAL_INFOSTR "\nureal-value %s"
#define TOK_UBYTEVAL_INFOSTR "\nubyte-value: %s"
#define TOK_IDVAL_INFOSTR    "\nidentifier: [%s]"
#define TOK_LABELDEF_INFOSTR "\nlabel: [%s]"
#define TOK_STRING_INFOSTR   "\nstring: [%s]"
#define TOK_COMMENT_INFOSTR  "\ncomment: \n[%s]"

// initial size of memory block for token-infostr. after infostr
// is built realloc is called on it to trim it to size.
#define TOK_INIT_INFOSTR_LEN     500
#define INVALID_TOK_INFOSTR_LEN 1000
#define LINEBUF_INIT_SIZE        500
#define LINEBUF_GROWTH_FACTOR    500
#define TOKBUF_INIT_SIZE         LINEBUF_GROWTH_FACTOR
#define TOKBUF_GROWTH_FACTOR     LINEBUF_GROWTH_FACTOR

#define DEFAULT_TAB_WIDTH          4
#define MAX_ID_LEN                40
#define MIN_ID_LEN                 1

// numeric type prefixes.
#define SIGNED_INT_PREFIX    '$'
#define UNSIGNED_INT_PREFIX  '%'
#define ADDR_INT_PREFIX      '@'
#define REAL_PREFIX          '~'

// other prefixes or signifiers.
#define BREAKPOINT_INDICATOR   '^'
#define COMMENT_INDICATOR      '#'
#define STRING_DELIM           '"'
#define LABEL_DEF_SUFFIX       ':'
#define START_LABEL_ID         "main"
#define MACRO_DEF_KEYWORD      "let"
#define IMPORT_DEF_KEYWORD     "import"

// token type codes.
#define NO_TOK           0
#define SIGNED_TOK       1
#define UNSIGNED_TOK     2 // these top 3 tokens are only used by the parser itself. 

#define OPCODE_TOK       3
#define MACRO_DEF_TOK    4
#define LABEL_DEF_TOK    5
#define OCT_TOK          6
#define HEX_TOK          7
#define UOCT_TOK         8
#define UHEX_TOK         9
#define UINT_TOK        10
#define INT_TOK         11
#define UBYTE_TOK       12
#define REAL_TOK        13
#define UREAL_TOK       14
#define ADDR_TOK        15
#define ID_TOK          16
#define STRING_TOK      17
#define COMMENT_TOK     18
#define BRKPNT_TOK      19
#define ADDOP_TOK       43
#define SUBOP_TOK       45
#define MULOP_TOK       42
#define DIVOP_TOK       47
#define MODOP_TOK       37
#define LBRKTOP_TOK     91
#define RBRKTOP_TOK     93
#define LPRN_TOK        40
#define RPRN_TOK        41
#define EXPREND_TOK    100
#define IMPORT_DEF_TOK 101
#define PATH_TOK       102

#define OPERATOR_COUNT 10
#define ASMERR_MSG_BUFSIZE 300
#define MIN_PATH_LEN 2
struct
NodePrecedence
{
	u32 typecode;
	u32 precedence;
};

static
NodePrecedence
tok_precedence_tbl[] =
{
	{ADDOP_TOK,   20},
	{SUBOP_TOK,   20},
	{MULOP_TOK,   20},
	{DIVOP_TOK,   20},
	{MODOP_TOK,   20},
	{LBRKTOP_TOK, 20},
	{RBRKTOP_TOK, 20},
	{OCT_TOK,     15},
	{HEX_TOK,     15},
	{UOCT_TOK,    15},
	{UHEX_TOK,    15},
	{UINT_TOK,    15},
	{INT_TOK,     15},
	{UBYTE_TOK,   15},
	{REAL_TOK,    15},
	{UREAL_TOK,   15},
	{ADDR_TOK,    15},
	{ID_TOK,      15}
};

static
const char*
op_asso_str[] = {
	"",
	"<left>",
	"<right>",
	NULL
};

static
u8
asso_tbl[OPERATOR_COUNT][2] =
{
		{ADDOP_TOK, LEFT},
		{SUBOP_TOK, LEFT},
		{MULOP_TOK, LEFT},
		{DIVOP_TOK, LEFT},
		{MODOP_TOK, LEFT},
		{LBRKTOP_TOK, LEFT},
		{RBRKTOP_TOK, LEFT},
		{LPRN_TOK, LEFT},
		{RPRN_TOK, LEFT},
		{EXPREND_TOK, LEFT}
};

// token type mnemoincs
static
const char*
toktype_mnemonics[] = {
    "no token",
    "signed token",
    "unsigned token",
    "opcode token",
    "macro-def token",
    "label-def token",
    "oct value token",
    "hex value token",
    "uoct value token",
    "uhex value token",
    "uint value token",
    "int value token",
    "ubyte value token",
    "real value token",
    "ureal value token",
    "address value token",
    "identifier token",
    "string token",
    "comment token",
    "breakpoint token", // 19
	"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	"mod op token", // 37
	"", "",
	"l-paren op token", // 40
	"r-paren op token", // 41
	"mul op token", // 42
	"add op token", // 43
	"", // 44
	"sub op token", // 45
	"", // 46
	"div op token", // 47
	"", "", "", "", "", "", "", "", "", "", // 57
	"", "", "", "", "", "", "", "", "", "", // 67
	"", "", "", "", "", "", "", "", "", "", // 77
	"", "", "", "", "", "", "", "", "", "", // 87
	"", "", "", // 90 
	"l-bracket op token", // 91
	"", // 92
	"r-bracket op token", // 93
	"", "", "", "", "", "",
	"expr-end op token", // 100
	"import-def token"
};

struct
Token
{
    u8     type;
    char*  linestr; // first tok of line carries copy of source line.
    char*  infostr;
    char*  srcstr;
    size_t line_printlen;
    size_t ndx;
    size_t line;
    size_t col;
    Value  val;
	u8     val_type;

    Token(u32 _ndx,
		   u8 _type,
	   size_t _line,
	   size_t _col,
		char* _srcstr,
		char* _linestr);

    ~Token();

    char*  build_infostr();
    void print_tok();
};

struct TokenStream
{
    std::vector<Token*>* vec;

    TokenStream();
    ~TokenStream();

    void create(u8    _type,
		        u32   _line,
		        u8    _col,
		        char* _srcstr,
		        char* _linestr,
		       size_t _line_printlen);

    void freetoks();
    void print_tokvec();
};


struct
Parser
{
    bool   eof_reached;
    bool   isFirstTokOnLine;
    u8     tab_width_offset;
    char*  invalid_tok_infostr;
    size_t linebuf_size;
    size_t linelen;
    size_t linenum;
    char*  linebuf;
    FILE*  input_file;
    char*  input_path;
    size_t line_printlen;

	void readline();
	void tokenize_line();

    TokenStream* tokstream;
    size_t       tokcount;
    u8           last_opcode;

	std::vector<char*>* linevec;


	Parser();
	~Parser();

	void parse_file(const char* path);
	void print_source();

	void throw_error_here(Token* token,
		                  char*  errmsg);

};

u8 count_linenum_digits(size_t num);
u8 get_tok_precedence(Token* tok);
u8 loopup_tok_association(u8 tokcode);

#endif // PARSER.H