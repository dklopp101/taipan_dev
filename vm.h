#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)


#ifndef VM_H
#define VM_H


#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <vector>
#include <new>
#include <exception>
#include <stdexcept>
#include <memory>

union Value
{
	uint32_t  uintval;
	int32_t  intval;
	uint8_t   ubyteval;
	float realval;
	char* strval;
};

#define DIVIDER_BAR_STRING "\n--------------------------------------------------------\n\n"

//cstdint type aliases for convinience.
#define real float
#define f32  float
#define u64  uint64_t
#define i64  int64_t
#define u32  uint32_t
#define i32  int32_t
#define u16  uint16_t
#define i16  int16_t
#define u8   uint8_t
#define i8   int8_t
#define EOL  '\n'
#define TRUE  1
#define FALSE 0
#define NONE  0

#define newline() printf("\n")
#define newlines(N) for (u8 i=0; i < (N); i++) { newline(); }

#define print_bits(VALUE) \
			for (int i = 7; i >= 0; i--) printf("%c", (((VALUE)) & (1u << i)) ? '1' : '0')

#include "opcode.h"

// datatype codes.
#define VOID_TYPE   0
#define UINT_TYPE   1
#define INT_TYPE    2 
#define UBYTE_TYPE  3
#define REAL_TYPE   4
#define ADDR_TYPE   5
#define STRING_TYPE 6
#define BOOL_TYPE   7
#define ID_TYPE     8
#define UINT16_TYPE 9

#define LEFT  1
#define RIGHT 2

static
const char*
datatype_str[] =
{
	"void",
	"uint",
	"int",
	"ubyte",
	"real",
	"addr",
	"string",
	"bool",
	"identifier",
	"uint-16",
	"", "", "" , "", "", "", "" , "", "", "", "" ,
	"symtab-data"
};


static
const char*
bool_strings[] =
{ 
	"false",
	"true"
};

#define WORDSIZE        4
#define DOUBLE_WORDSIZE 8
#define WS              WORDSIZE
#define DWS             DOUBLE_WORDSIZE
#define WS_BITS         32
#define DWS_BITS        64
#define OPCODE_SIZE     4
#define RECUR_MAX       0x3E8
#define STACK_SIZE      100000
#define INSTR_MAX_SIZE  13
#define FILEID_MAX_LEN  20

#define UBYTE_SIZE      1
#define UINT_SIZE       4
#define INT_SIZE        4
#define REAL_SIZE       4

// codes and strings used for the data-typerecord system.
#define NOTYPE_CODE         0
#define UINT32_CODE         1
#define INT32_CODE          2   
#define UINT8_CODE          3
#define REAL_CODE           4
#define ADDR_CODE           5
#define STRING_CODE         6
#define BOOL_CODE           7
#define ID_CODE             8
#define UINT16_CODE         9

#define INSIDE_UINT_CODE   10
#define INSIDE_INT_CODE    11
#define INSIDE_UINT16_CODE 12
#define INSIDE_SYMTAB_CODE 13
#define INSIDE_UINT8_PADDING_CODE 14
#define NOSIGN_CODE        15
#define NOVALUE_CODE       16

// constants relevent to the moduleFileData struct.
#define PATH_DIR_MAX        20

#define FILEID_BUFFER_SIZE  20
#define EXT_BUFFER_SIZE     10
#define DRIVE_BUFFER_SIZE    5
#define DIR_BUFFER_SIZE    150

static
const char*
datatype_rec_code_str[] =
{
	"no-type",       // 0
	"uint",          // 1
	"int",           // 2
	"ubyte",         // 3
	"real",          // 4
	"addr",          // 5
	"string",        // 6
	"bool",          // 7
	"identifier",    // 8
	"uint-16",       // 9
	"inside-uint",   // 10
	"inside-int",    // 11
	"inside-uint16", // 12
	"inside-symtab", // 13
	"inside-ubyte-padding", // 14
	"no-sign",   // 15
	"no-value" // 16
};

// another late night eyes-hanging out of my head
// function, seems to work, might rewrite in future.
static
int
file_exists(const char* path)
{
    FILE* file = fopen(path, "rb");
    int retval;

    if (file)
	{
        retval = 1;
        fclose(file);
    }
	
	else
	{
        retval = 0;
    }

    return retval;
}

// contains all required name and path related information
// of an input module for the assembler.
struct
moduleFileData
{
	char* fullpath;
	char* fileid;
	char* ext;
	char* drive;
	char* dir;

	moduleFileData(char* _path)
	{
		fullpath = (char*)malloc(strlen(_path) + 1);
		fileid   = (char*)malloc(FILEID_BUFFER_SIZE);
		ext      = (char*)malloc(EXT_BUFFER_SIZE);
		drive    = (char*)malloc(DRIVE_BUFFER_SIZE);
		dir      = (char*)malloc(DIR_BUFFER_SIZE);

		if (!fullpath) throw AssemblerError("\nmalloc failed in moduleFileData::moduleFileData [fullpath = (char*)malloc(strlen(_path) + 1);]");
		if (!fileid)   throw AssemblerError("\nmalloc failed in moduleFileData::moduleFileData [fileid = (char*)malloc(FILEID_BUFFER_SIZE);]");
		if (!ext)      throw AssemblerError("\nmalloc failed in moduleFileData::moduleFileData [ext = (char*)malloc(EXT_BUFFER_SIZE);]");
		if (!drive)    throw AssemblerError("\nmalloc failed in moduleFileData::moduleFileData [drive = (char*)malloc(DRIVE_BUFFER_SIZE);]");
		if (!dir)      throw AssemblerError("\nmalloc failed in moduleFileData::moduleFileData [dir = (char*)malloc(DIR_BUFFER_SIZE);]");

		strcpy(fullpath, _path);
		_splitpath(fullpath, drive, dir, fileid, ext);

		fileid = (char*)realloc(fileid, strlen(fileid) + 1);
		ext    = (char*)realloc(ext, strlen(ext) + 1);

		if (!fileid) throw AssemblerError("\nrealloc failed in moduleFileData::moduleFileData [fileid = (char*)realloc(fileid, strlen(fileid) + 1);]");
		if (!ext)    throw AssemblerError("\nrealloc failed in moduleFileData::moduleFileData [ext = (char*)realloc(ext, strlen(ext) + 1);]");

		if (strlen(drive))
		{
			drive = (char*)realloc(drive, strlen(drive) + 1);
			if (!drive) throw AssemblerError("\nrealloc failed in moduleFileData::moduleFileData [drive = (char*)realloc(drive, strlen(drive) + 1);]");
		}

		else
		{
			drive = NULL;
		}

		if (strlen(dir))
		{
			dir = (char*)realloc(dir, strlen(dir) + 1);
			if (!dir) throw AssemblerError("\nrealloc failed in moduleFileData::moduleFileData [dir = (char*)realloc(dir, strlen(dir) + 1);]");
		}

		else
		{
			drive = NULL;
		}
	}

	~moduleFileData()
	{
		if (fullpath) free(fullpath);
		if (fileid)   free(fileid);
		if (ext)      free(ext);
		if (drive)    free(drive);
		if (dir)      free(dir);
	}
};

//
// FROM BELOW THIS LINE MARKS REDUNDANT CODE.
//

// takes a path(with .frt) to an .frt file and returns a
// path to the local folder.
//
// info: local folder refers to the folder of the file
// being assembled and doing the importing.
// 
// take a path to a module: "/next/start.frt", the local
// path would be this: "/next/". 
//
// the local path is used for importing files.
static
char*
get_local_path(char* path)
{
	char* ch       = path;
	u32   pathlen  = strlen(path);
	u32   pos      = 0;

	// first job is to get a ptr to the first char
	// of the

	// advance ptr along the path until
	// the exten's period is hit.
	while (*(ch++) != '.')
	{
		if ((pos++) > pathlen)
			return 0;
	}

	// now traverse backwards along the path
	// until a slash or the beginning of
	// the string is found.
	for (;; --ch)
	{
		if ((*ch == '/') || (*ch == '\\'))
			break;

		if (!(--pos))
			break;
	}

}

