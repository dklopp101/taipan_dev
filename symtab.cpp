#pragma warning(disable : 6386) // buffer-overruns.
#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#include "symtab.h"


// this function modifies a symbols
void
Symbol::
modify_id(char* newid)
{
	size_t newid_len = strlen(newid);

	//if (!newid_len)
		// THROW ERROR

	char* newid_buf = (char*)malloc(newid_len);
	strcpy(newid_buf, newid);

	// now we gotta deal with current id.
	free(id);
	id = newid_buf;
}

Symbol::
Symbol(size_t _ndx,
	   u8     _type,
	   char*  _id)
:
	ndx(_ndx),
	type(_type),
	id(_id),
	val_type(VOID_TYPE)
{
	val.uintval = 0; // init the val union.
}

Symbol::
Symbol(u8* bytestream)
{
	deserialise(bytestream);
}

Symbol::
~Symbol()
{
	free(id);
	if (val_type == STRING_TYPE)
		free(val.strval);
}

size_t
Symbol::
serial_size()
{
	size_t sz = sizeof(Symbol);
	sz += strlen(id) + 1;

	if (val_type == STRING_TYPE)
		sz += strlen(val.strval) + 1;

	return sz;
}

void
Symbol::
print()
{
	printf(SYMBOL_BASE_INFOSTR, ndx, symbolTypeStr[type], id);

	switch (val_type) {
		case UINT_TYPE:
			printf("\nuint-value: %u", val.uintval);
			break;
		case ADDR_TYPE:
			printf("\naddr-value: %u", val.uintval);
			break;
		case INT_TYPE:
			printf("\nint-value: %d", val.intval);
			break;
		case UBYTE_TYPE:
			printf("\nubyte-value: %u", val.ubyteval);
			break;
		case REAL_TYPE:
			printf("\nreal-value: %f", val.realval);
			break;
		case STRING_TYPE:
			printf("\nstring-value: [%s]", val.strval);
			break;
	}
}

/* SYMBOL OBJECT SERIALISATION MMAP

  0	- char*   id;       (8 bytes)
  8	- Value   val;      (8 bytes)
 16	- size_t  ndx;      (8 bytes)
 24 - uint8_t type;     (1 byte)
 25	- uint8_t datatype; (1 byte)
 26 - PADDING BYTES     (6 bytes)
 32 - ID STRING
 32 + (strlen(id)+1) - STRING VALUE

 To unserialise the symbol construct a Symbol() object from the
 symobj proper bytes then allocate memory for the strings under
 symobj-propery set the string pointers in Symbol() obj to them.
 */
size_t 
Symbol::
serialise(u8* buf)
{
	size_t id_len      = strlen(id) + 1; // len of identifier + null-terminator.
	size_t strval_len  = 0;
	size_t symobj_size = sizeof(Symbol);
	u8*    symptr      = (u8*) this;
	u8*    bufptr      = buf;

	// do we have a string-value?
	if (val_type == STRING_TYPE)
		strval_len = strlen(val.strval) + 1; // len of macro string-value + null-terminator.

	// copy Symbol() object's variables to the buffer.
	memcpy((void*) bufptr, (void*) symptr, symobj_size);
	bufptr += symobj_size;

	// copy identifier string directly after the Symbol() obj bytes.
	strcpy((char*) bufptr, id);
	bufptr += id_len;

	// check if symbol object is a string value, if so write it to the buffer.
	if (val_type == STRING_TYPE)
	{
		strcpy((char*) bufptr, val.strval);
		bufptr += strval_len;
	}

	// return the number of bytes written.
	return symobj_size + id_len + strval_len;
}

