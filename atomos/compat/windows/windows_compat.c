/*
 * AtomOS - Windows Compatibility Layer Implementation
 * PE loader and Win32 API emulation
 */

#include "windows_compat.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/pmm.h"
#include "../../kernel/mm/vmm.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/fs/vfs.h"
#include "../../kernel/process/process.h"
#include "../../kernel/drivers/timer.h"

/* Last error code */
static DWORD last_error = 0;

/* Standard handles */
#define STD_INPUT_HANDLE    ((DWORD)-10)
#define STD_OUTPUT_HANDLE   ((DWORD)-11)
#define STD_ERROR_HANDLE    ((DWORD)-12)

/*
 * Load a PE executable
 */
int windows_load_pe(const char *path, uint32_t *entry_point) {
    /* Open file */
    fd_t fd = vfs_open(path, O_RDONLY, 0);
    if (fd < 0) {
        kerror("Failed to open PE file: %s\n", path);
        return -1;
    }
    
    /* Read DOS header */
    pe_dos_header_t dos_header;
    if (vfs_read(fd, &dos_header, sizeof(dos_header)) != sizeof(dos_header)) {
        kerror("Failed to read DOS header\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Verify DOS magic */
    if (dos_header.magic != PE_DOS_MAGIC) {
        kerror("Invalid DOS magic: 0x%x\n", dos_header.magic);
        vfs_close(fd);
        return -1;
    }
    
    /* Seek to PE header */
    vfs_seek(fd, dos_header.pe_offset, SEEK_SET);
    
    /* Read PE header */
    pe_header_t pe_header;
    if (vfs_read(fd, &pe_header, sizeof(pe_header)) != sizeof(pe_header)) {
        kerror("Failed to read PE header\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Verify PE signature */
    if (pe_header.signature != PE_SIGNATURE) {
        kerror("Invalid PE signature: 0x%x\n", pe_header.signature);
        vfs_close(fd);
        return -1;
    }
    
    /* Check if i386 */
    if (pe_header.machine != 0x14C) {
        kerror("Only i386 PE supported\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Read optional header */
    pe_optional_header_t opt_header;
    if (vfs_read(fd, &opt_header, sizeof(opt_header)) != sizeof(opt_header)) {
        kerror("Failed to read optional header\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Verify PE32 */
    if (opt_header.magic != 0x10B) {
        kerror("Only PE32 supported\n");
        vfs_close(fd);
        return -1;
    }
    
    /* Get current process */
    process_t *proc = process_get_current();
    if (!proc) {
        vfs_close(fd);
        return -1;
    }
    
    uint32_t image_base = opt_header.image_base;
    
    /* Allocate memory for entire image */
    uint32_t image_size = opt_header.image_size;
    for (uint32_t offset = 0; offset < image_size; offset += PAGE_SIZE) {
        uint32_t vaddr = image_base + offset;
        if (!vmm_is_mapped(proc->page_dir, vaddr)) {
            void *phys = pmm_alloc_page();
            if (!phys) {
                kerror("Out of memory loading PE\n");
                vfs_close(fd);
                return -1;
            }
            vmm_map_page(proc->page_dir, vaddr, (uint32_t)phys, 
                         PTE_USER | PTE_WRITABLE);
        }
    }
    
    /* Load headers */
    vfs_seek(fd, 0, SEEK_SET);
    vfs_read(fd, (void *)image_base, opt_header.headers_size);
    
    /* Calculate position of section headers */
    uint32_t section_offset = dos_header.pe_offset + sizeof(pe_header_t) + 
                              pe_header.optional_header_size;
    
    /* Load sections */
    for (int i = 0; i < pe_header.num_sections; i++) {
        pe_section_header_t section;
        
        vfs_seek(fd, section_offset + i * sizeof(pe_section_header_t), SEEK_SET);
        if (vfs_read(fd, &section, sizeof(section)) != sizeof(section)) {
            kerror("Failed to read section header %d\n", i);
            continue;
        }
        
        if (section.raw_data_size > 0) {
            vfs_seek(fd, section.raw_data_offset, SEEK_SET);
            vfs_read(fd, (void *)(image_base + section.virtual_address),
                     section.raw_data_size);
        }
        
        /* Zero padding */
        if (section.virtual_size > section.raw_data_size) {
            memset((void *)(image_base + section.virtual_address + section.raw_data_size),
                   0, section.virtual_size - section.raw_data_size);
        }
    }
    
    vfs_close(fd);
    
    /* TODO: Process imports and relocations */
    
    *entry_point = image_base + opt_header.entry_point;
    return 0;
}

/*
 * Execute a Windows PE binary
 */
int windows_exec(const char *path, const char *cmdline) {
    UNUSED const char *cmd = cmdline;
    
    uint32_t entry;
    
    /* Load PE */
    if (windows_load_pe(path, &entry) < 0) {
        return -1;
    }
    
    /* Get current process */
    process_t *proc = process_get_current();
    if (!proc) return -1;
    
    /* Mark process as Windows binary */
    proc->flags |= PROC_FLAG_WINDOWS;
    
    /* Setup stack */
    uint32_t *stack = (uint32_t *)USER_STACK_TOP;
    
    /* Enter user mode */
    enter_usermode(entry, (uint32_t)stack);
    
    /* Should never reach here */
    return 0;
}

/*
 * Win32 API implementations
 */

HANDLE win32_CreateFileA(LPCSTR name, DWORD access, DWORD share, 
                         void *security, DWORD creation, DWORD flags, HANDLE templ) {
    UNUSED DWORD acc = access;
    UNUSED DWORD sh = share;
    UNUSED void *sec = security;
    UNUSED DWORD creat = creation;
    UNUSED DWORD fl = flags;
    UNUSED HANDLE t = templ;
    
    int open_flags = 0;
    if ((access & 0x80000000) && (access & 0x40000000)) {
        open_flags = O_RDWR;
    } else if (access & 0x80000000) {
        open_flags = O_RDONLY;
    } else if (access & 0x40000000) {
        open_flags = O_WRONLY;
    }
    
    if (creation == 2) open_flags |= O_CREAT | O_TRUNC;
    else if (creation == 4) open_flags |= O_CREAT;
    
    fd_t fd = vfs_open(name, open_flags, 0644);
    if (fd < 0) {
        last_error = 2;  /* ERROR_FILE_NOT_FOUND */
        return INVALID_HANDLE_VALUE;
    }
    
    return (HANDLE)(intptr_t)fd;
}

BOOL win32_CloseHandle(HANDLE handle) {
    if (handle == INVALID_HANDLE_VALUE) {
        last_error = 6;  /* ERROR_INVALID_HANDLE */
        return FALSE;
    }
    
    int fd = (int)(intptr_t)handle;
    if (vfs_close(fd) < 0) {
        last_error = 6;
        return FALSE;
    }
    
    return TRUE;
}

BOOL win32_ReadFile(HANDLE file, LPVOID buf, DWORD count, DWORD *read, void *overlapped) {
    UNUSED void *ovl = overlapped;
    
    int fd = (int)(intptr_t)file;
    ssize_t result = vfs_read(fd, buf, count);
    
    if (result < 0) {
        last_error = 5;  /* ERROR_ACCESS_DENIED */
        return FALSE;
    }
    
    if (read) *read = result;
    return TRUE;
}

BOOL win32_WriteFile(HANDLE file, LPCVOID buf, DWORD count, DWORD *written, void *overlapped) {
    UNUSED void *ovl = overlapped;
    
    int fd = (int)(intptr_t)file;
    ssize_t result = vfs_write(fd, buf, count);
    
    if (result < 0) {
        last_error = 5;
        return FALSE;
    }
    
    if (written) *written = result;
    return TRUE;
}

DWORD win32_GetFileSize(HANDLE file, DWORD *high) {
    int fd = (int)(intptr_t)file;
    stat_t st;
    
    if (vfs_fstat(fd, &st) < 0) {
        last_error = 6;
        return 0xFFFFFFFF;
    }
    
    if (high) *high = 0;
    return st.st_size;
}

LPVOID win32_VirtualAlloc(LPVOID addr, DWORD size, DWORD type, DWORD protect) {
    UNUSED DWORD t = type;
    UNUSED DWORD p = protect;
    
    process_t *proc = process_get_current();
    if (!proc) return NULL;
    
    uint32_t vaddr = (uint32_t)addr;
    if (vaddr == 0) {
        vaddr = proc->heap_end;
    }
    
    uint32_t pages = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    
    for (uint32_t i = 0; i < pages; i++) {
        void *phys = pmm_alloc_page();
        if (!phys) return NULL;
        vmm_map_page(proc->page_dir, vaddr + i * PAGE_SIZE, 
                     (uint32_t)phys, PTE_USER | PTE_WRITABLE);
    }
    
    if (vaddr + pages * PAGE_SIZE > proc->heap_end) {
        proc->heap_end = vaddr + pages * PAGE_SIZE;
    }
    
    return (LPVOID)vaddr;
}

BOOL win32_VirtualFree(LPVOID addr, DWORD size, DWORD type) {
    UNUSED DWORD t = type;
    
    process_t *proc = process_get_current();
    if (!proc) return FALSE;
    
    uint32_t pages = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    uint32_t vaddr = (uint32_t)addr;
    
    for (uint32_t i = 0; i < pages; i++) {
        vmm_unmap_page(proc->page_dir, vaddr + i * PAGE_SIZE);
    }
    
    return TRUE;
}

DWORD win32_GetLastError(void) {
    return last_error;
}

void win32_SetLastError(DWORD error) {
    last_error = error;
}

HANDLE win32_GetProcessHeap(void) {
    return (HANDLE)1;  /* Dummy handle */
}

LPVOID win32_HeapAlloc(HANDLE heap, DWORD flags, DWORD size) {
    UNUSED HANDLE h = heap;
    UNUSED DWORD f = flags;
    return kmalloc(size);
}

BOOL win32_HeapFree(HANDLE heap, DWORD flags, LPVOID mem) {
    UNUSED HANDLE h = heap;
    UNUSED DWORD f = flags;
    kfree(mem);
    return TRUE;
}

void win32_ExitProcess(UINT code) {
    process_exit(code);
}

HMODULE win32_GetModuleHandleA(LPCSTR name) {
    if (!name) {
        /* Return handle to current module */
        process_t *proc = process_get_current();
        return proc ? (HMODULE)proc : NULL;
    }
    
    /* TODO: Handle DLL loading */
    return NULL;
}

LPVOID win32_GetProcAddress(HMODULE module, LPCSTR name) {
    UNUSED HMODULE m = module;
    UNUSED LPCSTR n = name;
    /* TODO: Export table lookup */
    return NULL;
}

HMODULE win32_LoadLibraryA(LPCSTR name) {
    UNUSED LPCSTR n = name;
    /* TODO: DLL loading */
    last_error = 126;  /* ERROR_MOD_NOT_FOUND */
    return NULL;
}

BOOL win32_FreeLibrary(HMODULE module) {
    UNUSED HMODULE m = module;
    return TRUE;
}

int win32_MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type) {
    UNUSED HWND h = hwnd;
    UNUSED UINT t = type;
    
    kprintf("[MessageBox] %s: %s\n", caption ? caption : "Message", text);
    return 1;  /* IDOK */
}

HANDLE win32_GetStdHandle(DWORD type) {
    switch (type) {
        case STD_INPUT_HANDLE:  return (HANDLE)0;
        case STD_OUTPUT_HANDLE: return (HANDLE)1;
        case STD_ERROR_HANDLE:  return (HANDLE)2;
        default: return INVALID_HANDLE_VALUE;
    }
}

BOOL win32_WriteConsoleA(HANDLE console, LPCVOID buf, DWORD len, DWORD *written, void *reserved) {
    UNUSED HANDLE c = console;
    UNUSED void *res = reserved;
    
    const char *str = (const char *)buf;
    for (DWORD i = 0; i < len; i++) {
        kprintf("%c", str[i]);
    }
    
    if (written) *written = len;
    return TRUE;
}

void win32_OutputDebugStringA(LPCSTR str) {
    kprintf("[DEBUG] %s\n", str);
}

DWORD win32_GetTickCount(void) {
    return timer_get_ms();
}

void win32_Sleep(DWORD ms) {
    thread_sleep(ms);
}

DWORD win32_GetCurrentProcessId(void) {
    process_t *proc = process_get_current();
    return proc ? proc->pid : 0;
}

DWORD win32_GetCurrentThreadId(void) {
    return 1;  /* TODO: Implement properly */
}

BOOL win32_GetVersionExA(void *info) {
    struct {
        DWORD dwOSVersionInfoSize;
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        DWORD dwBuildNumber;
        DWORD dwPlatformId;
        char szCSDVersion[128];
    } *vi = info;
    
    vi->dwMajorVersion = 6;
    vi->dwMinorVersion = 1;
    vi->dwBuildNumber = 7601;
    vi->dwPlatformId = 2;  /* VER_PLATFORM_WIN32_NT */
    strcpy(vi->szCSDVersion, "AtomOS Windows Compatibility");
    
    return TRUE;
}

/*
 * Initialize Windows compatibility layer
 */
void windows_compat_init(void) {
    kprintf("Windows compatibility layer initialized\n");
}
