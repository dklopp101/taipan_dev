
import datetime
import pickle
import struct
import copy
import sys
import os

# .FBIN MEMORY-MAP
#                                           
# 0x00 (0)                      : START_INSTR_ADDR
# 0x04 (4)                      : PROG_SIZE
# 0x08 (8)                      : FLAGS
# 0x0A (10)                     : CREATION_DATE
# 0x0C (12)                     : FIRST_INSTR_ADDR
# 0x10 (16)                     : LAST_INSTR_ADDR
# 0x24 (20)                     : FIRST_USER_BYTE_ADDR
# 0x18 (24)                     : TOTAL_SYMTAB_SIZE
# 0x1C (28)                     : LABEL_TABLE_SIZE
# 0x20 (32)                     : MACRO_TABLE_SIZE
# 0x24 (36)                     : SERIALISED_LABEL_TABLE
# 0x24 (36) + LABEL_TABLE_SIZE  : SERIALISED_MACRO_TABLE
# 0x18 (24) + TOTAL_SYMTAB_SIZE : FIRST_PROGRAM_INSTRUCTION
#                              : PROGRAM

SIGNED_SIGNCODE   = 1
UNSIGNED_SIGNCODE = 2
NOSIGN_SIGNCODE   = 3
ADDR_SIGNCODE     = 4

signcode_mnemonic_tbl = { 1 : 'SIGNED-INT',
                          2 : 'UNSIGNED-INT',
                          3 : 'NOSIGN-INT',
                          4 : 'ADDRESS-INT'
}

WORDSIZE    = 4
OPCODE_SIZE = 1

MAX_ID_LEN  = 40
MIN_ID_LEN  = 1

START_LABEL_IDENTIFIER = 'main'
MACRO_KEYWORD          = 'macro'
LABEL_DEF_SUFFIX       = ':'
SIGNED_INT_PREFIX      = '$'
UNSIGNED_INT_PREFIX    = '%'
ADDR_INT_PREFIX        = '@'

# dict mapping mnemonic to their opcodes.
optbl = {
    'die'     :  0,   # 0
    'nop'     :  1,   # 1
    'nspct'   :  2,   # 2
    'nspctst' :  3,   # 3
    'test_die':  4,   # 4
    'call'    :  5,   # 5
    'ret'     :  6,   # 6
    'swtch'   :  7,   # 7
    'jmp'     :  8,   # 8
    'je'      :  9,   # 9
    'jn'      : 10,   # 10
    'jl'      : 11,   # 11
    'jg'      : 12,   # 12
    'jls'     : 13,   # 13
    'jgs'     : 14,   # 14
    'loop'    : 15,   # 15
    'lcont'   : 16,   # 16
    'lbrk'    : 17,   # 17
    'psh'     : 18,   # 18
    'pop'     : 19,   # 19
    'pop2'    : 20,   # 20
    'popn'    : 21,   # 21
    'pshfr'   : 22,   # 22
    'poptr'   : 23,   # 23
    'movtr'   : 24,   # 24
    'stktr'   : 25,   # 25
    'cpyr'    : 26,   # 26
    'setr'    : 27,   # 27
    'pshfrr'  : 28,   # 28
    'pshfrs'  : 29,   # 29
    'inc'     : 30,   # 30
    'dec'     : 31,   # 31
    'add'     : 32,   # 32
    'sub'     : 33,   # 33
    'mul'     : 34,   # 34
    'div'     : 35,   # 35
    'mod'     : 36,   # 36
    'incs'    : 37,   # 37
    'decs'    : 38,   # 38
    'adds'    : 39,   # 39
    'subs'    : 40,   # 40
    'muls'    : 41,   # 41
    'divs'    : 42,   # 42
    'mods'    : 43,   # 43
    'and'     : 44,   # 44
    'not'     : 45,   # 45
    'xor'     : 46,   # 46
    'or'      : 47,   # 47
    'lshft'   : 48,   # 48
    'rshft'   : 49,   # 49
    'lrot'    : 50,   # 50
    'rrot'    : 51,   # 51
    'ands'    : 52,   # 52
    'nots'    : 53,   # 53
    'xors'    : 54,   # 54
    'ors'     : 55,   # 55
    'lshfts'  : 56,   # 56
    'rshfts'  : 57,   # 57
    'lrots'   : 58,   # 58
    'rrots'   : 59    # 59
}


# dict mapping opcodes to their mnemonic.
mnemonic_tbl = {
    0  : 'die',
    1  : 'nop',
    2  : 'nspct',
    3  : 'nspctst',
    4  : 'test_die',
    5  : 'call',
    6  : 'ret',
    7  : 'swtch',
    8  : 'jmp',
    9  : 'je',
    10 : 'jn',
    11 : 'jl',
    12 : 'jg',
    13 : 'jls',
    14 : 'jgs',
    15 : 'loop',
    16 : 'lcont',
    17 : 'lbrk',
    18 : 'psh',
    19 : 'pop',
    20 : 'pop2',
    21 : 'popn',
    22 : 'pshfr',
    23 : 'poptr',
    24 : 'movtr',
    25 : 'stktr',
    26 : 'cpyr',
    27 : 'setr',
    28 : 'pshfrr',
    29 : 'pshfrs',
    30 : 'inc',
    31 : 'dec',
    32 : 'add',
    33 : 'sub',
    34 : 'mul',
    35 : 'div',
    36 : 'mod',
    37 : 'incs',
    38 : 'decs',
    39 : 'adds',
    40 : 'subs',
    41 : 'muls',
    42 : 'divs',
    43 : 'mods',
    44 : 'and',
    45 : 'not',
    46 : 'xor',
    47 : 'or',
    48 : 'lshft',
    49 : 'rshft',
    50 : 'lrot',
    51 : 'rrot',
    52 : 'ands',
    53 : 'nots',
    54 : 'xors',
    55 : 'ors',
    56 : 'lshfts',
    57 : 'rshfts',
    58 : 'lrots',
    59 : 'rrots'
}


# dict mapping opcodes to their sizes in memory.
opsize_tbl = {
    0  : 1,  # 'die'
    1  : 1,  # 'nop'
    2  : 5,  # 'nspct'
    3  : 5,  # 'nspctst'
    4  : 1,  # 'test_die'
    5  : 5,  # 'call'
    6  : 1,  # 'ret'
    7  : 1,  # 'swtch'
    8  : 5,  # 'jmp'
    9  : 5,  # 'je'
    10 : 5,  # 'jn'
    11 : 5,  # 'jl'
    12 : 5,  # 'jg'
    13 : 5,  # 'jls'
    14 : 5,  # 'jgs'
    15 : 13,  # 'loop'
    16 : 1,  # 'lcont'
    17 : 1,  # 'lbrk'
    18 : 5,  # 'psh'
    19 : 1,  # 'pop'
    20 : 1,  # 'pop2'
    21 : 5,  # 'popn'
    22 : 5,  # 'pshfr'
    23 : 5,  # 'poptr'
    24 : 5,  # 'movtr'
    25 : 9,  # 'stktr'
    26 : 9,  # 'cpyr'
    27 : 9,  # 'setr'
    28 : 5,  # 'pshfrr'
    29 : 5,  # 'pshfrs'
    30 : 1,  # 'inc'
    31 : 1,  # 'dec'
    32 : 1,  # 'add'
    33 : 1,  # 'sub'
    34 : 1,  # 'mul'
    35 : 1,  # 'div'
    36 : 1,  # 'mod'
    37 : 1,  # 'incs'
    38 : 1,  # 'decs'
    39 : 1,  # 'adds'
    40 : 1,  # 'subs'
    41 : 1,  # 'muls'
    42 : 1,  # 'divs'
    43 : 1,  # 'mods'
    44 : 1,  # 'and'
    45 : 1,  # 'not'
    46 : 1,  # 'xor'
    47 : 1,  # 'or'
    48 : 1,  # 'lshft'
    49 : 1,  # 'rshft'
    50 : 1,  # 'lrot'
    51 : 1,  # 'rrot'
    52 : 1,  # 'ands'
    53 : 1,  # 'nots'
    54 : 1,  # 'xors'
    55 : 1,  # 'ors'
    56 : 1,  # 'lshfts'
    57 : 1,  # 'rshfts'
    58 : 1,  # 'lrots'
    59 : 1   # 'rrots'
}