void
Symbol::
deserialise(u8* bytestream)
{
	size_t id_len     = strlen((char*) bytestream + SYMBOL_IDSTR_OFFSET) + 1;
	size_t strval_len = 0;
	u8* bsptr = bytestream;

	// make the id string.
	id = (char*) malloc(id_len);

	if (!id)
		throw AssemblerError("\nmalloc() failed in Symbol::deserialise() [id = (char*) malloc(id_len);]", (u8) AsmErrorCode::SYM_DESERIALISE_ID_MALLOC);

	strcpy(id, (char*) bytestream + SYMBOL_IDSTR_OFFSET);

	// check the datatype of the value in the symbol.
	bsptr = bytestream + SYMBOL_VALUE_OFFSET;

	switch (*(bytestream + SYMBOL_DATATYPE_OFFSET))
	{
		case ADDR_TYPE:
		case UINT_TYPE:
			val.uintval = *(u32*)bsptr;
			break;

		case INT_TYPE:
			val.intval = *(i32*)bsptr;
			break;

		case UBYTE_TYPE:
			val.ubyteval = *bsptr;
			break;

		case REAL_TYPE:
			val.realval = *(f32*)bsptr;
			break;

		case STRING_TYPE:
			strval_len = strlen((char*) bytestream + SYMBOL_IDSTR_OFFSET + id_len) + 1;
			val.strval = (char*) malloc(strval_len);

			if (!val.strval)
				throw AssemblerError("\nmalloc() failed in Symbol::deserialise() [val.strval = (char*) malloc(strval_len);]", (u8)AsmErrorCode::SYM_DESERIALISE_STRVAL_MALLOC);

			strcpy(val.strval, (char*) (bytestream + SYMBOL_IDSTR_OFFSET + id_len));
			break;
	}

	bsptr = bytestream + SYMBOL_NDX_OFFSET;
	ndx   = *(u32*) bsptr;

	bsptr = bytestream + SYMBOL_TYPE_OFFSET;
	type  = *(u32*) bsptr;

	bsptr    = bytestream + SYMBOL_DATATYPE_OFFSET;
	val_type = *(u32*) bsptr;
}

SymbolTable::
SymbolTable(bool _keep_symbols)
:
	keep_symbols(_keep_symbols),
	bytestream_size(0),
	start_addr(0)
{
	try
	{
		vec = new std::vector<Symbol*>();
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in SymbolTable::SymbolTable(bool _keep_symbols)\n[vec = new std::vector<Symbol*>();]");
	}

	create_builtin_symbols();
}

SymbolTable::
SymbolTable(u8*  buf,
	        bool _keep_symbols)
:
	keep_symbols(_keep_symbols),
	start_addr(0),
	bytestream_size(0)
{
	try
	{
		vec = new std::vector<Symbol*>();
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in SymbolTable::SymbolTable(u8* buf, bool _keep_symbols)\n[vec = new std::vector<Symbol*>();]");
	}

	deserialise(buf);
}

SymbolTable::
~SymbolTable()
{
	for (size_t i = 0; i < vec->size(); i++)
		delete vec->at(i);
}

