#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <windows.h>

// thread handle
static pthread_t thread_id;
int thread_working = 0;

typedef struct cmd
{
    char* type;
    char* str;
    int val;
} cmd;


void type_char_std(char Char)
{
    INPUT input[4] = {};

    SHORT vkState = VkKeyScan(Char);
    if (vkState == -1) return;
    UINT virtualKey = LOBYTE(vkState);
    UINT scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
    if (scanCode == 0) return;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE;
    input[1].ki.wScan = scanCode;

    input[2].type = INPUT_KEYBOARD;
    input[2].ki.wVk = 0;
    input[2].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    input[2].ki.wScan = scanCode;

    UINT SC_LSHIFT = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);

    // Shift modifier
    if (HIBYTE(vkState) & 1)
    {
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = 0;
        input[0].ki.wScan = SC_LSHIFT;
        input[0].ki.dwFlags = KEYEVENTF_SCANCODE; 

        input[3].type = INPUT_KEYBOARD;
        input[3].ki.wVk = 0;
        input[3].ki.wScan = SC_LSHIFT;
        input[3].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

        SendInput(4, &input[0], sizeof(INPUT));
    }
    else
    {
        SendInput(2, &input[1], sizeof(INPUT));
    }
}

void type_char_exd(UINT virtualKey)
{
    INPUT input[2] = {};

    UINT scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
    if (scanCode == 0) return;

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
    input[0].ki.wScan = scanCode;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
    input[1].ki.wScan = scanCode;

    SendInput(2, input, sizeof(INPUT));
}

void type_string_single(char* String, int us)
{
    int i = 0;
    while (String[i] != '\0')
    {
        type_char_std(String[i]);
        usleep(us);
        i++;
    }
}

void type_string_batch(char* String)
{
    int len = strlen(String);
    INPUT* input = (INPUT*) malloc(sizeof(INPUT) * len * 4);
    if (input == NULL) return;

    int input_ptr = 0;
    for (int i = 0; i < len ; i++)
    {
        SHORT vkState = VkKeyScan(String[i]);
        if (vkState == -1) continue;
        UINT virtualKey = LOBYTE(vkState);
        UINT scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
        if (scanCode == 0) continue;
        int Shift = HIBYTE(vkState) & 1;

        UINT SC_LSHIFT = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);

        // Shift modifier
        if (Shift)
        {
            input[input_ptr].type = INPUT_KEYBOARD;
            input[input_ptr].ki.wVk = 0;
            input[input_ptr].ki.dwFlags = KEYEVENTF_SCANCODE;
            input[input_ptr].ki.wScan = SC_LSHIFT;
            input_ptr++;
        }

        input[input_ptr].type = INPUT_KEYBOARD;
        input[input_ptr].ki.wVk = 0;
        input[input_ptr].ki.dwFlags = KEYEVENTF_SCANCODE;
        input[input_ptr].ki.wScan = scanCode;
        input_ptr++;

        input[input_ptr].type = INPUT_KEYBOARD;
        input[input_ptr].ki.wVk = 0;
        input[input_ptr].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        input[input_ptr].ki.wScan = scanCode;
        input_ptr++;

        // Shift modifier
        if (Shift)
        {
            input[input_ptr].type = INPUT_KEYBOARD;
            input[input_ptr].ki.wVk = 0;
            input[input_ptr].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            input[input_ptr].ki.wScan = SC_LSHIFT;
            input_ptr++;
        }
    }
    SendInput(input_ptr, input, sizeof(INPUT));

    free(input);
}

void PerformCMD(cmd command)
{
    char* type = command.type;
    char* string = command.str;
    int number = command.val;
    if (0 == strcmp(type, "type"))
    {
        type_string_batch(string);
        if (number == 1)
        {
            type_char_std('\r');
        }
    }
    else if (0 == strcmp(type, "wait"))
    {
        if (0 == strcmp(string, "s"))
        {
            sleep(number);
        }
        else if (0 == strcmp(string, "ms"))
        {
            Sleep(number);
        }
        else if (0 == strcmp(string, "us"))
        {
            if (number > 1000000)
            {
                int s_sleep = number / 1000000;
                int us_sleep = number % 1000000;

                sleep(s_sleep);
                usleep(us_sleep);
            }
            else
            {
                usleep(number);
            }
        }
    }
    else if (0 == strcmp(type, "press"))
    {
        if (0 == strcmp(string, "enter"))
        {
            type_char_std('\r');
        }
        else if (0 == strcmp(string, "up"))
        {
            type_char_exd(VK_UP);
        }
        else if (0 == strcmp(string, "down"))
        {
            type_char_exd(VK_DOWN);
        }
        else if (0 == strcmp(string, "left"))
        {
            type_char_exd(VK_LEFT);
        }
        else if (0 == strcmp(string, "right"))
        {
            type_char_exd(VK_RIGHT);
        }
    }
}

