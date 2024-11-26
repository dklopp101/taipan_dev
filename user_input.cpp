#pragma warning(disable : 6031)
#pragma warning(disable : 6001)
#pragma warning(disable : 4996)
#pragma warning(disable : 6308)
#pragma warning(disable : 101)
#pragma warning(disable : 102)

#include "dvm.h"
#include "user_input.h"

void
hide_console_cursor() {
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;

	GetConsoleCursorInfo(consoleHandle, &cursorInfo); // Get current cursor info
	cursorInfo.bVisible = FALSE;                      // Set the cursor visibility to false
	SetConsoleCursorInfo(consoleHandle, &cursorInfo); // Apply the change
}

void
print_main_menu()
{
	for (int i = 0;; i++)
	{
		if (!mainmenu_str_arr[i]) break;
		printf(mainmenu_str_arr[i]);
	}

    fflush(stdout);
}

void
print_prog_complete_menu_str_arr()
{
	newlines(2);
	for (int i = 0;; i++)
	{
		if (!prog_complete_menu_str_arr[i]) break;
		printf(prog_complete_menu_str_arr[i]);
	}

	fflush(stdout);
}

void
print_bp_menu()
{
	for (int i = 0;; i++)
	{
		if (!bp_menu_str_arr[i]) break;
		printf(bp_menu_str_arr[i]);
	}

	fflush(stdout);
}

void
print_ram_menu()
{
	for (int i = 0;; i++)
	{
		if (!ram_veiwer_menu_str_arr[i]) break;
		printf(ram_veiwer_menu_str_arr[i]);
	}

	fflush(stdout);
}

void
print_wstk_menu()
{
	for (int i = 0;; i++)
	{
		if (!wstk_veiwer_menu_str_arr[i]) break;
		printf(wstk_veiwer_menu_str_arr[i]);
	}

	fflush(stdout);
}

void
print_cstk_menu()
{
	for (int i = 0;; i++)
	{
		if (!cstk_veiwer_menu_str_arr[i]) break;
		printf(cstk_veiwer_menu_str_arr[i]);
	}

	fflush(stdout);
}

// handles getting user input for main-menu.
// put call to this function inside a loop
// with sleep(370) directly after the call.
// NOTE: remove all printf calls, they purely for debugging.
u8
get_main_menu_input()
{
    for (;;)
	{
        for (int vk = 0; vk < 256; vk++)
		{
            // check if any key has been pressed.
            if ((GetAsyncKeyState(vk) & 0x8000))
			{
                // if shift has been pressed jump to shift handling code.
                if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
                    goto lshift_pressed;

				else if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
					goto lctrl_pressed;

                // handle single key menu inputs.
                switch (vk)
				{
                    case UI_QUIT_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_QUIT)");
                        return UI_QUIT;

                    case UI_EXEC_CURR_INSTR_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_EXEC_CURR_INSTR)");
                        return UI_EXEC_CURR_INSTR;

                    case UI_UNDO_PREV_INSTR_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_UNDO_PREV_INSTR)");
                        return UI_UNDO_PREV_INSTR;

					case UI_SHOW_CURR_INSTR_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_UNDO_PREV_INSTR)");
						return UI_SHOW_CURR_INSTR;

                    case UI_MAIN_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_MAIN_MENU)");
                        return UI_MAIN_MENU;

                    case UI_HELP_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_HELP)");
                        return UI_HELP;

                    case UI_EXEC_UNTIL_BP_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_EXEC_UNTIL_BP)");
                        return UI_EXEC_UNTIL_BP;

					case UI_BP_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_BP_MENU)");
						return UI_BP_MENU;

                    case UI_RAM_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_RAM_MENU)");
                        return UI_RAM_MENU;

                    case UI_WSTACK_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_WSTACK_MENU)");
                        return UI_WSTACK_MENU;

					case UI_CSTACK_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_CSTACK_MENU)");
						return UI_CSTACK_MENU;

					case UI_RESET_DEBUGGER_TRIGGER:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_RESET_DEBUGGER)");
						return UI_RESET_DEBUGGER;

					case UI_INSERT_INSTR:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_INSERT_INSTR)");
						return UI_INSERT_INSTR;

					case UI_DISPLAY_IFRAMES:
						DEBUG_user_input_received_from_get_main_menu_input(" (UI_DISPLAY_IFRAMES)");
						return UI_DISPLAY_IFRAMES;
                }

            lshift_pressed:
                // check if left-shift has been pressed.
                if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
				{
                    switch (vk)
					{
                        case UI_SHOW_NEXT_INSTR_TRIGGER:
							DEBUG_user_input_received_from_get_main_menu_input(" (UI_SHOW_NEXT_INSTR)");
                            return UI_SHOW_NEXT_INSTR;

                        case UI_SHOW_LAST_INSTR_TRIGGER:
							DEBUG_user_input_received_from_get_main_menu_input(" (UI_SHOW_LAST_INSTR)");
                            return UI_SHOW_LAST_INSTR;

                        case UI_EXEC_UNTIL_END_TRIGGER:
							DEBUG_user_input_received_from_get_main_menu_input(" (UI_EXEC_UNTIL_END)");
                            return UI_EXEC_UNTIL_END;
                    }
                }

			lctrl_pressed:
				if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
				{
					switch (vk)
					{
						case UI_SHOW_IMD_NEXT_INSTR_TRIGGER:
							DEBUG_user_input_received_from_get_main_menu_input(" (UI_SHOW_IMD_NEXT_INSTR)");
							return UI_SHOW_IMD_NEXT_INSTR;

						case UI_SHOW_IMD_LAST_INSTR_TRIGGER:
							DEBUG_user_input_received_from_get_main_menu_input("(UI_SHOW_IMD_LAST_INSTR)");
							return UI_SHOW_IMD_LAST_INSTR;
					}
				}
            }
        }
    }
}