// returns true when successful or false when not.
void
SymbolTable::
import_symtab(IMForm* srcimf)
{

	SymbolTable* src_symtab = srcimf->symtab;
	char*        src_fileid = srcimf->in_fdata->fileid;

	Symbol* srcsym;
	Symbol* newsym;
	char*   newsym_id;

	for (size_t i = 0; i < src_symtab->vec->size(); i++)
	{
		// get a convinience ptr to symbol being imported.
		srcsym = src_symtab->vec->at(i);

		// skip any atom-symbols. check for it now and not in switch
		// below to avoid the malloc calls below if possible.
		if (srcsym->type == ATOM_SYMBOL)
			continue;

		// importing the src_symtab means creating clones of
		// each symbol that we want from src_symtab. we then 
		// keep the clone. the id of the cloned symbol becomes
		// this: "fileid.original_symid" so all imported symbols
		// can easy be distinguished.
		
		// create fileid symtab id.
		newsym_id = (char*)malloc(100);
		if (!newsym_id)
			throw AssemblerError("\nmalloc error inside symtab->import_symtab() [newsym_id = (char*)malloc(100);]");

		sprintf(newsym_id, "%s.%s", src_fileid, srcsym->id);
		newsym_id = (char*)realloc(newsym_id, strlen(newsym_id) + 1);

		if (!newsym_id)
			throw AssemblerError("\nrealloc error inside symtab->import_symtab() [newsym_id = (char*)realloc(newsym_id, strlen(newsym_id) + 1);]");

		// here we will create the cloned-symbol object at newsym.
		// taking the required action for the different symbol types.
		switch (srcsym->type)
		{
			case LABEL_SYMBOL:
				newsym = create_new_symbol(LABEL_SYMBOL, newsym_id);
				newsym->val.uintval = srcsym->val.uintval;
				newsym->val_type = UINT_TYPE;
				continue;

			case MACRO_SYMBOL:
				newsym = create_new_symbol(MACRO_SYMBOL, newsym_id);
				newsym->val_type = srcsym->val_type;

				switch (srcsym->val_type)
				{
				case UINT_TYPE:
					newsym->val.uintval = srcsym->val.uintval;
					continue;

				case INT_TYPE:
					newsym->val.intval = srcsym->val.intval;
					continue;

				case UBYTE_TYPE:
					newsym->val.ubyteval = srcsym->val.ubyteval;
					continue;

				case REAL_TYPE:
					newsym->val.realval = srcsym->val.realval;
					continue;

			}
		}
	}
}

void
SymbolTable::
reset_all()
{
	for (size_t i = 0; i < vec->size(); i++)
		delete vec->at(i);
		
	vec->clear();
	start_addr = 0;
	bytestream_size = 0;
}

void
SymbolTable::
create_label_symbol(char* _id,
	                u32   _addr)
{
	try
	{
		Symbol* sym      = new Symbol(vec->size(), LABEL_SYMBOL, _id);
		sym->val_type    = ADDR_TYPE;
		sym->val.uintval = _addr;

		vec->push_back(sym);
	} 
	
	catch (const std::bad_alloc& e)
	{
		free(_id);
		throw AssemblerError("\nnew failed in SymbolTable::create_label_symbol(char* _id, u32 _addr)\n[Symbol* sym = new Symbol(vec->size(), LABEL_SYMBOL, _id);]");
	}
}

void
SymbolTable::
create_macro_symbol(char* _id)
{
	try {
		Symbol* sym      = new Symbol(vec->size(), MACRO_SYMBOL, _id);
		sym->val_type    = VOID_TYPE;
		sym->val.uintval = 0;
		vec->push_back(sym);
	}

	catch (const std::bad_alloc& e)
	{
		free(_id);
		throw AssemblerError("\nnew failed in SymbolTable::create_macro_symbol(char* _id)\n[Symbol* sym = new Symbol(vec->size(), MACRO_SYMBOL, _id);]");
	}
}

// NOTE: this function performs no duplicate identifier checks because
// it's creating symbol from previously serialised symbol-table which
// would have already performed this check.
size_t
SymbolTable::
create_new_symbol(u8* buf)
{
	try
	{
		Symbol* sym = new Symbol(buf);
		vec->push_back(sym);
		return sym->serial_size();
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in SymbolTable::create_new_symbol(u8* buf)\n[Symbol* sym = new Symbol(buf);]");
	}
}

Symbol*
SymbolTable::
create_new_symbol(u8    _type,
	              char* _id)
{
	try
	{
		Symbol* _sym = new Symbol(vec->size(), _type, _id);
		vec->push_back(_sym);
		return _sym;
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in SymbolTable::create_new_symbol(u8 _type, char* _id)\n[Symbol* _sym = new Symbol(vec->size(), _type, _id);]");
	}
}

bool
SymbolTable::
id_inuse(char* _id)
{
	for (size_t i = 0; i < vec->size(); i++)
	{
		if (strcmp(vec->at(i)->id, _id) == 0)
			return true;
	}

	return false;
}