# dict that maps opcodes to their operand count.
opargc_tbl = { 
    0  : 0,  # 'die'
    1  : 0,  # 'nop'
    2  : 1,  # 'nspct'
    3  : 1,  # 'nspctst'
    4  : 0,  # 'test_die'
    5  : 1,  # 'call'
    6  : 0,  # 'ret'
    7  : 0,  # 'swtch'
    8  : 1,  # 'jmp'
    9  : 1,  # 'je'
    10 : 1,  # 'jn'
    11 : 1,  # 'jl'
    12 : 1,  # 'jg'
    13 : 1,  # 'jls'
    14 : 1,  # 'jgs'
    15 : 3,  # 'loop'
    16 : 1,  # 'lcont'
    17 : 0,  # 'lbrk'
    18 : 1,  # 'psh'
    19 : 0,  # 'pop'
    20 : 0,  # 'pop2'
    21 : 1,  # 'popn'
    22 : 1,  # 'pshfr'
    23 : 1,  # 'poptr'
    24 : 1,  # 'movtr'
    25 : 2,  # 'stktr'
    26 : 2,  # 'cpyr'
    27 : 2,  # 'setr'
    28 : 1,  # 'pshfrr'
    29 : 1,  # 'pshfrs'
    30 : 0,  # 'inc'
    31 : 0,  # 'dec'
    32 : 0,  # 'add'
    33 : 0,  # 'sub'
    34 : 0,  # 'mul'
    35 : 0,  # 'div'
    36 : 0,  # 'mod'
    37 : 0,  # 'incs'
    38 : 0,  # 'decs'
    39 : 0,  # 'adds'
    40 : 0,  # 'subs'
    41 : 0,  # 'muls'
    42 : 0,  # 'divs'
    43 : 0,  # 'mods'
    44 : 0,  # 'and'
    45 : 0,  # 'not'
    46 : 0,  # 'xor'
    47 : 0,  # 'or'
    48 : 0,  # 'lshft'
    49 : 0,  # 'rshft'
    50 : 0,  # 'lrot'
    51 : 0,  # 'rrot'
    52 : 0,  # 'ands'
    53 : 0,  # 'nots'
    54 : 0,  # 'xors'
    55 : 0,  # 'ors'
    56 : 0,  # 'lshfts'
    57 : 0,  # 'rshfts'
    58 : 0,  # 'lrots'
    59 : 0   # 'rrots'
}

oprtype_tbl = { 
    0  : 0,  # 'die'
    1  : 0,  # 'nop'
    2  : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'nspct'
    3  : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'nspctst'
    4  : 0,  # 'test_die'
    5  : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'call'
    6  : 0,  # 'ret'
    7  : 0,  # 'swtch'
    8  : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE), # 'jmp'
    9  : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'je'
    10 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'jn'
    11 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'jl'
    12 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'jg'
    13 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'jls'
    14 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE), # 'jgs'
    15 : (UNSIGNED_SIGNCODE, (ADDR_SIGNCODE, UNSIGNED_SIGNCODE), (ADDR_SIGNCODE, UNSIGNED_SIGNCODE)), # 'loop'
    16 : 0,  # 'lcont'
    17 : 0,  # 'lbrk'
    18 : (UNSIGNED_SIGNCODE, SIGNED_SIGNCODE, ADDR_SIGNCODE),  # 'psh'
    19 : 0,  # 'pop'
    20 : 0,  # 'pop2'
    21 : (UNSIGNED_SIGNCODE),  # 'popn'
    22 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'pshfr'
    23 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'poptr'
    24 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'movtr'
    25 : ((ADDR_SIGNCODE, UNSIGNED_SIGNCODE), (ADDR_SIGNCODE, UNSIGNED_SIGNCODE)),  # 'stktr'
    26 : ((ADDR_SIGNCODE, UNSIGNED_SIGNCODE), (ADDR_SIGNCODE, UNSIGNED_SIGNCODE)),  # 'cpyr'
    27 : ((ADDR_SIGNCODE, UNSIGNED_SIGNCODE), (UNSIGNED_SIGNCODE, SIGNED_SIGNCODE, ADDR_SIGNCODE)),  # 'setr'
    28 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'pshfrr'
    29 : (ADDR_SIGNCODE, UNSIGNED_SIGNCODE),  # 'pshfrs'
    30 : 0,  # 'inc'
    31 : 0,  # 'dec'
    32 : 0,  # 'add'
    33 : 0,  # 'sub'
    34 : 0,  # 'mul'
    35 : 0,  # 'div'
    36 : 0,  # 'mod'
    37 : 0,  # 'incs'
    38 : 0,  # 'decs'
    39 : 0,  # 'adds'
    40 : 0,  # 'subs'
    41 : 0,  # 'muls'
    42 : 0,  # 'divs'
    43 : 0,  # 'mods'
    44 : 0,  # 'and'
    45 : 0,  # 'not'
    46 : 0,  # 'xor'
    47 : 0,  # 'or'
    48 : 0,  # 'lshft'
    49 : 0,  # 'rshft'
    50 : 0,  # 'lrot'
    51 : 0,  # 'rrot'
    52 : 0,  # 'ands'
    53 : 0,  # 'nots'
    54 : 0,  # 'xors'
    55 : 0,  # 'ors'
    56 : 0,  # 'lshfts'
    57 : 0,  # 'rshfts'
    58 : 0,  # 'lrots'
    59 : 0   # 'rrots'
}

DEFAULT_OUTPUT_PATH = "yson_output.fbin"

# METADATA_SEGMENT_SIZE, doesn't refer to the full metadata-segment-size, due to
# how i wrote yson, the 3 uint32 values, total-symtab-size, label-symtab-size &
# macro-symtab-size are excluded from what is refered to as "the metadata" within
# this file. the actual full metadata size is METADATA_SEGMENT_SIZE + 12.
# see ram.h for more information on the metadata and symbol-table.
METADATA_SEGMENT_SIZE = 24 # size measured in bytes.

METADATA_SEGMENT      = 0 # index within prog-list of metadata-list.
METADATA_FIELD_COUNT  = 7

# indexes of metadata items within the metadata list.
START_ADDR_NDX           = 0 
PROG_SIZE_NDX            = 1
FLAGS_NDX                = 2
CREATION_DATE_NDX        = 3
FIRST_INSTR_ADDR_NDX     = 4
LAST_INSTR_ADDR_NDX      = 5
FIRST_USER_BYTE_ADDR_NDX = 6 # first byte after last program instruction.

