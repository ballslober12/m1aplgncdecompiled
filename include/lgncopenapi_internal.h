/*
 * lgncopenapi_internal.h
 *
 * Reconstructed internal header for liblgncopenapi.so (MIPS32, ET_DYN).
 *
 * This header was rebuilt from a Ghidra disassembly listing of the shared
 * object. The binary retains a surprising amount of ground truth: original
 * source file names embedded in debug comments (lgnc_system.c, lgnc_media.c,
 * lgnc_log.c, lgnc_dl_hoa.c, lgnc_in_utils.c, minified_hoa*.c), full struct
 * field names for the dynamic-loader tables (DL_HOA_LIB_T, DL_HOA_FUNC_T),
 * and literal string/const-table contents. Where the listing gave an exact
 * name, it is used verbatim. Where only a numeric value or an offset was
 * visible (most LGNC_STATUS_T / HOA_STATUS_T constants, most struct field
 * names for the public LGNC_*_T API types), the name is my best-effort
 * reconstruction and is marked "(inferred)".
 */
#ifndef LGNCOPENAPI_INTERNAL_H
#define LGNCOPENAPI_INTERNAL_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- primitive typedefs (names taken directly from the DWARF-derived
 *      parameter comments in the listing, e.g. "__BOOLEAN", "__UINT32") ---- */
typedef int8_t   __SINT8;
typedef uint8_t  __UINT8;
typedef int16_t  __SINT16;
typedef uint16_t __UINT16;
typedef int32_t  __SINT32;
typedef uint32_t __UINT32;
typedef char     __CHAR;
typedef int      __BOOLEAN;   /* 0 = false, 1 = true, as produced by slt/sltu */

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

/* ---- LGNC_STATUS_T --------------------------------------------------
 * The only literal values the function bodies ever load into v0/v1 are:
 *   0        -> success (default/zero-initialized "res")
 *  -1 (0xFFFFFFFF)  -> generic failure
 *  -3 (0xFFFFFFFD)  -> invalid parameter (used on every NULL-pointer check)
 * -11 (0xFFFFFFF5)  -> "not initialized" (used whenever LGNC_DL_HOA_IsInitialized() is false)
 * The symbolic names below are inferred from usage; the numeric values are
 * exact (taken straight from `li v0,-0x3` / `li v1,-0xb` etc.).
 */
typedef enum LGNC_STATUS_T {
    LGNC_STATUS_OK               = 0,
    LGNC_STATUS_FAIL             = -1,
    LGNC_STATUS_INVALID_PARAM    = -3,
    LGNC_STATUS_NOT_INITIALIZED  = -11,
} LGNC_STATUS_T;

/* ---- HOA_STATUS_T / DTV_STATUS_T -------------------------------------
 * Both are converted to/from LGNC_STATUS_T through small lookup tables
 * (enum_hoa_status_t @ .data:000250b8, enum_dtv_status_t @ .data:00025070)
 * recovered verbatim below in lgnc_in_utils.c. The enumerator *names* are
 * not recoverable from the binary (only the numeric values are), so they
 * are named generically HOA_STATUS_0 .. HOA_STATUS_n / DTV_STATUS_0 .. .
 */
typedef int HOA_STATUS_T;
typedef int DTV_STATUS_T;

/* ---- LGNC_MSG_TYPE_T / HOA_MSG_TYPE_T --------------------------------
 * enum_hoa_msg_type_t @ .data:00025000, 14 entries, values 0..13 on the
 * HOA side mapping to a smaller LGNC-side set (0..4, with several HOA
 * values mapping to "no LGNC equivalent", encoded as -1 in the table and
 * specially handled by LGNC_UTILS_Conv_HOA_MSG_TYPE_T/_LGNC_MSG_TYPE_T).
 */
typedef int HOA_MSG_TYPE_T;
typedef int LGNC_MSG_TYPE_T;
#define LGNC_MSG_TYPE_INVALID  ((LGNC_MSG_TYPE_T)-1)

/* ---- misc enums used as parameters (names from the DWARF parameter
 *      comments where available) ---- */
typedef int LGNC_APP_TYPE_T;
typedef int LGNC_CHANNEL_T;
typedef int LGNC_KEY_COND_T;
typedef int LGNC_CB_MSG_T;
typedef int MINIFIED_HOA_MSG_TYPE_T;

