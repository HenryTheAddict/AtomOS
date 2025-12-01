/*
 * AtomOS - Windows Compatibility Layer
 * Provides Windows PE binary support and Win32 API emulation
 */

#ifndef _ATOMOS_WINDOWS_COMPAT_H
#define _ATOMOS_WINDOWS_COMPAT_H

#include "../../kernel/include/types.h"

/* PE signatures */
#define PE_DOS_MAGIC    0x5A4D      /* 'MZ' */
#define PE_SIGNATURE    0x00004550  /* 'PE\0\0' */

/* DOS header */
typedef struct {
    uint16_t magic;             /* MZ */
    uint16_t last_page_bytes;
    uint16_t pages;
    uint16_t relocations;
    uint16_t header_paragraphs;
    uint16_t min_alloc;
    uint16_t max_alloc;
    uint16_t initial_ss;
    uint16_t initial_sp;
    uint16_t checksum;
    uint16_t initial_ip;
    uint16_t initial_cs;
    uint16_t reloc_table_offset;
    uint16_t overlay_number;
    uint16_t reserved1[4];
    uint16_t oem_id;
    uint16_t oem_info;
    uint16_t reserved2[10];
    uint32_t pe_offset;         /* Offset to PE header */
} PACKED pe_dos_header_t;

/* PE header */
typedef struct {
    uint32_t signature;         /* PE\0\0 */
    uint16_t machine;           /* 0x14C = i386 */
    uint16_t num_sections;
    uint32_t timestamp;
    uint32_t symbol_table;
    uint32_t num_symbols;
    uint16_t optional_header_size;
    uint16_t characteristics;
} PACKED pe_header_t;

/* Optional header (PE32) */
typedef struct {
    uint16_t magic;             /* 0x10B = PE32 */
    uint8_t  major_linker;
    uint8_t  minor_linker;
    uint32_t code_size;
    uint32_t init_data_size;
    uint32_t uninit_data_size;
    uint32_t entry_point;
    uint32_t code_base;
    uint32_t data_base;
    
    /* NT additional fields */
    uint32_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    uint16_t major_os;
    uint16_t minor_os;
    uint16_t major_image;
    uint16_t minor_image;
    uint16_t major_subsystem;
    uint16_t minor_subsystem;
    uint32_t win32_version;
    uint32_t image_size;
    uint32_t headers_size;
    uint32_t checksum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint32_t stack_reserve;
    uint32_t stack_commit;
    uint32_t heap_reserve;
    uint32_t heap_commit;
    uint32_t loader_flags;
    uint32_t num_data_directories;
} PACKED pe_optional_header_t;

/* Section header */
typedef struct {
    char     name[8];
    uint32_t virtual_size;
    uint32_t virtual_address;
    uint32_t raw_data_size;
    uint32_t raw_data_offset;
    uint32_t relocations_offset;
    uint32_t line_numbers_offset;
    uint16_t num_relocations;
    uint16_t num_line_numbers;
    uint32_t characteristics;
} PACKED pe_section_header_t;

/* Data directory */
typedef struct {
    uint32_t virtual_address;
    uint32_t size;
} PACKED pe_data_directory_t;

/* Import directory entry */
typedef struct {
    uint32_t import_lookup;
    uint32_t timestamp;
    uint32_t forwarder_chain;
    uint32_t name;
    uint32_t import_address;
} PACKED pe_import_descriptor_t;

/* Export directory */
typedef struct {
    uint32_t characteristics;
    uint32_t timestamp;
    uint16_t major_version;
    uint16_t minor_version;
    uint32_t name;
    uint32_t base;
    uint32_t num_functions;
    uint32_t num_names;
    uint32_t address_table;
    uint32_t name_table;
    uint32_t ordinal_table;
} PACKED pe_export_directory_t;

/* Windows data types */
typedef void *HANDLE;
typedef void *HMODULE;
typedef void *HINSTANCE;
typedef void *HWND;
typedef void *HDC;
typedef void *HBRUSH;
typedef void *HFONT;
typedef void *HICON;
typedef void *HCURSOR;
typedef void *HMENU;
typedef void *HBITMAP;
typedef void *HRGN;
typedef void *HPEN;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef int BOOL;
typedef unsigned int UINT;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef int32_t LRESULT;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;

#define TRUE    1
#define FALSE   0
#define INVALID_HANDLE_VALUE ((HANDLE)-1)

/* Function declarations */
void windows_compat_init(void);
int windows_load_pe(const char *path, uint32_t *entry_point);
int windows_exec(const char *path, const char *cmdline);

/* Win32 API emulation */
HANDLE win32_CreateFileA(LPCSTR name, DWORD access, DWORD share, 
                         void *security, DWORD creation, DWORD flags, HANDLE templ);
BOOL win32_CloseHandle(HANDLE handle);
BOOL win32_ReadFile(HANDLE file, LPVOID buf, DWORD count, DWORD *read, void *overlapped);
BOOL win32_WriteFile(HANDLE file, LPCVOID buf, DWORD count, DWORD *written, void *overlapped);
DWORD win32_GetFileSize(HANDLE file, DWORD *high);
BOOL win32_SetFilePointer(HANDLE file, LONG low, LONG *high, DWORD method);
LPVOID win32_VirtualAlloc(LPVOID addr, DWORD size, DWORD type, DWORD protect);
BOOL win32_VirtualFree(LPVOID addr, DWORD size, DWORD type);
DWORD win32_GetLastError(void);
void win32_SetLastError(DWORD error);
HANDLE win32_GetProcessHeap(void);
LPVOID win32_HeapAlloc(HANDLE heap, DWORD flags, DWORD size);
BOOL win32_HeapFree(HANDLE heap, DWORD flags, LPVOID mem);
void win32_ExitProcess(UINT code);
HMODULE win32_GetModuleHandleA(LPCSTR name);
LPVOID win32_GetProcAddress(HMODULE module, LPCSTR name);
HMODULE win32_LoadLibraryA(LPCSTR name);
BOOL win32_FreeLibrary(HMODULE module);
int win32_MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type);
HANDLE win32_GetStdHandle(DWORD type);
BOOL win32_WriteConsoleA(HANDLE console, LPCVOID buf, DWORD len, DWORD *written, void *reserved);
void win32_OutputDebugStringA(LPCSTR str);
DWORD win32_GetTickCount(void);
void win32_Sleep(DWORD ms);
DWORD win32_GetCurrentProcessId(void);
DWORD win32_GetCurrentThreadId(void);
BOOL win32_GetVersionExA(void *info);

#endif /* _ATOMOS_WINDOWS_COMPAT_H */