/*
	Labels & Addresses within .frt files:
		Until an .frt file is assembled any addresses referenced or
		used within it are all relative to the first instruction.

		Meaning first instruction's opcode is considered to be at
		address 0, as the file is assembled the instructions addresses
		are recorded like this, these address values are assigned to any
		labels or address macros/values within the .frt file.

		Upon assembly completion the symbol-table is "finalised". Part
		of this process is "calculating addr offsets" which just means
		every label / addr macro/value has the metadata-segment size and
		size of symbol-table once serialised, in bytes, added to them.

		Because within the VM's memory it's laid out like this:
			address 0             : METADATA-SEGMENT
			address metadata-size : SERIALISED SYMBOL TABLE
			address metadata-size + serialised symtab-size : FIRST PROGRAM INSTR.

		So once the offsets have been calculated all the addresses within
		symbol table will be accurate and be the correct values.

		If symbols aren't being kept then symtab size is set to 0 and
		the offsets will still be calculated which will just be the
		metadata segment size added to their addresses.

		See the .fbin memory map for a more clearer picture of all this.
*/

void
SymbolTable::
calc_addr_offsets()
{
	size_t offset = METADATA_SIZE;

	if (keep_symbols)
	{
		offset += bytestream_size;

		// ensure proper 32 bit alignment.
		if (offset % 4 != 0)
			offset += 4 - (offset % 4);
	}

	for (size_t i = 0; i < vec->size(); i++) 
	{
		switch ((*vec)[i]->type)
		{
			case MACRO_SYMBOL:
				if ((*vec)[i]->val_type != ADDR_TYPE)
					continue;

			case LABEL_SYMBOL:
				(*vec)[i]->val.uintval += offset;
				continue;
		}
	}
}

void
SymbolTable::
print(bool print_atoms = false)
{
	size_t label_count = 0;
	size_t macro_count = 0;
	size_t atom_count  = 0;
	size_t total_count = vec->size();

	if (!total_count) return;

	// get a count on each macro type.
	for (size_t i = 0; i < total_count; i++)
	{
		switch ((*vec)[i]->type)
		{
			case LABEL_SYMBOL:
				label_count++;
				break;

			case MACRO_SYMBOL:
				macro_count++;
				break;

			case ATOM_SYMBOL:
				atom_count++;
				break;
		}
	}

	printf("\nsymbol table: \n\t%zu symbols. %zu labels, %zu macros. %zu builtins", total_count, label_count, macro_count, core_count);

	if (label_count)
	{
		printf(SYMTAB_LABELS_INTRO_INFOSTR);

		for (size_t i = 0; i < total_count; i++)
		{
			if ((*vec)[i]->type == LABEL_SYMBOL)
				(*vec)[i]->print();
		}
	}

	if (macro_count)
	{
		printf(SYMTAB_MACROS_INTRO_INFOSTR);

		for (size_t i = 0; i < total_count; i++)
		{
			if ((*vec)[i]->type == MACRO_SYMBOL)
				(*vec)[i]->print();
		}
	}

	if (print_builtins)
	{
		if (core_count)
		{
			printf("\n\n built-in core symbols:");

			for (size_t i = 0; i < total_count; i++)
			{
				if ((*vec)[i]->type == CORE_SYMBOL)
					(*vec)[i]->print();
			}
		}
	}
}

// NOTE: serial_size() *must* be called before this function is called!
// this is because bytestream_size needs to be calculated!
// also this function *cannot* be called while keep_symbols = false!
void
SymbolTable::
serialise(u8* bytestream)
{
	u8* bsptr = bytestream;
	bsptr    += sizeof(size_t);

	// iterate through the vector and serialise each symbol as
	// we go, serialise() returns number of bytes written which is neat.
	for (size_t i = 0; i < vec->size(); i++)
		bsptr += (vec->at(i))->serialise(bsptr);

	// write the total number of bytes written to the bytestream,
	// since it sits right at the start of the bytestream.
	*((size_t*) bytestream) = bytestream_size;
}