u8
get_bp_menu_input()
{
	for (;;)
	{
		for (int vk = 0; vk < 256; vk++)
		{
			// check if any key has been pressed.
			if ((GetAsyncKeyState(vk) & 0x8000))
			{
				// handle single key menu inputs.
				switch (vk)
				{
					case UI_QUIT_TRIGGER:
						DEBUG_user_input_received_from_get_bp_menu_input(" (UI_QUIT)");
						return UI_QUIT;

					case UI_MAIN_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_bp_menu_input(" (UI_MAIN_MENU)");
						return UI_MAIN_MENU;

					case BP_MENU_HELP_TRIGGER:
						DEBUG_user_input_received_from_get_bp_menu_input(" (BP_MENU_HELP)");
						return BP_MENU_HELP;

					case BP_MENU_SET_HERE_TRIGGER:
						DEBUG_user_input_received_from_get_bp_menu_input(" (BP_MENU_SET_HERE)");
						return BP_MENU_SET_HERE;

					case BP_MENU_SET_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_bp_menu_input(" (BP_MENU_SET_ADDR)");
						return BP_MENU_SET_ADDR;

					case BP_MENU_DISPLAY_ALL_TRIGGER:
						DEBUG_user_input_received_from_get_bp_menu_input(" (BP_MENU_DISPLAY_ALL)");
						return BP_MENU_DISPLAY_ALL;

					case BP_MENU_SEARCH_TRIGGER:
						DEBUG_user_input_received_from_get_bp_menu_input(" (BP_MENU_SEARCH)");
						return BP_MENU_SEARCH;
				}
			}
		}
	}
}

