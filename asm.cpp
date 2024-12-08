#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#include "asm.h"

void
MetadataTable::
print()
{
	printf("\nMetadataTable() object print-out:\n");
	printf("\nstart_instr_addr: %u", start_instr_addr);
	printf("\nmem_size: %u", mem_size);
	printf("\nprog_size: %u", prog_size);
	printf("\ninstr_count: %u", instr_count);
	printf("\nfirst_instr_addr: %u", first_instr_addr);
	printf("\nlast_instr_addr: %u", last_instr_addr);
	printf("\nfirst_user_byte_addr: %u", first_user_byte_addr);
	printf("\nflags: %u", flags);
	printf("\ncreation_date: %u", creation_date);
	printf("\nsymtab_size: %u", symtab_size);

	newlines(2);
}

void
OperandResolver::
print()
{
	printf("\nOperandResolver :: operator-nodes: %u :: value-nodes: %u", op_count, val_count);
	
	printf("\n operator-stack printed from top to bottom:");
	for (u8 i = 0; i < op_count; i++)
		(*(opstk_top - i))->print();

	printf("\n value-stack printed from top to bottom:");
	for (u8 i = 0; i < val_count; i++)
		(*(valstk_top - i))->print();

	newlines(2);
}

void
Node::
print()
{
	switch (node_type)
	{
		case OPNODE:
			printf("\noperator-node %c - num: %u :: op-prec: %u :: op-asso: %s", (char)parent_tok->type, stack_num, op_prec, op_asso_str[op_asso]);
			break;

		case VALNODE:
			printf("\nvalue-node num: %u :: datatype: %s :: value: ", stack_num, datatype_str[val_type]);

			switch (val_type)
			{
				case UINT_TYPE:
					printf("%u", val.uintval);
					break;

				case INT_TYPE:
					printf("%d", val.intval);
					break;

				case UBYTE_TYPE:
					printf("%u", val.ubyteval);
					break;

				case REAL_TYPE:
					printf("%f", val.realval);
					break;
			}

			break;

		default:
			printf("\ninvalid node");
			break;
	}

	newline();
}

void
InstrBlock::
print()
{
	printf("\ninstr-block :: num: %zu :: opcode: %s (%u) instr :: operand-count: %u", instr_num, mnemonic_strings[opcode], opcode, opargc_tbl[opcode]);

	for (u8 i = 0; i < INSTR_OPR_MAX; i++)
	{
		if (val_type[i] == VOID_TYPE)
			continue;

		printf("\noperand%u ", i);

		switch (val_type[i])
		{
		case UINT_TYPE:
			printf("uint value: %u", val[i]->uintval);
			break;

		case INT_TYPE:
			printf("int value: %d", val[i]->intval);
			break;

		case UBYTE_TYPE:
			printf("ubyte value: %u", val[i]->ubyteval);
			break;

		case REAL_TYPE:
			printf("real value: %f", val[i]->realval);
			break;
		}
	}

	newline();
}

void
MetadataTable::
serialise(u8* _ram_base)
{
	u32* u32p;
	u16* u16p;

	u32p = (u32*)(_ram_base + MD_START_INSTR_ADDR_OFFSET);
	*u32p = start_instr_addr;

	u32p = (u32*)(_ram_base + MD_MEMORY_SIZE_OFFSET);
	*u32p = mem_size;

	u32p = (u32*)(_ram_base + MD_PROG_SIZE_OFFSET);
	*u32p = prog_size;

	u32p = (u32*)(_ram_base + MD_INSTR_COUNT_OFFSET);
	*u32p = instr_count;

	u32p = (u32*)(_ram_base + MD_FIRST_INSTR_ADDR_OFFSET);
	*u32p = first_instr_addr;

	u32p = (u32*)(_ram_base + MD_LAST_INSTR_ADDR_OFFSET);
	*u32p = last_instr_addr;

	u32p = (u32*)(_ram_base + MD_FIRST_USER_BYTE_ADDR_OFFSET);
	*u32p = first_user_byte_addr;

	u32p = (u32*)(_ram_base + MD_FLAGS_OFFSET);
	*u32p = flags;

	u32p = (u32*)(_ram_base + MD_CREATION_DATE_OFFSET);
	*u32p = creation_date;
}


// creates a node used in evaluating expressions within instruction operands.
Node::
Node(OperandResolver* _resolver,
	 Token*           _owner_tok,
	 u8               _node_type,
	 u8               _stack_num,
	 u8               _val_type)
:
	parent_tok(_owner_tok),
	resolver(_resolver),
	node_type(_node_type),
	val_type(_val_type),
	stack_num(_stack_num),
	op_prec(NONE),
	op_asso(NONE)

{
	val.uintval = 0;

	// if node is an operator-node get its precedence value then dip.
	if (node_type == OPNODE)
	{
		op_prec = get_tok_precedence(parent_tok);
		op_asso = loopup_tok_association(parent_tok->type);
		return;
	}
	
	// if node is a value-node set node's val field.
	// if it's a constant the value comes from the owner
	// token's val field, if the owner-tok instead is
	// an identifier token, then it is looked up inside
	// the symbol table and the value is the symbols val field.
	if (node_type == VALNODE)
	{
		// load the symbol this identifier refers to.
		if (_owner_tok->type == ID_TOK)
		{
			// check if left operand is coming from a identifier.
			char* _fileid = _resolver->asmobj->imform->in_fdata->fileid;

			if (strchr(parent_tok->srcstr, '.'))
			{
				if (!(resolver->asmobj->symtab->id_inuse(parent_tok->srcstr)))
					resolver->asmobj->throw_error_here(parent_tok, "identifier doesn't match any symbols in the symbol-table.");
			
				// set value node's val var to the symbol's val var.
				val = resolver->asmobj->symtab->lookup_symbol(parent_tok->srcstr)->val;
				val_type = resolver->asmobj->symtab->lookup_symbol(parent_tok->srcstr)->val_type;
			}

			else
			{
				char fullid[SYMBOL_FULLID_MAX_SIZE];
				sprintf(fullid, "%s.%s", _fileid, parent_tok->srcstr);

				if (!(resolver->asmobj->symtab->id_inuse(fullid)))
					resolver->asmobj->throw_error_here(parent_tok, "identifier doesn't match any symbols in the symbol-table.");
				
				// set value node's val var to the symbol's val var.
				val = resolver->asmobj->symtab->lookup_symbol(fullid)->val;
				val_type = resolver->asmobj->symtab->lookup_symbol(fullid)->val_type;
			}

			return;
		}

		// value is coming from owner-tok's val field because
		// it's a constant value token not an identifier.
		val = parent_tok->val;
		val_type = parent_tok->val_type;
	}
}


// this function is for operands which are addresses. takes the operand 
// number and applies the recorded pos or negative offset to it's
// base uint value. Also returns point to the value object effected.
void
InstrBlock::
apply_offset(const u8 datatype,
	         const u8 _oprnum)
{
	switch (datatype)
	{
		case UINT_TYPE:
			if (pos_offset[_oprnum])
				(*val + _oprnum)->uintval += pos_offset[_oprnum]->uintval;

			else if (neg_offset[_oprnum])
				(*val + _oprnum)->uintval += neg_offset[_oprnum]->uintval;

			break;

		case INT_TYPE:
			if (pos_offset[_oprnum])
				(*val + _oprnum)->intval += pos_offset[_oprnum]->intval;

			else if (neg_offset[_oprnum])
				(*val + _oprnum)->intval += neg_offset[_oprnum]->intval;

			break;

		case UBYTE_TYPE:
			if (pos_offset[_oprnum])
				(*val + _oprnum)->ubyteval += pos_offset[_oprnum]->ubyteval;

			else if (neg_offset[_oprnum])
				(*val + _oprnum)->ubyteval += neg_offset[_oprnum]->ubyteval;

			break;
	}
}

InstrBlock*
InstrBlock::
make_clone()
{
	InstrBlock* ret_instr = new InstrBlock(opcode, instr_num);
	ret_instr->ram_addr = ram_addr;

	for (u32 i = 0; i < INSTR_OPR_MAX; i++)
	{
		ret_instr->val_type[i]   = val_type[i];
		ret_instr->val[i]        = val[i];
		ret_instr->pos_offset[i] = pos_offset[i];
		ret_instr->neg_offset[i] = neg_offset[i];
	}

	return ret_instr;
}

InstrBlock::
InstrBlock(u8 _opcode, size_t _instr_num)
:
	opcode(_opcode),
	instr_num(_instr_num),
 	ram_addr(0)
{
	val        = (Value**) malloc(INSTR_OPR_MAX * sizeof(Value));
	pos_offset = (Value**) malloc(INSTR_OPR_MAX * sizeof(Value));
	neg_offset = (Value**) malloc(INSTR_OPR_MAX * sizeof(Value));

	if (!val || (!pos_offset || !neg_offset))
		throw AssemblerError("\nmalloc failed in InstrBlock::InstrBlock()\n[val = (Value**) malloc(INSTR_OPR_MAX * sizeof(Value));]", (u8)AsmErrorCode::INSTR_BLOCK_CONSTR_NEW_ERR);

	try
	{
		for (u8 i = 0; i < INSTR_OPR_MAX; i++)
		{
			val_type[i]   = VOID_TYPE;
			val[i]        = new Value;
			pos_offset[i] = new Value;
			neg_offset[i] = new Value;

			pos_offset[i]->uintval = 0;
			neg_offset[i]->uintval = 0;
		}
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in InstrBlock::InstrBlock()\n[val[i] = new Value;]", (u8)AsmErrorCode::INSTR_BLOCK_CONSTR_NEW_ERR);
	}
}

InstrBlock::
~InstrBlock()
{
	for (u8 i = 0; i < INSTR_OPR_MAX; i++)
		delete val[i];

	for (u8 i = 0; i < INSTR_OPR_MAX; i++)
		delete pos_offset[i];

	for (u8 i = 0; i < INSTR_OPR_MAX; i++)
		delete neg_offset[i];
}

IMForm::
IMForm()
:
		bytestream_size(0),
		input_path(NULL),
		output_path(NULL),
		prog_bytestream(NULL),
	    mdata_byte_serialised(false),
	    instrs_byte_serialised(false),
	    symtab_byte_serialised(true),
		symtab(NULL),
		symbols_kept(false),
	    imform_file_written(false),
		in_fdata(NULL),
	    ibvec_changed(false)
{
	try
	{
		instr_vec = new std::vector<InstrBlock*>();
		metadata  = new MetadataTable();
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in IMForm::IMForm()\n[vec = new std::vector<InstrBlock*>();\nmetadata = new MetadataTable();]", (u8) AsmErrorCode::IMFORM_CONSTR_NEW_ERR);
	}
}

IMForm::
~IMForm()
{
	for (u32 i = 0; i < instr_vec->size(); i++)
		delete instr_vec->at(i);

	delete symtab;
	delete metadata;
}

void
IMForm::
build_infdata(char* _path)
{
	try
	{
		in_fdata = new moduleFileData(_path);
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in IMForm::build_fdata()\n[fdata = new moduleFileData(_path);]");
	}
}