/* _keyEventCallback receives this as an 8-byte value split across a2/a3
 * (standard o32 struct-by-value ABI); exact field meaning is not
 * recoverable, only the 2-word size. */
typedef struct LGNC_ADDITIONAL_INPUT_INFO_T {
    __UINT32 field_0;
    __UINT32 field_4;
} LGNC_ADDITIONAL_INPUT_INFO_T;

/* ---- LGNC_SYSTEM_CALLBACKS_T ------------------------------------------
 * LGNC_SYSTEM_Initialize() only dereferences offsets 0x0, 0x4, 0x8, 0xc
 * of *pCallbacks (reads all four, checks only field 0x0 == pfnMsgHandler
 * for NULL, then bulk-copies all four words into the global _sys_callbacks).
 * Field names beyond pfnMsgHandler are inferred from typical LGNC SDK
 * naming (pfnKeyHandler / pfnModeChanged-ish slots); field *order* and
 * *size* (16 bytes / 4 words) are exact.
 */
typedef HOA_STATUS_T (*LGNC_MSG_HANDLER_CB_T)(HOA_MSG_TYPE_T msg, __UINT32 submsg, __UINT8 *pData, __UINT16 dataSize);

typedef struct LGNC_SYSTEM_CALLBACKS_T {
    void *pfnMsgHandler;      /* offset 0x0 - required, checked for NULL. Called as
                                  HOA_STATUS_T(HOA_MSG_TYPE_T,UINT32,UINT8*,UINT16) and,
                                  on termination, with (3,0,NULL,0). */
    void *pfnKeyHandler;      /* offset 0x4 - called as __BOOLEAN(UINT32 key,LGNC_KEY_COND_T,
                                  LGNC_ADDITIONAL_INPUT_INFO_T*) from _keyEventCallback.      (inferred name) */
    void *pfnField8;          /* offset 0x8 - never read back by any function in this image. (inferred name) */
    void *pfnFieldC;          /* offset 0xc - never read back by any function in this image. (inferred name) */
} LGNC_SYSTEM_CALLBACKS_T;

/* Mirrors the global _sys_callbacks (.bss, 0x10 bytes) 1:1. */
typedef struct _SYS_CALLBACKS_T {
    void *pfnMsgHandler;
    void *pfnKeyHandler;
    void *pfnField8;
    void *pfnFieldC;
} _SYS_CALLBACKS_T;

/* ---- HOA_PROC_CALLBACKS_T ---------------------------------------------
 * Built on the stack by LGNC_SYSTEM_Initialize() immediately before
 * calling HOA_PROC_RegisterService():
 *   field 0x0 = &_msgHandlerCallback   (HOA_STATUS_T(HOA_MSG_TYPE_T,UINT32,UINT8*,UINT16))
 *   field 0x4 = &_keyEventCallback     (__BOOLEAN(UINT32,LGNC_KEY_COND_T,LGNC_ADDITIONAL_INPUT_INFO_T))
 *   field 0x8 = NULL                   (unused third callback slot)
 * HOA_PROC_RegisterService() itself only reads field 0x4 (the key
 * callback) at a fixed offset, see minified_hoa_handler.c below.
 */
typedef struct HOA_PROC_CALLBACKS_T {
    void *pfnMsgHandler;           /* offset 0x0 - field name confirmed in HOA_PROC_RegisterService */
    void *pfnKeyEventCallback;     /* offset 0x4 - field name confirmed in HOA_PROC_RegisterService */
    void *pfnMouseEventCallback;   /* offset 0x8 - field name confirmed in HOA_PROC_RegisterService; always NULL in this build */
} HOA_PROC_CALLBACKS_T;

/* GPROC_INFO_T (the type of the global `gprocInfo`) - only the three
 * trailing callback-pointer fields (copied from HOA_PROC_CALLBACKS_T by
 * HOA_PROC_RegisterService, read back by the three handler threads, and
 * cleared by HOA_PROC_UnregisterService/the handler threads themselves)
 * are exercised by any function in this image. Everything before offset
 * 0x104 is never touched here - it is presumably IPC/thread-sync state
 * owned by minified_hoa internals outside this build's visible code path,
 * so it is reproduced only as opaque padding to preserve the struct's
 * total size and the three fields' absolute offsets. */
typedef struct GPROC_INFO_T {
    __UINT8 _opaque_unused[0x104];
    void   *pfnMsgHandler;          /* offset 0x104 */
    void   *pfnKeyEventCallback;    /* offset 0x108 */
    void   *pfnMouseEventCallback;  /* offset 0x10c */
} GPROC_INFO_T;

