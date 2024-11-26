#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <windows.h>
#include <conio.h>
#include <cstdint>
#include <cstdio>

#include "infostrings.h"
#include "vm.h"

/* 
 * Here is a  list of the most-common VK codes as defined in Windows.h
 * 
 *
 * Virtual-Key Codes (VK Codes)
 *
 * Macro Name         Code  Key
 * -------------------------------
 * VK_LBUTTON         0x01  Left mouse button
 * VK_RBUTTON         0x02  Right mouse button
 * VK_CANCEL          0x03  Control-break processing
 * VK_MBUTTON         0x04  Middle mouse button
 * VK_XBUTTON1        0x05  X1 mouse button
 * VK_XBUTTON2        0x06  X2 mouse button
 * VK_BACK            0x08  BACKSPACE
 * VK_TAB             0x09  TAB
 * VK_CLEAR           0x0C  CLEAR
 * VK_RETURN          0x0D  ENTER
 * VK_SHIFT           0x10  SHIFT
 * VK_CONTROL         0x11  CTRL
 * VK_MENU            0x12  ALT
 * VK_PAUSE           0x13  PAUSE
 * VK_CAPITAL         0x14  CAPS LOCK
 * VK_ESCAPE          0x1B  ESC
 * VK_SPACE           0x20  SPACEBAR
 * VK_PRIOR           0x21  PAGE UP
 * VK_NEXT            0x22  PAGE DOWN
 * VK_END             0x23  END
 * VK_HOME            0x24  HOME
 * VK_LEFT            0x25  LEFT ARROW
 * VK_UP              0x26  UP ARROW
 * VK_RIGHT           0x27  RIGHT ARROW
 * VK_DOWN            0x28  DOWN ARROW
 * VK_SELECT          0x29  SELECT
 * VK_PRINT           0x2A  PRINT
 * VK_EXECUTE         0x2B  EXECUTE
 * VK_SNAPSHOT        0x2C  PRINT SCREEN
 * VK_INSERT          0x2D  INS
 * VK_DELETE          0x2E  DEL
 * VK_HELP            0x2F  HELP
 * VK_LWIN            0x5B  Left Windows key
 * VK_RWIN            0x5C  Right Windows key
 * VK_APPS            0x5D  Application key
 * VK_SLEEP           0x5F  Sleep
 * VK_NUMPAD0         0x60  Numeric keypad 0
 * VK_NUMPAD1         0x61  Numeric keypad 1
 * VK_NUMPAD2         0x62  Numeric keypad 2
 * VK_NUMPAD3         0x63  Numeric keypad 3
 * VK_NUMPAD4         0x64  Numeric keypad 4
 * VK_NUMPAD5         0x65  Numeric keypad 5
 * VK_NUMPAD6         0x66  Numeric keypad 6
 * VK_NUMPAD7         0x67  Numeric keypad 7
 * VK_NUMPAD8         0x68  Numeric keypad 8
 * VK_NUMPAD9         0x69  Numeric keypad 9
 * VK_MULTIPLY        0x6A  Multiply
 * VK_ADD             0x6B  Add
 * VK_SEPARATOR       0x6C  Separator
 * VK_SUBTRACT        0x6D  Subtract
 * VK_DECIMAL         0x6E  Decimal
 * VK_DIVIDE          0x6F  Divide
 * VK_F1              0x70  F1
 * VK_F2              0x71  F2
 * VK_F3              0x72  F3
 * VK_F4              0x73  F4
 * VK_F5              0x74  F5
 * VK_F6              0x75  F6
 * VK_F7              0x76  F7
 * VK_F8              0x77  F8
 * VK_F9              0x78  F9
 * VK_F10             0x79  F10
 * VK_F11             0x7A  F11
 * VK_F12             0x7B  F12
 * VK_F13             0x7C  F13 (on some keyboards)
 * VK_F14             0x7D  F14 (on some keyboards)
 * VK_F15             0x7E  F15 (on some keyboards)
 * VK_F16             0x7F  F16 (on some keyboards)
 * VK_F17             0x80  F17 (on some keyboards)
 * VK_F18             0x81  F18 (on some keyboards)
 * VK_F19             0x82  F19 (on some keyboards)
 * VK_F20             0x83  F20 (on some keyboards)
 * 
 * VirtualKey macros for alphanumeric codes arent defined in Windows.h for whatever reason
 * so i've set them manually here so i can use them conviniently.
 */