static
char*
SPARE_FUNCTION_extract_filename(char* path)
{
	char* ch = path;
	u32   pathlen = strlen(path);
	u32   pos = 0;

	// first job is to get a ptr to the first char
	// of the

	// advance ptr along the path until
	// the exten's period is hit.
	while (*(ch++) != '.')
	{
		if ((pos++) > pathlen)
			return 0;
	}

	// now traverse backwards along the path
	// until a slash or the beginning of
	// the string is found.
	for (;; --ch)
	{
		if ((*ch == '/') || (*ch == '\\'))
			break;

		if (!(--pos))
			break;
	}

}

// All taipan assembly text files are .frt files.
// fileid refers to the filename without the extension.
// race.frt : fileid is "race"
//
// get_fileid() returns a string of the fileid within a
// a file-path. null is returned upon all errors.
static
char*
get_fileid_ANY_PLATFORM(const char* path,
	       const char* required_extension)
{
	const char* exten_ptr = NULL;
	int exten_len = 0;
	int pos = -1;
	int slash_hit = 0;
	int colon_hit = 0;
	u32 exten_count = 0;
	int fileid_found = 0;

	char* retval = (char*)malloc(FILEID_MAX_LEN);
	if (!retval) return 0;

	// confirm that file-path is at least 3 chars long.
	if (strlen(path) < 3)
		goto func_fail;

	// iterate through chars in file-path one by one.
	do {
		// check if path is longer then max path len.
		// increment pos var aferwards.
		if ((pos++) > 259)
			goto func_fail;

		switch (*path)
		{
			// check for file-extension or '../'
		case '.':
			// check if next char is period denoting "../"
			if ((*(path + 1)) == '.')
			{
				// if pos isn't 0 then we have a char before the '.', confirm it's a slash or path is invalid.
				// eg: "../sfsd/" <- valid, "fssaf../saf/" <- invalid.
				if (pos)
				{
					if ((*(path - 1)) != '/')
					{
						if (*(path - 1) != '\\')
							goto func_fail;
					}
				}

				// check if next char after the ".." is a '/' or '\'
				if ((*(path + 2)) != '/')
				{
					if (*(path + 2) != '\\')
						goto func_fail;
				}

				// all is well so advance past it the "../"
				path++;
				pos++;
				continue;
			}

			// '.' char wasn't denoting "../" so it must be a file-extension.
			// if pos == 0 then we dont have a char before the '.' so path is invalid.
			if (!pos)
				goto func_fail;

			// set pointer at start of file extension we it can be checked after loop finishes.
			// then continue next iteration of loop.
			exten_ptr = path;
			continue;

			// these chars are invalid in windows paths and technically valid in unix paths
			// but strongly discouraged in them, so will be treated as invalid in both.
		case '<': case '>': case '"': case '|': case '+':
		case '?': case '*': case '-': case '`': case ',':
		case '!': case '#': case '$': case '%': case '^':
		case '&': case '(': case ')': case '=': case ';':
			goto func_fail;

			// check what platform we're on then check accordingly.
		case ':':
#if defined(_WIN32) || defined(_WIN64)
			// on windows ':' cannot come after a slash.
			if (slash_hit)
				goto func_fail;

			// also path can only have 1 ':' so check if already hit one.
			if (colon_hit)
				goto func_fail;

			// also cannot be the first char.
			if (!pos)
				goto func_fail;
			// also check if next char is not a slash, which is invalid.
			if ((*(path + 1)) != '/')
			{
				if (*(path + 1) != '\\')
					goto func_fail;
			}
			colon_hit = 1;
			continue;

#elif defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
			// on unix ':' is treated as a typical valid char.
			continue;
#endif

			// if theres a tilde confirm it's the first char.
		case '~':
			if (pos)
				goto func_fail;

		case '\\':
			// if on unix systems '\' is invalid.
#if defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
// on unix ':' is treated as a typical char.
			goto func_fail;
#endif
			continue;

		default:
			// check if we're inside the file extension and count the char so
			// extension length can be checked after the loop.

			// this is some dirty code we need to just execute on the first
			// call of this exten_ptr section of code in this switch stmt.
			//
			// this dirty code will copy the fileid from a path.
			if (exten_ptr)
			{
				exten_count++;
				if (exten_count == 1)
				{
					char* chb = (char*) path;
					char* xch = retval;

					// iterate backwards through the string until we
					// encounter a slash of some kind. "/fileid.frt"
					// so hitting the slash means we're at the start
					// of the fileid.
					for (;; --chb)
					{
						// either of these slashes indicate reaching 
						// beginning of a fileid.
						if ((*chb == '/') || (*chb == '\\'))
							break;
					}

					// increment path ptr so its pointing to first char after the slash.
					++chb;

					// copy from path var until '.'
					while (*chb != '.')
						*(xch++) = *(chb++);

					// null-terminate the return-fileid-string.
					xch = 0;

					// trim the memory to fit snug.
					retval = (char*) realloc(retval, strlen(retval) + 1);
					fileid_found = 1;

					// fileid has been found and neatly packed into a string.
					// function will complete its main execution now.
				}

				exten_len++;
			}
		}
	} while (*(++path));

	// check that last char wasn't a period, that there was
	// an extension in the path and it was at least 1 char long.
	if ((*(path - 1) != '.') && (exten_ptr && exten_len))
	{
		// check if there was a required extension, if not file-path is valid.
		if (strlen(required_extension))
		{
			// check if the file-path's extension matches the required extension.
			if (strcmp(exten_ptr, required_extension) == 0)
			{
				if (fileid_found)
					return retval;

				goto func_fail;

			}

			else
				goto func_fail;
		}
	}

func_fail:
	free(retval);
	return 0;
}



