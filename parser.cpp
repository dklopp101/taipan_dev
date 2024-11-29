#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#include "parser.h"

void
inputTextError::
runtime_end_err_handler()
{
	std::terminate();
}

inputTextError::
inputTextError(u32    _linenum,
	u32    _posnum,
	char* _line,
	const char* _extra_msg)
	:
	linenum(_linenum), line(_line), posnum(_posnum)
{
	build_errmsg_str(_extra_msg);
}

inputTextError::
~inputTextError()
{
	if (errmsg) free(errmsg);
}


void
inputTextError::
build_errmsg_str(const char* _extra_msg)
{
	errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);

	if (!errmsg)
		throw AssemblerError("\nmalloc() failed [errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);]", (u8) ParserError::ASM_ERR_BUF_ALLOC_ERR);

	size_t currline_len = strlen(line);
	char* ch = errmsg;

	if (_extra_msg)
	{
		sprintf(errmsg, "\nerror @ col %zu on line %zu\nmessage: %s", posnum, linenum, _extra_msg);
		ch += strlen(ch);
	}

	else
	{
		sprintf(errmsg, "\nerror @ col %zu on line %zu\n input text error!", posnum, linenum, _extra_msg);
		ch += strlen(ch);
	}

	// concat on the line with the error.
	sprintf(ch, "\n\nsource code:\n%s\n", line);
	ch += strlen(ch);

	// concat on up-pointing arrow showing where the error is.
	memset(ch, '-', posnum);
	ch += posnum;
	*(ch++) = '^';
	*(ch++) = '\n';
	*ch = '\0';
}

void
inputTextError::
print_errmsg()
{
	printf("\n%s", errmsg);
}

SystematicError::
SystematicError(char* _extra_msg,
	            bool  free_extra_msg = false)
{
	build_errmsg_str(_extra_msg);
	if (free_extra_msg) free(_extra_msg);
}

SystematicError::
~SystematicError()
{
	if (errmsg) free(errmsg);
}


void
SystematicError::
build_errmsg_str(char* _extra_msg)
{
	errmsg = (char*)malloc(ASMERR_MSG_BUFSIZE);

	if (!errmsg)
		throw AssemblerError("\nmalloc() failed in SystematicError::build_errmsg_str() [errmsg = (char*) malloc(ASMERR_MSG_BUFSIZE);]", (u8)ParserError::ASM_ERR_BUF_ALLOC_ERR);

	char* ch = errmsg;
	sprintf(errmsg, "\nassembly error: %s", _extra_msg);
	ch += strlen(ch);
}

void
SystematicError::
print_errmsg()
{
	printf("\n%s", errmsg);
}

void
SystematicError::
runtime_end_err_handler()
{
	std::terminate();
}

Token::
Token(u32    _ndx,
	  u8     _type,
	  size_t _line,
	  size_t _col,
	  char*  _srcstr,
	  char*  _linestr)
:
	type(_type),       line(_line),
	col(_col),         ndx(_ndx),
	srcstr(_srcstr),   infostr(NULL),
	linestr(_linestr), line_printlen(0),
	val_type(VOID_TYPE)
{
	// set Token() object's val field from the source-string.
	switch (type)
	{
		case OPCODE_TOK:
			val.ubyteval = get_mnemonic_opcode(srcstr);
			val_type = UBYTE_TYPE;
			break;

		case ID_TOK:
			val_type = ID_TYPE;
			break;

		case UINT_TOK:
		case ADDR_TOK:
			val.uintval = (u32)atoi(srcstr);
			val_type = UINT_TYPE;
			break;

		case INT_TOK:
			val.intval = (i32)atoi(srcstr);
			val_type = INT_TYPE;
			break;

		case UBYTE_TOK:
			val.ubyteval = (u32)atoi(srcstr);
			val_type = UBYTE_TYPE;
			break;

		case REAL_TOK:
			val.realval = (real)atof(srcstr);
			val_type = REAL_TYPE;
			break;

		case HEX_TOK:
			val.intval = (i32)strtol(srcstr, NULL, 16);
			val_type = INT_TYPE;
			break;

		case OCT_TOK:
			val.intval = (i32)strtol(srcstr, NULL, 8);
			val_type = INT_TYPE;
			break;

		case UHEX_TOK:
			val.uintval = (u32)strtoul(srcstr, NULL, 16);
			val_type = UINT_TYPE;
			break;

		case UOCT_TOK:
			val.uintval = (u32)strtoul(srcstr, NULL, 8);
			val_type = UINT_TYPE;
			break;
	}
	build_infostr();
}