// Takes a pointer to a buffer containing a serialised symbol table
// and populates this SymbolTable() object with the serialised symbols.
void
SymbolTable::
deserialise(u8* bytestream)
{
	u8*    bsptr    = bytestream;
	size_t counter  = sizeof(size_t);
	size_t sym_size = 0;

	bytestream_size = *((size_t*) bsptr);
	bsptr          += sizeof(size_t);

	while (counter < bytestream_size)
	{
		sym_size = create_new_symbol(bsptr);
		counter += sym_size;
		bsptr   += sym_size;

		if (counter >= bytestream_size)
			break;
	}
}

// Returns the length of the SymbolTable() object when serialised.
// Typically used for determining how much memory to allocate for serialisation.
// 
// NOTE: this function sets the member variable bytestream_size which is used by
// various other bits of code within the assembler.
//
// NOTE: this function also sets the start_addr variable so this address is known
// before symtab finalisation. the main symbol's value hasn't been offsetted yet
// that will be done in finalisation, simply the start_addr is set here.
size_t
SymbolTable::
serial_size()
{
	if (keep_symbols)
	{
		bytestream_size = sizeof(size_t);

		for (size_t i = 0; i < vec->size(); i++)
			bytestream_size += vec->at(i)->serial_size();

		Symbol* sym = lookup_symbol(START_LABEL_ID);

		if (!sym)
		{
			const char* errmsg = "\nno main label declared.";
			char* errmsg_ = (char*) errmsg;
			throw SystematicError(errmsg_, false);
		}

		start_addr = METADATA_SIZE + bytestream_size + sym->val.uintval;
	}

	else
	{
		start_addr = METADATA_SIZE;
	}

	return bytestream_size;
}

// finalising the symtab refers to resolving all the macro referals
// and to calculate all the final address values for addr-type macros
// and labels. all macros who point to another macro, to another ect
// these are all resolved and finalised. the last stage is serialising
// the symtab(if specified in opt-tbl) to the program-bytestream.
void
SymbolTable::
finalise(u8* prog_bytestream, IMForm* imform)
{
	// test if symtab-bytestream size has been calculated yet.
	if (!bytestream_size)
		serial_size();

	// resolve all symbol values.
	resolve_builtin_symbols(imform);
	resolve_all_macros();

	// now that symtab size is known add it into all address
	// label/addr macro values, which will now all be accurate.
	calc_addr_offsets();

	if (keep_symbols)
		serialise(prog_bytestream);
}

Symbol*
SymbolTable::
lookup_symbol(const char* identifier)
{
	for (size_t i = 0; i < vec->size(); i++)
	{
		if (strcmp(vec->at(i)->id, identifier) == 0)
			return vec->at(i);
	}

	return NULL;
}

void
SymbolTable::
resolve_builtin_symbols(IMForm* imform)
{
	Symbol* sym = lookup_symbol("bi_mem_size");
	sym->val_type = UINT_TYPE;
	sym->val.uintval = imform->metadata->mem_size;

	sym = lookup_symbol("bi_prog_size");
	sym->val_type = UINT_TYPE;
	sym->val.uintval = imform->metadata->prog_size;

	sym = lookup_symbol("bi_instr_count");
	sym->val_type = UINT_TYPE;
	sym->val.uintval = imform->metadata->instr_count;

	sym = lookup_symbol("bi_first_instr_addr");
	sym->val_type = UINT_TYPE;
	sym->val.uintval = imform->metadata->first_instr_addr;

	sym = lookup_symbol("bi_last_instr_addr");
	sym->val_type = UINT_TYPE;
	sym->val.uintval = imform->metadata->last_instr_addr;

	sym = lookup_symbol("bi_first_user_addr");
	sym->val_type = UINT_TYPE;
	sym->val.uintval = imform->metadata->first_user_byte_addr;

	sym = lookup_symbol("bi_symtab_size");
	sym->val_type = UINT_TYPE;
	sym->val.uintval = imform->metadata->symtab_size;
}