IMForm*
IMForm:: // ON KILL LIST!! NOT NEEDED.
make_clone()
{
	IMForm* ret_imform = new IMForm();
	InstrBlock* clone_instr;

	// clone metadata.
	ret_imform->metadata->start_instr_addr = metadata->start_instr_addr;
	ret_imform->metadata->mem_size = metadata->mem_size;
	ret_imform->metadata->prog_size = metadata->prog_size;
	ret_imform->metadata->instr_count = metadata->instr_count;
	ret_imform->metadata->first_instr_addr = metadata->first_instr_addr;
	ret_imform->metadata->last_instr_addr = metadata->last_instr_addr;
	ret_imform->metadata->first_user_byte_addr = metadata->first_user_byte_addr;
	ret_imform->metadata->flags = metadata->flags;
	ret_imform->metadata->creation_date = metadata->creation_date;
	ret_imform->metadata->symtab_size = metadata->symtab_size;

	// clone instruction blocks.
	for (u32 i = 0; i < instr_vec->size(); i++)
	{
		clone_instr = instr_vec->at(i)->make_clone();
		ret_imform->instr_vec->push_back(clone_instr);
	}

	// clone input/output paths.
	if (input_path)
	{
		size_t input_path_len = strlen(input_path);
		if (input_path_len)
		{
			ret_imform->input_path = (char*)malloc(input_path_len);

			if (!ret_imform->input_path)
			{
				// HANDLE ERROR
			}

			strcpy(ret_imform->input_path, input_path);
		}

		size_t output_path_len = strlen(output_path);
		if (output_path_len)
		{
			ret_imform->output_path = (char*)malloc(output_path_len);

			if (!ret_imform->output_path)
			{
				// HANDLE ERROR
			}

			strcpy(ret_imform->output_path, output_path);
		}
	}

	// clone bytestream.
	if (bytestream_size)
	{
		ret_imform->bytestream_size = bytestream_size;
		ret_imform->prog_bytestream = (u8*)malloc(bytestream_size);

		if (!ret_imform->prog_bytestream)
		{
			// HANDLE ERROR
		}

		memcpy(ret_imform->prog_bytestream, prog_bytestream, bytestream_size);
	}

	// clone remaining variables.
	ret_imform->symbols_kept           = symbols_kept;
	ret_imform->mdata_byte_serialised  = mdata_byte_serialised;
	ret_imform->symtab_byte_serialised = symtab_byte_serialised;
	ret_imform->instrs_byte_serialised = instrs_byte_serialised;
	ret_imform->imform_file_written    = imform_file_written;

	return ret_imform;
}

void
IMForm::
merge_imform(ImportCard* import_card)
{
	IMForm* src_imform = import_card->imform;

	// merge src_imform's symtab with this->symtab's.
	// except the atomic symbols obviously.
	symtab->import_symtab(src_imform);

	// so insert a cloned version of each instr of src_imform
	// starting at target_index(technically infront of target_index).
	InstrBlock* cloned_instr;
	for (u32 i = 0; i < src_imform->instr_vec->size(); i++)
	{
		cloned_instr = src_imform->instr_vec->at(i)->make_clone();
		instr_vec->insert(instr_vec->begin() + (import_card->insertion_index + i), cloned_instr);
	}
			
	// traverse instr_vec so we can update the instr-block's instr_num.
	for (u32 inum = 0; inum < instr_vec->size(); inum++)
		instr_vec->at(inum)->instr_num = inum;

	// set all flags to notify that the current outputs aren't valid.
	//ibvec_changed          = true;
	//mdata_byte_serialised  = false;
	//symtab_byte_serialised = false;
	//instrs_byte_serialised = false;
	//imform_file_written    = false;
}


u32
IMForm::
get_ram_addr(u8* ram_base,
	         u8* cptr)
{
	u32 ADDR_VAL = (cptr - ram_base);
	if (ADDR_VAL % 4 != 0) \
		ADDR_VAL += 4 - (ADDR_VAL % 4);
	return ADDR_VAL;
}

void
IMForm::
print_metadata_bytestream(u8* _bytestream)
{
	u8* bf = _bytestream;

	printf("\naddr: %u     :: value: %u (start-addr)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u     :: value: %u (mem-size)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u     :: value: %u (prog-size)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u    :: value: %u (instr-count)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u    :: value: %u (first-instr-addr)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u    :: value: %u (last-instr-addr)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u    :: value: %u (first-user-byte)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u    :: value: %u (keep-symbols-flag)", get_ram_addr(_bytestream, bf), (*(u32*)bf));
	bf += UINT_SIZE;

	printf("\naddr: %u    :: value: %u (creation-date) decoded: %s", get_ram_addr(_bytestream, bf), (*(u32*)bf), decode_datestamp((*(u32*)bf)) );
	bf += UINT_SIZE;
}

//void
//IMForm::
//register_fileid(char* _id)
//{
//	size_t idlen = strlen(_id);
//	fileid = (char*)malloc(idlen + 1);
//	strcpy(fileid, _id);
//}

void
IMForm::
print_program_bytestream(u8* _bytestream)
{
	u8* bf = _bytestream + metadata->first_instr_addr;
	u8 opcode;

	for (size_t b = 0; b < metadata->prog_size;)
	{
		printf("\n\naddr: %u    :: op: %u (%s)", get_ram_addr(_bytestream, bf), *bf, mnemonic_strings[*bf]);
		b  += opsize_tbl[*bf];
 		opcode = *bf;
		bf += OPCODE_SIZE;

		if (b >= metadata->prog_size)
			break;

		switch (opcode)
		{
			// one uint operand instructions.
			case perr_op:
			case systime_op:
			case htime_op:
			case getutc_op:
			case getlocal_op:
			case delay_op:
			case waituntil_op:
			case elapsed_op:
			case getweekday_op:
			case monthdays_op:
			case normtime_op:
			case ftel_op:
			case rwnd_op:
			case sputs_op:
			case sgets_op:
			case serr_op:
			case sdup_op:
			case call_op:
			case jmp_op:
			case jz_op:
			case jnz_op:
			case je_op:
			case jn_op:
			case jl_op:
			case jg_op:
			case jls_op:
			case jgs_op:
			case pshu_op:
			case popnu_op:
			case pshfru_op:
			case poptru_op:
			case movtru_op:
			case stktru_op:
			case pshi_op:
			case pshfri_op:
			case poptri_op:
			case movtri_op:
			case stktri_op:
			case pshr_op:
			case pshfrr_op:
			case poptrr_op:
			case movtrr_op:
			case stktrr_op:
			case slen_op:
			case fcls_op:
				printf("\naddr: %u    :: value: %u", get_ram_addr(_bytestream, bf), (*(u32*)bf));
				bf += UINT_SIZE;
				continue;

				// 3 uint operand instructions.
			case frd_op:
			case fwr_op:
			case fprntf_op:
			case sprntf_op:
			case sscnf_op:
			case mcpy_op:
			case mmov_op:
			case sncy_op:
			case snct_op:
			case mcmp_op:
			case sncm_op:
			case mset_op:
				printf("\naddr: %u    :: value: %u", get_ram_addr(_bytestream, bf), (*(u32*)bf));
				bf += UINT_SIZE;

				printf("\naddr: %u    :: value: %u", get_ram_addr(_bytestream, bf), (*(u32*)bf));
				bf += UINT_SIZE;

				printf("\naddr: %u    :: value: %u", get_ram_addr(_bytestream, bf), (*(u32*)bf));
				bf += UINT_SIZE;
				continue;

				// two uint operand instructions.
			case timeadd_op:
			case timesub_op:
			case settime_op:
			case tstampstr_op:
			case cmptime_op:
			case timediff_op:
			case fopn_op:
			case fsk_op:
			case prntf_op:
			case scnf_op:
			case fgts_op:
			case fpts_op:
			case scpy_op:
			case scat_op:
			case scmp_op:
			case schr_op:
			case srch_op:
			case sstr_op:
			case stok_op:
			case sspn_op:
			case scspn_op:
			case sfrm_op:
			case cpyru_op:
			case setru_op:
			case cpyri_op:
			case setri_op:
			case cpyrr_op:
			case setrr_op:
				printf("\naddr: %u    :: value: %u", get_ram_addr(_bytestream, bf), (*(u32*)bf));
				bf += UINT_SIZE;

				printf("\naddr: %u    :: value: %u", get_ram_addr(_bytestream, bf), (*(u32*)bf));
				bf += UINT_SIZE;
				continue;
		}
	}
}

void
IMForm::
print_bytestream(u8* _bytestream)
{
	print_metadata_bytestream(_bytestream);

	if (symtab && symtab->vec->size())
		symtab->print(false);

	print_program_bytestream(_bytestream);
}

void
IMForm::
print()
{
	printf("\nIM-FORM vector :: %zu instruction-blocks", instr_vec->size());

	metadata->print();

	printf("\tInstruction-Blocks:");
	for (size_t i = 0; i < instr_vec->size(); i++)
		instr_vec->at(i)->print();

	newlines(2);
}


void
IMForm::
write_prog_to_bytes(u8* buffer)
{
	write_metadata_to_bytes(buffer);
	write_instrs_to_bytes(buffer + metadata->first_instr_addr);
	instrs_byte_serialised = true;
}


// takes a buffer pointer to large enough memory block and
// serialises each instruction to the passed pointer
// which is updated as it's written to, upon the
// serialisation completeing the pointer is
// pointing to the first-user-byte, first byte after program.
//
// NOTE: does not serialise symtab and MUST be called AFTER metadata is built.
void
IMForm::
write_instrs_to_bytes(u8* buffer)
{
	u8* bf = buffer;
	InstrBlock* instr;

	for (u32 i = 0; i < instr_vec->size(); i++)
	{ 
		instr = instr_vec->at(i);
		instr->ram_addr = get_ram_addr(prog_bytestream, bf);
 		instr->serialise(bf);
		bf += opsize_tbl[instr_vec->at(i)->opcode];
	//	printf("\n-<<>> %u, opcode: %u", opsize_tbl[instr_vec->at(i)->opcode], instr_vec->at(i)->opcode);
	}

	instrs_byte_serialised = true;
}

void
IMForm::
write_metadata_to_bytes(u8* buffer)
{
	metadata->serialise(buffer);
	mdata_byte_serialised = true;
}

void
IMForm::
write_symtab_to_bytes(u8* buffer)
{
	symtab->finalise(buffer + SYMTAB_DATA_OFFSET, this);
	symtab_byte_serialised = true;
}

void
IMForm::
read_symtab_from_bytes(u8* buffer)
{
	symtab = new SymbolTable(buffer, (bool)symbols_kept);
}

void
IMForm::
interpret_flags(u8* flagbytes)
{
	symbols_kept = (bool) *flagbytes;
}

void
IMForm::
read_metadata_from_bytes(u8* _buf)
{
	metadata->start_instr_addr     = *(u32*)(_buf + MD_START_INSTR_ADDR_OFFSET);
	metadata->mem_size             = *(u32*)(_buf + MD_MEMORY_SIZE_OFFSET);
	metadata->prog_size            = *(u32*)(_buf + MD_PROG_SIZE_OFFSET);
	metadata->instr_count          = *(u32*)(_buf + MD_INSTR_COUNT_OFFSET);
	metadata->first_instr_addr     = *(u32*)(_buf + MD_FIRST_INSTR_ADDR_OFFSET);
	metadata->last_instr_addr      = *(u32*)(_buf + MD_LAST_INSTR_ADDR_OFFSET);
	metadata->first_user_byte_addr = *(u32*)(_buf + MD_FIRST_USER_BYTE_ADDR_OFFSET);
	metadata->flags                = *(u32*)(_buf + MD_FLAGS_OFFSET);
	metadata->creation_date        = *(u32*)(_buf + MD_CREATION_DATE_OFFSET);

	interpret_flags(_buf + MD_FLAGS_OFFSET);
}

void
IMForm::
read_program_from_bytes(u8* _buf)
{
	InstrBlock* instr;

	read_metadata_from_bytes(_buf);
	u8* bf   = _buf + metadata->first_instr_addr;
	u32 addr = metadata->first_instr_addr;

	while (instr_vec->size() < metadata->instr_count)
	{
		if (*bf > OPCOUNT)
			// throw error here...
			;

		instr = new_instr(*bf);
		instr->instr_num = metadata->instr_count;
		instr->opcode = *bf;
		instr->ram_addr = addr;

		// assure the instr's addr is aligned correctly.
		if (instr->ram_addr % 4 != 0)
			instr->ram_addr += 4 - (instr->ram_addr % 4);

		addr += OPCODE_SIZE;
		bf += OPCODE_SIZE;

		switch (instr->opcode)
		{
			// one uint operand instructions.
		case perr_op:
		case systime_op:
		case htime_op:
		case getutc_op:
		case getlocal_op:
		case delay_op:
		case waituntil_op:
		case elapsed_op:
		case getweekday_op:
		case monthdays_op:
		case normtime_op:
		case ftel_op:
		case rwnd_op:
		case sputs_op:
		case sgets_op:
		case serr_op:
		case sdup_op:
		case call_op:
		case jmp_op:
		case jz_op:
		case jnz_op:
		case je_op:
		case jn_op:
		case jl_op:
		case jg_op:
		case jls_op:
		case jgs_op:
		case pshu_op:
		case popnu_op:
		case pshfru_op:
		case poptru_op:
		case movtru_op:
		case stktru_op:
		case pshi_op:
		case pshfri_op:
		case poptri_op:
		case movtri_op:
		case stktri_op:
		case pshr_op:
		case pshfrr_op:
		case poptrr_op:
		case movtrr_op:
		case stktrr_op:
		case slen_op:
		case fcls_op:
			(*(instr->val) + 1)->uintval = (u32)(*(u32*)bf);
			instr->val_type[1] = UINT_TYPE;
			addr += UINT_SIZE;
			bf += UINT_SIZE;
			continue;

			// three uint operand instructions.
		case frd_op:
		case fwr_op:
		case fprntf_op:
		case sprntf_op:
		case sscnf_op:
		case mcpy_op:
		case mmov_op:
		case sncy_op:
		case snct_op:
		case mcmp_op:
		case sncm_op:
		case mset_op:
			(*(instr->val) + 1)->uintval = (u32)(*(u32*)bf);
			instr->val_type[1] = UINT_TYPE;
			addr += UINT_SIZE;
			bf += UINT_SIZE;

			(*(instr->val) + 2)->uintval = (u32)(*(u32*)bf);
			instr->val_type[2] = UINT_TYPE;
			addr += UINT_SIZE;
			bf += UINT_SIZE;

			(*(instr->val) + 3)->uintval = (u32)(*(u32*)bf);
			instr->val_type[3] = UINT_TYPE;
			addr += UINT_SIZE;
			bf += UINT_SIZE;
			continue;

			// two uint operand instructions.
		case timeadd_op:
		case timesub_op:
		case settime_op:
		case tstampstr_op:
		case cmptime_op:
		case timediff_op:
		case fopn_op:
		case fsk_op:
		case prntf_op:
		case scnf_op:
		case fgts_op:
		case fpts_op:
		case scpy_op:
		case scat_op:
		case scmp_op:
		case schr_op:
		case srch_op:
		case sstr_op:
		case stok_op:
		case sspn_op:
		case scspn_op:
		case sfrm_op:
		case cpyru_op:
		case setru_op:
		case cpyri_op:
		case setri_op:
		case cpyrr_op:
		case setrr_op:
			(*(instr->val) + 1)->uintval = (u32)(*(u32*)bf);
			instr->val_type[1] = UINT_TYPE;
			addr += UINT_SIZE;
			bf += UINT_SIZE;

			(*(instr->val) + 2)->uintval = (u32)(*(u32*)bf);
			instr->val_type[2] = UINT_TYPE;
			addr += UINT_SIZE;
			bf += UINT_SIZE;
			continue;

			// no operand instructions.
		case die_op:
		case nop_op:
		case stopprof_op:
		case startprof_op:
		case brkp_op:
		case nspctr_op:
		case ret_op:
		case swtch_op:
		case popu_op:
		case pop2u_op:
		case incu_op:
		case decu_op:
		case addu_op:
		case subu_op:
		case mulu_op:
		case divu_op:
		case modu_op:
		case inci_op:
		case deci_op:
		case addi_op:
		case subi_op:
		case muli_op:
		case divi_op:
		case modi_op:
		case addr_op:
		case subr_op:
		case mulr_op:
		case divr_op:
		case modr_op:
		case sqrt_op:
		case ceil_op:
		case floor_op:
		case sin_op:
		case cos_op:
		case tan_op:
		case powu_op:
		case pows_op:
		case powr_op:
		case absu_op:
		case abss_op:
		case absr_op:
		case fabs_op:
			continue;

			// default:
			// ERRORS HERE
		}
	}
}

Assembler::
Assembler(bool*      _option_tbl,
	   	  char*      _input_path,
		  char*      _output_path,
	      std::vector<ImportCard*>* _import_list,
		  size_t     _prog_size,
		  size_t     _next_byte_addr)
:
	import_list(_import_list),
	output_path(_output_path),
	input_path(_input_path),
	prog_bytestream(NULL),
	is_main_module(false),
	bytestream_size(0),
	next_byte_addr(0),
	output_file(NULL),
	input_file(NULL),
	currline(NULL),
	instr_count(0),
	main_retval(0),
	currtok(NULL),
	bufptr(NULL),
	prog_size(0),
	toknum(0)
{
	// copy option-table.
	for (int i = 0; i < ASM_ARGV_OPT_COUNT; i++)
		option_tbl[i] = _option_tbl[i];

	try
	{
		resolver    = new OperandResolver(this);
		import_list = new std::vector<ImportCard*>();
		parser      = new Parser();

		imform = new IMForm();
		imform->build_infdata(input_path);
		imform->symtab = new SymbolTable(option_tbl[KEEP_SYMBOLS_OPT]);
		imform->symbols_kept = option_tbl[KEEP_SYMBOLS_OPT];

		symtab = imform->symtab;
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in Assembler::Assembler()\n[parser = new Parser();]", (u8)AsmErrorCode::ASMOBJ_CONSTR_NEW_ERR);
	}

	oprval.uintval = 0;
}

Assembler::
~Assembler()
{
	delete resolver;
	delete symtab;
	delete parser;
	if (imform) delete imform;
}


void
Assembler::
merge_import_symbols()
{
	for (u32 i = 0; i < import_list->size(); i++)
		symtab->import_symtab(import_list->at(i)->imform);

	if (is_main_module)
		symtab->main_module_serial_size(imform->in_fdata->fileid);

	else
		symtab->serial_size();
}

void
Assembler::
throw_error_here(Token*      token,
	             const char* errmsg)
{
	printf("\n ERROR HERE");
	exit(1);
	//throw inputTextError(token->line, token->col, parser->linevec->at(token->line), errmsg);
}

// doesn't check tokens exactly but will catch some errors.
// 100% error checking will occur when expressions are evalauted.
void
Assembler::
count_uint_operand()
{
	u8 opstk[20];
	u8 oprstk[20];

	size_t operand_line; // operand's line number.
	bool   inside_expr = false;
	bool   value_hit = false;
	bool   inside_parens = false;
	bool   inside_brackets = false;

	u8* opstk_top = opstk;
	u8* oprstk_top = oprstk;

	prog_size += UINT_SIZE;
	next_byte_addr += UINT_SIZE;

	// determine if there's either a valid uint constant
	// or symbol refering to a constant, or it could be 
	// an entire expression.
	// so just traverse through the tokens looking for
	// either manditory expr-end operator or opcode,
	// macro or label def tokens.

	// upon entering this function toknum must be pointing
	// at the first token of the operand. however here
	// toknum is decremented so while loop below lines up
	// nicer so start of the loop.
	toknum--;
	operand_line = currtok->line;
	while (toknum < parser->tokcount)
	{
		currtok = parser->tokstream->vec->at(++toknum);

		switch (currtok->type)
		{
			// these token's should signify the normal end to the uint operand.
			// but only if the're on the next line.
		case OPCODE_TOK:
		case MACRO_DEF_TOK:
		case LABEL_DEF_TOK:
			if (currtok->line == operand_line)
				throw_error_here(currtok, "misplaced opcode, macro-def or label-def token");

			if (!value_hit)
				throw_error_here(currtok, "missing operand value");

			// execution falling here means the end of valid expression.
			return;

		case UINT_TOK:
		case ADDR_TOK:
		case ID_TOK:
			// if inside expression continue parsing it.
			if (inside_expr) continue;

			// if this is second time hitting a uint and
			// we're not in an expression then we've found
			// the next operand's token.
			if (value_hit) return;

			value_hit = true;
			continue;

		case EXPREND_TOK:
			if (!inside_expr)
				throw_error_here(currtok, "misplaced expr-end operator");

			else if (!value_hit)
				throw_error_here(currtok, "missing operand value");

			else if (inside_parens)
				throw_error_here(currtok, "missing left paren operator");

			else if (inside_brackets)
				throw_error_here(currtok, "missing left bracket operator");

			toknum++;
			return;

		case ADDOP_TOK: case MODOP_TOK:
		case SUBOP_TOK: case DIVOP_TOK: case MULOP_TOK: 
			inside_expr = true;
			continue;

		case RBRKTOP_TOK:
			if (!inside_expr)
				throw_error_here(currtok, "misplaced expr-end operator");

			else if (!value_hit)
				throw_error_here(currtok, "missing operand value");


			else if (!inside_brackets)
				throw_error_here(currtok, "missing left bracket operator");

			inside_brackets = false;
			continue;

		case RPRN_TOK:
			if (!inside_expr)
				throw_error_here(currtok, "misplaced expr-end operator");

			else if (!value_hit)
				throw_error_here(currtok, "missing operand value");


			else if (inside_parens)
				throw_error_here(currtok, "missing left bracket operator");

			inside_parens = false;
			continue;

		case LPRN_TOK:
			inside_expr   = true;
			inside_parens = true;
			continue;

		case LBRKTOP_TOK:
			inside_expr     = true;
			inside_brackets = true;
			continue;
		}
	}
}

ImportCard::
ImportCard(size_t  _insertion_index,
	       size_t  _curr_progsize)
:
	insertion_index(_insertion_index),
	curr_progsize(_curr_progsize) {}


/* 
	MODULE-IMPORTATION-EXPLANATION:
		NOTE: .frt file is synonymous with the term "module".

		first understand that the first-pass on a module builds it's
		symtab & determines that size of the module when assembled in memory.

		importation is simple on one hand, we simply build the module's imform
		then merge that imform into the main-module's imform at the correct index.

		importation is complicated by all the module's addresses. in an .frt file
		the addresses are all relevent to the first instruction. instruction #1's
		opcode is at address 0.

		this works fine because this file will be assembled into a .fbin file which
		will have a metadata segment and possibly it's symbol-table proceeding that,
		*then* the first instruction's opcode. at this point all the addresses within
		the .frt file's symtab are all out by the size of metadata-segment + symtab size.

		so we go through the symtab and simply add (metadata-size + symtab-size) to each
		address bringing them all where they should be.

		this works perfect right up until we try to import other .frt files. importing is
		basically copy and pasting the program at the import stmt.

		how shall we import/merge other .frt file's into other .frt files?
		this is my solution:

		each Assembler() object assembles just one module. Assembler() does it's first-pass
		which handles symbol definitions, determines the memory-size of the module when
		assembled and processes each import stmt instance. it also keeps track of the
		next address to be used so as to process label definitions.

		import statement:

		file1.frt:
			import <dir1/dir2/filename.frt>
			import <dir1/dir2/filename2.frt>
			import <dir1/dir2/filenam3.frt>

		each module has a list of import-card objects.

		ImportCard
		{
			size_t  insertion_index;
			size_t  curr_progsize; // prog-size of importer imform at time of import call.
			IMForm* imform;
		}

		on the first pass when an import stmt is hit, the next token is the file's path.
		so another assembler object is created which builds it's imform.


		
*/