#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39
#define VK_A 0x41
#define VK_B 0x42
#define VK_C 0x43
#define VK_D 0x44
#define VK_E 0x45
#define VK_F 0x46
#define VK_G 0x47
#define VK_H 0x48
#define VK_I 0x49
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_N 0x4E
#define VK_O 0x4F
#define VK_P 0x50
#define VK_Q 0x51
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x5A

#define USER_INPUT_RESPONSE_TIME           300
#define INVALID_USER_INPUT_TEXT_STAY_TIME 1500

// user input codes, universal codes.
#define UI_MAIN_MENU          80
#define UI_HELP               81
#define UI_INVALID            82
#define UI_QUIT               83

// user input codes, main menu codes.
#define UI_EXEC_CURR_INSTR     2
#define UI_UNDO_PREV_INSTR     3
#define UI_EXEC_UNTIL_BP       4
#define UI_EXEC_UNTIL_END      5
#define UI_SHOW_IMD_NEXT_INSTR 6 // show immediate next instr. 
#define UI_SHOW_CURR_INSTR     7 
#define UI_SHOW_IMD_LAST_INSTR 8 // show immediate last instr.
#define UI_SHOW_NEXT_INSTR     9 // show next instr, if triggered again show next after that, and so on.
#define UI_SHOW_LAST_INSTR    10 // same but for last instr.
#define UI_RAM_MENU           11
#define UI_WSTACK_MENU        12
#define UI_CSTACK_MENU        13
#define UI_BP_MENU            14
#define UI_RESET_DEBUGGER     15
#define UI_INSERT_INSTR       16
#define UI_DISPLAY_IFRAMES    17

// user input codes, breakpoint menu.
#define BP_MENU_HELP          90
#define BP_MENU_SET_HERE      91
#define BP_MENU_SET_ADDR      92
#define BP_MENU_DISPLAY_ALL   93
#define BP_MENU_SEARCH        94

// user input codes for ram menu.
#define RAM_MENU_HELP         70
#define RAM_DISPLAY_TR_ALL    71
#define RAM_DISPLAY_TR_ADDR   72
#define RAM_DISPLAY_ALL       73
#define RAM_DISPLAY_RANGE     74
#define RAM_DISPLAY_ADDR      75
#define RAM_SET_WATCH         76
#define RAM_MODIFY            77
#define RAM_DISPLAY_SIZE      78
#define RAM_MODIFY_SIZE       79

// user input codes for work-stack menu.
#define WSTK_MENU_HELP         20
#define WSTK_DISPLAY_TR_ALL    21
#define WSTK_DISPLAY_TR_ADDR   22
#define WSTK_DISPLAY_ALL       23
#define WSTK_DISPLAY_ADDR      24
#define WSTK_SET_WATCH         25
#define WSTK_MODIFY            26
#define WSTK_PUSH              27
#define WSTK_POP               28

// user input codes for call-stack menu.
#define CSTK_MENU_HELP         50
#define CSTK_DISPLAY_ALL       51
#define CSTK_DISPLAY_NEXT      52
#define CSTK_MODIFY_RECUR      53
#define CSTK_PUSH_ADDR         54
#define CSTK_POP_ADDR          55
#define CSTK_REMOVE_ADDR       56
#define CSTK_CLEAR_ALL         57

// ACTION TRIGGER KEYS
// 
// alter these macros to change what keys can trigger each option.

#define UI_MAIN_MENU_TRIGGER           VK_M
#define UI_HELP_TRIGGER                VK_H
#define UI_QUIT_TRIGGER                VK_ESCAPE