// All taipan assembly text files are .frt files.
// fileid refers to the filename without the extension.
// race.frt : fileid is "race"
//
// get_fileid() returns a string of the fileid within a
// a file-path. null is returned upon all errors.
static
char*
get_fileid_win32(const char* path,
	const char* required_extension)
{
	const char* exten_ptr = NULL;
	int exten_len = 0;
	int pos = -1;
	int slash_hit = 0;
	int colon_hit = 0;
	u32 exten_count = 0;
	int fileid_found = 0;

	char* retval = (char*)malloc(FILEID_MAX_LEN);
	if (!retval) return 0;

	// confirm that file-path is at least 3 chars long.
	if (strlen(path) < 3)
		goto func_fail;

	// iterate through chars in file-path one by one.
	do {
		// check if path is longer then max path len.
		// increment pos var aferwards.
		if ((pos++) > 259)
			goto func_fail;

		switch (*path)
		{
			// check for file-extension or '../'
		case '.':
			// check if next char is period denoting "../"
			if ((*(path + 1)) == '.')
			{
				// if pos isn't 0 then we have a char before the '.', confirm it's a slash or path is invalid.
				// eg: "../sfsd/" <- valid, "fssaf../saf/" <- invalid.
				if (pos)
				{
					if ((*(path - 1)) != '/')
					{
						if (*(path - 1) != '\\')
							goto func_fail;
					}
				}

				// check if next char after the ".." is a '/' or '\'
				if ((*(path + 2)) != '/')
				{
					if (*(path + 2) != '\\')
						goto func_fail;
				}

				// all is well so advance past it the "../"
				path++;
				pos++;
				continue;
			}

			// '.' char wasn't denoting "../" so it must be a file-extension.
			// if pos == 0 then we dont have a char before the '.' so path is invalid.
			if (!pos)
				goto func_fail;

			// set pointer at start of file extension we it can be checked after loop finishes.
			// then continue next iteration of loop.
			exten_ptr = path;
			continue;

			// these chars are invalid in windows paths and technically valid in unix paths
			// but strongly discouraged in them, so will be treated as invalid in both.
		case '<': case '>': case '"': case '|': case '+':
		case '?': case '*': case '-': case '`': case ',':
		case '!': case '#': case '$': case '%': case '^':
		case '&': case '(': case ')': case '=': case ';':
			goto func_fail;

		case ':':
			// on windows ':' cannot come after a slash.
			if (slash_hit)
				goto func_fail;

			// also path can only have 1 ':' so check if already hit one.
			if (colon_hit)
				goto func_fail;

			// also cannot be the first char.
			if (!pos)
				goto func_fail;
			// also check if next char is not a slash, which is invalid.
			if ((*(path + 1)) != '/')
			{
				if (*(path + 1) != '\\')
					goto func_fail;
			}

			colon_hit = 1;
			continue;

			// if theres a tilde confirm it's the first char.
		case '~':
			if (pos)
				goto func_fail;

		case '\\':
			continue;

		default:
			// check if we're inside the file extension and count the char so
			// extension length can be checked after the loop.

			if (exten_ptr)
			{
				exten_count++;
				if (exten_count == 1)
				{
					char* pbf = (char*)path;
					char* rbf = retval;

					// iterate backwards through the string until we
					// encounter a slash of some kind. "/fileid.frt"
					// so hitting the slash means we're at the start
					// of the fileid.
					for (;; --pbf)
					{
						// either of these slashes indicate reaching 
						// beginning of a fileid.

						if ((*pbf == '/') || (*pbf == '\\'))
							break;
					}

					// increment path ptr so its pointing to first char after the slash.
					++pbf;

					// copy from path var until '.'
					while (*pbf != '.')
						*(rbf++) = *(pbf++);

					// null-terminate the return-fileid-string.
					rbf = 0;

					// trim the memory to fit snug.
					retval = (char*)realloc(retval, strlen(retval) + 1);
					fileid_found = 1;

					// fileid has been found and neatly packed into a string.
					// function will complete its main execution now.
				}

				exten_len++;
			}
		}
	} while (*(++path));

	// check that last char wasn't a period, that there was
	// an extension in the path and it was at least 1 char long.
	if ((*(path - 1) != '.') && (exten_ptr && exten_len))
	{
		// check if there was a required extension, if not file-path is valid.
		if (strlen(required_extension))
		{
			// check if the file-path's extension doesn't
			// match the required extension.
			if (strcmp(exten_ptr, required_extension) == 1)
				goto func_fail;
		}

		// return the fileid-buffer ptr if fileid
		// found if not found goto func_fail.
		if (fileid_found)
			return retval;

		goto func_fail;
	}

// "safe" way to fail the function because it
// will ensure retval is freed.
func_fail:
	free(retval);
	return 0;
}