// This program is extremely inefficient but its fast enough for the current purpose because i used C
void Interpreter(cmd* commands, int cmd_size)
{
    for (int i = 0; i < cmd_size; i++)
    {
        cmd command = commands[i];
        char* type = command.type;
        char* string = command.str;
        int number = command.val;
        if (0 == strcmp(type, "repeat"))
        {
            if (number <= 0)
            {
                // loop expired so jump to end
                for (int j = i + 1; j < cmd_size; j++)
                {
                    cmd next_cmd = commands[j];
                    if (0 == strcmp(next_cmd.type, "end") && 0 == strcmp(next_cmd.str, string))
                    {
                        // Label to jump to
                        i = j;
                        break;
                    }
                }
            }
            else
            {
                commands[i].val = number - 1;
            }
        }
        else if (0 == strcmp(type, "random"))
        {
            // go to the end until found end then select random value to jump to
            int rand_start = i;
            int rand_size = 0;
            for (int j = i + 1; j < cmd_size; j++)
            {
                cmd current_cmd = commands[j];
                if (0 == strcmp(current_cmd.type, "end") && 0 == strcmp(current_cmd.str, string))
                {
                    // Found end
                    break;
                }
                else
                {
                    rand_size++;
                }
            }

            if (rand_size == 0)
            {
                // Goto end then skip end next round
                i++;
            }
            else
            {
                // Perform random
                int random = i + 1 + rand() % rand_size;
                PerformCMD(commands[random]);
                i += rand_size + 1;
            }
        }
        else if (0 == strcmp(type, "end"))
        {
            // loop finished so jump back to repeat
            for (int j = 0; j < cmd_size; j++)
            {
                cmd current_cmd = commands[j];
                if (0 == strcmp(current_cmd.type, "repeat") && 0 == strcmp(current_cmd.str, string))
                {
                    // Label to jump to
                    i = j - 1;
                    break;
                }
            }
        }
        else
        {
            PerformCMD(command);
        }
    }
}

void FreeMemory(cmd* commands, int cmd_size)
{
    for (int i = 0; i < cmd_size; i++)
    {
        free(commands[i].type);
        free(commands[i].str);
    }
}