u8
get_ram_menu_input()
{
	for (;;)
	{
		for (int vk = 0; vk < 256; vk++)
		{
			// check if any key has been pressed.
			if ((GetAsyncKeyState(vk) & 0x8000))
			{
				// handle single key menu inputs.
				switch (vk)
				{
					case UI_QUIT_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (UI_QUIT)");
						return UI_QUIT;

					case UI_MAIN_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (UI_MAIN_MENU)");
						return UI_MAIN_MENU;

					case RAM_MENU_HELP_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_MENU_HELP)");
						return RAM_MENU_HELP;

					case RAM_DISPLAY_TR_ALL_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_DISPLAY_TR_ALL)");
						return RAM_DISPLAY_TR_ALL;

					case RAM_DISPLAY_TR_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_DISPLAY_TR_ADDR)");
						return RAM_DISPLAY_TR_ADDR;

					case RAM_DISPLAY_ALL_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_DISPLAY_ALL)");
						return RAM_DISPLAY_ALL;

					case RAM_DISPLAY_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_DISPLAY_ADDR)");
						return RAM_DISPLAY_ADDR;

					case RAM_DISPLAY_RANGE_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_DISPLAY_RANGE)");
						return RAM_DISPLAY_RANGE;

					case RAM_SET_WATCH_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_SET_WATCH)");
						return RAM_SET_WATCH;

					case RAM_MODIFY_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_EDIT)");
						return RAM_MODIFY;

					case RAM_DISPLAY_SIZE_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_DISPLAY_SIZE)");
						return RAM_DISPLAY_SIZE;

					case RAM_MODIFY_SIZE_TRIGGER:
						DEBUG_user_input_received_from_get_ram_menu_input(" (RAM_MODIFY_SIZE)");
						return RAM_MODIFY_SIZE;
				}
			}
		}
	}
}

u8
get_wstk_menu_input()
{
	for (;;)
	{
		for (int vk = 0; vk < 256; vk++)
		{
			// check if any key has been pressed.
			if ((GetAsyncKeyState(vk) & 0x8000))
			{
				// handle single key menu inputs.
				switch (vk)
				{
					case UI_QUIT_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (UI_QUIT)");
						return UI_QUIT;

					case UI_MAIN_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (UI_MAIN_MENU)");
						return UI_MAIN_MENU;

					case WSTK_MENU_HELP_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_MENU_HELP)");
						return WSTK_MENU_HELP;

					case WSTK_DISPLAY_TR_ALL_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_DISPLAY_TR_ALL)");
						return WSTK_DISPLAY_TR_ALL;

					case WSTK_DISPLAY_TR_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_DISPLAY_TR_ADDR)");
						return WSTK_DISPLAY_TR_ADDR;

					case WSTK_DISPLAY_ALL_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_DISPLAY_ALL)");
						return WSTK_DISPLAY_ALL;

					case WSTK_DISPLAY_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_DISPLAY_ADDR)");
						return WSTK_DISPLAY_ADDR;

					case WSTK_SET_WATCH_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_SET_WATCH)");
						return WSTK_SET_WATCH;

					case WSTK_MODIFY_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_MODIFY)");
						return WSTK_MODIFY;

					case WSTK_PUSH_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_PUSH)");
						return WSTK_PUSH;

					case WSTK_POP:
						DEBUG_user_input_received_from_get_wstk_menu_input(" (WSTK_POP)");
						return WSTK_POP;
				}
			}
		}
	}
}

u8
get_cstk_menu_input()
{
	for (;;)
	{
		for (int vk = 0; vk < 256; vk++)
		{
			// check if any key has been pressed.
			if ((GetAsyncKeyState(vk) & 0x8000))
			{
				// handle single key menu inputs.
				switch (vk)
				{
					case UI_QUIT_TRIGGER:
						DEBUG_user_input_received_from_get_wstk_menu_input("ESC (UI_QUIT)");
						return UI_QUIT;

					case UI_MAIN_MENU_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("M (UI_MAIN_MENU)");
						return UI_MAIN_MENU;

					case CSTK_MENU_HELP_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("H (CSTK_MENU_HELP)");
						return CSTK_MENU_HELP;


					case CSTK_DISPLAY_ALL_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("D (CSTK_DISPLAY_ALL)");
						return CSTK_DISPLAY_ALL;

					case CSTK_DISPLAY_NEXT_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("N (CSTK_NEXT)");
						return CSTK_DISPLAY_NEXT;

					case CSTK_MODIFY_RECUR_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("R (CSTK_MODIFY_RECUR)");
						return CSTK_MODIFY_RECUR;

					case CSTK_PUSH_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("O (CSTK_PUSH_ADDR)");
						return CSTK_PUSH_ADDR;

					case CSTK_POP_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("F (CSTK_POP_ADDR)");
						return CSTK_POP_ADDR;

					case CSTK_REMOVE_ADDR_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("G (CSTK_REMOVE_ADDR)");
						return CSTK_REMOVE_ADDR;

					case CSTK_CLEAR_ALL_TRIGGER:
						DEBUG_user_input_received_from_get_cstk_menu_input("C (CSTK_CLEAR_ALL)");
						return CSTK_CLEAR_ALL;

				}
			}
		}
	}
}

