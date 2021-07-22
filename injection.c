#include <Windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <tlhelp32.h>
#include "utilis.h"



int
main(int argc, char *argv[])
{
    ProcessInfo info = {0}; 
    get_info_about_game_process("ac_client.exe",&info);
    LPCSTR dll_path = "C:\\Program Files (x86)\\projects\\ac_internal_cheat\\build\\dllhack.dll";
    
    int dll_path_string_length = strlen(dll_path) + 1;
    
    // NOTE handle to the target program
    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, info.PID);
    if(process_handle != INVALID_HANDLE_VALUE)
    {
        // NOTE allocates memory in the target program
        LPVOID dll_path_mem = VirtualAllocEx(process_handle, 0, dll_path_string_length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(dll_path_mem)
        {
            // NOTE copys dll path name to target program
            WriteProcessMemory(process_handle, (LPVOID)dll_path_mem, (LPCVOID)dll_path,dll_path_string_length,0);
            
            void *load_lib_addr = GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA");
            
            HANDLE new_thread = CreateRemoteThread(process_handle,0,0,(LPTHREAD_START_ROUTINE)load_lib_addr,dll_path_mem,0,0);
            if((new_thread != 0) && (new_thread != INVALID_HANDLE_VALUE))
            {
                WaitForSingleObject(new_thread,INFINITE);
                
                DWORD exit_code = 0;
                GetExitCodeThread(new_thread,&exit_code);
                
                printf("SUCCESSFULY INJECTED!\n");
            }
            else
            {
                printf("Failure\n");
            }
            CloseHandle(new_thread);
            VirtualFreeEx(process_handle, dll_path_mem, dll_path_string_length, MEM_RELEASE);
        }
        else
        {
            printf("Mem allocation for dll path has failed\n");
        }
    }
    else
    {
        printf("Invalid hanlde value\n");
    }
    
    CloseHandle(process_handle);
    return 0;
}