// performs the first pass over the input text. 
// tasks are to verify tokens aren't misplaced 
// or missing, build all symbols defined within
// the input text, to populate the symtab. it also
// calculate the size of the program-bytestream when assembled.
void
Assembler::
first_stage_pass()
{
	bool option_tbl[ASM_ARGV_OPT_COUNT] = { false };
	ImportCard* import_card;
	Assembler* import_asm;
	Symbol* sym;
	toknum = 0;

	while (toknum < parser->tokcount)
	{
		// currtok is set via the toknum index var.
		// upon each iteration of this while loop first
		// thing to happen is currtok is set as follows:
		
		// currtok is always being set to an opcode token here.
		currtok = parser->tokstream->vec->at(toknum);

		switch (currtok->type)
		{
			case OPCODE_TOK:
				next_byte_addr += OPCODE_SIZE;
				prog_size      += OPCODE_SIZE;
				toknum++; // toknum is pointing to next token after the opcode token.
				instr_count++;

				switch (currtok->val.ubyteval)
				{
					// one uint operand instructions.
				case perr_op:      case systime_op: case htime_op:      case getutc_op:    case getlocal_op: case waituntil_op: case elapsed_op: case getweekday_op: case monthdays_op: case normtime_op:
				case delay_op:     case ftel_op:    case rwnd_op:       case sputs_op:     case sgets_op:    case serr_op:      case sdup_op:    case call_op:       case jmp_op:       case jz_op:
				case jnz_op:       case je_op:      case jn_op:         case jl_op:        case jg_op:       case jls_op:       case jgs_op:     case pshu_op:       case popnu_op:     case pshfru_op:
				case poptru_op:    case movtru_op:  case stktru_op:     case pshi_op:      case pshfri_op:   case poptri_op:    case movtri_op:  case stktri_op:     case pshr_op:      case pshfrr_op:
				case poptrr_op:    case movtrr_op:  case stktrr_op:     case slen_op:      case fcls_op:
					count_uint_operand();
					continue;

					// three uint operand instructions.
				case frd_op:  case fwr_op:  case fprntf_op: case sprntf_op: case sscnf_op: case sncm_op: case mcpy_op: case mmov_op: case sncy_op:   case snct_op:   case mcmp_op:  case mset_op:
					count_uint_operand();
					count_uint_operand();
					count_uint_operand();
					continue;

					// two uint operand instructions.
				case timeadd_op:  case timesub_op: case settime_op: case tstampstr_op: case cmptime_op: case timediff_op: case fopn_op:    case fsk_op:     case prntf_op:     case scnf_op:
				case fgts_op:     case fpts_op:    case scpy_op:    case scat_op:      case scmp_op:    case schr_op:     case srch_op:    case sstr_op:    case stok_op:      case sspn_op:
				case scspn_op:    case sfrm_op:    case cpyru_op:   case setru_op:     case cpyri_op:   case setri_op:    case cpyrr_op:   case setrr_op:
					count_uint_operand();
					count_uint_operand();
					continue;

					// no operand instructions.
				case die_op:    case nop_op:   case stopprof_op: case startprof_op: case brkp_op: case nspctr_op: case ret_op:   case swtch_op:    case popu_op:      case pop2u_op:
				case incu_op:   case decu_op:  case addu_op:     case subu_op:      case mulu_op: case divu_op:   case modu_op:  case inci_op:     case deci_op:      case addi_op:
				case subi_op:   case muli_op:  case divi_op:     case modi_op:      case addr_op: case subr_op:   case mulr_op:  case divr_op:     case modr_op:      case sqrt_op:
				case ceil_op:   case floor_op: case sin_op:      case cos_op:       case tan_op:  case powu_op:   case pows_op:  case powr_op:     case absu_op:      case abss_op:
				case absr_op:   case fabs_op:
					continue;
				}

			case MACRO_DEF_TOK:
				// advance to next tok, should be an id-tok.
				currtok = parser->tokstream->vec->at(++toknum);

				if (currtok->type != ID_TOK)
					throw_error_here(currtok, "identifier missing from macro definition");

				else if (symtab->id_inuse(currtok->srcstr))
					throw_error_here(currtok, "symbol with same id already exists");
				

				// create the macro symbol object in the symtab.
				sym = symtab->create_new_symbol(MACRO_SYMBOL, generate_symbol_id(currtok->srcstr));

				// advance to next tok, should be any value token.
				currtok = parser->tokstream->vec->at(++toknum);

				// cases at top of this switch are the valid value cases
				// for the macro's numeric value. cases below that are
				// error cases to catch token's that shouldn't be
				// encountered here(only value tokens should be).
				switch (currtok->type)
				{
					// all int typed tokens.
					case OCT_TOK:
					case HEX_TOK:
					case INT_TOK:
						sym->val_type = INT_TYPE;
						sym->val.intval = currtok->val.intval;
						sym = NULL;
						break;

					// addresses are a special type when used within macros.
					// when the value for a macro is from an address-token
					// the symbol's datatype becomes addr meaning it will
					// have it's final offset calculated by calculate_addr_offsets().
					case ADDR_TOK:
						sym->val_type = ADDR_TYPE;
						sym->val.uintval = currtok->val.uintval;
						sym = NULL;
						break;

					// all uint typed tokens.
					case UOCT_TOK:
					case UHEX_TOK:
					case UINT_TOK:
					case UBYTE_TOK:
						sym->val_type = UINT_TYPE;
						sym->val.uintval = currtok->val.uintval;
						sym = NULL;
						break;

					// all floating point typed tokens.
					case REAL_TOK:
					case UREAL_TOK:
						sym->val_type = REAL_TYPE;
						sym->val.realval = currtok->val.realval;
						sym = NULL;
						break;

					// identifiers are caught here. at this point
					// they are simply added as id's then on symtab
					// finalisation their actual values will be looked
					// up. if id isn't a valid symbol it's a token error.
					case ID_TOK:
						sym->val_type = ID_TYPE;
						sym->val.strval = currtok->val.strval;
						sym = NULL;
						break;

					case STRING_TOK:
						sym->val_type = STRING_TYPE;
						sym->val.strval = currtok->val.strval;
						sym = NULL;
						break;

					// invalid macro value tokens.
					// anything not caught by the default label has it's own
					// error-msg being passed to tok_validate_err().
					case COMMENT_TOK:
						throw_error_here(currtok, "\ncomment must come after a complete label/macro definition or instruction");

					case BRKPNT_TOK:
						throw_error_here(currtok, "\nbreakpoint must be on it's own line");

					default:
						throw_error_here(currtok, "\nmacro definition requires a valid value (ubyte, uint, int, real, hex, octal, char*, id)");
				}

				// point toknum at the first token after the macro-def.
				toknum++;
				continue;

			case LABEL_DEF_TOK:
				// label identifier string is in the label-def-token, in it's srcstr var.
				if (symtab->id_inuse(currtok->srcstr))
					throw_error_here(currtok, "symbol with same id already exists");

				// create the label symbol object in the symtab.
				sym = symtab->create_new_symbol(LABEL_SYMBOL, generate_symbol_id(currtok->srcstr));

				// set the label's address value.
				sym->val.uintval = next_byte_addr;
				sym->val_type    = UINT_TYPE;

				// point toknum at the first token after the label-def.
				toknum++;
				continue;

			case IMPORT_DEF_TOK:
				// get ourselves a ptr to the import's path token.
				currtok = parser->tokstream->vec->at(++toknum);

				// check for invalid tokens.
				if (currtok->type != PATH_TOK)
				{
					printf("\n major error inside! import path invalid");
					exit(1);
				}

				// ok at this point i *think* all paths will be abs
				// but might have to convert to abs, job for another day..
				//
				// anyway lets check if module is already on import-list.
				// if it is simply skip importing it.
				if (module_is_imported(currtok->srcstr))
				{
					toknum++;
					continue;
				}

				// create import-card and assembler object for the to-be imported module.
				try
				{ 
					import_asm  = new Assembler(option_tbl, currtok->srcstr, (char*)"NO-OUTPUT-PATH", import_list, prog_size, next_byte_addr);
					import_card = new ImportCard(prog_size, next_byte_addr);
				}

				catch (const std::bad_alloc& e)
				{
					throw AssemblerError("\nnew failed in Assembler::first_stage_pass()\n[import_card = new ImportCard();]");
				}

				import_list->push_back(import_card);
				import_card->imform = import_asm->imform;
				import_card->imform->Asm = import_asm;

				// parse module into token-stream then do first-pass
				// on it to calculate size, import modules, build symbol-table.
				import_asm->parser->parse_file(import_asm->input_path);
				import_asm->first_stage_pass(); 

				// each module merges the symtabs of all imported modules in that file.
				// the main-module simply iterates through the import-list merging each
				// import-cards symtab with the main-module.
				import_asm->merge_import_symbols(); 

				prog_size      = import_asm->prog_size;
				next_byte_addr = import_asm->next_byte_addr;

				import_card->insertion_index = instr_count;
				instr_count += import_asm->instr_count;

				toknum++; // increment toknum past the path token. then continue.
				continue;

			case COMMENT_TOK:
			case BRKPNT_TOK:
				toknum++;
				continue;

			case OCT_TOK:
			case HEX_TOK:
			case UINT_TOK:
			case INT_TOK:
			case UBYTE_TOK:
			case REAL_TOK:
			case ADDR_TOK:
			case ID_TOK:
			case STRING_TOK:
				throw_error_here(currtok, "value tokens must be within an instruction or macro definition");
		}
	}
}

void
OperandResolver::
push_operator(Token* token)
{
	try
	{
		// create the new operator's node object making currtok it's owner.
		*(++opstk_top) = new Node(this, token, OPNODE, val_count);
		++op_count;
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in EvalStack::push_value()\n[*(++opstk_top) = new Node(currtok, OPNODE);]", (u8)AsmErrorCode::NODE_NEW_ERR);
	}
}

void
OperandResolver::
push_value(Token* token)
{
	try
	{
		// create the new operator's node object making currtok it's owner.
		*(++valstk_top) = new Node(this, token, VALNODE, val_count, token->val_type);
		++val_count;
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in EvalStack::push_value()\n[*(++valstk_top) = new Node(currtok, OPNODE);]", (u8)AsmErrorCode::NODE_NEW_ERR);
	}
}

void
OperandResolver::
push_value(Value* val,
	       u8     datatype)
{
	try
	{
		// create the new operator's node object making currtok it's owner.
		*(++valstk_top) = new Node(this, NULL, VALNODE, val_count, datatype);
		++val_count;
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in EvalStack::push_value()\n[*(++valstk_top) = new Node(currtok, OPNODE);]", (u8)AsmErrorCode::NODE_NEW_ERR);
	}
}

void
OperandResolver::
popdel_operator()
{
	delete *(opstk_top--);
	--op_count;
}

Node*
OperandResolver::
pop_operator()
{
	--op_count;
	return *(opstk_top--);
}

void
OperandResolver::
popdel_value()
{
	delete *(valstk_top--);
	--val_count;
}

Node*
OperandResolver::
pop_value()
{
	--val_count;
	return *(valstk_top--);
}

void
OperandResolver::
resolve_uint_operand(InstrBlock* instr,
	                 const u8    argnum)
{
	bool   last_tok_is_value = false;
	u32    final_operand_value = 0;
	size_t toks_parsed = 0;

	// token processing loop.
	for (;;)
	{
	token_processing_loop:
		asmobj->currtok = asmobj->parser->tokstream->vec->at(++asmobj->toknum);
		++toks_parsed;

		// confirm this is not the last token in the stream if so we have an error,
		// a missing expr ending token.
		//if ((asmobj->currtok->ndx) == ((asmobj->parser->tokcount) - 1))
		//	asmobj->throw_error_here(asmobj->currtok, "misformed expression");

		switch (asmobj->currtok->type)
		{
			case OPCODE_TOK:
			case MACRO_DEF_TOK:
			case LABEL_DEF_TOK:
				--asmobj->toknum; // this keeps toknum synced.
				goto token_parsing_complete;

			case EXPREND_TOK:
				last_tok_is_value = false;

				// offset & paren expr must be closed at this point.
				if (inside_paren)
					asmobj->throw_error_here(asmobj->currtok, "misformed expression, missing closing right paren");

				else if (inside_offset)
					asmobj->throw_error_here(asmobj->currtok, "misformed expression, missing closing right bracket");

				//asmobj->toknum++; // increment toknum past the EXPREND_TOK token.
				goto token_parsing_complete;

			case ADDOP_TOK:
			case SUBOP_TOK:
			case MULOP_TOK:
			case DIVOP_TOK:
			case MODOP_TOK:
				last_tok_is_value = false;

				// if theres an operator on the opstack then process opstack accordingly.
				if (op_count)
				{
					if ((*opstk_top)->op_prec >= get_tok_precedence(asmobj->currtok))
						eval_opstack_top();
				}

				// push new operator onto opstack.
				push_operator(asmobj->currtok);
				continue;

			case RBRKTOP_TOK:
				last_tok_is_value = false;

				if (!inside_offset)
					asmobj->throw_error_here(asmobj->currtok, "misformed expression, missing left paren");

				eval_opstack_until(LBRKTOP_TOK);

				// reaching here signals end of offset expression.
				goto token_parsing_complete;

			case RPRN_TOK:
				last_tok_is_value = false;

				if (!inside_paren)
					asmobj->throw_error_here(asmobj->currtok, "misformed expression, missing left paren");

				eval_opstack_until(LPRN_TOK);
				continue;

			case LBRKTOP_TOK:
				last_tok_is_value = false;
				inside_offset = true;
				push_operator(asmobj->currtok);
				continue;

			case LPRN_TOK:
				last_tok_is_value = false;
				inside_paren = true;
				push_operator(asmobj->currtok);
				continue;

			case OCT_TOK:
			case HEX_TOK:
			case UHEX_TOK:
			case UINT_TOK:
			case INT_TOK:
			case UBYTE_TOK:
			case REAL_TOK:
			case ADDR_TOK:
			case ID_TOK:
				// check if last token was a value token.
				if (last_tok_is_value)
				{
					// check if that wasn't the first token parsed,
					// meaning we have an error, two values sitting like this "5 7"
					// without any operator in-between. this is only valid like this:
					// loop 50 40 50, so u look at 50, look at 40, bam 50 is the operand.
					if (toks_parsed != 2)
						asmobj->throw_error_here(asmobj->currtok, "misformed expression, adjecent values missing operator");

					asmobj->toknum--; // toknum must be pointing to the last tok of the operand here.
					goto token_parsing_complete;
				}

				// push value onto value stack.
				push_value(asmobj->currtok);
				last_tok_is_value = true;
				continue;
		}
	}

	token_parsing_complete:
	// token parsing is complete, now evaluate the opstack
	// if any operators are on it still.
	while (op_count) eval_opstack_top();

	// are we inside an array expression?
	if (inside_offset)
	{
		// confirm there's exactly two value nodes.
		if (val_count != 2)
			asmobj->throw_error_here(asmobj->currtok, "misformed expression, invalid array expression");

		// set the instruction offset value.
		instr->pos_offset[argnum]->uintval = (*valstk_top)->val.uintval;

		// pop the offset value off the value-stack.
		--valstk_top;
		--val_count;
	}

	// confirm there's only 1 value node remaining on the value stack.
	if (val_count != 1)
		asmobj->throw_error_here(asmobj->currtok, "misformed expression, invalid array expression");

	// at this point toknum must be set to the last token of the operand.
	instr->val[argnum]->uintval = (*valstk_top)->val.uintval;
	(*(instr->val_type + argnum)) = UINT_TYPE;


	// reset the eval-stack so it's ready for use by another operand.
	reset();

	// incrementing toknum here makes it so it is pointing to the first token
	// after the last token of the operand. 
	asmobj->toknum++;
}


OperandResolver::
OperandResolver(Assembler* asm_owner_obj)
	:
	opstk_top(opstk), valstk_top(valstk),
	op_count(0), val_count(0), asmobj(asm_owner_obj),
	inside_offset(false), inside_paren(false)
{}

OperandResolver::
~OperandResolver()
{
	// ensure any remaining objects on both stacks are deallocated.
	while (op_count) popdel_operator();
	while (val_count) popdel_value();
}

// evaluates the top operator on the opstack.
void
OperandResolver::
eval_opstack_top()
{
	Node* result_node;
	Value result_val;
	Value left_val;
	Value right_val;

	// switch on the top operator's parent-tok type which
	// is it's operation-type-code.
	switch ((*opstk_top)->parent_tok->type)
	{
	case ADDOP_TOK:
		// confirm there's enough value node's on the valstack for the operation.
		if (val_count < 2)
			asmobj->throw_error_here((*opstk_top)->parent_tok, "misformed expression");

		// set right / left operation operand values.
		right_val = (*valstk_top)->val;
		left_val = (*(valstk_top - 1))->val;

		// since the left operand ends up being result node,
		// the result datatype is the left operand datatype.
		switch ((*(valstk_top - 1))->val_type)
		{
		case UINT_TYPE:
		case ADDR_TYPE:
		case UBYTE_TYPE:
			result_val.uintval = left_val.uintval + right_val.uintval;
			break;

		case INT_TYPE:
			result_val.intval = left_val.intval + right_val.intval;
			break;

		case REAL_TYPE:
			result_val.realval = left_val.realval + right_val.realval;
			break;
		}

		// pop the operator from opstack and the top value of valstack.
		// which is the right operand off the add operation. left operand
		// will be left on the val-stack and become to result value node.
		// simulating the popping of the two operands and pushing of the result.
		popdel_operator();
		popdel_value();
		result_node = *valstk_top;
		result_node->val = result_val;
		break;

	case SUBOP_TOK:
		if (val_count < 2)
			asmobj->throw_error_here((*opstk_top)->parent_tok, "misformed expression");

		left_val = (*valstk_top)->val;
		right_val = (*(valstk_top - 1))->val;

		switch ((*(valstk_top - 1))->val_type)
		{
		case UINT_TYPE:
		case ADDR_TYPE:
		case UBYTE_TYPE:
			result_val.uintval = left_val.uintval - right_val.uintval;
			break;

		case INT_TYPE:
			result_val.intval = left_val.intval - right_val.intval;
			break;

		case REAL_TYPE:
			result_val.realval = left_val.realval - right_val.realval;
			break;
		}

		popdel_operator();
		popdel_value();
		result_node = *valstk_top;
		result_node->val = result_val;
		break;

	case MULOP_TOK:
		if (val_count < 2)
			asmobj->throw_error_here((*opstk_top)->parent_tok, "misformed expression");

		left_val = (*valstk_top)->val;
		right_val = (*(valstk_top - 1))->val;

		switch ((*(valstk_top - 1))->val_type)
		{
		case UINT_TYPE:
		case ADDR_TYPE:
		case UBYTE_TYPE:
			result_val.uintval = left_val.uintval * right_val.uintval;
			break;

		case INT_TYPE:
			result_val.intval = left_val.intval * right_val.intval;
			break;

		case REAL_TYPE:
			result_val.realval = left_val.realval * right_val.realval;
			break;
		}

		popdel_operator();
		popdel_value();
		result_node = *valstk_top;
		result_node->val = result_val;
		break;

	case DIVOP_TOK:
		if (val_count < 2)
			asmobj->throw_error_here((*opstk_top)->parent_tok, "misformed expression");

		left_val = (*valstk_top)->val;
		right_val = (*(valstk_top - 1))->val;

		switch ((*(valstk_top - 1))->val_type)
		{
		case UINT_TYPE:
		case ADDR_TYPE:
		case UBYTE_TYPE:
			result_val.uintval = left_val.uintval / right_val.uintval;
			break;

		case INT_TYPE:
			result_val.intval = left_val.intval / right_val.intval;
			break;

		case REAL_TYPE:
			result_val.realval = left_val.realval / right_val.realval;
			break;
		}

		popdel_operator();
		popdel_value();
		result_node = *valstk_top;
		result_node->val = result_val;
		break;

	case MODOP_TOK:
		if (val_count < 2)
			asmobj->throw_error_here((*opstk_top)->parent_tok, "misformed expression");

		left_val = (*valstk_top)->val;
		right_val = (*(valstk_top - 1))->val;

		switch ((*(valstk_top - 1))->val_type)
		{
		case UINT_TYPE:
		case ADDR_TYPE:
		case UBYTE_TYPE:
			result_val.uintval = left_val.uintval % right_val.uintval;
			break;

		case INT_TYPE:
			result_val.intval = left_val.intval % right_val.intval;
			break;
		}

		popdel_operator();
		popdel_value();
		result_node = *valstk_top;
		result_node->val = result_val;
		break;
	}

}

// evaluates the operator stack until
// the operator specified by the toktype
// variable is encountered. if not encountered
// opstack is evaled until no ops remain.
// otherwise opstack is evaled until target is
// reached, target is popped.
// function return true if the target was found
// or false if it was not.
bool
OperandResolver::
eval_opstack_until(u8 toktype) 
{
	while (op_count)
	{
		if ((*opstk_top)->parent_tok->type == toktype)
		{
			popdel_operator();
			return true;
		}

		eval_opstack_top();
	}

	// target operator wasn't found before the opstack was fully evaluated.
	return false;
}

// function to reset the state of eval state
// so it's ready for evaluating a new expression.
void
OperandResolver::
reset()
{
	// ensure any remaining objects on both
	// stacks are deallocated.
	while (op_count) popdel_operator();
	while (val_count) popdel_value();

	// reset any other vars.
	opstk_top = opstk;
	valstk_top = valstk;
	inside_offset = false;
	inside_paren = false;
}

void
Assembler::
build_imform_metadata()
{
	imform->metadata->start_instr_addr = symtab->start_addr;
	imform->metadata->mem_size = bytestream_size;
	imform->metadata->prog_size = prog_size;
	imform->metadata->instr_count = instr_count;
	imform->metadata->first_user_byte_addr = METADATA_SIZE + symtab->bytestream_size + prog_size;
	imform->metadata->last_instr_addr = (imform->metadata->first_user_byte_addr) - (opsize_tbl[parser->last_opcode]);
	imform->metadata->first_instr_addr = (imform->metadata->first_user_byte_addr) - prog_size;
	imform->metadata->flags = build_flags_u32();
	imform->metadata->creation_date = get_packed_datestamp();

	// ensure prog's first-instr starts at address aligned to u32 boundry(divible by 4).
	if (imform->metadata->first_instr_addr % 4 != 0)
		imform->metadata->first_instr_addr += 4 - (imform->metadata->first_instr_addr % 4);

	if (imform->metadata->last_instr_addr % 4 != 0)
		imform->metadata->last_instr_addr += 4 - (imform->metadata->last_instr_addr % 4);

	if (imform->metadata->first_user_byte_addr % 4 != 0)
		imform->metadata->first_user_byte_addr += 4 - (imform->metadata->first_user_byte_addr % 4);
}

// build the intermediate-form of the program which are instr datastructures that contain the opcode
// and any operands, but the final value. the program just needs to be translated into runnable code.
void
Assembler::
build_im_form()
{
	InstrBlock* instr = NULL;

	// iterate through the token stream translating them into intermediate-form.
	toknum = 0;
	while (toknum < parser->tokcount)
	{
		currtok = parser->tokstream->vec->at(toknum);

		switch (currtok->type)
		{
			case IMPORT_DEF_TOK:
				currtok = parser->tokstream->vec->at(++toknum);
				//if (currtok->type != ID_TOK)
					// THROW ERROR

				++toknum;
				continue;

			case OPCODE_TOK:
				// create new instruction block object in im-form object.
				instr = imform->new_instr(currtok->val.ubyteval);

				// handle building of specific instructions, all of them will be built within this switch.
				switch (currtok->val.ubyteval)
				{
					// one uint operand instructions.
				case perr_op:      case systime_op: case htime_op:      case getutc_op:    case getlocal_op:
				case waituntil_op: case elapsed_op: case getweekday_op: case monthdays_op: case normtime_op:
				case delay_op:     case ftel_op:    case rwnd_op:       case sputs_op:     case sgets_op:
				case serr_op:      case sdup_op:    case call_op:       case jmp_op:       case jz_op:
				case jnz_op:       case je_op:      case jn_op:         case jl_op:        case jg_op:
				case jls_op:       case jgs_op:     case pshu_op:       case popnu_op:     case pshfru_op:
				case poptru_op:    case movtru_op:  case stktru_op:     case pshi_op:      case pshfri_op:
				case poptri_op:    case movtri_op:  case stktri_op:     case pshr_op:      case pshfrr_op:
				case poptrr_op:    case movtrr_op:  case stktrr_op:     case slen_op:      case fcls_op:
					resolver->resolve_uint_operand(instr, 1);
					break;

					// three uint operand instructions.
				case frd_op:  case fwr_op:  case fprntf_op: case sprntf_op: case sscnf_op: case sncm_op:
				case mcpy_op: case mmov_op: case sncy_op:   case snct_op:   case mcmp_op:  case mset_op:
					resolver->resolve_uint_operand(instr, 1);
					toknum--;
					resolver->resolve_uint_operand(instr, 2);
					toknum--;
					resolver->resolve_uint_operand(instr, 3);
					break;

					// two uint operand instructions.
				case timeadd_op:  case timesub_op: case settime_op: case tstampstr_op: case cmptime_op:
				case timediff_op: case fopn_op:    case fsk_op:     case prntf_op:     case scnf_op:
				case fgts_op:     case fpts_op:    case scpy_op:    case scat_op:      case scmp_op:
				case schr_op:     case srch_op:    case sstr_op:    case stok_op:      case sspn_op:
				case scspn_op:    case sfrm_op:    case cpyru_op:   case setru_op:     case cpyri_op:
				case setri_op:
				case cpyrr_op:
				case setrr_op:
					resolver->resolve_uint_operand(instr, 1);
					resolver->resolve_uint_operand(instr, 2);
					break;

					// no operand instructions.
				case die_op:    case nop_op:   case stopprof_op: case startprof_op: case brkp_op:
				case nspctr_op: case ret_op:   case swtch_op:    case popu_op:      case pop2u_op:
				case incu_op:   case decu_op:  case addu_op:     case subu_op:      case mulu_op:
				case divu_op:   case modu_op:  case inci_op:     case deci_op:      case addi_op:
				case subi_op:   case muli_op:  case divi_op:     case modi_op:      case addr_op:
				case subr_op:   case mulr_op:  case divr_op:     case modr_op:      case sqrt_op:
				case ceil_op:   case floor_op: case sin_op:      case cos_op:       case tan_op:
				case powu_op:   case pows_op:  case powr_op:     case absu_op:      case abss_op:
				case absr_op:   case fabs_op:
					++toknum; // make sure toknum is pointing at the next token.
					break;
				}

				continue;

			default:
				toknum++;
		}
	}

	imform->metadata->instr_count = imform->instr_vec->size();
}

InstrBlock*
IMForm::
new_instr(u8 _opcode)
{
	InstrBlock* instr;

	try
	{
		instr = new InstrBlock(_opcode, instr_vec->size());
		instr_vec->push_back(instr);
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in IMForm::new_instr()\n[instr = new InstrBlock();]", (u8)AsmErrorCode::BUILD_IM_NEW_ERR);
	}

	return instr;
}

// writes program-bytestream to output file.
void
Assembler::
write_prog_bytestream()
{
    size_t bytes_written = 0;

    output_file   = fopen(output_path, "wb");
    bytes_written = fwrite(prog_bytestream, 1, bytestream_size, output_file);
    fclose(output_file);

	// perform all optional actions.
    printf("\nassembly complete.");

    if (option_tbl[KEEP_SYMBOLS_OPT])
        printf(" symbols kept.\n");

    printf(ASM_COMPLETE_INFOSTR, bytes_written, bytestream_size, output_path);
}

void
InstrBlock::
write_operand(const u8 datatype,
	          const u8 oprnum,
	               u8* buff_)
{
	apply_offset(datatype, oprnum);

	switch (datatype)
	{
		case UINT_TYPE:
			*((u32*)buff_) = val[oprnum]->uintval;
			buff_ += UINT_SIZE;
			break;

		case INT_TYPE:
			*((i32*)buff_) = val[oprnum]->intval;
			buff_ += INT_SIZE;
			break;

		case UBYTE_TYPE:
			*buff_ = val[oprnum]->ubyteval;
			buff_ += UBYTE_SIZE;
			break;

		case REAL_TYPE:
			*((f32*)buff_) = val[oprnum]->realval;
			buff_ += REAL_SIZE;
			break;

		default:
			throw AssemblerError("internal error in InstrBlock::write_operand()");
	}
}


void
InstrBlock::
serialise(u8* buff_)
{
	// write the opcode.
	*buff_ = opcode;
	buff_ += OPCODE_SIZE;

	// switch on the instr's opcode.
	switch (opcode)
	{
		// one uint operand instructions.
	case perr_op:
	case systime_op:
	case htime_op:
	case getutc_op:
	case getlocal_op:
	case delay_op:
	case waituntil_op:
	case elapsed_op:
	case getweekday_op:
	case monthdays_op:
	case normtime_op:
	case ftel_op:
	case rwnd_op:
	case sputs_op:
	case sgets_op:
	case serr_op:
	case sdup_op:
	case call_op:
	case jmp_op:
	case jz_op:
	case jnz_op:
	case je_op:
	case jn_op:
	case jl_op:
	case jg_op:
	case jls_op:
	case jgs_op:
	case pshu_op:
	case popnu_op:
	case pshfru_op:
	case poptru_op:
	case movtru_op:
	case stktru_op:
	case pshi_op:
	case pshfri_op:
	case poptri_op:
	case movtri_op:
	case stktri_op:
	case pshr_op:
	case pshfrr_op:
	case poptrr_op:
	case movtrr_op:
	case stktrr_op:
	case slen_op:
	case fcls_op:
		write_operand(UINT_TYPE, 1, buff_);
		break;

		// three uint operand instructions.
	case frd_op:
	case fwr_op:
	case fprntf_op:
	case sprntf_op:
	case sscnf_op:
	case mcpy_op:
	case mmov_op:
	case sncy_op:
	case snct_op:
	case mcmp_op:
	case sncm_op:
	case mset_op:
		write_operand(UINT_TYPE, 1, buff_);
		write_operand(UINT_TYPE, 2, buff_);
		write_operand(UINT_TYPE, 3, buff_);
		break;

		// two uint operand instructions.
	case timeadd_op:
	case timesub_op:
	case settime_op:
	case tstampstr_op:
	case cmptime_op:
	case timediff_op:
	case fopn_op:
	case fsk_op:
	case prntf_op:
	case scnf_op:
	case fgts_op:
	case fpts_op:
	case scpy_op:
	case scat_op:
	case scmp_op:
	case schr_op:
	case srch_op:
	case sstr_op:
	case stok_op:
	case sspn_op:
	case scspn_op:
	case sfrm_op:
	case cpyru_op:
	case setru_op:
	case cpyri_op:
	case setri_op:
	case cpyrr_op:
	case setrr_op:
		write_operand(UINT_TYPE, 1, buff_);
		write_operand(UINT_TYPE, 2, buff_);
		break;

		// no operand instructions.
	case die_op:
	case nop_op:
	case stopprof_op:
	case startprof_op:
	case brkp_op:
	case nspctr_op:
	case ret_op:
	case swtch_op:
	case popu_op:
	case pop2u_op:
	case incu_op:
	case decu_op:
	case addu_op:
	case subu_op:
	case mulu_op:
	case divu_op:
	case modu_op:
	case inci_op:
	case deci_op:
	case addi_op:
	case subi_op:
	case muli_op:
	case divi_op:
	case modi_op:
	case addr_op:
	case subr_op:
	case mulr_op:
	case divr_op:
	case modr_op:
	case sqrt_op:
	case ceil_op:
	case floor_op:
	case sin_op:
	case cos_op:
	case tan_op:
	case powu_op:
	case pows_op:
	case powr_op:
	case absu_op:
	case abss_op:
	case absr_op:
	case fabs_op:
		break;
	}
}

u32
Assembler::
build_flags_u32()
{
	return (u32)option_tbl[KEEP_SYMBOLS_OPT];
}


// loads program's metadata segment into the program-bytestream.
void
Assembler::
build_metadata_segment()
{
	u32* u32p;
	u16* u16p;

	u32p = (u32*)(prog_bytestream + MD_START_INSTR_ADDR_OFFSET);
	*u32p = imform->metadata->start_instr_addr;

	u32p = (u32*)(prog_bytestream + MD_MEMORY_SIZE_OFFSET);
	*u32p = imform->metadata->mem_size;

	u32p = (u32*)(prog_bytestream + MD_PROG_SIZE_OFFSET);
	*u32p = imform->metadata->prog_size;

	u32p = (u32*)(prog_bytestream + MD_INSTR_COUNT_OFFSET);
	*u32p = imform->metadata->instr_count;

	u32p = (u32*)(prog_bytestream + MD_FIRST_INSTR_ADDR_OFFSET);
	*u32p = imform->metadata->first_instr_addr;

	u32p = (u32*)(prog_bytestream + MD_LAST_INSTR_ADDR_OFFSET);
	*u32p = imform->metadata->last_instr_addr;

	u32p = (u32*)(prog_bytestream + MD_FIRST_USER_BYTE_ADDR_OFFSET);
	*u32p = imform->metadata->first_user_byte_addr;

	u32p = (u32*)(prog_bytestream + MD_FLAGS_OFFSET);
	*u32p = imform->metadata->flags;

	u32p = (u32*)(prog_bytestream + MD_CREATION_DATE_OFFSET);
	*u32p = imform->metadata->creation_date;

}

// responsible for calculating the sizeof and allocating
// the prog-bytestream which will be the output of the asm.
//
// alloc_bytestream() *must* only be called once prog_size
// Assembler() var has been calculated and set.
void
Assembler::
alloc_bytestream()
{
	// calculate the final size for the program's memory block.
	bytestream_size += METADATA_SIZE;
	bytestream_size += symtab->serial_size();
	bytestream_size += prog_size;

	// add the imported modules size to the main size.
	for (u32 i = 0; i < import_list->size(); i++)
		bytestream_size += import_list->at(i)->imform->metadata->prog_size;

	// allocate the program's memory block(prog-bytestream).
	prog_bytestream = (u8*) malloc(bytestream_size);

	if (!prog_bytestream)
		throw AssemblerError("\nnew failed in Assembler::alloc_bytestream()\n[prog_bytestream = (u8*) malloc(bytestream_size);]", (u8)AsmErrorCode::PROG_BYTESTREAM_MALLOC);

	imform->prog_bytestream = prog_bytestream;
}

void
IMForm::
write_bytes_to_file(char* output_path_,
	                bool  print_info = false)
{
	if (!mdata_byte_serialised)
		throw AssemblerError("\ncan't write program to file, metadata isn't serilaised, in: write_prog_to_file::IMForm()\n[if (!mdata_byte_serialised)]");

	if (symbols_kept)
	{
		if (!symtab_byte_serialised)
			throw AssemblerError("\ncan't write program to file, symtab isn't serilaised, in: write_prog_to_file::IMForm()\n[(!symtab_byte_serialised)]");
	}

	if (!instrs_byte_serialised)
		throw AssemblerError("\ncan't write program to file, instruction-blocks aren't serilaised, in: write_prog_to_file::IMForm()\n[if (!mdata_byte_serialised)]");

	if (!is_valid_filepath(output_path_, OUTPUT_FILE_EXTENSION))
	{
		printf("\ninvalid output path, %s", output_path);
		throw AssemblerError("\ncan't write program to file, output file path isn't valid, in: write_prog_to_file::IMForm()\n[if (!is_valid_filepath(input_path, OUTPUT_FILE_EXTENSION))]");
	}

	output_path = output_path_;
	size_t bytes_written = 0;
	FILE* output_file = fopen(output_path, "wb");
	bytes_written = fwrite(prog_bytestream, 1, metadata->mem_size, output_file);
	fclose(output_file);

	if (print_info)
		printf("%zu instructions. %zu of %u bytes written to [%s]", metadata->instr_count, bytes_written, metadata->mem_size, output_path);
}