u32
get_u32_user_input(const char* prompt_msg, const char* invalid_input_msg)
{
	char input_buffer[15];
	u32 inputed_value;
	int ch;

	for (;;)
	{
		printf(prompt_msg);

		if (fgets(input_buffer, sizeof(input_buffer), stdin))
		{
			if (strlen(input_buffer) > 10)
			{
				printf(invalid_input_msg);
				while ((ch = getchar()) != '\n' && ch != EOF);
				continue;
			}

			if (sscanf(input_buffer, "%u", &inputed_value) == 1)
				return inputed_value;
			else
				printf(invalid_input_msg);
		}
	}
}

i32
get_i32_user_input(const char* prompt_msg, const char* invalid_input_msg)
{
	char input_buffer[15];
	i32 inputed_value;
	int ch;

	for (;;)
	{
		printf(prompt_msg);

		if (fgets(input_buffer, sizeof(input_buffer), stdin))
		{
			if (strlen(input_buffer) > 10)
			{
				printf(invalid_input_msg);
				while ((ch = getchar()) != '\n' && ch != EOF);
				continue;
			}

			if (sscanf(input_buffer, "%d", &inputed_value) == 1)
				return inputed_value;
			else
				printf(invalid_input_msg);
		}
	}
}

void
clear_console_line()
{
		// Get the handle to the console
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		// Retrieve console screen buffer information
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		GetConsoleScreenBufferInfo(hConsole, &consoleInfo);

		// Calculate the number of characters to clear
		int length = consoleInfo.dwSize.X;  // Width of the console

		// Move the cursor to the beginning of the line
		COORD newPos = { 0, consoleInfo.dwCursorPosition.Y };
		SetConsoleCursorPosition(hConsole, newPos);

		// Overwrite the entire line with spaces
		for (int i = 0; i < length; i++)
			printf(" ");

		// Move the cursor back to the beginning of the line
		SetConsoleCursorPosition(hConsole, newPos);
		fflush(stdout);  // Flush to ensure the changes take effect
}

// displays a prompt msg then prompts the user to
// enter either y or n to confirm or cancel an action.
// returns true if y pressed and false if n pressed.
bool
get_user_confirmation(const char* msg)
{
	printf("\n%s", msg);

user_prompt:
	printf("\npress y to confirm or n to cancel");
	Sleep(1000);
	for (;;)
	{
		for (int vk = 0; vk < 256; vk++)
		{
			if ((GetAsyncKeyState(vk) & 0x8000))
			{
				Sleep(USER_INPUT_RESPONSE_TIME);
				switch (vk)
				{
					case VK_Y:
						DEBUG_user_input_received_from_get_user_confirmation("Y (CONFIRMATION)");
						return true;

					case VK_N:
						DEBUG_user_input_received_from_get_user_confirmation("N (CANCELLATION)");
						return false;

					default:
						printf("\ninvalid input, enter either y or n");
						Sleep(USER_INPUT_RESPONSE_TIME);
						goto user_prompt;
				}
			}
		}
	}
}

char*
get_user_instr_input()
{
	size_t buffer_size = 100;
	char* input_buffer = (char*) malloc(buffer_size);

	if (!input_buffer)
		return NULL;

	printf("Enter instruction code:\n ->>> ");
	if (fgets(input_buffer, buffer_size, stdin) == NULL) {
		fprintf(stderr, "Error reading input\n");
		free(input_buffer);  // Free memory if input fails
		return NULL;
	}

	// Remove newline character if it exists
	size_t input_len = strlen(input_buffer);
	if (input_len > 0 && input_buffer[input_len - 1] == '\n')
	{
		input_buffer[input_len - 1] = '\0';
	}

	return input_buffer;
}

//int main() {
//	while (1)
//	{
//		u8 user_input = get_main_menu_input();
//		Sleep(370);
//		if (user_input == UI_QUIT)
//			break;
//	}
//	printf("\n FNIISHED");
//	return 0;
//}