//
// FROM ABOVE THIS LINE MARKS REDUNDANT CODE.
//


// confirms validity of a file-path, set a required_extension(".exe") if needed.
// if any extension is okay set required_extension to "".
static
int
is_valid_filepath(const char* path, const char* required_extension)
{
    const char* exten_ptr = NULL;
    int exten_len = 0;
    int pos = -1;
    int slash_hit = 0;
    int colon_hit = 0;

    // confirm that file-path is at least 3 chars long.
    if (strlen(path) < 3) return 0;

    // iterate through chars in file-path one by one.
    do {
        // check if path is longer then max path len.
        // increment pos var aferwards.
        if ((pos++) > 259) return 0;

        switch (*path)
		{
            // check for file-extension or '../'
            case '.':
                // check if next char is period denoting "../"
                if ((*(path + 1)) == '.')
				{
                    // if pos isn't 0 then we have a char before the '.', confirm it's a slash or path is invalid.
                    // eg: "../sfsd/" <- valid, "fssaf../saf/" <- invalid.
                    if (pos)
					{
                        if ((*(path - 1)) != '/')
						{
                            if (*(path - 1) != '\\')
                                return 0;
                        }
                    }

                    // check if next char after the ".." is a '/' or '\'
                    if ((*(path + 2)) != '/')
					{
                        if (*(path + 2) != '\\')
                            return 0;
                    }

                    // all is well so advance past it the "../"
                    path++;
                    pos++;
                    continue;
                }

                // '.' char wasn't denoting "../" so it must be a file-extension.
                // if pos == 0 then we dont have a char before the '.' so path is invalid.
                if (!pos) return 0;

                // set pointer at start of file extension we it can be checked after loop finishes.
                // then continue next iteration of loop.
                exten_ptr = path;
                continue;

            // these chars are invalid in windows paths and technically valid in unix paths
            // but strongly discouraged in them, so will be treated as invalid in both.
            case '<': case '>': case '"': case '|': case '+':
            case '?': case '*': case '-': case '`': case ',':
            case '!': case '#': case '$': case '%': case '^':
            case '&': case '(': case ')': case '=': case ';':
                return 0;

            // check what platform we're on then check accordingly.
            case ':':
                #if defined(_WIN32) || defined(_WIN64)
                // on windows ':' cannot come after a slash.
                if (slash_hit) return 0;
                // also path can only have 1 ':' so check if already hit one.
                if (colon_hit) return 0;
                // also cannot be the first char.
                if (!pos) return 0;
                // also check if next char is not a slash, which is invalid.
                if ((*(path + 1)) != '/')
				{
                    if (*(path + 1) != '\\')
                        return 0;
                }
                colon_hit = 1;
                continue;

                #elif defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
                // on unix ':' is treated as a typical valid char.
                continue;
                #endif

            // if theres a tilde confirm it's the first char.
            case '~':
                if (pos) return 0;

            case '\\':
                // if on unix systems '\' is invalid.
                #if defined(__unix__) || defined(__unix) || defined(__APPLE__) && defined(__MACH__)
                // on unix ':' is treated as a typical char.
                return 0;
                #endif
                continue;

            default:
                // check if we're inside the file extension and count the char so
                // extension length can be checked after the loop.
                if (exten_ptr)
                    exten_len++;
            }
    } while (*(++path));

    // check that last char wasn't a period, that there was
    // an extension in the path and it was at least 1 char long.
    if ((*(path - 1) != '.') && (exten_ptr && exten_len))
	{
        // check if there was a required extension, if not file-path is valid.
        if (strlen(required_extension))
		{
            // check if the file-path's extension matches the required extension.
            return strcmp(exten_ptr, required_extension) == 0;
        }
        else {
            return 1;
        }
    }

    return 0;
}