Token::
~Token()
{
	free(srcstr);
	free(infostr);
	if (linestr) free(linestr);
}

char*
Token::
build_infostr()
{
	// calculate approx length required for infostr.
	infostr = (char*) malloc(TOK_INIT_INFOSTR_LEN);

	if (!infostr)
		throw AssemblerError("\nmalloc() failed [infostr = (char*) malloc(TOK_INIT_INFOSTR_LEN);]", (u8) ParserError::TOK_INFOSTR_MALLOC_ERR);

	char* cursor = infostr;

	// start actually composing the token's info-string.
	sprintf(cursor, "\ntoknum: %zu <%s> @ col: %zu line: %zu", ndx, toktype_mnemonics[type], col, line);
	cursor += strlen(cursor);

	// write line-string to info-string if this is the first tok of the line.
	if (linestr)
	{
		sprintf(cursor, TOK_LINESTR_INFOSTR, linestr);
		cursor += strlen(cursor);
	}

	switch (type)
	{
	case OPCODE_TOK:
		sprintf(cursor, TOK_OPCODE_INFOSTR, val.ubyteval, mnemonic_strings[val.ubyteval]);
		cursor += strlen(cursor);
		break;

	case ADDR_TOK:
	case UINT_TOK:
		sprintf(cursor, TOK_UINTVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case INT_TOK:
		sprintf(cursor, TOK_INTVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case REAL_TOK:
		sprintf(cursor, TOK_REALVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case UREAL_TOK:
		sprintf(cursor, TOK_UREALVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case UBYTE_TOK:
		sprintf(cursor, TOK_UBYTEVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case OCT_TOK:
		sprintf(cursor, TOK_OCTVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case HEX_TOK:
		sprintf(cursor, TOK_HEXVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case ID_TOK:
		sprintf(cursor, TOK_IDVAL_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case LABEL_DEF_TOK:
		sprintf(cursor, TOK_LABELDEF_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case STRING_TOK:
		sprintf(cursor, TOK_STRING_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;

	case COMMENT_TOK:
		sprintf(cursor, TOK_COMMENT_INFOSTR, srcstr);
		cursor += strlen(cursor);
		break;
	}

	// trim info-string memory to the exact length of the info-string.
	infostr = (char*) realloc(infostr, strlen(infostr) + 1);
	if (!infostr)
	{
		free(infostr);
		throw AssemblerError("\nnew failed in void TokenStream::create\n[Token* tok = new Token()]", (u8) ParserError::TOK_INFOSTR_REALLOC_ERR);
	}

}

void
Token::
print_tok()
{
	printf("\n%s", infostr);
}

void
TokenStream::
create(u8     _type,
	u32    _line,
	u8     _col,
	char* _srcstr,
	char* _linestr,
	size_t _line_printlen)
{
	try
	{
		Token* tok = new Token(vec->size(),
			_type,
			_line,
			_col,
			_srcstr,
			_linestr);

		tok->line_printlen = _line_printlen;
		vec->push_back(tok);
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in void TokenStream::create\n[Token* tok = new Token()]");
	}
}

void
TokenStream::
freetoks()
{
	for (size_t i = 0; i < vec->size(); i++)
		delete vec->at(i);
}

void
TokenStream::
print_tokvec()
{
	for (size_t i = 0; i < vec->size(); i++)
		vec->at(i)->print_tok();
}

TokenStream::
TokenStream()
{
	try {
		vec = new std::vector<Token*>();
	}
	catch (const std::bad_alloc& e) {
		throw AssemblerError("\nnew failed in TokenStream::TokenStream\n[Token* tok = new Token()]");
	}
}

TokenStream::
~TokenStream()
{
	delete vec;
}

void
Parser::
throw_error_here(Token* token,
	              char* errmsg)
{
	throw inputTextError(token->line, token->col, linevec->at(token->line), errmsg);
}

void
Parser::
readline()
{
read_next_line:
	size_t ch_count = 0;
	char* bufch = linebuf;
	int    ch;

	memset(linebuf, '\0', linelen); // clear the prev line from linebuf.
	isFirstTokOnLine = true;
	linenum++;

	for (;;)
	{
		ch = fgetc(input_file);
		ch_count++;

		if (((*linebuf) == '\n') || ((*linebuf) == '\r'))
			goto read_next_line;

		switch (ch)
		{
		case EOF:
			if (ferror(input_file))
				throw AssemblerError("\nnew failed in parser.cpp void Parser:: readline()\n[if (ferror(input_file))]", (u8) ParserError::LINEBUF_FGETC_ERR);

			else if (feof(input_file))
				eof_reached = true;

		case EOL:
		case 13: // '/r'
			*bufch = '\0';
			linelen = ch_count;
			return;

		default:
			if (ch_count == linebuf_size)
			{
				linebuf_size += LINEBUF_GROWTH_FACTOR;
				linebuf = (char*)realloc(linebuf, linebuf_size);

				if (!linebuf)
				{
					free(linebuf);
					throw AssemblerError("\nrealloc failed in parser.cpp void Parser:: readline()\n[linebuf = (char*) realloc(linebuf, linebuf_size);]", (u8) ParserError::LINEBUFFER_REALLOC_ERR);
				}
			}

			*(bufch++) = (char)ch;

			if (*bufch == '/t')
				line_printlen += DEFAULT_TAB_WIDTH;
			else
				line_printlen++;
		}
	}
}

void
Parser::
tokenize_line()
// NOTE: all numeric types are initially typed as NUM_TOK,
// they are then properly typed once more chars are parsed.
{
	bool   notFirstIter = false;
	bool   commentWasDelim = false;
	bool   last_ch_ws = false;
	bool   isLastTok = false;
	char* ch = linebuf; // line cursor.
	char* tbch = NULL; // tokbuf cursor / temporary holding ptr for tok line-string.
	size_t linepos = 1;
	size_t toklen = 0;
	size_t tokcol = 0; // holds the col of the first char of tok.
	u8     toktype = NO_TOK;
	char* tokbuf = (char*)malloc(linebuf_size); // gets taken by tok who must handle freeing.
	u8 operator_code = 0;

	if (!tokbuf)
		throw AssemblerError("\nmalloc failed in void Parser::tokenize_line()\n[char*  tokbuf = (char*) malloc(linebuf_size);]", (u8)ParserError::TOKBUF_MALLOC_ERR);

	tbch = tokbuf;
	*tbch = '\0';

	for (;;)
	{
	parsing_loop:
		if (notFirstIter)
		{
			linepos++;
			ch++;
		} notFirstIter = true;

		if (commentWasDelim)
			goto finalise_comment_tok;

		// process digits.
		if (isdigit(*ch))
		{
			last_ch_ws = false;

			switch (toktype)
			{
			case NO_TOK:
			case UNSIGNED_TOK:
				tokcol = linepos;

			case SIGNED_TOK:
				// if ch is 0 we have hex/oct otherwise we have a real.
				if ((*ch) == '0')
				{
					if (((*(ch + 1)) == 'x') || ((*(ch + 1)) == 'X'))
					{ // is it hex?
						if ((toktype == SIGNED_TOK) || (toktype == NO_TOK))
							toktype = HEX_TOK;
						else
							toktype = UHEX_TOK;
					}

					else if (isdigit(*(ch + 1))) {// is it oct-tok?
						if ((toktype == SIGNED_TOK) || (toktype == NO_TOK))
							toktype = OCT_TOK;
						else
							toktype = UOCT_TOK;
					}

					else
					{ // must be a real-tok because no other signed/unsigned tok can start with 0.
						if ((toktype == SIGNED_TOK) || (toktype == NO_TOK))
							toktype = REAL_TOK;
						else
							toktype = UREAL_TOK;
					}
				}

				// we have a no-tok, signed or unsigned tok but first tok isn't 0 so must be integer.
				else
				{
					tokcol = linepos;

					if (toktype == SIGNED_TOK)
						toktype = INT_TOK;
					else if (toktype == NO_TOK)
						toktype = UINT_TOK;

				}

				break;

				// we're in the middle of any of these toks below so we just break. 
			case UINT_TOK:
			case INT_TOK:
			case UBYTE_TOK:
			case ADDR_TOK:
			case REAL_TOK:
			case UREAL_TOK:
			case HEX_TOK:
			case UHEX_TOK:
			case OCT_TOK:
			case UOCT_TOK:
				break;
			}

			*tbch++ = *ch;
			continue;
		}

		// process alphabetical chars.
		if (isalpha(*ch))
		{
			last_ch_ws = false;

			switch (toktype)
			{
				// if we dont have toktype yet then we have first char of id tok.
				// set new-tok vars then drop through and append ch to tok.
			case NO_TOK:
				toktype = ID_TOK;
				tokcol = linepos;

			case ID_TOK: // if in id or comment tok append char to tok.
				*tbch++ = *ch;
				continue;

			case HEX_TOK: // already check that ch has to be 'x' here.
			case UHEX_TOK:
				switch (*ch)
				{
				case 'X': case 'x':
				case 'A': case 'a':
				case 'B': case 'b':
				case 'C': case 'c':
				case 'D': case 'd':
				case 'E': case 'e':
				case 'F': case 'f':
					break;

				default:
					free(tokbuf);
					throw inputTextError(linenum, linepos, linebuf, "syntax error.");
				}

				*tbch++ = *ch;
				continue;

				// if inside any num tok(except hex/uhex tok), tok is invalid.
			case UINT_TOK:
			case INT_TOK:
			case UBYTE_TOK:
			case ADDR_TOK:
			case REAL_TOK:
			case UREAL_TOK:
			case OCT_TOK:
			case UOCT_TOK:
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");
			}
		}

		// handle non-alphanumeric chars, top 5 cases are tok delimiters.
		switch (*ch)
		{
			// comments may or may not be directly preceeded by a tok.
			// eg: psh 50#comment vs psh 50 # comment, in first example the tok is
			// delimited by the # char, next example by the ' ' char. so whenever we
			// hit a comment-indicator char we set the comment_found flag to true,
			// then we check if we've been building a tok by checking toktype var.
			// if it's not 0(NO_TOK) we jump to create_tok which creates the tok as
			// usual, resets vars then execution jumps to top of the main loop which
			// checks if comment_found is true before ch & linepos are incremented.
			// if it's true tokcol is set to linepos which is the col of the comment-indicator
			// char then jumps to create_comment_tok and the tok is created.
			// HOWEVER, if toktype 0(NO_TOK) then we jump to create_comment_tok, which
			// then gets the vars reset & comment_found set to false and next iteration
			// of the main loop proceeds. -check comments at top of main-loop for more info.
		case COMMENT_INDICATOR:
			if (toktype)
			{
				commentWasDelim = true;
				goto finalise_tok;
			}

			goto finalise_comment_tok;

		case '\0':
			isLastTok = true;
			if (toktype) goto finalise_tok; // have we found delimiter of current tok?
			return;

		case '\t':
			linepos += tab_width_offset;

		case ' ':
			last_ch_ws = true;
			if (toktype) goto finalise_tok; // have we found delimiter of current tok?
			continue; // no tok so continue parsing next ch of line.

		case '_':
			last_ch_ws = false;

			switch (toktype)
			{
				// cannot have '_' in *any* numeric tok.
			case UINT_TOK:
			case INT_TOK:
			case UBYTE_TOK:
			case ADDR_TOK:
			case REAL_TOK:
			case UREAL_TOK:
			case HEX_TOK:
			case UHEX_TOK:
			case OCT_TOK:
			case UOCT_TOK:
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");

			case NO_TOK: // found first ch of id tok.
				toktype = ID_TOK;
				tokcol = linepos;

			case ID_TOK:
				*tbch++ = *ch;
				continue;
			}

		case '.':
			switch (toktype)
			{
			case SIGNED_TOK:
				toktype = REAL_TOK;
				break;

				//case NUM_TOK:
			case UNSIGNED_TOK:
				toktype = UREAL_TOK;
				break;

			case REAL_TOK:
				break;

				// '.' cannot be in any of these toks.
			case MACRO_DEF_TOK:
			case LABEL_DEF_TOK:
			case OCT_TOK:
			case UOCT_TOK:
			case HEX_TOK:
			case UHEX_TOK:
			case INT_TOK:
			case UBYTE_TOK:
			case ADDR_TOK:
			case ID_TOK:
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");

			// '.' was first char of the tok.
			case NO_TOK:
				toktype = UREAL_TOK;
				tokcol = linepos;
				break;
			}

			*tbch++ = *ch;
			continue;

		case LABEL_DEF_SUFFIX: // if in id tok signals end of tok, otherwise invalid.
			if (toktype != ID_TOK)
			{
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");
			}

			toktype = LABEL_DEF_TOK;
			goto finalise_tok;

		case SIGNED_INT_PREFIX:
		case UNSIGNED_INT_PREFIX:
		case ADDR_INT_PREFIX:
		case REAL_PREFIX:
			last_ch_ws = false;

			if (toktype)
			{
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");
			}

			last_ch_ws = false;
			toktype = (u8)*ch;
			goto finalise_tok;

		case STRING_DELIM:
			last_ch_ws = false;

			if (toktype)
			{
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");
			}

			toktype = STRING_TOK;
			tokcol = linepos;

			for (; *++ch != STRING_DELIM; linepos++)
				*tbch++ = *ch;

			goto finalise_tok;

		case BREAKPOINT_INDICATOR:
			last_ch_ws = false;

			if (toktype)
			{
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");
			}

			toktype = BRKPNT_TOK;
			tokcol = linepos;
			*tbch++ = *ch;
			continue;

			// operators:
		case ';':
			if (toktype)
			{
				--linepos;
				--ch;
				goto finalise_tok; // have we found delimiter of current tok?
			}

			toktype = EXPREND_TOK;
			*tbch++ = *ch;
			goto finalise_tok;

		case '+': case '-': case '*': case '/': case '/%':
		case '[': case ']': case '(': case ')':
			if (toktype)
			{
				--linepos;
				--ch;
				goto finalise_tok; // have we found delimiter of current tok?
			}
			toktype = (u8)*ch;
			*tbch++ = *ch;
			goto finalise_tok;
		}
	}

finalise_tok:
	// *(tbch++) = '\0'; removed the incrementing, no need i think.
	*tbch = '\0';
	toklen = strlen(tokbuf);

	switch (toktype)
	{
		// create id/label-def/opcode/macro-def tokens.
	case ID_TOK:
	case LABEL_DEF_TOK:
		if (toklen < MIN_ID_LEN || toklen > MAX_ID_LEN)
		{
			free(tokbuf);
			throw inputTextError(linenum, linepos, linebuf, "syntax error.");
		}

		if (is_op_mnemonic(tokbuf))
		{
			if (toktype == LABEL_DEF_TOK)
			{
				free(tokbuf);
				throw inputTextError(linenum, linepos, linebuf, "syntax error.");
			}

			toktype = OPCODE_TOK;
		}

		if (strcmp(tokbuf, MACRO_DEF_KEYWORD) == 0)
			toktype = MACRO_DEF_TOK;

		break;

		// create uint, int, ubyte, addr & real tokens.
	case UINT_TOK:
		toktype = UINT_TOK;

	}

	// realloc the tokbuf to trim it to size.
create_tok:
	tokbuf = (char*)realloc(tokbuf, toklen + 1);

	if (!tokbuf)
		throw AssemblerError("\nrealloc failed in void Parser::tokenize_line()\n[tokbuf = (char*) realloc(tokbuf, toklen + 1);]", (u8)ParserError::TOK_INFOSTR_REALLOC_ERR);

	// if tok is the first on the line it will have a copy of the line.
	// so we'll use tbch purely to point to the memory we're about to alloc
	// for the line-string.
	if (isFirstTokOnLine)
	{
		tbch = (char*)malloc(linelen + 1);

		if (!tbch)
			throw AssemblerError("\nmalloc failed in void Parser::tokenize_line()\n[tbch = (char*) malloc(linelen + 1);]", (u8)ParserError::TOK_LINESTR_MALLOC_ERR);

		strcpy(tbch, linebuf);
		linevec->push_back(tbch);
	}

	// tok isn't the first of the line so set tbch to NULL to reflect that.
	else
	{
		tbch = NULL;
	}

	tokstream->create(toktype, linenum, tokcol, tokbuf, tbch, line_printlen);


	// check if that was last token of line, if so return func.
	if (isLastTok || toktype == COMMENT_TOK)
		return;

	// wasn't last tok so jump to reset_vars which will continue parsing the line.
	goto reset_vars;

finalise_comment_tok:
	// copy rest of line to tokbuf, create comment tok then return func.
	if (commentWasDelim) tokcol = linepos - 1;
	else                 tokcol = linepos;

	toktype = COMMENT_TOK;
	strcpy(tbch, ch);
	toklen = strlen(tokbuf);

	goto create_tok;

reset_vars:
	// allocate a new tokbuf for the next tok & reset vars.
	tokbuf = (char*)malloc(linebuf_size);

	if (!tokbuf)
		throw AssemblerError("\nmalloc failed in void Parser::tokenize_line()\n[tokbuf = (char*) malloc(linebuf_size);]", (u8)ParserError::TOKBUF_MALLOC_ERR);


	isFirstTokOnLine = false;
	tbch = tokbuf;
	toktype = NO_TOK;

	goto parsing_loop; // jump back into tokenization loop.
}

Parser::
Parser() :
	linenum(0),
	linelen(0),
	input_path(NULL),
	eof_reached(false),
	last_opcode(0),
	input_file(NULL),
	tab_width_offset(DEFAULT_TAB_WIDTH - 1),
	isFirstTokOnLine(true),
	invalid_tok_infostr(NULL),
	tokcount(0),
	line_printlen(0)
{
	linebuf_size = LINEBUF_INIT_SIZE;
	linebuf = (char*)malloc(LINEBUF_INIT_SIZE);

	if (!linebuf)
		throw AssemblerError("\nmalloc failed in void Parser::Parser\n[linebuf = (char*) malloc(LINEBUF_INIT_SIZE);]", (u8)ParserError::LINEBUFFER_MALLOC_ERR);

	try
	{

		tokstream = new TokenStream();
		linevec = new std::vector<char*>();
	}

	catch (const std::bad_alloc& e)
	{
		throw AssemblerError("\nnew failed in void Parser::Parser\n[tokstream = new TokenStream();]", (u8)ParserError::TOKVEC_NEW_ERR);
	}

	linevec->push_back(NULL);
}

Parser::
~Parser()
{
	if (input_file) fclose(input_file);
	delete tokstream;
	delete linevec;
	free(input_path);
}

void
Parser::
parse_file(const char* path)
{
	input_path = (char*)malloc(strlen(path + 1));

	if (!input_path)
		throw AssemblerError("\nmalloc failed in void Parser::parse_file\n[input_path = (char*) malloc(strlen(path + 1));]", (u8)ParserError::INPUT_PATH_MALLOC_ERR);

	strcpy(input_path, path);

	input_file = fopen(input_path, "r");

	if (!input_file)
		throw AssemblerError("\nfopen failed in void Parser::parse_file\ninput_file = fopen(input_path, \"r\");", (u8)ParserError::INPUT_FILE_FOPEN_ERR);

	try
	{
		while (!eof_reached)
		{
			readline();
			tokenize_line();
		}

		// this var is just for convience of the assembler.
		tokcount = tokstream->vec->size();

		// look backwards through the tokstream to find the last opcode
		// token so we can set last_opcode var which will be used by the
		// assembler to determine the last instr's addr.
		for (int i = (tokcount - 1); i > 0; i--)
		{
			if (tokstream->vec->at(i)->type == OPCODE_TOK)
			{
				last_opcode = tokstream->vec->at(i)->val.ubyteval;
				break;
			}
		}
	}

	// handle any errors that have been thrown by printing their print-string
	// to the console, freeing any memory that needs freeing then throwing parser error.

	// catch any assembly-errors that were thrown during parsing
	// performing any memory clean up then rethrowing the exception.
	catch (AssemblerError e)
	{
		switch ((ParserError) e.errcode)
		{
			case ParserError::INVALID_TOK_ERR:
				printf(INVALID_TOK_ERR_PRINTSTR, invalid_tok_infostr);
				free(invalid_tok_infostr);
				free(linebuf);
				break;

			case ParserError::INVALID_TOK_MALLOC_ERR:
				free(linebuf);
				printf(INVALID_TOK_MALLOC_ERR_PRINTSTR);
				break;

			case ParserError::TOKBUF_MALLOC_ERR:
				printf(TOKBUF_MALLOC_ERR_PRINTSTR);
				free(linebuf);
				break;

			case ParserError::TOK_LINESTR_MALLOC_ERR:
				printf(TOK_LINESTR_MALLOC_ERR_PRINTSTR);
				free(linebuf);
				break;

			case ParserError::LINEBUFFER_MALLOC_ERR:
				printf(LINEBUFFER_MALLOC_ERR_PRINTSTR);
				break;

			case ParserError::LINEBUFFER_REALLOC_ERR:
				printf(LINEBUFFER_REALLOC_ERR_PRINTSTR);
				break;

			case ParserError::LINEBUF_FGETC_ERR:
				free(linebuf);
				printf(LINEBUF_FGETC_ERR_PRINTSTR);
				break;

			case ParserError::TOK_INFOSTR_MALLOC_ERR:
				free(linebuf);
				printf(TOK_INFOSTR_MALLOC_ERR_PRINTSTR);
				break;

			case ParserError::TOKVEC_NEW_ERR:
				free(linebuf);
				printf(TOKVEC_NEW_ERR_PRINTSTR);
				break;

			case ParserError::TOKVEC_CREATE_TOK_ERR:
				free(linebuf);
				printf(TOKVEC_CREATE_TOK_ERR_PRINTSTR);
				break;
		}

		// rethrow the exception to the Parser() object owner, telling it
		// an error has occured, this will ensure Parser()'s destructor
		// is called, cleaning up TokVec() and it's tokens too.
		throw e;
	}
}

void
Parser::
print_source()
{
	char   linenum_spacer[10];
	size_t linenum = 0;

	printf("\n\nsource code from %s\n", input_path);
	for (int i = 0; i < tokcount; i++)
	{
		if (tokstream->vec->at(i)->linestr)
		{
			memset(linenum_spacer, 0, 10);
			linenum++;
			memset(linenum_spacer, 32, 7 - count_linenum_digits(linenum));
			*(linenum_spacer + (7 - count_linenum_digits(linenum) + 1)) = '/0';

			printf("\n%zu%s| %s", linenum, linenum_spacer, tokstream->vec->at(i)->linestr);
		}
	}
	printf("\n");
}

u8
count_linenum_digits(size_t num)
{
	u8 count = 0;
	for (; (num /= 10) != 0; count++);
	return count;
}

u8
get_tok_precedence(Token* tok)
{
	for (const auto& entry : tok_precedence_tbl)
	{
		if (entry.typecode == tok->type)
			return entry.precedence;
	}

	return 0; // invalid token.
}

u8
loopup_tok_association(u8 tokcode)
{
	for (u8 i = 0; i < OPERATOR_COUNT; i++)
	{
		if (asso_tbl[i][0] == tokcode)
			return asso_tbl[i][1];
	}

	return 0;
}


//void
//Parser:: // REDUNDANT VERY SOON!
//handle_invalid_tok(size_t linepos)
//{
//	// tokbuf is passed to this function to recycle it for th
//	// invalid-tok-infostr and to ensure it gets freed.
//	size_t currline_len = strlen(linebuf);
//	invalid_tok_infostr = (char*)malloc(INVALID_TOK_INFOSTR_LEN);
//	char* ch;
//
//	if (!invalid_tok_infostr)
//		throw ParserError::INVALID_TOK_MALLOC_ERR;
//	ch = invalid_tok_infostr;
//
//	// build infostr showing error info.
//	sprintf(ch, "\ninvalid token @ col: %zu line: %zu\n", linepos, linenum);
//	ch += strlen(ch);
//
//	// concat on the line with the error.
//	sprintf(ch, "%s\n", linebuf);
//	ch += strlen(ch);
//
//	// concat on up-pointing arrow showing where the error is.
//	memset(ch, '-', currline_len);
//	ch += currline_len;
//	*(ch++) = '^';
//	*(ch++) = '\n';
//	*ch = '\0';
//
//	throw ParserError::INVALID_TOK_ERR;
//}