void
SymbolTable::
resolve_all_macros()
{
	Symbol* sym;
	Symbol* macro;

	// all symbols with the datatype ID_TYPE are currently
	// macro symbols who's value is simply another macro.
	// so it's macro value is the id and thus then value
	// of another macro. eg: x = 1, y = x, z = y, m = z,
	// all of these resolve to 1.

	for (size_t i = 0; i < vec->size(); i++)
	{
		sym   = vec->at(i);
		macro = sym; // keep a ptr to the original symbol being resolved.
	
		// only need to resolve symbols that refer to other symbols.
		if (sym->type != ID_TYPE) continue;

		// this loop traces the references back to the actual
		// source value containing symbol.
		// eg: x = 1, y = x, t = y, z = t. so we get the z symbol,
		// lookup it's id, we get t, then we get y, then x finally
		// giving us actual value. so upon leaving this loop below
		// sym will be pointing at the final Value() object.
		u8 ref_level = 0;
		while (sym->val_type == ID_TYPE)
		{
			// check if we're exceeding the reference maximum.
			if (++ref_level == MACRO_REFERENCE_MAX)
			{
				char* errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);

				if (!errmsg)
					throw AssemblerError("\nmalloc failed in SymbolTable::resolve_all_macros()\n[char* errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);]");
				
				sprintf(errmsg, "\nmacro symbol: [%s] exceeds the reference limit", macro->id);
				throw SystematicError(errmsg, true);
			}

			sym = lookup_symbol(sym->val.strval);

			if (!sym)
			{
				char* errmsg = (char*)malloc(ASMERR_MSG_BUFSIZE);

				if (!errmsg)
					throw AssemblerError("\nmalloc failed in SymbolTable::resolve_all_macros()\n[char* errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);]");

				sprintf(errmsg, "\nmacro symbol: [%s] refers to another symbol: %s that does not exist", macro->id, sym->val.strval);
				throw SystematicError(errmsg, true);
			}
		}

		// resolve the macro's value.
		macro->val = sym->val;
	}
}

Value
SymbolTable::
lookup_symbol_value(const char* _id)
{
	for (size_t i = 0; i < vec->size(); i++)
	{
		if (strcmp(vec->at(i)->id, _id) == 0)
		{
			return vec->at(i)->val;
		}
	}

	char* errmsg = (char*)malloc(ASMERR_MSG_BUFSIZE);

	if (!errmsg)
		throw AssemblerError("\nmalloc failed in SymbolTable::lookup_symbol_value(char* _id)\n[char* errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);]");

	sprintf(errmsg, "\nfailed symbol lookup, [%s] doesn't exist", _id);
	throw SystematicError(errmsg, true);
}

/*
	built-in symbols: a way for the programmer of a furst process
	to access certain constants relative to the vm or process itself.

	NOTE: all built-ins are resolved when resolve_all_macros() is called.
		  this function just declares them affectively reserving the identifiers.

	list of builtins:

		bi_mem_size
		bi_prog_size
		bi_instr_count;
		bi_first_instr_addr;
		bi_last_instr_addr;
		bi_first_mem_addr
		bi_symtab_size
*/
void
SymbolTable::
create_builtin_symbols()
{
	// metadata built-in macros.
	create_new_symbol(ATOM_SYMBOL, (char*) "bi_mem_size");
	create_new_symbol(ATOM_SYMBOL, (char*) "bi_prog_size");
	create_new_symbol(ATOM_SYMBOL, (char*) "bi_instr_count");
	create_new_symbol(ATOM_SYMBOL, (char*) "bi_first_instr_addr");
	create_new_symbol(ATOM_SYMBOL, (char*) "bi_last_instr_addr");
	create_new_symbol(ATOM_SYMBOL, (char*) "bi_first_user_addr");
	create_new_symbol(ATOM_SYMBOL, (char*) "bi_symtab_size");
}