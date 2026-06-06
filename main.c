#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <windows.h>

// thread handle
static pthread_t thread_id;

void typeString(char* String, int len)
{
    INPUT input[2] = {};
    int i;
    for (i = 0; i < len; i++)
    {
        SHORT vkState = VkKeyScan(String[i]);
        if (vkState == -1)
        {
            continue;
        }
        UINT virtualKey = LOBYTE(vkState);
        UINT scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);

        input[0].type = INPUT_KEYBOARD;
        input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
        input[0].ki.wScan = scanCode;

        input[1].type = INPUT_KEYBOARD;
        input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        input[1].ki.wScan = scanCode;

        SendInput(2, input, sizeof(INPUT));
        usleep(1000); //sleep for 0.001 seconds
    }
}

void typeRawExtended(char Char)
{
    INPUT input[2] = {};
    UINT scanCode = MapVirtualKey(Char, MAPVK_VK_TO_VSC);

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
    input[0].ki.wScan = scanCode;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
    input[1].ki.wScan = scanCode;

    SendInput(2, input, sizeof(INPUT));
    usleep(5000); //sleep for 0.001 seconds
}

void* AsyncType(void* args)
{
    // slight delay to prepare
    sleep(1);
    printf("started command script\n");

    // Open the command file
    FILE *file = fopen("cmd.txt", "r");
    
    // Check if the file exists and opened successfully
    if (file == NULL) {
        perror("Error opening cmd.txt\n");
        return NULL;
    }

    // Buffer to temporarily store each line
    char line[1024];

    // 2. Read line by line until End-Of-File (EOF) or error
    while (fgets(line, sizeof(line), file) != NULL) {
        // Print the line to the console
        line[strcspn(line, "\n")] = '\0';
        printf("%s\n", line);
        float delay;
        if (sscanf(line, "wait %f", &delay) == 1)
        {
            // ignore minor rounding issues
            int s_delay = (int) delay;
            float float_us_delay = (float) delay - s_delay;
            int us_delay = (int) (float_us_delay * 1000000);
            sleep(s_delay);
            usleep(us_delay);
        }
        else if (strncmp(line, "up\\", 3) == 0)
        {
            typeRawExtended(VK_UP);
        }
        else if (strncmp(line, "down\\", 3) == 0)
        {
            typeRawExtended(VK_DOWN);
        }
        else if (strncmp(line, "enter\\", 6) == 0)
        {
            char buffer[2] = "\r\0";
            typeString(buffer, 1);
        }
        else
        {
            int len = strlen(line);
            line[len] = '\r';
            line[len + 1] = '\0';
            typeString(line, len + 1);
        }
    }
    printf("finished command script\n");
    fclose(file);
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
            if (keycode == VK_RSHIFT) // '\' key
            {
                // Type message into here
                pthread_create(&thread_id, NULL, AsyncType, NULL);
                pthread_detach(thread_id);
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