#define UI_EXEC_CURR_INSTR_TRIGGER     VK_RIGHT
#define UI_UNDO_PREV_INSTR_TRIGGER     VK_LEFT
#define UI_EXEC_UNTIL_BP_TRIGGER       VK_SPACE
#define UI_SHOW_CURR_INSTR_TRIGGER     VK_DOWN 
#define UI_SHOW_IMD_NEXT_INSTR_TRIGGER VK_RIGHT // + LCTRL
#define UI_SHOW_IMD_LAST_INSTR_TRIGGER VK_LEFT  // + LCTRL
#define UI_SHOW_NEXT_INSTR_TRIGGER     VK_RIGHT // + LSHIFT
#define UI_SHOW_LAST_INSTR_TRIGGER     VK_LEFT  // + LSHIFT
#define UI_EXEC_UNTIL_END_TRIGGER      VK_SPACE // + LSHIFT
#define UI_RAM_MENU_TRIGGER            VK_R
#define UI_WSTACK_MENU_TRIGGER         VK_S
#define UI_CSTACK_MENU_TRIGGER         VK_C
#define UI_BP_MENU_TRIGGER             VK_B
#define UI_RESET_DEBUGGER_TRIGGER      VK_X
#define UI_INSERT_INSTR_TRIGGER        VK_I
#define UI_DISPLAY_IFRAMES_TRIGGER     VK_F

#define BP_MENU_HELP_TRIGGER           VK_H
#define BP_MENU_SET_HERE_TRIGGER       VK_B
#define BP_MENU_SET_ADDR_TRIGGER       VK_J
#define BP_MENU_DISPLAY_ALL_TRIGGER    VK_D
#define BP_MENU_SEARCH_TRIGGER         VK_S

#define RAM_MENU_HELP_TRIGGER          VK_H
#define RAM_DISPLAY_TR_ALL_TRIGGER     VK_A
#define RAM_DISPLAY_TR_ADDR_TRIGGER    VK_Y
#define RAM_DISPLAY_ALL_TRIGGER        VK_D
#define RAM_DISPLAY_RANGE_TRIGGER      VK_R
#define RAM_DISPLAY_ADDR_TRIGGER       VK_P
#define RAM_SET_WATCH_TRIGGER          VK_W
#define RAM_MODIFY_TRIGGER             VK_E
#define RAM_DISPLAY_SIZE_TRIGGER       VK_Q
#define RAM_MODIFY_SIZE_TRIGGER        VK_T

#define WSTK_MENU_HELP_TRIGGER         VK_H
#define WSTK_DISPLAY_TR_ALL_TRIGGER    VK_A
#define WSTK_DISPLAY_TR_ADDR_TRIGGER   VK_Y
#define WSTK_DISPLAY_ALL_TRIGGER       VK_D
#define WSTK_DISPLAY_ADDR_TRIGGER      VK_P
#define WSTK_SET_WATCH_TRIGGER         VK_W
#define WSTK_MODIFY_TRIGGER            VK_E
#define WSTK_PUSH_TRIGGER              VK_I
#define WSTK_POP_TRIGGER               VK_U

#define CSTK_MENU_HELP_TRIGGER         VK_H
#define CSTK_DISPLAY_ALL_TRIGGER       VK_D
#define CSTK_DISPLAY_NEXT_TRIGGER      VK_N
#define CSTK_MODIFY_RECUR_TRIGGER      VK_R
#define CSTK_PUSH_ADDR_TRIGGER         VK_O
#define CSTK_POP_ADDR_TRIGGER          VK_F
#define CSTK_REMOVE_ADDR_TRIGGER       VK_G
#define CSTK_CLEAR_ALL_TRIGGER         VK_C

// menu string arrays.
static
const char*
mainmenu_str_arr[] =
{
	"\n\t\t\t\tMain Menu\n",
	"\t\t\t     --------------\n",
	"\t[M] ----------- main menu\n",
	"\t[H] ----------- help\n\n",
	"\t[ESC] --------- end debugging session\n",
	"\t[>] ----------- execute next instr\n",
	"\t[<] ----------- undo prev instr\n",
	"\t[SPACE] ------- execute until breakpoint\n",
	"\t[LSHFT + SPACE] execute program until end\n",
	"\t[LSHFT + >] --- show next instr\n",
	"\t[LSHFT + <] --- show last instr\n",
	"\t[B] ----------- open breakpoint menu\n",
	"\t[R] ----------- open ram menu\n",
	"\t[S] ----------- open work-stack menu\n",
	"\t[C] ----------- open call-stack menu\n",
	"\t[I] ----------- insert instruction\n",
	"\t[F] ----------- display instruction frames\n",
	"\t[X] ----------- reset session\n",
	NULL
};