def isDecInteger(string):
# returns 0 if not an integer.
# returns 1 if an unsigned integer.
# returns 2 if a signed integer.
# returns 3 if no prefix.

    retval = NOSIGN_SIGNCODE
    s = string

    if s[0] == UNSIGNED_INT_PREFIX:
        retval = UNSIGNED_SIGNCODE
        s = string[1:]

    elif s[0] in (SIGNED_INT_PREFIX, '-', '+'):
        retval = SIGNED_SIGNCODE
        s = string[1:]

    elif s[0] == ADDR_INT_PREFIX:
        retval = ADDR_SIGNCODE
        s = string[1:]

    try:
        int(s)
    except ValueError:
        return 0
    else:
        return retval

def isHexInteger(string):
    retval = NOSIGN_SIGNCODE
    s = string

    if s[0] == UNSIGNED_INT_PREFIX:
        retval = UNSIGNED_SIGNCODE
        s = string[1:]

    elif s[0] in (SIGNED_INT_PREFIX, '-', '+'):
        retval = SIGNED_SIGNCODE
        s = string[1:]

    elif s[0] == ADDR_INT_PREFIX:
        retval = ADDR_SIGNCODE
        s = string[1:]

    elif s[0:2] != '0x':
        return 0

    # test whether s is a valid hex value.
    try:
        int(s, 16)
    except ValueError:
        return 0
    else:
        return retval

def isIdentifier(string):
# returns -1 if too short
# returns -2 if too long
# returns -3 if first char is numeric.
# returns -4 if invalid char in string.
# returns  1 if string is valid identifier.

    length = len(string)

    # check if identifer an invalid length.
    if length < MIN_ID_LEN: return -1
    if length > MAX_ID_LEN: return -2

    # check if first char is a number.
    if string[0].isnumeric(): return -3

    # iterate through strings chars and check them one by one.
    # chars can only be underscores or alphanumeric chars, except
    # first char can be a dollarsign.
    for i in range(len(string)):

        # is char not alphanum and isn't an underscore?
        if not string[i].isalnum() and string[i] != '_':
            return -4

    return 1

class SymbolTable:
# throughout this class and Assembler() the term 'symtab'/'symbol table'
# refers to the label-table and macro-table both, together. they make up
# what is the symbol-table.
#
# NOTE: this class only checks that the label & macro values are a 2 item list.
#       it's the owner of the SymbolTable() object's responsibility to ensure
#       that the correct items are in said list. (SIGNCODE, INTEGER_VALUE)

    def __init__(self,
                 keep_symbols = None ):

        self.labels_bytearray = bytearray()
        self.macros_bytearray = bytearray()
        self.bytearray        = bytearray()
        self.labels_size      = 0
        self.macros_size      = 0
        self.bytearray_size   = 0
        self.labels           = {}
        self.macros           = {}
        self.keep_symbols     = keep_symbols

    def calculate_addr_offsets(self):
    # furst-vm reads the entire .fbin contents into it's RAM.
    # if symbols have been kept this then includes the entire symtab.
    # this means all labels and addr macros can only have their values
    # calculated once the final size of the symtab is known. so this is
    # done *after* assembly. doing it this way allows macros to be defined
    # anywhere in the file in future.

        if self.keep_symbols:

            for label_id in self.labels.keys():
                self.labels[label_id][1] += self.bytearray_size

            for macro_id in self.macros.keys():
                if self.macros[macro_id][0] == ADDR_SIGNCODE:
                    self.macros[macro_id][1] += self.bytearray_size

    def build_label_tbl_str(self):
        if len(self.labels):
            s = ''
        else:
            return 'Label Table Empty'
        for label_id in self.labels.keys():
            s += '\n[%s] = %d' % (label_id, self.labels[label_id][1])
        return s

    def build_macro_tbl_str(self):
        if len(self.macros):
            s = ''
        else:
            return 'Macro Table Empty'
        for macro_id in self.macros.keys():
            s += '\n[%s] = %d' % (macro_id, self.macros[macro_id][1])
        return s

    def print_symtbl(self):
        s  = 'LABELS:\n%s' % self.build_label_tbl_str()
        s += '\n\nMACROS:\n%s' % self.build_macro_tbl_str()
        print(s)

    def new_label(self, id, int_list):
        if id in self.labels:
            raise Exception("INTERNAL ERROR: label [%s] declared twice!" % id)

        if not isinstance(int_list, list):
            raise Exception("INTERNAL ERROR: label [%s] must be a list!" % id)

        self.labels[id] = int_list

    def new_macro(self, id, int_list):
        if id in self.macros:
            raise Exception("INTERNAL ERROR: macro [%s] declared twice!" % id)

        if not isinstance(int_list, list):
            raise Exception("INTERNAL ERROR: macro [%s] must be an integer!" % id)

        self.macros[id] = int_list

    def build_label_bytearray(self):
        try:
            # first serialise the label dict & grab its size.
            data = pickle.dumps(self.labels)

            # set aside size of label table for when we build the symtbl bytearray.
            self.labels_size = len(data)

            # write label table's bytes.

            for Byte in data:
                self.labels_bytearray.append(Byte)

        except pickle.PicklingError as e:
            raise Exception("INTERNAL ERROR: Error when building label-table bytearray!\n  More info: %s" % e)

    def build_macro_bytearray(self):
        try:
            # first serialise the macro dict & grab its size.
            data = pickle.dumps(self.macros)

            # set aside size of macro table for when we build the symtbl bytearray.
            self.macros_size = len(data)

            # write label table's bytes.

            for Byte in data:
                self.macros_bytearray.append(Byte)

        except pickle.PicklingError as e:
            raise Exception("INTERNAL ERROR: Error when building macro-table bytearray!\n  More info: %s" % e)

    def build_bytearray(self):
		# .FBIN MEMORY-MAP
        #                                           
		# 0x00                         : METADATA_SEGMENT       
		# 0x18 (24)                     : TOTAL_SYMTAB_SIZE  ------
        # 0x1C (28)                     : LABEL_TABLE_SIZE         |
		# 0x20 (32)                     : MACRO_TABLE_SIZE         |--> self.bytearray when when in .fbin file.
		# 0x24 (36)                     : SERIALISED_LABEL_TABLE   |
		# 0x24 (36) + LABEL_TABLE_SIZE  : SERIALISED_MACRO_TABLE --
        # 0x18 (24) + TOTAL_SYMTAB_SIZE : FIRST_PROGRAM_INSTRUCTION
        #
        # NOTE: actual size of symtab is LABEL_TABLE_SIZE + MACRO_TABLE_SIZE
        #       TOTAL_SYMTAB_SIZE is LABEL_TABLE_SIZE + MACRO_TABLE_SIZE + 12(4*3, 3 size values, TOTAL_SYMTAB_SIZE, LABEL_SYMTAB_SIZE, MACRO_SYMTAB_SIZE)
		
        try:
            self.labels_size    = len(pickle.dumps(self.labels))
            self.macros_size    = len(pickle.dumps(self.macros))

            # the 12 below refers to the size of the memory for the 3 size values,
            # TOTAL_SYMTAB_SIZE, LABEL_SYMTAB_SIZE, MACRO_SYMTAB_SIZE
            self.bytearray_size = 12 + self.labels_size + self.macros_size

        except pickle.PicklingError as e:
            raise Exception("INTERNAL ERROR: Error when building symbol-table bytearray!\n  More info: %s" % e)

        # now we calculate addr-offsets and update labels & addr-macros.
        self.calculate_addr_offsets()

        # now do the proper final serialisation of the symtab.
        try:
            # build label & macro bytearrays.
            self.build_label_bytearray()
            self.build_macro_bytearray()

            # first write sizes to symtab-bytearray.
            self.bytearray.extend(struct.pack('<I', self.bytearray_size))
            self.bytearray.extend(struct.pack('<I', self.labels_size))
            self.bytearray.extend(struct.pack('<I', self.macros_size))

            # now copy the labels and macros bytes to the bytearray.
            self.bytearray.extend(self.labels_bytearray)
            self.bytearray.extend(self.macros_bytearray)

        except struct.error as e:
            raise Exception("INTERNAL ERROR: Error when building symbol-table bytearray!\n  More info: %s" % e)

        # quick check on the bytearray size, not really required but just for safety.
        if len(self.bytearray) != self.bytearray_size:
            s  = "INTERNAL ERROR: error here: [if len(self.bytearray) != self.bytearray_size:] !!!\n"
            s += "len(self.bytearray)=%d, self.bytearray_size=%d" % (len(self.bytearray), self.bytearray_size)
            raise Exception(s)

        return self.bytearray