/* ---- LGNC_CTRL_INFO_T ---------------------------------------------------
 * LGNC_SYSTEM_GetSystemInfo() forwards pSystemInfo straight through to the
 * dynamically-loaded RPC call as an opaque output buffer pointer, with no
 * field-level access of its own - so neither this struct's size nor its
 * field layout is recoverable from this binary at all. It is declared as
 * an opaque type; callers must size/lay it out from the real SDK header,
 * not from anything reconstructed here. */
typedef struct LGNC_CTRL_INFO_T LGNC_CTRL_INFO_T; /* intentionally opaque - see note above */

/* ---- LGNC_AUDIO_PCM_INFO_T / LGNC_PLAY_CB_T ---------------------------
 * LGNC_MEDIA_Initialize(LGNC_PLAY_CB_T callbackFunc, LGNC_AUDIO_PCM_INFO_T *PCMInfo)
 * never dereferences PCMInfo itself in this function (it is forwarded
 * verbatim to the dynamically-loaded RPC call), so only the pointer/value
 * semantics are known, not the struct layout.
 */
typedef void (*LGNC_PLAY_CB_T)(LGNC_CHANNEL_T ch, LGNC_CB_MSG_T msg);
typedef struct LGNC_AUDIO_PCM_INFO_T LGNC_AUDIO_PCM_INFO_T; /* opaque: forwarded by pointer only */

/* =====================================================================
 * HOA process-level ("minified HOA") types - minified_hoa*.c
 * =====================================================================
 */

/* IPC_* helpers operate on classic System V IPC primitives (msgget/msgsnd/
 * msgrcv/msgctl + ftok), so __SINT32 here really is `key_t`/`int`. */

typedef struct HOA_IPC_MSG_T {
    long     mtype;         /* required first field for msg{snd,rcv} */
    __UINT8  data[4096];    /* exact bound unknown; sized generously, only
                                the pointer + size are passed through */
} HOA_IPC_MSG_T;

typedef __BOOLEAN  (*HOA_KEY_CB_T)(__UINT32 key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *event);
typedef HOA_STATUS_T (*HOA_MSG_CB_T)(HOA_MSG_TYPE_T msg, __UINT32 submsg, __UINT8 *pData, __UINT16 dataSize);

/* _sys_exitcode @ .data:00025260 (lgnc_system.c:51) - the only value
 * actually observed being stored into it in this image is 1, annotated by
 * Ghidra itself as LGNC_EXITCODE_BACK; 2 and 3 are also stored (from the
 * two hardware EXIT-class keys) without a recovered symbolic name. */
typedef enum LGNC_EXITCODE_T {
    LGNC_EXITCODE_BACK = 1,
} LGNC_EXITCODE_T;

/* =====================================================================
 * Dynamic-loader tables - lgnc_dl_hoa.c / lgnc_dl_libs_func_table.inc
 * These are reproduced EXACTLY as laid out in .data: field names (fname,
 * funcs, size, hndl / fptr, name) come straight from the Ghidra struct
 * annotations, which is as close to "ground truth" as a binary-only
 * reconstruction gets.
 * =====================================================================
 */
typedef struct DL_HOA_FUNC_T {
    void       **fptr;   /* address of a .sbss function-pointer slot, filled in by dlsym() */
    const char  *name;   /* symbol name to dlsym() for */
} DL_HOA_FUNC_T;

typedef struct DL_HOA_LIB_T {
    const char    *fname;  /* library file name passed to dlopen() */
    DL_HOA_FUNC_T *funcs;  /* table of symbols to resolve inside it */
    int            size;   /* number of entries in funcs[] */
    void          *hndl;   /* dlopen() handle, filled in at init time */
} DL_HOA_LIB_T;

/* =====================================================================
 * Cross-module globals and internal (non-exported) helper declarations.
 * Defined in the .c file named in each comment.
 * =====================================================================
 */

/* lgnc_dl_hoa.c */
extern __BOOLEAN _dl_hoa_init;
extern void *_dl_preload_hnds[4];

extern void *_dl_DirectFBInit;
extern void *_dl_DirectFBCreate;
extern void *_dl_RPC_OpenLGNCAPISystem;
extern void *_dl_RPC_CloseLGNCAPISystem;
extern void *_dl_RPC_OpenLGNCAPIAppDRM;
extern void *_dl_RPC_LGNCAPIAppDRM_GetKey;
extern void *_dl_RPC_OpenLGNCAPIMedia;
extern void *_dl_RPC_CloseLGNCAPIMedia;
extern void *_dl_RPC_CloseLGNCAPIAppDRM;
extern void *_dl_RPC_LGNCAPISystem_GetVolume;
extern void *_dl_RPC_LGNCAPISystem_SetVolume;
extern void *_dl_RPC_LGNCAPISystem_GetSystemInfo;
extern void *_dl_RPC_LGNCAPISystem_GetMute;
extern void *_dl_RPC_LGNCAPISystem_SetMute;
extern void *_dl_RPC_LGNCAPISystem_GetLanguage;
extern void *_dl_RPC_LGNCAPISystem_Finalize;
extern void *_dl_RPC_LGNCAPIMedia_Initialize;
extern void *_dl_RPC_LGNCAPIMedia_Finalize;
extern void *_dl_RPC_LGNCAPIMedia_PlayFile;
extern void *_dl_RPC_LGNCAPIMedia_Pause;
extern void *_dl_RPC_LGNCAPIMedia_Resume;
extern void *_dl_RPC_LGNCAPIMedia_Stop;

extern void *_dl_API_APPDRM_Open;
extern void *_dl_API_APPDRM_Read;
extern void *_dl_API_APPDRM_Lseek;
extern void *_dl_API_APPDRM_Write;
extern void *_dl_API_APPDRM_Fstat;
extern void *_dl_API_APPDRM_Close;
extern void *_dl_API_APPDRM_CheckEncrypted;
extern void *_dl_API_APPDRM_InitKey;
extern void *_dl_API_APPDRM_Shift_Keybytes;
extern void *_dl_API_APPDRM_GetAppMediaKey;

/* lgnc_system.c */
extern char              _sys_app_path[0x1000];
extern _SYS_CALLBACKS_T  _sys_callbacks;
extern LGNC_EXITCODE_T   _sys_exitcode;
extern int               gLGNCDebugEnable;

void          _sendTerminationSignal(void);
__BOOLEAN     _keyEventCallback(__UINT32 key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T event);
HOA_STATUS_T  _msgHandlerCallback(HOA_MSG_TYPE_T msg, __UINT32 submsg, __UINT8 *pData, __UINT16 dataSize);

/* minified_hoa.c / minified_hoa_handler.c */
HOA_STATUS_T _IPC_DeleteMsg(__SINT32 msgQId);
HOA_STATUS_T _IPC_ReceiveMsg(__SINT32 msgQId, void *pMsgBuf, __UINT32 msgSize, __SINT32 msgType, __SINT32 msgFlag);
HOA_STATUS_T _IPC_SendMsg(__SINT32 msgQId, void *pMsgBuf, __UINT32 msgSize, __SINT32 msgFlag);
HOA_STATUS_T _IPC_CreateMsg(__SINT32 key, __SINT32 msgFlag, __SINT32 *pMsgQId);
HOA_STATUS_T _IPC_CreateKey(__CHAR *pPathName, __SINT32 projId, __SINT32 *pKey);

void     _minified_hoa_handler_thread_exit(void);
__UINT32 _minified_hoa_callback_handler_thread_create(void);
__UINT32 _minified_hoa_key_handler_thread_create(void);
__UINT32 _minified_hoa_event_handler_thread_create(void);
void    *_minified_hoa_callback_handler(void *arg);
void    *_minified_hoa_event_handler(void *arg);
void    *_minified_hoa_key_handler(void *arg);

/* minified_hoa.c globals */
extern __SINT32    msgQId;                /* .bss:msgQId */
extern __BOOLEAN   exit_handler_thread;   /* .bss:exit_handler_thread */
extern GPROC_INFO_T gprocInfo;            /* .bss:gprocInfo */
extern __UINT32     threadIdCallback;     /* .sbss:threadIdCallback */
extern __UINT32     threadIdKey;          /* .sbss:threadIdKey */
extern __UINT32     threadIdEvent;        /* .sbss:threadIdEvent */

/* minified_hoa_callback_function.c */
extern LGNC_PLAY_CB_T gMediaCBFn;         /* .bss:gMediaCBFn */

#ifdef __cplusplus
}
#endif

#endif /* LGNCOPENAPI_INTERNAL_H */
