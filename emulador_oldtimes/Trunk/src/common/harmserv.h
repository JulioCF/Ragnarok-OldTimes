// (c) 2008 - 2011 Harmony Project; Daniel Stelter-Gliese / Sirius_White
//
//  - white@siriuswhite.de
//  - ICQ #119-153
//  - MSN msn@siriuswhite.de
//
// This file is NOT public - you are not allowed to distribute it.

#ifndef _HARMSERV_H
#define _HARMSERV_H

#define HARMSRV_VERSION_MAJOR 3
#define HARMSRV_VERSION_MINOR 3
#define HARMSRV_VERSION_PATCH 7

#define HARMSRV_VERSION (HARMSRV_VERSION_MAJOR * 10000 + HARMSRV_VERSION_MINOR * 100 + HARMSRV_VERSION_PATCH)

#if defined(__64BIT__) && defined(_MSC_VER)
	#define _FASTCALL
	#define _STDCALL
	#define _TIMERCALL
#elif defined(__64BIT__)
	#if !defined(__GNUC__)
		#error Invalid compiler
	#elif __GNUC__  < 4 || (__GNUC__  == 4 && __GNUC_MINOR__ < 4)
		#error GCC 4.4 required!
		#define _FASTCALL
		#define _STDCALL
		#define _TIMERCALL
	#else
		#define _FASTCALL __attribute__((ms_abi))
		#define _STDCALL __attribute__((ms_abi))
		#define _TIMERCALL __attribute__((ms_abi))
	#endif
#elif defined(_MSC_VER)
	#define _FASTCALL __fastcall
	#define _STDCALL __stdcall
	#define _TIMERCALL __cdecl
#else
	#define _FASTCALL __attribute__((fastcall))
	#define _STDCALL __attribute__((stdcall))
	#define _TIMERCALL __attribute__((cdecl))
#endif

#define HSRV_CALL(name, rettype, args) rettype (_FASTCALL * name)args

#ifdef __cplusplus
extern "C" {	
#endif

enum {
	HARMTASK_UNKNOWN = 0,
	HARMTASK_KICK,
	HARMTASK_ATCMD,
	HARMTASK_MSG,
	HARMTASK_PACKET,
	HARMTASK_GET_ID,
	HARMTASK_DC,
	HARMTASK_LOGIN_ACTION,
	HARMTASK_ZONE_ACTION,
	HARMTASK_GET_FD,
	HARMTASK_IS_ACTIVE,
	HARMTASK_SET_LOG_METHOD,

	HARMTASK_LAST
};

enum {
	HARMID_AID = 0,
	HARMID_GID,
	HARMID_GDID,
	HARMID_PID,
	HARMID_CLASS,
	HARMID_GM,
};

typedef int (_TIMERCALL *HarmTimerProc)(int tid, unsigned int tick, int id, intptr data);

// http://gcc.gnu.org/onlinedocs/gcc/Structure_002dPacking-Pragmas.html
#pragma pack(push, 1)

/* Function table */
struct HARMSRV_HARM_FUNCS {
	// CORE
	HSRV_CALL(net_send, void, (int fd, uint8 *buf, int len));
	HSRV_CALL(net_recv, int, (int fd, uint8 *buf, int len, uint8 *base_buf, size_t base_len));
	HSRV_CALL(session_new, int, (int fd, uint32 client_addr));
	HSRV_CALL(session_del, void, (int fd));

	HSRV_CALL(is_secure_session, int, (int fd));

	HSRV_CALL(init, void, (void));
	HSRV_CALL(final, void, (void));

	// LOGIN
	HSRV_CALL(login_init, void, (void));
	HSRV_CALL(login_final, void, (void));
	HSRV_CALL(login_process_auth, int, (int fd, uint8* buf, size_t buf_len, int8* username, int8* password, uint32* version));
	HSRV_CALL(login_process_auth2, int, (int fd, int level));
	HSRV_CALL(login_process, void, (int fd, uint8* buf, size_t buf_len));
	HSRV_CALL(login_get_mac_address, void, (int fd, int8 *buf));

	// ZONE
	HSRV_CALL(zone_init, void, (void));
	HSRV_CALL(zone_final, void, (void));
	HSRV_CALL(zone_process, int, (int fd, uint16 cmd, uint8* buf, size_t buf_len));
	HSRV_CALL(zone_logout, void, (int fd));
	HSRV_CALL(zone_reload, void, (void));
	HSRV_CALL(zone_grf_reload, void, (void));
	HSRV_CALL(zone_autoban_show, void, (int fd));
	HSRV_CALL(zone_autoban_lift, void, (uint32 ip));
	HSRV_CALL(zone_login_pak, void, (uint8* buf, size_t buf_len));
	HSRV_CALL(zone_get_mac_address, void, (int fd, int8 *buf));
};

struct HARMSRV_EA_FUNCS {
	HSRV_CALL(alloc, void*, (size_t size));
	HSRV_CALL(free, void, (void *ptr));

	HSRV_CALL(exit, void, (int code));

	HSRV_CALL(fopen, void*, (const char* file, const char *mode));
	HSRV_CALL(fclose, int, (void* file));
	HSRV_CALL(fgets, char*, (char* buf, int max_count, void* file));
	HSRV_CALL(fread, size_t, (void* ptr, size_t size, size_t count, void* file));

	HSRV_CALL(ea_tick, unsigned int, (void));
	HSRV_CALL(ea_fd2harmsession, void**, (int fd));
	HSRV_CALL(ea_is_mac_banned, bool, (const int8 *mac));
	HSRV_CALL(harmsrv_abnormal_error, void, (int code));

	HSRV_CALL(timer_add, int, (unsigned int tick, HarmTimerProc func, int id, intptr data));
	HSRV_CALL(timer_del, int, (int tid, HarmTimerProc func));

	HSRV_CALL(action_request, void, (int fd, int task, int id, intptr data));

	HSRV_CALL(socket_disconnect, void, (int fd));

	HSRV_CALL(player_log, void, (int fd, const char *msg));

	HSRV_CALL(harm_msg, void, (const char*));
};
/* Function table END */

#define HARMSRV_MOD_HEADER_SIGNATURE 0x56525348
struct HARMSRV_MOD_HEADER {
	uint32 signature;
	uint32 memory_size;
	uint32 reloc_count;
	uint32 export_count;
	uint32 mem_offset;
	/* reloc_block */
	/* export_block */
	/* padding to multiple of 0x1000 */
	/* memory_block */
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif
