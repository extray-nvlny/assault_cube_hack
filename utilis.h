/* date = July 11th 2021 5:01 pm */

#ifndef UTILIS_H
#define UTILIS_H

typedef struct ProcessInfo
{
    DWORD PID;
    char *base_address;
    char *process_name;
}ProcessInfo;

void
list_process_modules(DWORD pid, ProcessInfo *info)
{
    HANDLE module_snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);
    if(module_snap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 module_entry = {0};
        module_entry.dwSize = sizeof(MODULEENTRY32);
        if(Module32First(module_snap, &module_entry))
        {
            do
            {
                // zt means zero terminated
                if(strcmp(module_entry.szModule, "ac_client.exe") == 0)
                {
                    info->base_address = module_entry.modBaseAddr;
                    break;
                }
            }while(Module32Next(module_snap, &module_entry));
        }
        else
        {
            printf("Module32First has failed\n");
        }
    }
    else
    {
        printf("Invalid handle value\n");
    }
    CloseHandle(module_snap);
}

bool
get_info_about_game_process(char *process, ProcessInfo *info)
{
    bool success = true;
    HANDLE process_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(process_snap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 process_entry = {0};
        process_entry.dwSize = sizeof(PROCESSENTRY32);
        if(Process32First(process_snap, &process_entry))
        {
            do
            {
                if(strcmp(process_entry.szExeFile,process) == 0)
                {
                    info->process_name = malloc(strlen(process_entry.szExeFile) + 1);
                    memcpy(info->process_name, process_entry.szExeFile,strlen(process_entry.szExeFile) + 1);
                    info->PID = process_entry.th32ProcessID;
                    list_process_modules(process_entry.th32ProcessID, info);
                    printf("Process has been found!\n");
                    break;
                }
            }while(Process32Next(process_snap,&process_entry));
            
        }
        else
        {
            printf("Process32First failed\n");
            success = false;
        }
    }
    else
    {
        printf("Invalid handle value\n");
        success = false;
    }
    CloseHandle(process_snap);
    return true;
}



#endif //UTILIS_H
