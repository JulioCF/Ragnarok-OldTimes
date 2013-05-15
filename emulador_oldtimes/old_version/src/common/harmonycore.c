// (c) 2008 - 2011 Harmony Project; Daniel Stelter-Gliese / Sirius_White
//
//  - white@siriuswhite.de
//  - ICQ #119-153
//  - MSN msn@siriuswhite.de
//
// This file is NOT public - you are not allowed to distribute it.
#include "../common/cbasetypes.h"
#include "../common/showmsg.h"
#include "../common/db.h"
#include "../common/strlib.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/harmony.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <fcntl.h>
#endif

#define HARMONY_USE_EA_MEMMGR

// ---

// ---

void *harmony_module = NULL;

static DBMap *mod_exports = NULL;
static DBMap *harm_timer = NULL;

struct HARMSRV_EA_FUNCS   *ea_funcs   = NULL;
struct HARMSRV_HARM_FUNCS *harm_funcs = NULL;

#ifdef __64BIT__
	#define HARMCODEFILE "./harmony/data64.bin"
	typedef uint64 hsysint;
#else
	#define HARMCODEFILE "./harmony/data.bin"
	typedef uint32 hsysint;
#endif

// ---

static void* harmony_get_symbol(const char *name);
static bool harmony_load_module(const char *path);

void _FASTCALL harmony_abnormal_start(int code);
void** _FASTCALL ea_fd2harmsession(int fd);
unsigned int _FASTCALL ea_tick(void);
int _FASTCALL ea_timer_add(unsigned int tick, HarmTimerProc func, int id, intptr data);
int _FASTCALL ea_timer_del(int tid, HarmTimerProc func);
void _FASTCALL ea_socket_disconnect(int fd);
void _FASTCALL harm_msg(const char *format);


void * _FASTCALL crt_alloc(size_t size);
void   _FASTCALL crt_free(void * ptr);

void  _FASTCALL crt_exit(int code);

void* _FASTCALL crt_fopen(const char* file, const char *mode);
int   _FASTCALL crt_fclose(void* file);
char* _FASTCALL crt_fgets(char* buf, int max_count, void* file);
size_t _FASTCALL crt_fread(void* ptr, size_t size, size_t count, void* file);


// ---

void harmony_core_final() {
	db_destroy(mod_exports);
	db_destroy(harm_timer);
	harm_funcs->final();
}

void harmony_core_init() {
	int *module_version;
	void (*module_init)();

	if (!harmony_load_module(HARMCODEFILE)) {
		ShowFatalError("Unable to load Harmony module.\n");
		exit(EXIT_FAILURE);
	}

	module_version = (int*)harmony_get_symbol("version");
	if (!module_version) {
		ShowFatalError("Unable to determine Harmony version.\n");
		exit(EXIT_FAILURE);
	}

	if (*module_version != HARMSRV_VERSION) {
		ShowFatalError("Invalid Harmony version! Expecting %d, have %d.\n", HARMSRV_VERSION, *module_version);
		ShowFatalError("Did you forget to recompile after updating?\n");
		exit(EXIT_FAILURE);
	}
	ShowStatus("Harmony Version: %d.%d.%d\n", HARMSRV_VERSION_MAJOR, HARMSRV_VERSION_MINOR, HARMSRV_VERSION_PATCH);

	harm_funcs = (struct HARMSRV_HARM_FUNCS*)harmony_get_symbol("harm_funcs");
	ea_funcs = (struct HARMSRV_EA_FUNCS*)harmony_get_symbol("ea_funcs");
	module_init = (void(*)())harmony_get_symbol("Init");
	if (!harm_funcs || !ea_funcs || !module_init) {
		ShowFatalError("Invalid harmony module exports.\n");
		exit(EXIT_FAILURE);
	}

	ea_funcs->alloc = crt_alloc;
	ea_funcs->free = crt_free;
	ea_funcs->exit = crt_exit;
	ea_funcs->fopen = crt_fopen;
	ea_funcs->fclose = crt_fclose;
	ea_funcs->fread = crt_fread;
	ea_funcs->fgets = crt_fgets;

	ea_funcs->harm_msg = harm_msg;
	ea_funcs->harmsrv_abnormal_error = harmony_abnormal_start;
	ea_funcs->ea_fd2harmsession = ea_fd2harmsession;
	ea_funcs->ea_tick = ea_tick;
	ea_funcs->timer_add = ea_timer_add;
	ea_funcs->timer_del = ea_timer_del;
	ea_funcs->socket_disconnect = ea_socket_disconnect;

	harm_timer = idb_alloc(DB_OPT_BASE);

	module_init();
	harm_funcs->init();
}

static uint8 *harmony_map_file(const char *path, size_t *size) {
	uint8 *buf;

#ifdef WIN32
	HANDLE hFile;
	DWORD dwBytesWritten;

	hFile = CreateFile(path, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile)
		return NULL;
	*size = GetFileSize(hFile, NULL);

	buf = (uint8*)VirtualAlloc(NULL, *size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!buf) {
		CloseHandle(hFile);
		return NULL;
	}

	if (!ReadFile(hFile, buf, (DWORD)*size, &dwBytesWritten, NULL) || dwBytesWritten != (DWORD)*size) {
		CloseHandle(hFile);
		return NULL;
	}
	
	CloseHandle(hFile);
#else
	struct stat statinf;

	if (stat(path, &statinf))
		return NULL;

	int fd = open(path, O_RDONLY);
	if (!fd)
		return NULL;

	buf = mmap(NULL, statinf.st_size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	*size = statinf.st_size;
#endif
	return buf;
}

static bool harmony_load_module(const char *path) {
	size_t i;
	struct HARMSRV_MOD_HEADER *head;
	size_t pos, size = 0;
	uint8 *buf = harmony_map_file(path, &size);
	if (!buf) {
		ShowFatalError("Failed to open %s.\n", path);
		return false;
	}

	if (size < sizeof(struct HARMSRV_MOD_HEADER))
		return false;

	head = (struct HARMSRV_MOD_HEADER *)buf;
	if (head->signature != HARMSRV_MOD_HEADER_SIGNATURE)
		return false;
	pos = sizeof(*head);
	// Relocations
	{
		hsysint delta = (hsysint)(buf + head->mem_offset);
		for (i = 0; i < head->reloc_count; i++) {
			*(hsysint*)(buf + head->mem_offset + *(uint32*)(buf + pos + i * sizeof(uint32))) += delta;
		}
		pos += head->reloc_count * sizeof(uint32);
	}
	// Exports
	{
		mod_exports = strdb_alloc(DB_OPT_BASE, 50);
		for (i = 0; i < head->export_count; i++) {
			uint32 offset = *(uint32*)(buf + pos);
			strdb_put(mod_exports, (int8*)buf+pos+5, buf + head->mem_offset + offset);
			pos += 4 + 1 + *(uint8*)(buf + pos + 4) + 1;
		}
	}
	return true;
}

// ---

void * _FASTCALL crt_alloc(size_t size) {
#ifdef HARMONY_USE_EA_MEMMGR
	return aCalloc(size, 1);
#else
	return malloc(size);
#endif
}

void _FASTCALL crt_free(void *ptr) {
#ifdef HARMONY_USE_EA_MEMMGR
	aFree(ptr);
#else
	free(ptr);
#endif
}

void  _FASTCALL crt_exit(int code) {
	exit(code);
}

void* _FASTCALL crt_fopen(const char* file, const char *mode) {
	return (void*)fopen(file, mode);
}

int   _FASTCALL crt_fclose(void* file) {
	return fclose((FILE*)file);
}

char* _FASTCALL crt_fgets(char* buf, int max_count, void* file) {
	return fgets(buf, max_count, (FILE*)file);
}

size_t _FASTCALL crt_fread(void* ptr, size_t size, size_t count, void* file) {
	return fread(ptr, size, count, (FILE*)file);
}

void _FASTCALL ea_socket_disconnect(int fd) {
	session[fd]->flag.eof = 1;
}

/* GCC sure is fun. */

struct GccBinaryCompatibilityDoesNotSeemToExist {
	intptr data;
	int id;
	HarmTimerProc func;
};

int ea_timer_wrap(int tid, unsigned int tick, int id, intptr data) {
	struct GccBinaryCompatibilityDoesNotSeemToExist *e = (struct GccBinaryCompatibilityDoesNotSeemToExist *)data;
	
	e->func(tid, tick, e->id, e->data);
	idb_remove(harm_timer, tid);
	aFree(e);

	return 0;
}

int _FASTCALL ea_timer_add(unsigned int tick, HarmTimerProc func, int id, intptr data) {
#if !defined(__64BIT__)
	return add_timer(tick, (TimerFunc)func, id, data);
#else
	struct GccBinaryCompatibilityDoesNotSeemToExist *e;
	int tid;

	CREATE(e, struct GccBinaryCompatibilityDoesNotSeemToExist, 1);

	tid = add_timer(tick, ea_timer_wrap, 0, (intptr)e);
	e->data = data;
	e->id = id;
	e->func = func;
	idb_put(harm_timer, tid, e);

	return tid;
#endif
}

int _FASTCALL ea_timer_del(int tid, HarmTimerProc func) {
#if !defined(__64BIT__)
	return delete_timer(tid, (TimerFunc)func);
#else
	struct GccBinaryCompatibilityDoesNotSeemToExist *e = (struct GccBinaryCompatibilityDoesNotSeemToExist *)idb_get(harm_timer, tid);

	if (!e) {
		ShowWarning("Trying to remove non-existing timer %d\n", tid);
		return -1;
	}

	if (e->func != func) {
		ShowWarning("ea_timer_del: FUnction mismatch!\n");
		return -1;
	}

	idb_remove(harm_timer, tid);
	aFree(e);

	return delete_timer(tid, ea_timer_wrap);
#endif
}

/* --- */

unsigned int _FASTCALL ea_tick(void) {
	return gettick();
}

void** _FASTCALL ea_fd2harmsession(int fd) {
	return &session[fd]->harm_sd;
}

void _FASTCALL harmony_abnormal_start(int code) {
	ShowFatalError("Harmony module reported critical startup code: %d\n", code);
	exit(EXIT_FAILURE);
}

static void* harmony_get_symbol(const char *name) {
	return strdb_get(mod_exports, name);
}

void _FASTCALL harm_msg(const char *msg) {
	ShowMessage(""CL_MAGENTA"[Harmony]"CL_RESET": %s", msg);
}