static
const char*
prog_complete_menu_str_arr[] =
{
	"\n\t\t\t\tProgram Complete Menu\n",
	"\t[ESC] --------- end debugging session\n",
	"\t[R] ----------- open ram menu\n",
	"\t[S] ----------- open work-stack menu\n",
	"\t[C] ----------- open call-stack menu\n",
	"\t[F] ----------- display instruction frames\n",
	"\t[X] ----------- reset session\n",
	NULL
};

static
const char*
bp_menu_str_arr[] =
{
	"\n\t\t\t\tBreakpoint Menu\n",
	"\t\t\t     --------------\n",
	"\t[M] ----------- main menu\n",
	"\t[H] ----------- breakpoint help\n\n",
	"\t[ESC] --------- end debugging session\n",
	"\t[B] ----------- set breakpoint here\n",
	"\t[J] ----------- set breakpoint at address\n",
	"\t[D] ----------- display all breakpoints\n",
	"\t[S] ----------- search for breakpoint\n",
	NULL
};

static
const char*
ram_veiwer_menu_str_arr[] =
{
	"\n\t\t\t\tRam Menu\n",
	"\t\t\t     --------------\n",
	"\t[M] ----------- main menu\n",
	"\t[H] ----------- ram help\n\n",
    "\t[ESC] --------- end debugging session\n",
	"\t[A] ----------- display entire type-record\n",
	"\t[Y] ----------- display partial type-record\n",
	"\t[D] ----------- display entire ram (in hex format)\n",
	"\t[R] ----------- display range of addresses\n",
	"\t[P] ----------- display ram addr\n",
	"\t[W] ----------- set address watch\n",
	"\t[E] ----------- modify ram directly\n",
	"\t[Q] ----------- display ram size\n",
	"\t[T] ----------- modify ram size\n",
	NULL
};

static
const char*
wstk_veiwer_menu_str_arr[] =
{
	"\n\t\t\t\tWork-Stack Menu\n",
	"\t\t\t     --------------\n",
	"\t[M] ----------- main menu\n",
	"\t[H] ----------- work-stack help\n\n",
	"\t[ESC] --------- end debugging session\n",
	"\t[A] ----------- display entire type-record\n",
	"\t[Y] ----------- display partial type-record\n",
	"\t[D] ----------- display entire work-stack (in hex format)\n",
	"\t[W] ----------- set address watch\n",
	"\t[E] ----------- modify work-stack address\n",
	"\t[I] ----------- push value onto work-stack\n",
	"\t[U] ----------- pop value off work-stack\n",
	NULL
};

static
const char*
cstk_veiwer_menu_str_arr[] =
{
	"\n\t\t\t\tCall-Stack Menu\n",
	"\t\t\t     --------------\n",
	"\t[M] ----------- main menu\n",
	"\t[H] ----------- call-stack help\n\n",
	"\t[ESC] --------- end debugging session\n",
	"\t[D] ----------- display entire call-stack\n",
	"\t[N] ----------- display next return address\n",
	"\t[R] ----------- modify recursion limit\n",
	"\t[O] ----------- push address onto callstack\n",
	"\t[F] ----------- pop address off callstack top\n",
	"\t[G] ----------- remove address from callstack\n",
	"\t[C] ----------- clear entire call-stack\n",
	NULL
};

void print_main_menu();
void print_prog_complete_menu_str_arr();
void print_bp_menu();
void print_ram_menu();
void print_wstk_menu();
void print_cstk_menu();

u8   get_main_menu_input();
u8   get_bp_menu_input();
u8   get_ram_menu_input();
u8   get_wstk_menu_input();
u8   get_cstk_menu_input();

u32  get_u32_user_input(const char* prompt_msg, const char* invalid_input_msg);
i32  get_i32_user_input(const char* prompt_msg, const char* invalid_input_msg);

void  hide_console_cursor();
void  clear_console_line();
bool  get_user_confirmation(const char* prompt_msg);
char* get_user_instr_input();

#endif // USER_INPUT_H