static
u8
count_digits(i64 num)
{
    u8 count = 0;
    if (num == 0) return 1;
    for (; (num /= 10) != 0; count++);
    return count;
}


// ALL FILE IO, MEMORY ALLOCATON/DEALLOCATION AND ANY OTHER
// NON PROGRAM-RUNTIME RELATED ERRORS ARE DEALT WITH BY
// THROWING AN INSTANCE OF THIS CLASS.
//
// -refer to the asmErrorObject defined in parser.h for information
// on how program-runtime errors are handled. program-runtime errors
// being input text errors, syntax/token errors ect.
struct
AssemblerError : public std::exception
{
	const char* errmsg;
	const u8    errcode;

	AssemblerError(const char* _msg,
		           const u8    _errcode = 0)
	: 
	errmsg(_msg),
	errcode(_errcode) {}

	void
	runtime_end_err_handler()
	{
		std::terminate();
	}
	
	void
	print_errmsg()
	{
		printf("\n%s", errmsg);
	}
};

static
size_t
load_program_from_file(char* file_path,
	                   u8*   mem)
{
	FILE*  file;
	size_t file_size;
	size_t bytes_read;

	file = fopen(file_path, "rb");

	if (file == NULL)
		return 0;

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	bytes_read = fread(mem, sizeof(u8), file_size, file);

	if (bytes_read != file_size)
		return 0;

	fclose(file);
	return bytes_read;
}


#endif // VM.H