void
IMForm::
read_prog_from_file(char* input_path_)
{
	u8* bf = NULL;

	input_path = input_path_;

	// check input path is valid. will check if it exists when it is opened.
	if (!is_valid_filepath(input_path, INPUT_FILE_EXTENSION))
	{
		printf("\ninvalid input path, %s", input_path);
		// ERROR HANDLING HERE!!
		exit(1);
	}

	// build imform file-data obj.
	build_infdata(input_path_);

	size_t file_size;
	size_t bytes_read;

	FILE* input_file = fopen(input_path, "rb");

	if (input_file == NULL)
		exit(1);

	fseek(input_file, 0, SEEK_END);
	file_size = ftell(input_file);
	rewind(input_file);

	prog_bytestream = (u8*)malloc(file_size);
	if (!prog_bytestream)
		exit(1);

	bytes_read = fread(prog_bytestream, sizeof(u8), file_size, input_file);

	if (bytes_read != file_size)
		exit(1);

	fclose(input_file);

	read_metadata_from_bytes(prog_bytestream);

	if (symbols_kept)
		read_symtab_from_bytes(prog_bytestream + METADATA_SIZE);

	read_program_from_bytes(prog_bytestream + (metadata->first_instr_addr));

}

void
Assembler::
merge_import_list()
{
	for (u32 i = 0; i < import_list->size(); i++)
		imform->merge_imform(import_list->at(i));
}

// Assembly:
//	* parse file text into a token-stream.
//
//  * do first_stage_pass() on the token-stream to
//     - determine program exact size in bytes.
//     - create to populate symtab based on the token-stream.
//     - perform some initial token validation.
//
//  * calculate bytestream size then allocate required memory for it.
//
//  * symtab->finalise() is called which finalises the symtab, all
//    memory offsets are calculated and the symtab is serialised
//    to the program bytestream if symbols are being kept.
//
//  * build_im_form() takes the token-stream and converts it into a
//    IMForm() object, intermediate form object which is a vector
//    of instruction-block objects where each instr block contains
//    the final values for it to be written as vm code or whatever.
//
//  * the im-form object's serialise() function is called which writes
//    the instructions to a passed pointer which is the prog-bytestream.
//
//  * finally Assembler() object's function write_prog_bytestream() is
//    called to perform the final write to the specified output file.
//
//  * next all of the specified optional tasks are performed.
void
Assembler::
assemble_main_module()
{
	// produce a token-stream from the input file.
	parser->parse_file(input_path);

	// peform the first pass of the input text, tokens are validated and all symbols created.
	// program size is also calculated here. import-card-table for modules is built here.
	// for each import stmt an ImportCard is created, containing an insertion index and an
	// imform() object created from the path-token that came after the import-token.
	first_stage_pass();

	// any modules that a module imports will have their symtab merged with their importer's
	// symtab. in short: each module's symtab contains the symbols of all imported modules.
	// at this point, main-module has created it's own symtab it must now merge the symtab
	// from each import-card in the import-list.
	merge_import_symbols();

	alloc_bytestream();
	build_imform_metadata();

	// at this point the entire input text has been processed for symbol declarations.
	// finalise the symtab which serialises it and calculates all the address offets ect.
	// all values are resolved, symtab is 100% accurate and complete after finalise() is called.
	symtab->finalise(prog_bytestream + SYMTAB_DATA_OFFSET, imform);
	imform->symtab_byte_serialised = true;

	// build the core imform for the program.
	// build intermediate-form which is a vector of instr-block objects.
	// these objects hold the final information to write an instr to a bytestream.
 	build_im_form();

	// core imform is built now we can insert the import imforms now.
	merge_import_list();

	// serialise im form to prog-bytestream as per the .fbin memory map.
	imform->write_prog_to_bytes(prog_bytestream);

	// at this point the input has been translated from text into a token stream,
	// token-stream into intermediate-form, from im-form into binary form contained
	// token-stream into intermediate-form, from im-form into binary form contained
	// in the perfectly sized memory block pointed to by prog_bytestream.

	// now actual perform the final write of prog_bytestream to the output file.
	imform->write_bytes_to_file(output_path, true);

	// 
	// Assembly Complete. execute all optional actions now.
	//
	if (option_tbl[SHOW_SOURCE_OPT])
		parser->print_source();

	if (option_tbl[SHOW_PARSER_OPT])
	{
		printf("\n\nparser output:");
		parser->tokstream->print_tokvec();
	}

	if (option_tbl[SHOW_SYMBOLS_OPT])
		symtab->print(true);

	if (option_tbl[SHOW_DASM_OPT])
	{
		imform->print_bytestream(imform->prog_bytestream);
		//disbin(output_path);
	}

	if (option_tbl[SHOW_BIN_OPT])
		printbin(input_path);

	printf("\n\n");
}

void
Assembler::
import_module()
{
	parser->parse_file(input_path); // generate token-stream.
	first_stage_pass();             // calculate size, import modules, build symbol-table.
}

void
Assembler::
reset_all(bool reset_symtab = true)
{
	delete resolver;
	delete parser;
	if (reset_symtab) delete symtab;
	if (imform) delete imform;

	currtok = NULL;
	bufptr = NULL;
	currline = NULL;
	currtok = NULL;
	prog_bytestream = NULL;
	output_file = NULL;
	input_file = NULL;

	prog_size = 0;
	toknum = 0;
	instr_count = 0;
	main_retval = 0;
	bytestream_size = 0;
	next_byte_addr = 0;
	bytestream_size = 0;

	parser = new Parser();
	imform = new IMForm();
	imform->symtab = new SymbolTable(option_tbl[KEEP_SYMBOLS_OPT]);
	resolver = new OperandResolver(this);
}

/* Command Line Arguments:
   ------------------------

        Assembly Options:

             *description*             *long-form*   *short-form*
        [0] show source code          -showsource       -ss
        [1] show parser output        -showparser       -sp
        [2] show symbols              -showsymbols      -ssb
        [2] keep symbols              -keepsymbols      -ks
        [3] debug mode                -debug            -dbg
        [4] show disassembled binary  -showdasm         -sdsm
        [5] show binary               -showbin          -sb


        Argument Vector:

        arg0: program-name FIXED      (supplied automatically by os)
        arg1: input-path   MANDATORY  (supplied by user, must be in this order.)
        arg2: output-path  OPTIONAL   (supplied by user, must be in this order.)

                input-path is mandatory and must be supplied by the user,
                output-path can be omitted resulting in output-path being
                set to DEFAULT_OUTPUT_PATH. arguments can be supplied in any
                order from this point.

        arg3 - argN: assembly-options either one or all of the assembly options can
                     be present in any order using either the long or short form from
                     this point onwards, examples:

                        ./tyasm input.frt output.fbin -ss -dbg -sdsm
                        ./tyasm input.frt -ss -dbg -sdsm
                        ./tyasm input.frt -dbg -sdsm -ss
                        ./tyasm input.frt -ss -dbg -sdsm
*/

u8
get_arg_option(const char* string)
{
    for (u8 i = 0; i < ASM_ARGV_OPT_COUNT; i++)
	{
        if (strcmp(string, opttbl_sf_str[i]) == 0)
            return i;
    }

    return 0;
}

// to test Assembler_main() paste this in this part of the function:
//
// print_option_tbl(argc, argv, option_tbl, option_tbl_record);
//
// invalid_option:
// printf(ASM_INVALID_OPT_ARG_INFOSTR);
// return 0;
//
// then comment out this part:
// assembler = new Assembler(option_tbl);
// return assembler->main_retval;
// then run the function as normal.

void
print_option_tbl(int argc, char* argv[], bool* option_tbl, bool* option_tbl_record)
{
    printf("\n argc: %d", argc);
    for (int i = 0; i < argc; i++)
        printf("\narg: %d, \"%s\" ", i, argv[i]);

    printf("\nprinting option_tbl:");
    for (int i = 0; i < ASM_ARGV_OPT_COUNT; i++)
        printf("\narg[%d] = %u", i, (u32) option_tbl[i]);

    printf("\nprinting option_tbl_record:");
    for (int i = 0; i < ASM_ARGV_OPT_COUNT; i++)
        printf("\narg[%d] = %u", i, (u32)option_tbl_record[i]);
}

// main() has to be moved to another file and assembler recompiled
// due to DVM needing to use assembler, dvm has it's own main()
int
Assembler_main(int argc, char* argv[])
{
	Assembler* assembler = NULL;
	u8  opt = 0;

	// array to hold option values.
	bool option_tbl[ASM_ARGV_OPT_COUNT] = { false };

	// array to record if option has received input.
	bool option_tbl_record[ASM_ARGV_OPT_COUNT] = { false };

	// anything within this try-block will be caught as an
	// error to report to the user.
	try
	{
		// first check that argv has the minimum required length.
		if (argc < ASM_MINIMUM_ARGC)
		{
			printf("\nno input file.");
			return 0;
		}

		// check input path is valid. will check if it exists when it is opened.
		if (!is_valid_filepath(argv[1], INPUT_FILE_EXTENSION))
		{
			printf("\ninvalid input path, %s", argv[1]);
			return 0;
		}

		// check if output path was supplied, if not use default.
		if (!is_valid_filepath(argv[2], OUTPUT_FILE_EXTENSION))
		{
			printf("\ninvalid output path, %s", argv[2]);
			return 0;
		}

		// process options if there's any.
		if (argc > 3)
		{
			// arg reader loop.
			for (u8 argi = 3; argi < argc; argi++)
			{
				// check what option was given.
				opt = get_arg_option(argv[argi]);
				if (!opt)
				{
					printf("\ninvalid option argument, %s", argv[argi]);
					return 0;
				}

				// check if option has already been given.
				if (option_tbl_record[opt])
				{
					printf("\noption given twice, %s", argv[argi]);
					return 0;
				}

				else
				{
					option_tbl_record[opt] = true;
					option_tbl[opt] = true;
				}
			}
		}

		assembler = new Assembler(option_tbl, argv[1], argv[2], new std::vector<ImportCard*>());
		assembler->is_main_module = true;
		assembler->assemble_main_module();
	}

	catch (inputTextError input_err)
	{
		input_err.print_errmsg();
	}

	catch (AssemblerError asm_err)
	{
		asm_err.print_errmsg();
	}

	return 0;
}

int
main(int argc, char* argv[])
{
    return Assembler_main(argc, argv);
}

//
// OG UTLITY FUNCTIONS.
//
u32
GETADDR__(u8* bytestream,
	u8* bsptr)
{
	u32 ADDR_VAL = (bsptr - bytestream);
	if (ADDR_VAL % 4 != 0) \
		ADDR_VAL += 4 - (ADDR_VAL % 4);
	return ADDR_VAL;
}