void* AsyncWork(void* args)
{
    // slight delay to prepare
    sleep(1);

    // Open the command file
    FILE *file = fopen("cmd.txt", "r");
    
    // Check if the file exists and opened successfully
    if (file == NULL) 
    {
        thread_working = 0;
        perror("Error opening cmd.txt\n");
        return NULL;
    }

    cmd commands[2048]; // Max script size: 2048 lines
    int cmd_ptr = 0;

    int line_count = 0;
    char line[1024]; // Buffer to temporarily store each line
    while (fgets(line, sizeof(line), file) != NULL)
    {
        line_count++;
        // Print the line to the console
        int line_ptr = 0;

        // Get type match
        {
            char* type = (char*) malloc(16); // Max type = 16
            int type_ptr = 0;

            int front_fill = 0;
            for (line_ptr; line_ptr < 1024; line_ptr++)
            {
                if (type_ptr >= 15)
                {
                    // Break to prevent overflow
                    type[15] = '\0';
                    break;
                }

                // Filter break characters and tab
                char Char = line[line_ptr];
                if (Char == '\0' || Char == '\n' || Char == '\r')
                {
                    free(type);
                    goto _exit;
                }
                else if (Char == ' ')
                {
                    if (front_fill)
                    {
                        type[type_ptr] = '\0';
                        break;
                    }
                }
                else if (Char == '_')
                {
                    // _comment with an underscore at the start
                    free(type);
                    goto _exit;
                }
                else if (Char != '\t')
                {
                    type[type_ptr] = Char;
                    front_fill = 1;
                    type_ptr++;
                }
            }

            commands[cmd_ptr].type = type;
        }
        
        // Get str matchs
        {
            char* str = (char*) malloc(1000); // Max str len = 1000
            int str_ptr = 0;

            int start_str = 0;
            for (line_ptr; line_ptr < 1024; line_ptr++)
            {
                if (str_ptr >= 999)
                {
                    // Break to prevent overflow
                    str[999] = '\0';
                    break;
                }
                // Filter break characters and tab
                char Char = line[line_ptr];
                if (Char == '\0' || Char == '\n' || Char == '\r')
                {
                    free(str);
                    goto _exit;
                }
                else if (Char == '\"')
                {
                    if (!start_str)
                    {
                        start_str = 1;
                    }
                    else
                    {
                        str[str_ptr] = '\0';
                        break;
                    }
                }
                else if (start_str)
                {
                    if (Char == '\t')
                    {
                        str[str_ptr] = ' ';
                        str_ptr++;
                    }
                    else if (Char == '\\')
                    {
                        line_ptr++;
                        char nextChar = line[line_ptr];
                        char retChar = ' ';
                        switch (nextChar)
                        {
                            // Allow these following characters
                            case '\\':
                                retChar = '\\';
                                break;
                            case 'n':
                                retChar = '\n';
                                break;
                            case 't':
                                retChar = '\t';
                                break;
                            case 'r':
                                retChar = '\r';
                                break;
                            case 'b':
                                retChar = '\b';
                                break;
                            case '\'':
                                retChar = '\'';
                                break;
                            case '\"':
                                retChar = '\"';
                                break;
                            default:
                                retChar = '\\';
                                break;
                        }
                        str[str_ptr] = retChar;
                        str_ptr++;
                    }
                    else
                    {
                        str[str_ptr] = Char;
                        str_ptr++;
                    }
                }
            }

            commands[cmd_ptr].str = str;
        }

        // Get value match
        {
            char value[16]; // Max value len = 16
            int value_ptr = 0;

            int digit_start = 0;
            for (line_ptr; line_ptr < 1024; line_ptr++)
            {
                if (value_ptr >= 8)
                {
                    // Break to prevent overflow
                    break;
                }
                // Filter break characters and tab
                char Char = line[line_ptr];
                if (Char == '\0' || Char == '\n' || Char == '\r')
                {
                    break;
                }
                else if (isdigit(Char))
                {
                    value[value_ptr] = Char;
                    value_ptr++;
                    digit_start = 1;
                }
                else
                {
                    if (digit_start)
                    {
                        break;
                    }
                }
            }
            
            int final = 0;
            for (int i = 0; (i < value_ptr) && (i < 8); i++) // Only process up to 8 digits to stay below integer limit
            {
                char ch = value[i];
                final = final * 10 + (ch - '0');
            }

            commands[cmd_ptr].val = final;
        }

        cmd_ptr++;
        _exit:

        if (cmd_ptr >= 2048) break; // Break to prevent overflow
    }

    fclose(file);

    Interpreter(commands, cmd_ptr);
    FreeMemory(commands, cmd_ptr);

    thread_working = 0;
    return NULL;
}

HHOOK hKeyboardHook;

// Callback function for keystrokes
LRESULT CALLBACK KeyboardProcedure(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Check if an event
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*) lParam;
            DWORD keycode = pKeyInfo->vkCode;
            if (keycode == VK_HOME) // 'home' key
            {
                // Type message into here
                if (!thread_working)
                {
                    if (0 != pthread_create(&thread_id, NULL, AsyncWork, NULL))
                    {
                        printf("thread creation failed");
                        PostQuitMessage(0);
                    }
                    else
                    {
                        thread_working = 1;
                        pthread_detach(thread_id);
                    }
                }
            }
            else if (keycode == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
        }
    }
    // Pass the event
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

int main()
{
    // Seed the random number generator
    srand(time(NULL));

    // Set the global low level keyboard hook
    hKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        KeyboardProcedure,
        GetModuleHandle(NULL),
        0
    );

    if (hKeyboardHook == NULL) {
        printf("Failed to install hook\n");
        return 1;
    }

    printf("Program started\n");

    // Message loop to keep the application alive and process hook events
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    printf("Program exited\n");

    // Unhook the keyboard before exiting
    UnhookWindowsHookEx(hKeyboardHook);
    return 0;
}