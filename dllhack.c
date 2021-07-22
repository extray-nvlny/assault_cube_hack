#include <Windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define DEBUGGER_TRAP __asm int 3
#define DEBUGGER_TRAP_VKEY(vkey) if(GetAsyncKeyState(vkey) & 1) DEBUGGER_TRAP
#define PI 3.14159265359f
#define MAX_ENTITIES 64

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float   f32;
typedef double  f64;


typedef struct v4
{
    union
    {
        f32 e[4];
        struct
        {
            f32 x,y,z,w;
        };
    };
}v4;

typedef struct v3
{
    union
    {
        f32 e[3];
        struct
        {
            f32 x,y,z;
        };
    };
}v3;


typedef struct v2
{
    union
    {
        f32 e[2];
        struct
        {
            f32 x,y;
        };
    };
}v2;


typedef struct m4x4
{
    union
    {
        f32 e[16];
        struct
        {
            f32 e00,e01,e02,e03;
            f32 e10,e11,e12,e13;
            f32 e20,e21,e22,e23;
            f32 e30,e31,e32,e33;
        };
    };
}m4x4;


#define v4f(x,y,z,w) v4f_create(x,y,z,w)

v4
v4f_create(f32 x, f32 y, f32 z, f32 w)
{
    v4 result = {0};
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}


#define v3f(x,y,z) v3f_create(x,y,z)

v3
v3f_create(f32 x, f32 y, f32 z)
{
    v3 result = {0};
    
    result.x = x;
    result.y = y;
    result.z = z;
    
    return result;
}


#define v2f(x,y) v2f_create(x,y)

v2
v2f_create(f32 x, f32 y)
{
    v2 result = {0};
    
    result.x = x;
    result.y = y;
    
    return result;
}


enum PlayerState
{
    PlayerState_alive = 0,
    PlayerState_dead  = 1,
    PlayerState_ghost = 5,
    
};

typedef struct Entity
{
    char pad1[0x4];	
    v3 head_p;
	char pad_0010[36]; //0x0010
	v3 body_p;
	v3 view_angles;
	char pad_004C[8]; //0x004C
    s32 gravity; //0x0054
	char pad_0058[40]; //0x0058
    char is_player_moving; //0x0080
	char pad_0081[11]; //0x0081
    s32 move_direction; //0x008C
	char pad_0090[104]; //0x0090
    s32 health; //0x00F8
    s32 armor; //0x00FC
	char pad_0100[60]; //0x0100
    s32 pistol_ammo; //0x013C
	char pad_0140[8]; //0x0140
    s32 carabine_ammo; //0x0148
	char pad_014C[20]; //0x014C
	s32 knife_delay; //0x0160
	s32 pistol_delay; //0x0164
	s32 tmp_delay; //0x0168
	s32 v19_delay; //0x016C
	s32 carrabine_delay; //0x0170
	s32 ad81_delay; //0x0174
	s32 assault_delay; //0x0178
	char pad_017C[168]; //0x017C
	s8 is_fire; //0x0224
	char nickname[16]; //0x0225
	char pad_0235[247]; //0x0235
	s32 team; //0x032C
	char pad_0330[8]; //0x0330
	s32 player_state; //0x0338
}Entity;
Entity g_entities[MAX_ENTITIES];


typedef Entity* (__cdecl* get_crosshair_entity_t)();
get_crosshair_entity_t get_crosshair_entity;

f32
square(f32 A)
{
    f32 result = A*A;
    return result;
}

bool
call_hook(char *hook_at, char *new_function, DWORD length)
{
    bool result = false;
    if(length >= 5)
    {
        DWORD new_offset = new_function - hook_at - 5;
        
        DWORD old_protection = 0;
        VirtualProtect(hook_at,length,PAGE_EXECUTE_READWRITE,&old_protection);
        
        *(BYTE*)hook_at = 0xE9; // Relative jump
        *(DWORD*)(hook_at + 0x1) = new_offset; // Address where to jump
        
        // NOPing remaining bytes
        for(DWORD bytes_index = 0x5;
            bytes_index < length;
            bytes_index++)
        {
            *(hook_at + bytes_index) = 0x90;
        }
        
        VirtualProtect(hook_at,length,old_protection,0);
        
        result = true;
    }
    else
    {
        printf("Hook has been failed, not enough space for near jump(E9) instruction\n");
    }
    
    return result;
}

char*
get_state(bool state)
{
    return state ? "ON" : "OFF";
}

f32 
get_distance(v3 from_p, v3 to_p)
{
    f32 result = 0;
    
    result = sqrt(square(from_p.x - to_p.x) + square(from_p.y - to_p.y) +
                  square(from_p.z - to_p.z));
    
    return result;
}

void
draw_text(HDC device_context,char *text,s32 x, s32 y, COLORREF color, HFONT font)
{
    SetTextAlign(device_context,TA_CENTER | TA_NOUPDATECP);
    
    SetBkColor(device_context, RGB(0,0,0));
    SetBkMode(device_context,TRANSPARENT);
    
    SetTextColor(device_context, color);
    SelectObject(device_context, font);
    
    TextOutA(device_context, x, y, text, strlen(text));
}

v3
calc_angle(v3 player_p, v3 entity_p)
{
    // NOTE takes two 3D vectors and outputs angles
    // we need to figure out the viewangles that will make us aim directly at our enemy.
    v3 result = {0};
    
    v3 direction;
    direction.x = entity_p.x - player_p.x;
    direction.y = entity_p.y - player_p.y;
    direction.z = entity_p.z - player_p.z;
    
    f32 hypotenuze = sqrt(direction.x*direction.x + direction.y*direction.y);
    
    result.x = (f32)atan2f(direction.y,direction.x) * (180.0f / PI) + 90; // YAW  
    result.y = (f32)atan2f(direction.z, hypotenuze)  * (180.0f / PI); // PITCH
    result.z = 0.0f;
    
    return result;
}

void
patch_bytes(char *address, char *buffer, u32 size)
{
    DWORD old_protection = 0;
    VirtualProtect(address,size,PAGE_EXECUTE_READWRITE,&old_protection);
    memcpy(address, buffer, size);
    VirtualProtect(address,size,old_protection,0);
}

void
nop(char *address, u32 size)
{
    DWORD old_protection = 0;
    VirtualProtect(address,size,PAGE_EXECUTE_READWRITE,&old_protection);
    memset(address, 0x90, size);
    VirtualProtect(address,size,old_protection,0);
}

void
draw_filled_rect(HDC device_context,HBRUSH brush,s32 x, s32 y, s32 width, s32 height)
{
    RECT rect = {x,y,x+width,y+height};
    FillRect(device_context, &rect,brush);
}

void
draw_border_box(HDC device_context, HBRUSH brush,
                s32 x, s32 y, s32 width, s32 height, s32 thickness)
{
    // BOTTOM LINE
    draw_filled_rect(device_context, brush, x,y, width, thickness);
    // RIGHT LINE
    draw_filled_rect(device_context, brush, x,y,thickness, height);
    // LEFT LINE
    draw_filled_rect(device_context, brush, x+width, y,thickness,height);
    // UP LINE
    draw_filled_rect(device_context, brush, x,y+height,width+thickness,thickness);
}

bool
world_to_screen(v3 p, m4x4 matrix,s32 window_width,s32 window_height,v2 *screen_out)
{
    bool result = false;
    // COLUMN MAJOR
    v4 clip_coords;
    clip_coords.x = p.x * matrix.e[0] + p.y * matrix.e[4] + p.z * matrix.e[8] + matrix.e[12];
    clip_coords.y = p.x * matrix.e[1] + p.y * matrix.e[5] + p.z * matrix.e[9] + matrix.e[13];
    clip_coords.z = p.x * matrix.e[2] + p.y * matrix.e[6] + p.z * matrix.e[10] + matrix.e[14];
    clip_coords.w = p.x * matrix.e[3] + p.y * matrix.e[7] + p.z * matrix.e[11] + matrix.e[15];
    
    if(clip_coords.w > 0.1f)
    {
        
        // PERSPECTIVE DIVIDE
        v3 NDC;
        NDC.x = clip_coords.x / clip_coords.w;
        NDC.y = clip_coords.y / clip_coords.w;
        NDC.z = clip_coords.z / clip_coords.w;
        
        // TO WINDOW COORDINATES
        screen_out->x = (window_width / 2 * NDC.x) + (NDC.x + window_width / 2);
        screen_out->y = -(window_height / 2 * NDC.y) + (NDC.y + window_height / 2);
        result = true;
    }
    else
    {
        
    }
    return result;
}

/*For functions declared with the naked attribute, 
 the compiler generates code without prolog and epilog code. */
DWORD g_ammo_jump_back = 0;
__declspec(naked) void infinite_ammo()
{
    __asm inc [esi];
    __asm push edi;
    __asm mov edi, dword ptr ss:[esp+0x14];
    __asm jmp [g_ammo_jump_back];
}

void
main_hack(HANDLE module)
{
    AllocConsole();
    freopen("CONOUT$","w",stdout);
    printf("SUCCESSFULLY INJECTED!\n");
    printf("-----------------------\n");
    printf("made by Interrupt\n");
    printf("Credits: Guided hack, Null, Zero Memory, unknown cheats and the rest of internet\n");
    printf("This is my first internal cheat. I am very happy with current quality state of it\n");
    printf("Last update 21.07.2021\n");
    printf("-----------------------\n");
    
    bool is_running = true;
    bool is_trigger_bot_on = false;
    bool is_aimbot_on = false;
    bool is_esp_on = false;
    
    HANDLE window = FindWindow(0, "AssaultCube");
    
    char *module_address = (char*)GetModuleHandle("ac_client.exe");
    
    DWORD *local_player_base = (DWORD*)(module_address + 0x10f4f4);
    Entity *player_entity = (Entity*)*local_player_base;
    // First element in entity_list is null
    s32 *entity_list = *(s32**)(module_address + 0x10f4f8);
    get_crosshair_entity = (get_crosshair_entity_t)(module_address + 0x607C0);
    s32 view_matrix = 0x501AE8;
    m4x4  my_view_matrix = {0};
    // hook
    g_ammo_jump_back = 0x004637F0;
    HDC device_context = GetDC(window);
    
    while(is_running)
    {
        s32 entities_count = *(s32*)(module_address + 0x10f500);
        memcpy(&my_view_matrix, (char*)view_matrix,16*sizeof(float));
        
        if(GetAsyncKeyState(VK_F8) & 0x1)
        {
            // TOOD Disable invinitive ammo
            printf("Successfully!. Infinitive ammo!\n");
            call_hook((char*)0x004637E9,(char*)infinite_ammo,7);
        }
        
        if((GetAsyncKeyState(VK_F1) & 0x1))
        {
            printf("Successfully!. Health increased to 1337!\n");
            player_entity->health = 1337;
        }
        
        if(GetAsyncKeyState(VK_F2) & 0x1)
        {
            printf("Successfully!. Armor increased to 1337!\n");
            player_entity->armor = 1337;
        }
        
        if(GetAsyncKeyState(VK_F3) & 0x1)
        {
            // DISABLE RECOIL
            printf("Successfully!. Recoil has been disabled!\n");
            nop((char*)0x00463786, 10);
        }
        
        if(GetAsyncKeyState(VK_F4) & 0x1)
        {
            printf("Successfully!. Triger bot is on turned on!\n");
            is_trigger_bot_on = true;
        }
        
        if(GetAsyncKeyState(VK_F5) & 0x1)
        {
            printf("Successfully!. Aimbot turned ON!\n");
            is_aimbot_on = true;
        }
        if(GetAsyncKeyState(VK_F6) & 0x1)
        {
            printf("Successfully!. Radar hack is working now!\n");
            // 00409FA7 31 NOPS
            nop((char*)0x00409FA7, 31);
        }
        
        if(GetAsyncKeyState(VK_F7) & 0x1)
        {
            printf("Successfully!.ESP Enabled!\n");
            is_esp_on = true;
        }
        if(GetAsyncKeyState(VK_F12) & 0x1)
        {
            is_running = false;
            printf("DLL UNINJECTED\n");
            fclose(stdout);
            FreeConsole();
            FreeLibraryAndExitThread(module,0);
        }
        
        if(is_esp_on)
        {
            for(u32 entity_index = 1;
                entity_index < entities_count;
                entity_index++)
            {
                Entity *entity = *(Entity**)((char*)entity_list + (4*entity_index));
                if(entity)
                {
                    if((entity->team != player_entity->team) &&
                       (entity->player_state == PlayerState_alive))
                    {
                        v2 player_base;
                        v2 player_head;
                        
                        v3 enemy_body_p = entity->body_p;
                        v3 enemy_head_p = entity->head_p;
                        
                        if(world_to_screen(enemy_body_p, my_view_matrix, 800,600,&player_base))
                        {
                            if(world_to_screen(enemy_head_p, my_view_matrix,800,600,&player_head))
                            {
                                HBRUSH brush = CreateSolidBrush(RGB(255,0,0));
                                f32 head   = player_head.y - player_base.y;
                                f32 width  = head / 2;
                                f32 center = width / -2;
                                f32 extra  = head / -6;
                                
                                draw_border_box(device_context,brush,player_base.x + center, player_base.y, width, head - extra, 1);
                                DeleteObject(brush);
                                
                                // NOTE displaying health and nicknames enemy entities
                                
                                HFONT health_font = CreateFont(20,0,0,0,FW_DONTCARE,FALSE,TRUE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                                                               CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,0);
                                HFONT nickname_font = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,TRUE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                                                                 CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,0);
                                char health_buf[255];
                                snprintf(health_buf, sizeof(health_buf), "%i",(int)(entity->health));
                                char nickname_buf[16];
                                snprintf(nickname_buf, sizeof(nickname_buf), "%s",(entity->nickname));
                                
                                draw_text(device_context,health_buf,player_base.x, player_base.y, RGB(255,255,0),health_font);
                                
                                draw_text(device_context,nickname_buf,player_head.x, player_head.y - extra - 15, RGB(0,255,0),nickname_font);
                                
                                DeleteObject(nickname_font);
                                DeleteObject(health_font);
                            }
                        }
                    }
                }
            }
        }
        
        if(is_aimbot_on)
        {
            Entity *best_entity = 0;
            f32 best_distance = -1.0f;
            
            for(u32 entity_index = 1;
                entity_index < entities_count;
                entity_index++)
            {
                Entity *entity = *(Entity**)((char*)entity_list + (4*entity_index));
                
                if(entity)
                {
                    if((player_entity->team != entity->team) && (entity->player_state != PlayerState_dead))
                    {
                        f32 distance = get_distance(player_entity->head_p, entity->head_p);
                        if((best_distance > distance) || (best_distance == -1.0f))
                        {
                            best_distance = distance;
                            best_entity = entity;
                        }
                    }
                }
            }
            if(best_entity)
            {
                player_entity->view_angles = calc_angle(player_entity->head_p,best_entity->head_p);
            }
        }
        if(is_trigger_bot_on)
        {
            Entity *entity = get_crosshair_entity();
            if(entity)
            {
                if(entity->team != player_entity->team)
                {
                    player_entity->is_fire = 1; 
                }
            }
            else
            {
                player_entity->is_fire = 0;
            }
        }
    }
}

BOOL APIENTRY DllMain(HANDLE hModule,// Handle to DLL module
                      DWORD ul_reason_for_call,// Reason for calling function
                      LPVOID lpReserved ) // Reserved
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH: // A process is loading the DLL.
        {
            CloseHandle(CreateThread(0,0,(LPTHREAD_START_ROUTINE)main_hack,(LPVOID)hModule,0,0));
            
        }break;
        case DLL_THREAD_ATTACH: // A process is creating a new thread.
        {
        }break;
        case DLL_THREAD_DETACH: // A thread exits normally.
        {
        }break;
        case DLL_PROCESS_DETACH: // A process unloads the DLL.
        {
        }break;
        default:
        {
        }break;
    }
    
    return TRUE;
}