class Assembler:
# Input file is assembled into a list of instruction lists.
# The first list contains the address of the first instruction
# to be executed. Hence why self.prog is initialised to [[0]].
# once the main label is declared 0 is overwritten with the 
# corresponding address of the main label.
# 
# When furst-vm executes a program it looks at the first address'
# value to set the program-counter to, so prog[0][0] holds the address
# of the first instruction to be executed.
#
# input_path   -> .frt file being assembled.
# output_path  -> .fbin file produced by assembling input .frt file.
# keep_symbols -> flag determining whether or not symbols are kept in output-file.
# show_all_ds  -> flag used for printing out all data structures and other data used for debugging ect.

    def __init__(self,
                 input_path   = None,
                 output_path  = None,
                 keep_symbols = True,
                 show_all_ds  = True ):

        self.input_path           = input_path
        self.output_path          = output_path
        self.keep_symbols         = keep_symbols # symbols kept on by default.
        self.show_all_ds          = show_all_ds # show-all-data-structures
        self.symtbl               = SymbolTable(keep_symbols) # holds labels & macros.
        self.prog_list            = self.init_prog_list()
        self.prog_bytearray       = bytearray()
        self.metadata_bytearray   = bytearray()
        self.input_file           = None
        self.output_file          = None
        self.in_comment           = False
        self.prog_byte_count      = 0
        self.prog_instr_count     = 0
        self.line_tokens          = None
        self.tokndx               = -1
        self.next_addr            = METADATA_SEGMENT_SIZE
        self.linenum              = 0
        self.first_instr_addr     = None
        self.last_instr_addr      = None
        self.first_user_byte_addr = None
        self.tok                  = None

    def print_prog_list(self):
        print("program-list: \n%s" % self.prog_list)
        print("\nprogram-list breakdown:")
        for i in range(len(self.prog_list)):

            if i == 0:
                print("metadata list: %s" % self.prog_list[i])
                continue

            for x in range(len(self.prog_list[i])):

                if x == 0: continue # skip the instr addr item in instr-list.

                if x == 1: # print instr opcode info.
                    print("\nopcode: %d(%s)" % (self.prog_list[i][x], mnemonic_tbl[self.prog_list[i][x]]))
                    continue

                print("arg%d: %d(%s)" % (x-1, self.prog_list[i][x][1], signcode_mnemonic_tbl[self.prog_list[i][x][0]]))

    def get_creation_date(self):
    # returns integer where bits are representing the current datestamp.

        datestamp = datetime.datetime.now()
        year      = datestamp.year % 100  # Get the last two digits of the year
        month     = datestamp.month
        day       = datestamp.day

        return ((year << 9) | (month << 5) | day) & 0xFFFF

    def get_flags(self):
    # returns integer holding the flags, may be expanded in future this why it has it's own function.
        return int(self.keep_symbols)

    def init_prog_list(self):
    # initialises the program-list which will hold the metadata and instruction lists.
    # this program-list will be used to build the final bytearray of the program.

        # build metadata list then set flags and creation date.
        metadata = [0] * METADATA_FIELD_COUNT

        # only flags & creation-date are written now, rest of metadata 
        # is written after program assembly has been performed.
        metadata[FLAGS_NDX]         = self.keep_symbols
        metadata[CREATION_DATE_NDX] = self.get_creation_date()

        # return program_list with first item being metadata list.
        return [metadata]

    def build_metadata_bytearray(self):
		# .FBIN MEMORY-MAP
        #                                           
		# 0x00 (0)                      : START_INSTR_ADDR
		# 0x04 (4)                      : PROG_SIZE
		# 0x08 (8)                      : FLAGS
        # 0x0A (10)                     : CREATION_DATE
		# 0x0C (12)                     : FIRST_INSTR_ADDR
		# 0x10 (16)                     : LAST_INSTR_ADDR
		# 0x24 (20)                     : FIRST_USER_BYTE_ADDR
		# 0x18 (24)                     : TOTAL_SYMTAB_SIZE
        # 0x1C (28)                     : LABEL_TABLE_SIZE
		# 0x20 (32)                     : MACRO_TABLE_SIZE
		# 0x24 (36)                     : SERIALISED_LABEL_TABLE
		# 0x24 (36) + LABEL_TABLE_SIZE  : SERIALISED_MACRO_TABLE
        # 0x18 (24) + TOTAL_SYMTAB_SIZE : FIRST_PROGRAM_INSTRUCTION

        # if symbols are kept build it's bytearray.
        if self.keep_symbols:
            self.symtbl.build_bytearray()

        # calculate last-instr & first-user(first byte after last instr) addrs.
        self.first_instr_addr     = self.symtbl.bytearray_size + METADATA_SEGMENT_SIZE
        self.last_instr_addr      = self.first_instr_addr + (self.prog_list[METADATA_SEGMENT][PROG_SIZE_NDX] - opsize_tbl[self.prog_list[-1][1]])
        self.first_user_byte_addr = self.last_instr_addr + opsize_tbl[self.prog_list[-1][1]]
        

        # update the metadata-segment list.
        self.prog_list[METADATA_SEGMENT][START_ADDR_NDX]            = self.symtbl.labels[START_LABEL_IDENTIFIER][1]
        self.prog_list[METADATA_SEGMENT][FIRST_INSTR_ADDR_NDX]      = self.first_instr_addr
        self.prog_list[METADATA_SEGMENT][LAST_INSTR_ADDR_NDX]       = self.last_instr_addr
        self.prog_list[METADATA_SEGMENT][FIRST_USER_BYTE_ADDR_NDX]  = self.first_user_byte_addr

        try:
            # now we can build the metadata-segment bytearray.
            self.metadata_bytearray.extend(struct.pack('<I', self.prog_list[METADATA_SEGMENT][START_ADDR_NDX]))
            self.metadata_bytearray.extend(struct.pack('<I', self.prog_list[METADATA_SEGMENT][PROG_SIZE_NDX]))
            self.metadata_bytearray.extend(struct.pack('<H', self.prog_list[METADATA_SEGMENT][FLAGS_NDX]))
            self.metadata_bytearray.extend(struct.pack('<H', self.prog_list[METADATA_SEGMENT][CREATION_DATE_NDX]))
            self.metadata_bytearray.extend(struct.pack('<I', self.prog_list[METADATA_SEGMENT][FIRST_INSTR_ADDR_NDX]))
            self.metadata_bytearray.extend(struct.pack('<I', self.prog_list[METADATA_SEGMENT][LAST_INSTR_ADDR_NDX]))
            self.metadata_bytearray.extend(struct.pack('<I', self.prog_list[METADATA_SEGMENT][FIRST_USER_BYTE_ADDR_NDX]))

        except struct.error:
            print('\nINTERNAL ERROR: Error when packing bytes into metadata bytearray!')
            sys.exit()

        # append on the symtab bytearray if we need to.
        if self.keep_symbols:
            self.metadata_bytearray.extend(self.symtbl.bytearray)

    def build_prog_bytearray(self):
    # takes the completed program-list(self.prog_list) and builds a bytearray from it.
    # this bytearray is exactly what will be written to the output file and executed by vm.

        # confirm there's actually program data.
        # test is < 2 because first item in program-list is metadata list.
        # so an empty program-list has at least one item in it.
        if len(self.prog_list) < 2:
            raise Exception("yson: No program data to assemble!")

        # all furst programs must have a main label 
        # so lets confirm that we have one.
        self.confirm_main_label_exists()

        # first build then append metadata bytearray to prog-bytearray which is empty bytearray right now.
        self.build_metadata_bytearray()
        self.prog_bytearray.extend(self.metadata_bytearray)

        # iterate through prog-list, then iterate through the instr-lists within prog-list.
        # any instr's operands that are a string are label or macro id's. these need to be
        # replaced with their respective values stored in the symbol-table. 
        #
        # label/macros are stored in the instr-lists as the id's not their values because the
        # actual values sometimes arent accurate until after the metadata bytearray has been
        # built. see functions calculate_addr_offsets() & build_bytearray() in SymbolTable()
        # for further information.
        for instr_ndx in range(len(self.prog_list)):

            if not instr_ndx: continue # skip first item in prog-list, the metadata list.

            for arg_ndx in range(len(self.prog_list[instr_ndx])):

                if not arg_ndx: continue # ignore first item in instr list as its the opcode.

                # check if instr-arg is a str if so it is a macro or label id.
                if isinstance(self.prog_list[instr_ndx][arg_ndx], str):

                    # replace all macros & labels with their values.
                    if self.prog_list[instr_ndx][arg_ndx] in self.symtbl.labels:
                        self.prog_list[instr_ndx][arg_ndx] = self.symtbl.labels[self.prog_list[instr_ndx][arg_ndx]]

                    elif self.prog_list[instr_ndx][arg_ndx] in self.symtbl.macros:
                        self.prog_list[instr_ndx][arg_ndx] = self.symtbl.macros[self.prog_list[instr_ndx][arg_ndx]]

                    # if an operand is a string but isn't a macro/label id then we have an error.
                    else:
                        raise Exception("yson: Invalid instruction operand, string: [%s] does not match any identifiers in the symbol-table" % self.prog_list[instr_ndx][arg_ndx])

            # instr-lists are now fully accurate and ready to be packed and appended onto the prog-bytearray.
            self.pack_instr_bytes(self.prog_list[instr_ndx])

    def pack_instr_bytes(self, instr):
        try:
            # write the opcode to prog-bytearray.
            # opcodes are literal int, NOT an int_list.

            if not isinstance(instr[1], int):
                raise Exception("INTERNAL ERROR: Instruction opcode is not int literal!\ninstr: %s" % instr)

            self.prog_bytearray.extend(struct.pack('<B', instr[1]))

            # are there any operands?
            if len(instr) > 1:

                # iterate through operands, they are in int_tupe format at this stage.
                # int_list = (SIGNCODE, VALUE)
                for operand in instr[2:]: # skip first 2 items in instr-list as they're the addr and opcode.

                    if isinstance(operand, list):

                        # operand is list so inspect it to see whether its signed or unsigned.
                        # operand[0] is the sign-code, operand[1] is the integer value.
                        if operand[0] == UNSIGNED_SIGNCODE or operand[0] == ADDR_SIGNCODE:
                            self.prog_bytearray.extend(struct.pack('<I', operand[1]))

                        elif operand[0] == SIGNED_SIGNCODE:
                            self.prog_bytearray.extend(struct.pack('<i', operand[1]))

                    # if this message does run it means an operand wasnt a proper int_list.
                    else:
                        raise Exception("INTERNAL ERROR: Instruction operand is not int-list or int-literal!\ninstr: %s" % instr)

        except struct.error as e:
            s  = '\nINTERNAL ERROR: Error when packing bytes from instr-list into program bytearray!'
            s += '\n    more info: %s' % e
            raise Exception(s)

    def open_input_file(self):
        try:
            self.input_file = open(self.input_path, 'r')

        except FileNotFoundError:
            print("The file [%s] does not exist." % self.input_path)
        except FileNotFoundError:
            print('\nError: Unable to open input-file: [%s], file does not exist!\n' % self.input_path)
        except IOError:
            print('yson: An I/O error occurred! when opening file: [%s]' % self.input_path)
        except PermissionError:
            print("yson: You do not have permission to access this file: [%s]" % self.input_path)
        except IsADirectoryError:
            print("yson: The specified input-path: [%s] for report is a directory, not a file!" % self.input_path)
        except OSError as e:
            print("yson: An I/O error occurred while attempting to open input-file: [%s] \n    more info: %s" % (self.input_path, e))

    def append_instr(self):
    # THIS METHOD WILL NEED TO BE UPDATED IF INSTRUCTION OPERANDS EVER BECOME DIFFERENT SIZES!!!
    # THIS METHOD ASSUMES OPERANDS CAN ONLY BE 4 BYTES LONG!!!

        # append completed instr-list onto program-list and increment instr counter.
        self.prog_list.append(copy.deepcopy(self.curr_instr))
        self.prog_instr_count += 1

        # instr size is (instr-list len - 2) * 4 + 1 because we're minusing off the addr and opcode items from the
        # list, them multiplying number of remaining items(the operands) by WORDSIZE, then adding back OPCODE_SIZE.
        # because the operands are always 4 bytes each
        # and the opcode itself is 1 byte. this is used
        # to keep track of total program size and to calculate
        # he next address which is used by label declarations.
        instr_len = ((len(self.curr_instr) - 2) * WORDSIZE) + OPCODE_SIZE #(1 byte, opcode itself is 1 byte)
        self.prog_list[METADATA_SEGMENT][PROG_SIZE_NDX] += instr_len
        self.next_addr += opsize_tbl[self.curr_instr[1]] # 1th item of instr list is the opcode.

    def advance_tok(self):
        try:
            self.tokndx += 1
            self.tok = self.line_tokens[self.tokndx]

        # this should NEVER run but just in case..
        except IndexError:
            s  = "\n\nINTERNAL ASSEMBLER ERROR: token-index has over-run the line's token stream!!!"
            s += "self.tokndx = %d, len(self.line_tokens)\" = %d" % (self.tokndx, len(self.line_tokens))
            raise Exception(s)

    def check_identifier(self,
                         ignore_last_char  = False,
                         ignore_first_char = False ):

        test_str = self.tok

        if ignore_last_char:
            test_str = test_str[:-1]

        if ignore_first_char:
            test_str = test_str[-1:]

        test_result = isIdentifier(test_str)

        if test_result == -4:
            raise Exception('yson: Invalid identifier [%s], invalid char in string, on line: %d' % (self.tok, self.linenum))

        if test_result == -3:
            raise Exception('yson: Invalid identifier [%s], first char is digit, on line: %d' % (self.tok, self.linenum))

        if test_result == -2:
            raise Exception('yson: Invalid identifier [%s], too long, max length: %d, on line: %d' % (self.tok, MAX_ID_LEN, self.linenum))

        if test_result == -1:
            raise Exception('yson: Invalid identifier [%s], too short, min length: %d, on line: %d' % (self.tok, MIN_ID_LEN, self.linenum))

        if test_str in optbl:
            raise Exception('yson: Invalid identifier [%s], is an instruction mnemonic, on line: %d' % (self.tok, self.linenum))

    def macro_dec_check(self):
        # macros are stored in symbol table in a dict where the identifier string
        # is the key and it's value is a list like so: (SIGN_CODE, VALUE)
        # this tells the sign information and the value itself.

        # is token a macro declaration?
        if self.tok == MACRO_KEYWORD:

            # advance to next token which is the macro identifier.
            self.advance_tok()

            # check we have a valid identifier for the macro.
            self.check_identifier()

            # check if macro name is already being used as a label name.
            if self.tok in self.symtbl.labels:
                raise Exception('yson: Invalid macro identifier: [%s], already used as label, on line: %d' % (self.tok, self.linenum))

            macro_id = copy.copy(self.tok)

            # advance to next token which is the macro's value.
            self.advance_tok()

            # attempt to parse integer.
            int_list = self.build_int_list()

            if int_list is None:
                raise Exception('yson: Invalid macro value: [%s], not integer, on line: %d' % (self.tok, self.linenum))

            # everything's good, create macro entry into symbol-table.
            self.symtbl.new_macro(macro_id, int_list)
            return True

        return False

    def label_dec_check(self):
        if self.tok[-1] == LABEL_DEF_SUFFIX:
            self.check_identifier(ignore_last_char=True)

            label_id = copy.copy(self.tok[:-1])

            # is the label already declared?
            if label_id in self.symtbl.labels:
                raise Exception('yson: Invalid label identifier: [%s], already in-use as a label identifier, on line: %d' % (self.tok, self.linenum))

            # all is well, create new label entry in symbol-table.
            self.symtbl.new_label(label_id, [ADDR_SIGNCODE, self.next_addr])
            return True

        return False

    def confirm_main_label_exists(self):
    # confirms that the main label has been declared and also
    # writes it to the metadata-segment list within prog-list.
        try:
            # tells vm whats initially to set vm program-counter to.
            self.prog_list[METADATA_SEGMENT][START_ADDR_NDX] = self.symtbl.labels[START_LABEL_IDENTIFIER][1]

        except KeyError:
            # main label missing.
            raise Exception('yson: Cannot assemble file, No main label declared!')

    def process_input(self):
    # opens input-file and processes it's tokens into a list of
    # lists(of integers), where each list inside main list represents
    # an instruction and it's operands. Except the first list which is metadata.

        self.open_input_file()

        for self.line_text in self.input_file:

            self.in_comment = False
            self.linenum += 1

            # only process the line if it has any 
            # text otherwise skip to next line.
            if self.line_text:

                self.assemble_line()

                # line has been assembled so we should have an instr
                # list, check we do then append it to prog-list.
                if len(self.curr_instr) > 1:
                    self.append_instr()

        # token processing complete, close file.
        self.input_file.close()

    def build_int_list(self):
    # NOTE: integers with no prefix default to unsigned.
    # attempts to build an int-list (SIGNCODE, VALUE) from self.tok
    # returns None if it fails and the list if it succeeds.

        # test if self.tok is a decimal integer.
        int_test = isDecInteger(self.tok)

        # if int_test is 0 then self.tok isn't a decimal integer.
        if int_test == 0:

            # self.tok is not a decimal integer so test if it's a hex.
            int_test = isHexInteger(self.tok)

            # self.tok is neither a decimal or hex integer so return None.
            if int_test == 0:
                return None

            # self.tok is a hex integer so build it's int_list.
            elif int_test == SIGNED_SIGNCODE:
                return [SIGNED_SIGNCODE, int(self.tok[-1:], 16)]

            elif int_test == UNSIGNED_SIGNCODE:
                return [UNSIGNED_SIGNCODE, int(self.tok[-1:], 16)]

            elif int_test == ADDR_SIGNCODE:
                return [ADDR_SIGNCODE, int(self.tok[-1:], 16)]

            # no sign on integer defaults to unsigned value.
            elif int_test == NOSIGN_SIGNCODE:
                return [UNSIGNED_SIGNCODE, int(self.tok, 16)]

        # self.tok was a decimal integer, so build it's int_list.
        elif int_test == SIGNED_SIGNCODE:
            return [SIGNED_SIGNCODE, int(self.tok[-1:])]

        elif int_test == UNSIGNED_SIGNCODE:
            return [UNSIGNED_SIGNCODE, int(self.tok[-1:])]

        elif int_test == ADDR_SIGNCODE:
            return [ADDR_SIGNCODE, int(self.tok[-1:])]

        # no sign on integer defaults to unsigned value.
        elif int_test == NOSIGN_SIGNCODE:
            return [UNSIGNED_SIGNCODE, int(self.tok)]

    def assemble_line(self):
        # flag used to tell whether or not opcode has been found on this line, if so 
        # and we find another one we have an error, can only have one opcode per line.
        mnemonic_found = False

        # tokenize the line-text using whitespace as delimiter.
        self.line_tokens = self.line_text.split()

        # initialise instruction-list and token-counter.
        # token_counter init'd to -1 because at start of
        # loop below it gets incremented by self.advance_tok()
        self.curr_instr = [self.next_addr] # first item of instr list is its address relative to first instr.
        self.tokndx = -1

        # iterate over line's tokens one at a time processing them accordingly.
        while self.tokndx+1 < len(self.line_tokens):
            self.advance_tok()

            # deal with comments, if '#' is encountered ignore remainder of line.
            if self.tok == '#' or self.tok[0] == '#':
                return

            # deal with label & macro declarations.
            elif self.label_dec_check(): continue
            elif self.macro_dec_check(): continue

            # is_valid_token() checks for token validity, if token is an
            # identifier it ensures it's a valid one.
            elif self.is_valid_token():

                # is token an instruction mnemonic?
                if self.tok in optbl:

                    # confirm that it is the first mnemonic of the line, if not we have
                    # an error. because cannot have two mnemonic on the same line.
                    if mnemonic_found:
                        raise Exception("yson: Can only have one instruction per line, second mnemonic: [%s] on line %d" % (self.tok, self.linenum))

                    # first mnemonic of this line has been found, use optbl to look up it's opcode
                    # then append it to self.curr_instr, set mnemonic_found flag to true so that
                    # if any other mnemonics are on this line they will be caught as error.
                    self.curr_instr.append(optbl[self.tok])
                    mnemonic_found = True
                    continue

                # is token a literal integer value?
                int_list = self.build_int_list()

                if int_list is not None:

                    # self.tok is indeed an integer, check that we have found
                    # an instr-mnemonic before appending the int_list to self.curr_instr.
                    if mnemonic_found:
                        self.curr_instr.append(int_list)
                        continue

                    else:
                        # check if we're declaring a macro otherwise we have an error.
                        # look back 2 tokens for a 'macro' token if not, we have an error.
                        if self.line_tokens[self.tokndx-2] != MACRO_KEYWORD:
                            raise Exception("yson: Integer value [%s] has no operation, on line %d" % (self.tok, self.linenum))
                
                # check for macro declaration which is an error because they cannot be within instructions
                # and must be up at the top of the file before the first instruction or label.
                elif self.tok == MACRO_KEYWORD:
                    raise Exception("yson: Illegal macro declaration: [%s], must be at top of file before instructions, on line %d" % (self.tok, self.linenum))
                
                # is token a label or macro identifier? possibly one that isn't declared yet?
                # we'll check the identifier is valid then append it to the instr-list as though
                # it is a a label/macro. then in build_prog_bytearray() where we process the
                # prog-list and convert all identifiers to their corresponding values looked up
                # from the symtbl we'll be able to check there if this identifier ends up being
                # associated with a label or macro.
                else:
                    self.check_identifier()

                    if mnemonic_found:
                        self.curr_instr.append(self.tok)
                        continue

                    raise Exception("yson: Identifier [%s] has no operation, on line %d" % (self.tok, self.linenum))

    def is_valid_token(self):
    # tests whether self.tok is a valid token.

        # is token a valid integer?
        if isDecInteger(self.tok) or isHexInteger(self.tok):
            return True

        # is token a instruction mnemonic?
        if self.tok in optbl:
            return True

        # check if tok is valid identifier.
        self.check_identifier()

        # self.tok is valid identifier.
        return True

    def write_prog(self):
    # do_silent specifies whether or not to print to 
    # the console the status of the assembly once completed.

        # build the program's bytearray then write it to the output file.
        self.build_prog_bytearray()

        try:
            with open(self.output_path, 'wb') as self.output_file:

                self.output_file.write(self.prog_bytearray)

        except IOError:
            print("yson: I/O error occurred! when writing to: [%s]" % self.output_path)
        except PermissionError:
            print("yson: You do not have permission to access this file: [%s]" % self.output_path)
        except IsADirectoryError:
            print("yson: The specified output-path: [%s] for report is a directory, not a file!" % self.output_path)
        except OSError as e:
            print("yson: An I/O error occurred while attempting to write to file: [%s] \n    more info: %s" % (self.output_path, e))

    def build_success_message(self):
            ks = 'yes\n' if self.keep_symbols else 'no\n'
            s  = '\nAssembly Successful!\n----------------------\n'
            s += 'symbols-included?: %s' % ks
            s += 'input-path: %s\n' % self.input_path
            s += 'output-path: %s\n' % self.output_path
            s += 'program-bytes-written: %d\n' % self.prog_list[METADATA_SEGMENT][PROG_SIZE_NDX]
            s += 'metadata-bytes-written: %d\n' % len(self.metadata_bytearray)
            s += 'total-bytes-written-to-file: %d\n' % len(self.prog_bytearray)
            s += 'instructions-written: %d\n' % self.prog_instr_count

            return s

    def assemble(self,
                 doSilent = False):

        self.process_input()
        self.write_prog()

        # assembly successful! print to the console if we need to.
        if not doSilent:
            print(self.build_success_message())

def disfbin(fbin_file_path):
# crude disassembler function for debugging assembler.

    metadata = []
    program = []
    symbols_kept = None
    byte_count = 0
    raw_lable_tbl = bytearray()
    raw_macro_tbl = bytearray()
    symtab_size = None

    file = open(fbin_file_path, 'rb')

    # read metadata bytes:
    # read starting addr.
    bytes_read = file.read(WORDSIZE)
    metadata.append((byte_count, struct.unpack('<I', bytes_read)[0]))
    byte_count += WORDSIZE

    # read program-size.
    bytes_read = file.read(WORDSIZE)
    metadata.append((byte_count, struct.unpack('<I', bytes_read)[0]))
    byte_count += WORDSIZE

    # read FLAGS.
    bytes_read = file.read(2)
    metadata.append((byte_count, struct.unpack('<H', bytes_read)[0]))
    byte_count += 2
    symbols_kept = metadata[2][1]

    # read creation datestamp.
    bytes_read = file.read(2)
    metadata.append((byte_count, struct.unpack('<H', bytes_read)[0]))
    byte_count += 2

    # read first-instr-addr.
    bytes_read = file.read(WORDSIZE)
    metadata.append((byte_count, struct.unpack('<I', bytes_read)[0]))
    byte_count += WORDSIZE

    # read last-instr-addr.
    bytes_read = file.read(WORDSIZE)
    metadata.append((byte_count, struct.unpack('<I', bytes_read)[0]))
    byte_count += WORDSIZE

    # read first-user-byte-addr
    bytes_read = file.read(WORDSIZE)
    metadata.append((byte_count, struct.unpack('<I', bytes_read)[0]))
    byte_count += WORDSIZE

    # check if we have a symbol table.
    if symbols_kept:

        #read total symtab size.
        bytes_read = file.read(WORDSIZE)
        symtab_size = struct.unpack('<I', bytes_read)[0]
        metadata.append((byte_count, symtab_size))
        byte_count += WORDSIZE

        # read label tbl size.
        bytes_read = file.read(WORDSIZE)
        lable_tbl_size = struct.unpack('<I', bytes_read)[0]
        metadata.append((byte_count, lable_tbl_size))
        byte_count += WORDSIZE

        # read macro tbl size.
        bytes_read = file.read(WORDSIZE)
        macro_tbl_size = struct.unpack('<I', bytes_read)[0]
        metadata.append((byte_count, macro_tbl_size))
        byte_count += WORDSIZE

        # read the symbol table and deserialise it.
        raw_lable_tbl.extend(file.read(lable_tbl_size))
        byte_count += lable_tbl_size

        raw_macro_tbl.extend(file.read(macro_tbl_size))
        byte_count += macro_tbl_size

        label_tbl = pickle.loads(raw_lable_tbl)
        macro_tbl = pickle.loads(raw_macro_tbl)

    # bytes from this point onwards are the actual program.
    while True:
        bytes_read = file.read(OPCODE_SIZE)

        # check if EOF has been reached.
        if not bytes_read: break

        # unpack opcode & make instr list.
        opcode = struct.unpack('<B', bytes_read)[0]
        instr = [(byte_count, opcode)]
        byte_count += 1

        # iterate through the intr's operands by looking
        # up its operand count in the opsize_tbl.
        for i in range(opargc_tbl[opcode]):
            bytes_read = file.read(WORDSIZE)

            if len(bytes_read) != WORDSIZE:
                break

            operand_value = struct.unpack('<I', bytes_read)[0]

            instr.append((byte_count, operand_value))
            byte_count += WORDSIZE

        program.append(copy.deepcopy(instr))

    file.close()

    # print out everything.

    if symbols_kept:
        print("----------------------------------------------------", end="")
        
        if label_tbl:
            print("\nLABEL TABLE:")
            for i in label_tbl.keys():
                print("id: [%s] = %d" % (i, label_tbl[i][1]) )

        if macro_tbl:
            print("\nMACRO TABLE:")
            for i in macro_tbl.keys():
                print("id: [%s] = %d" % (i, macro_tbl[i][1]))

    print("\n\nMETADATA:\n")
    for i in metadata:
        if i[0] == 0:
            print("addr: %d :: value: %d (start-addr)" % (i[0], i[1]))
        elif i[0] == 4:
            print("addr: %d :: value: %d (total-program-size)" % (i[0], i[1]))
        elif i[0] == 8:
            print("addr: %d :: value: %d (keep-symbols-flag)" % (i[0], i[1]))
        elif i[0] == 10:
            y, m, d = decode_datestamp(metadata[3][1])
            print("addr: %d :: value: %d (creation-date) %d / %d / %d " % (i[0], i[1], d, m, y))
        elif i[0] == 12:
            print("addr: %d :: value: %d (first-instr-addr)" % (i[0], i[1]))
        elif i[0] == 16:
            print("addr: %d :: value: %d (last-instr-addr)" % (i[0], i[1]))
        elif i[0] == 20:
            print("addr: %d :: value: %d (first-byte-after-program)" % (i[0], i[1]))    
        elif i[0] == 24:
            print("addr: %d :: value: %d (total-symtab-size)" % (i[0], i[1]))
        elif i[0] == 28:
            print("addr: %d :: value: %d (total-labeltab-size)" % (i[0], i[1]))
        elif i[0] == 32:
            print("addr: %d :: value: %d (total-macrotab-size)" % (i[0], i[1]))

    print("\n\nPROGRAM: ")

    for instr in program:
            print("\naddr: %d :: op: %d (%s) " % (instr[0][0], instr[0][1], mnemonic_tbl[instr[0][1]]))

            for operand in instr[1:]:
                print("addr: %d :: value: %d" % (operand[0], operand[1]))

    print("----------------------------------------------------\n")