void
print_prog_bytestream(u8* bytestream,
	size_t size)
{
	u32 addr_counter;
	u32 addr_counter_end;
	u8* bsptr = bytestream;
#define ADDR()1 (bsptr - bytestream)
#define ADDR() GETADDR__(bytestream, bsptr)
	u32* u32p = NULL;
	u16* u16p = NULL;
	u16 keep_symbols = 0;
	size_t first_user_byte = 0;
	size_t prog_size = 0;
	size_t mem_size = 0;
	size_t instr_count = 0;
	size_t addr = 0;
	size_t symtab_size = 0;
	size_t last_instr_addr = 0;
	SymbolTable* symtab = NULL;

	// print metadata segment.
	u32p = (u32*)bsptr;
	printf("\nMETADATA:\naddr: %zu     :: value: %zu (start-addr)", ADDR(), *u32p);
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	mem_size = *u32p;
	printf("\naddr: %zu     :: value: %zu (mem-size)", ADDR(), mem_size);
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	prog_size = *u32p;
	printf("\naddr: %zu     :: value: %zu (prog-size)", ADDR(), prog_size);
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	instr_count = *u32p;
	printf("\naddr: %zu     :: value: %zu (instr-count)", ADDR(), instr_count);
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	printf("\naddr: %zu    :: value: %zu (first-instr-addr)", ADDR(), *u32p);
	addr_counter = (u32)bytestream - (*u32p);
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	printf("\naddr: %zu    :: value: %zu (last-instr-addr)", ADDR(), *u32p);
	last_instr_addr = *u32p;
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	printf("\naddr: %zu    :: value: %zu (first-user-byte)", ADDR(), *u32p);
	first_user_byte = *u32p;
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	printf("\naddr: %zu    :: value: %zu (keep-symbols-flag)", ADDR(), *u32p);
	keep_symbols = (bool)*u32p;
	bsptr += 4; addr += 4;

	u32p = (u32*)bsptr;
	printf("\naddr: %zu    :: value: %zu (creation-date)", ADDR(), *u32p);
	addr_counter_end = (u32)bytestream - first_user_byte;
	bsptr += 4; addr += 4;

	if (keep_symbols)
	{
		u32p = (u32*)bsptr;
		symtab_size = *u32p;
		printf("\naddr: %zu    :: value: %zu (symtab-size)\n", ADDR(), symtab_size);
		symtab = new SymbolTable(bsptr, (bool)keep_symbols);
		symtab->print(true);
		bsptr += symtab_size; // possibly need to change to bsptr += (symtab_size + sizeof(size_t));
		addr += symtab_size;

		// make sure bsptr & addr are properly aligned after reading symtab.
		if (addr % 4 != 0)
			addr += 4 - (addr % 4);

		if ((uintptr_t)bsptr % 4 != 0)
		{
			size_t padding = 4 - ((uintptr_t)bsptr % 4);
			bsptr += padding;
		}
	}

	//while (ADDR() < first_user_byte)
	//for (; addr_counter < prog_size; ++addr_counter)
	for (size_t b = 0; b < prog_size;)
	{

		if (is_opcode(*bsptr))
		{
			printf("\n\naddr: %zu    :: op: %u (%s)", ADDR(), *bsptr, mnemonic_strings[*bsptr]);
			b += opsize_tbl[*bsptr];
		}

		else
		{
			printf("\n ERROR: missing opcode @ addr: %zu", ADDR());
		}

		if (b >= prog_size) break;

		switch (*bsptr)
		{
			// one uint operand instructions.
		case perr_op:
		case systime_op:
		case htime_op:
		case getutc_op:
		case getlocal_op:
		case delay_op:
		case waituntil_op:
		case elapsed_op:
		case getweekday_op:
		case monthdays_op:
		case normtime_op:
		case ftel_op:
		case rwnd_op:
		case sputs_op:
		case sgets_op:
		case serr_op:
		case sdup_op:
		case call_op:
		case jmp_op:
		case jz_op:
		case jnz_op:
		case je_op:
		case jn_op:
		case jl_op:
		case jg_op:
		case jls_op:
		case jgs_op:
		case pshu_op:
		case popnu_op:
		case pshfru_op:
		case poptru_op:
		case movtru_op:
		case stktru_op:
		case pshi_op:
		case pshfri_op:
		case poptri_op:
		case movtri_op:
		case stktri_op:
		case pshr_op:
		case pshfrr_op:
		case poptrr_op:
		case movtrr_op:
		case stktrr_op:
		case slen_op:
		case fcls_op:
			bsptr += 4;
			addr += 4;
			u32p = (u32*)bsptr;
			printf("\naddr: %zu    :: value: %u", ADDR(), *u32p);
			bsptr += 4;
			addr += 4;

			continue;

		case frd_op:
		case fwr_op:
		case fprntf_op:
		case sprntf_op:
		case sscnf_op:
		case mcpy_op:
		case mmov_op:
		case sncy_op:
		case snct_op:
		case mcmp_op:
		case sncm_op:
		case mset_op:
			bsptr += 4;
			addr += 4;
			for (int i = 0; i < 3; i++)
			{
				u32p = (u32*)bsptr;
				printf("\naddr: %zu    :: value: %u", ADDR(), *u32p);
				bsptr += 4;
				addr += 4;
			}
			continue;

			// two uint operand instructions.
		case timeadd_op:
		case timesub_op:
		case settime_op:
		case tstampstr_op:
		case cmptime_op:
		case timediff_op:
		case fopn_op:
		case fsk_op:
		case prntf_op:
		case scnf_op:
		case fgts_op:
		case fpts_op:
		case scpy_op:
		case scat_op:
		case scmp_op:
		case schr_op:
		case srch_op:
		case sstr_op:
		case stok_op:
		case sspn_op:
		case scspn_op:
		case sfrm_op:
		case cpyru_op:
		case setru_op:
		case cpyri_op:
		case setri_op:
		case cpyrr_op:
		case setrr_op:
			bsptr += 4;
			addr += 4;
			for (int i = 0; i < 2; i++)
			{
				u32p = (u32*)bsptr;
				printf("\naddr: %zu    :: value: %u", ADDR(), *u32p);
				bsptr += 4; addr += 4;
			}
			continue;

			// no operand instructions.
		case die_op:
		case nop_op:
		case stopprof_op:
		case startprof_op:
		case brkp_op:
		case nspctr_op:
		case ret_op:
		case swtch_op:
		case popu_op:
		case pop2u_op:
		case incu_op:
		case decu_op:
		case addu_op:
		case subu_op:
		case mulu_op:
		case divu_op:
		case modu_op:
		case inci_op:
		case deci_op:
		case addi_op:
		case subi_op:
		case muli_op:
		case divi_op:
		case modi_op:
		case addr_op:
		case subr_op:
		case mulr_op:
		case divr_op:
		case modr_op:
		case sqrt_op:
		case ceil_op:
		case floor_op:
		case sin_op:
		case cos_op:
		case tan_op:
		case powu_op:
		case pows_op:
		case powr_op:
		case absu_op:
		case abss_op:
		case absr_op:
		case fabs_op:
			bsptr += 4;
			addr += 4;
			continue;
		}
	}

	printf("\n\n file-size: %zu\n metadata-size: %zu, symtab-size: %zu, prog-size: %zu", size, METADATA_SIZE, symtab_size, prog_size);
}

bool 
Assembler::
module_is_imported(char* _abs_path) const
{
	ImportCard* ic = NULL;

	for (size_t i = 0; i < import_list->size(); i++)
	{
		ic = import_list->at(i);

		if (strcmp(ic->imform->in_fdata->abspath, _abs_path) == 0)
			return true;
	}

	return false;
}

char*
Assembler::
generate_symbol_idOLD(char* _id) const
{
	char* retval;
	// determine if sym-id already has a fileid in it (fileid.symid), meaning
	// it is a symbol from an imported module, or if it's just a lone symid (symid)
	// meaning the symbol is from the main-module.
	//
	// if we have an imported-symbol, no action is required except
	// duplicating the string and returning it. if it's a main symbol(no period).
	// we make it main._id then return.

	// symbol is imported.
	if (strchr(_id, '.'))
	{
		retval = (char*)malloc(strlen(imform->in_fdata->fileid) + strlen(_id) + 2); // 2 for null-term & period.
		if (!retval)
			throw AssemblerError("\nmalloc failed in Assembler::generate_symbol_id()\n[retval = (char*)malloc(strlen(imform->in_fdata->fileid) + strlen(_id) + 2);]");
		sprintf(retval, "%s.%s", imform->in_fdata->fileid, _id);
	}

	// symbol is main.
	else
	{
		retval = (char*)malloc(strlen(_id) + 6); // 6 for null-term, period and "main".
		if (!retval)
			throw AssemblerError("\nmalloc failed in Assembler::generate_symbol_id()\n[retval = (char*)malloc(strlen(_id) + 6);]");
		sprintf(retval, "main.%s", _id);
	}

	return retval;
}

char*
Assembler::
generate_symbol_id(char* _id) const
{
	char* retval = NULL;

	// if '.' in _id it's an imported identifier so just copy it.
	if (strchr(_id, '.'))
	{
		retval = (char*)malloc(strlen(_id) + strlen(_id) + 1); // 2 for null-term.
		if (!retval)
			throw AssemblerError("\nmalloc failed in Assembler::generate_symbol_id()\n[retval = (char*)malloc(strlen(_id) + strlen(_id) + 2);]");
		sprintf(retval, "%s", _id);
	}

	// if no '.' in _id then the identifier is from the same module calling the id.
	// so we just prepend the fildid to it.
	else
	{
		retval = (char*)malloc(strlen(imform->in_fdata->fileid) + strlen(_id) + 2); // 2 for null-term & period.
		if (!retval)
			throw AssemblerError("\nmalloc failed in Assembler::generate_symbol_id()\n[retval = (char*)malloc(strlen(imform->in_fdata->fileid) + strlen(_id) + 2);]");
		sprintf(retval, "%s.%s", imform->in_fdata->fileid, _id);

	}

	return retval;
}

// quick function to check how assembler is working
// disassembles input file and displays it.
void
disbin(const char* input_path)
{
	u8* base = NULL;
	size_t file_size = 0;
	size_t bytes_read = 0;

	FILE* file = fopen(input_path, "r");
	if (!file)
		printf("\ndisbin() error: input file %s failed to open.", input_path);

	printf("\ndisassembly of assembled file: %s", input_path);

	// get the file size then rewind file.
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	printf("\nfile-size is %zu bytes.", file_size);
	rewind(file);

	base = (u8*)malloc(file_size);

	if (!base) {
		printf("\ndisbin() error: failed to allocate memory for input buffer.");
		fclose(file);
		return;
	}

	// read file into ram
	bytes_read = fread(base, 1, file_size, file);

	if (bytes_read)
		printf("\nread %zu bytes.\n", bytes_read);
	else
		printf("\ndisbin() error: read 0 bytes from %s", input_path);

	fclose(file);

	print_prog_bytestream(base, bytes_read);

	free(base);
	printf("\ndisassembly complete.\n");
}

void
printbin(const char* input_path)
{
	FILE* file = fopen(input_path, "rb");

	if (!file)
		printf("\printbin() error: input file %s failed to open.", input_path);

	size_t byte_count = 0;
	u8     byte;

	printf("\nprinting binary data of %s\n", input_path);

	// Read each byte from the file until end of file
	while (fread(&byte, sizeof(byte), 1, file) == 1) {
		// Print each byte as two hex digits.
		printf("%02x ", byte);

		// Print 16 bytes per line.
		if (++byte_count % 16 == 0)
			printf("\n");
	}

	fclose(file);

	// Newline if last line didn't end with 16 bytes
	if (byte_count % 16 != 0) {
		printf("\n");
	}
}

char*
decode_datestamp(u32 datestamp)
{
	// Unpack the day, month, and year from the 32-bit datestamp
	int day = (datestamp >> 8) & 0x1F;       // Bits 11-8
	int month = (datestamp >> 12) & 0xF;     // Bits 15-12
	int year = (datestamp >> 16) & 0xFFFF;   // Bits 31-16

	// Allocate memory for the date string
	char* datestr = (char*)malloc(DATESTR_MALLOC_LEN);
	if (!datestr)
	{
		throw AssemblerError("\nnew failed in decode_datestamp()\n[char* datestr = (char*)malloc(DATESTR_MALLOC_LEN);]", (u8)AsmErrorCode::DATESTR_MALLOC);
	}

	// Format the date string
	sprintf(datestr, DATESTR_INFOSTR, day, month, year);
	return datestr;
}

u32
get_packed_datestamp()
{
	auto now = std::chrono::system_clock::now();
	auto now_time = std::chrono::system_clock::to_time_t(now);
	auto* local_time = std::localtime(&now_time);

	int day = local_time->tm_mday;          // 5 bits: 1-31
	int month = local_time->tm_mon + 1;    // 4 bits: 1-12
	int year = local_time->tm_year + 1900; // Full year (e.g., 2024)

	// Pack the date into 32 bits:
	// Bits: [31-16: Year | 15-12: Month | 11-8: Day]
	return (u32)(((year & 0xFFFF) << 16) | ((month & 0xF) << 12) | ((day & 0x1F) << 8));
}