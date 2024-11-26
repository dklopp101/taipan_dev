#ifndef SYMTAB_H
#define SYMTAB_H

#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#include "parser.h"
#include "asm.h"

#define SYMBOL_BASE_INFOSTR         "\nnumber: %zu : type: %s : identifier: [%s]"
#define SYMTAB_PRINT_BASE_INFOSTR   "\nsymbol table: \n\t%zu symbols. %zu labels, %zu macros."
#define SYMTAB_EMPTY_INFOSTR        "\nsymbol table: \n\t0 symbols."
#define SYMTAB_LABELS_INTRO_INFOSTR "\n label symbols:"
#define SYMTAB_MACROS_INTRO_INFOSTR "\n\n macro symbols:"


// symbol type codes.
#define NO_SYMBOL    0
#define LABEL_SYMBOL 1
#define MACRO_SYMBOL 2
#define CORE_SYMBOL  3


static
const char*
symbolTypeStr[] =
{
	"no symbol",
	"label symbol",
	"macro symbol"
};

// offsets for Symbol() variables relative
// to the address of the Symbol() obj.
#define SYMBOL_IDSTR_PTR_OFFSET  0
#define SYMBOL_VALUE_OFFSET      8
#define SYMBOL_NDX_OFFSET       16
#define SYMBOL_TYPE_OFFSET      24
#define SYMBOL_DATATYPE_OFFSET  25
#define SYMBOL_IDSTR_OFFSET     32

struct Symbol
{
	char*  id;
	Value  val;
	size_t ndx;
	u8     type;
	u8     val_type; 

	Symbol(size_t _ndx, u8 _type, char* _id);
	Symbol(u8* bytestream);
	~Symbol();

	void   print();
	size_t serialise(u8* buf);
	void   deserialise(u8* bytestream);
	size_t serial_size();
};

struct IMForm;

struct SymbolTable
{
	std::vector<Symbol*>* vec;
	bool   keep_symbols;
	u32    start_addr;
	size_t bytestream_size;

	SymbolTable(bool _keep_symbols);
	SymbolTable(u8* buf, bool _keep_symbols);
	~SymbolTable();

	void    resolve_all_macros();
	void    calc_addr_offsets();
	size_t  create_new_symbol(u8* buf);

	Symbol* create_new_symbol(u8    _type,
		                      char* _id);

	void    serialise(u8* buf);
	void    deserialise(u8* buf);

	void    create_label_symbol(char* _id,
		                        u32   _addr);

	void    create_macro_symbol(char* _id);
	size_t  serial_size();

	void    finalise(u8*     prog_bytestream,
		             IMForm* imform);

	void    print(bool print_builtins);
	Value   lookup_symbol_value(const char* _id);
	Symbol* lookup_symbol(const char* _id);
	bool    id_inuse(char* _id);
	void    reset_all();
	void    create_builtin_symbols();
	void    resolve_builtin_symbols(IMForm* imform);
};

#endif // SYMTAB.H