def decode_datestamp(bitstring):
     # Extract day (last 5 bits)
    day = bitstring & 0x1F

    # Extract month (next 4 bits)
    month = (bitstring >> 5) & 0xF

    # Extract year (remaining bits)
    year = (bitstring >> 9) & 0x7F

    # Convert year to full year format (assuming 2000s)
    year += 2000 if year < 100 else 1900

    return year, month, day

def main():
    # yson usage:
    # navigate terminal to yson directory, type python yson.py input_path output_path -options.
    
    # options:
    # -sdb : show-disassembled-binary, prints out the produced binary in a disassembled state.
    # -sfb : show-final-binary, prints out the raw bytes in hex format of the final binary.
    # -ns  : no-symbols-in-binary, excludes all symbols from the final binary.
    # -sds : show-assembler-datastructures, prints out data-structures used by assembler after assembly.
    
    # inputi-path argument is a argument. if no output-path is supplied yson's default output path
    # will be used instead, see DEFAULT_OUTPUT_PATH varible top of this file. 
    # options proceed the output-path arg.option args can be placed in any order, repeated option args are ignored.

    input_path   = None
    output_path  = None
    keep_symbols = True
    show_bin     = False
    show_dis_bin = False
    show_ds      = False
    do_silent    = False

    def do_sdb():
        nonlocal show_dis_bin
        if show_dis_bin:
            print("\nyson: '-sdb' option given twice.")
            sys.exit()
        show_dis_bin = True

    def do_sfb():
        nonlocal show_bin
        if show_bin:
            print("\nyson: '-sfb' option given twice.")
            sys.exit()
        show_bin = True

    def do_ns():
        nonlocal keep_symbols
        if not keep_symbols:
            print("\nyson: '-ns' option given twice.")
            sys.exit()
        keep_symbols = False

    def do_sds():
        nonlocal show_ds
        if show_ds:
            print("\nyson: '-sds' option given twice.")
            sys.exit()
        show_ds = True

    def do_ds():
        nonlocal do_silent
        if do_silent:
            print("\nyson: '-ds' option given twice.")
            sys.exit()
        do_silent = True

    option_map = {"-sdb" : do_sdb,
                  "-sfb" : do_sfb,
                  "-ns"  : do_ns,
                  "-sds" : do_sds,
                  "-ds"  : do_ds}

    if len(sys.argv) == 1:
        print("\nyson: no input file.")
        sys.exit()

    if len(sys.argv) == 2:
        if not os.path.exists(sys.argv[1]):
            print("\nyson: input file path invalid.")
            sys.exit()

        output_path = DEFAULT_OUTPUT_PATH

    input_path = sys.argv[1]

    if output_path is None:
        output_path  = sys.argv[2]
        if output_path in option_map.keys():
            output_path = DEFAULT_OUTPUT_PATH

    if len(input_path) < 3:
        print("\nyson invalid input path.")
        sys.exit()

    if len(output_path) < 3:
        print("\nyson: invalid output path.")
        sys.exit()

    if len(sys.argv) == 4:
        if output_path == DEFAULT_OUTPUT_PATH:
            try:
                option_map[sys.argv[2]]()
            except KeyError:
                print("\nyson: invalid optional arg: %s" % sys.argv[2])
                sys.exit()

        try:
            option_map[sys.argv[3]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[3])
            sys.exit()

    elif len(sys.argv) == 5:
        if output_path == DEFAULT_OUTPUT_PATH:
            try:
                option_map[sys.argv[2]]()
            except KeyError:
                print("\nyson: invalid optional arg: %s" % sys.argv[2])
                sys.exit()
        try:
            option_map[sys.argv[3]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[3])
            sys.exit()

        try:
            option_map[sys.argv[4]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[4])
            sys.exit()

    elif len(sys.argv) == 6:
        if output_path == DEFAULT_OUTPUT_PATH:
            try:
                option_map[sys.argv[2]]()
            except KeyError:
                print("\nyson: invalid optional arg: %s" % sys.argv[2])
                sys.exit()
        try:
            option_map[sys.argv[3]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[3])
            sys.exit()

        try:
            option_map[sys.argv[4]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[4])
            sys.exit()

        try:
            option_map[sys.argv[5]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[5])
            sys.exit()

    elif len(sys.argv) == 7:
        if output_path == DEFAULT_OUTPUT_PATH:
            try:
                option_map[sys.argv[2]]()
            except KeyError:
                print("\nyson: invalid optional arg: %s" % sys.argv[2])
                sys.exit()
        try:
            option_map[sys.argv[3]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[3])
            sys.exit()

        try:
            option_map[sys.argv[4]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[4])
            sys.exit()

        try:
            option_map[sys.argv[5]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[5])
            sys.exit()

        try:
            option_map[sys.argv[6]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[6])
            sys.exit()

    elif len(sys.argv) == 8:
        if output_path == DEFAULT_OUTPUT_PATH:
            try:
                option_map[sys.argv[2]]()
            except KeyError:
                print("\nyson: invalid optional arg: %s" % sys.argv[2])
                sys.exit()
        try:
            option_map[sys.argv[3]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[3])
            sys.exit()

        try:
            option_map[sys.argv[4]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[4])
            sys.exit()

        try:
            option_map[sys.argv[5]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[5])
            sys.exit()

        try:
            option_map[sys.argv[6]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[6])
            sys.exit()

        try:
            option_map[sys.argv[7]]()
        except KeyError:
            print("\nyson: invalid optional arg: %s" % sys.argv[7])
            sys.exit()

    elif len(sys.argv) > 7:
        print("\nyson: too many arguments.")
        sys.exit()

    try:
        asm = Assembler(input_path, output_path, keep_symbols)
        asm.assemble(doSilent=do_silent)
    except Exception as e:
        print('\n%s\n' % e)
        sys.exit()

    if show_ds:
        asm.print_prog_list()

    if show_dis_bin:
        disfbin(output_path)

if __name__ == "__main__":
    main()
