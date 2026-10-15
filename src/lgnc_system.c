/*
 * lgnc_system.c (source name confirmed by debug comment at .data:00025260,
 * ";lgnc_system.c:51").
 *
 * Every public getter/setter here follows one fixed pattern:
 *   1. check LGNC_DL_HOA_IsInitialized(), fail with LGNC_STATUS_NOT_INITIALIZED (-11) if not,
 *   2. forward the call through a dynamically-resolved _dl_RPC_LGNCAPISystem_* pointer,
 *   3. translate the DTV_STATUS_T result back to LGNC_STATUS_T via LGNC_UTILS_Conv_DTV_STATUS_T().
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include "lgncopenapi.h"

/* ---- globals (.bss / .data) ---- */
char                     _sys_app_path[0x1000];     /* .bss:_sys_app_path, 4096 bytes */
_SYS_CALLBACKS_T         _sys_callbacks;             /* .bss:_sys_callbacks, 0x10 bytes */
LGNC_EXITCODE_T          _sys_exitcode = LGNC_EXITCODE_BACK; /* .data:00025260, init value 1 */
int                      gLGNCDebugEnable;           /* .bss:gLGNCDebugEnable */

typedef DTV_STATUS_T (*RPC_GetLanguage_T)(__UINT32 *lcode);
typedef DTV_STATUS_T (*RPC_SetMute_T)(__BOOLEAN bShowVolumebar, __BOOLEAN bMute);
typedef DTV_STATUS_T (*RPC_GetMute_T)(__BOOLEAN *pbMute);
typedef DTV_STATUS_T (*RPC_SetVolume_T)(__BOOLEAN bShowVolumebar, LGNC_APP_TYPE_T appType, __BOOLEAN bRelative, __SINT8 volumeIn);
typedef DTV_STATUS_T (*RPC_GetVolume_T)(LGNC_APP_TYPE_T appType, __UINT8 *pVolume);
typedef DTV_STATUS_T (*RPC_GetSystemInfo_T)(void *outBuf);
typedef DTV_STATUS_T (*RPC_Finalize_T)(void);

/* LGNC_SYSTEM_GetVersion - .text:00012490
 * Purely hardcoded: always reports 2.0.0, no dynamic-loader interaction. */
LGNC_STATUS_T LGNC_SYSTEM_GetVersion(int *major, int *minor, int *patch)
{
    if (major == NULL || minor == NULL || patch == NULL)
        return LGNC_STATUS_INVALID_PARAM;   /* -3 */

    *major = 2;
    *minor = 0;
    *patch = 0;
    return LGNC_STATUS_OK;
}

/* LGNC_SYSTEM_GetLanguage - .text:000124c8
 * The dynamically-loaded RPC call writes a 32-bit packed language code;
 * the top 3 bytes become a 3-character code string, byte 3 is NUL. */
LGNC_STATUS_T LGNC_SYSTEM_GetLanguage(char *pLang)
{
    LGNC_STATUS_T res = LGNC_STATUS_OK;
    DTV_STATUS_T  dres;
    __UINT32      lcode = 0;

    if (pLang == NULL)
        return LGNC_STATUS_INVALID_PARAM;   /* -3 */

    dres = ((RPC_GetLanguage_T)_dl_RPC_LGNCAPISystem_GetLanguage)(&lcode);

    pLang[0] = (char)(lcode >> 24);
    pLang[1] = (char)(lcode >> 16);
    pLang[2] = (char)(lcode >> 8);
    pLang[3] = '\0';

    LGNC_UTILS_Conv_DTV_STATUS_T(dres, &res);
    return res;
}

/* LGNC_SYSTEM_GetSaveDirectory - .text:00012554
 * Appends the literal subdirectory name "/.lgapp_user_data" (a constant
 * string at .rodata:00014900, 18 bytes incl. NUL - confirmed verbatim in
 * the listing, copied via an unaligned swl/swr/lbu/sb byte sequence
 * rather than a libc call) to the app path. */
LGNC_STATUS_T LGNC_SYSTEM_GetSaveDirectory(char *save_dir)
{
    size_t len;

    if (save_dir == NULL)
        return LGNC_STATUS_INVALID_PARAM;   /* -3 */

    strncpy(save_dir, _sys_app_path, 0x1000);
    len = strlen(save_dir);
    memcpy(save_dir + len, "/.lgapp_user_data", 18);   /* 17 chars + NUL */

    return LGNC_STATUS_OK;
}

/* LGNC_SYSTEM_SetMute - .text:00012608 */
LGNC_STATUS_T LGNC_SYSTEM_SetMute(__BOOLEAN bShowVolumebar, __BOOLEAN bMute)
{
    LGNC_STATUS_T res = LGNC_STATUS_OK;
    DTV_STATUS_T  dres;

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;   /* -11 */

    dres = ((RPC_SetMute_T)_dl_RPC_LGNCAPISystem_SetMute)(bShowVolumebar, bMute);
    LGNC_UTILS_Conv_DTV_STATUS_T(dres, &res);
    return res;
}

/* LGNC_SYSTEM_GetMute - .text:00012680 */
LGNC_STATUS_T LGNC_SYSTEM_GetMute(__BOOLEAN *pbMute)
{
    LGNC_STATUS_T res = LGNC_STATUS_OK;
    DTV_STATUS_T  dres;

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    dres = ((RPC_GetMute_T)_dl_RPC_LGNCAPISystem_GetMute)(pbMute);
    LGNC_UTILS_Conv_DTV_STATUS_T(dres, &res);
    return res;
}

/* LGNC_SYSTEM_SetVolume - .text:000126f8
 * volumeIn is sign-extended from 8 bits (`sll/sra 0x18`) before the call. */
LGNC_STATUS_T LGNC_SYSTEM_SetVolume(__BOOLEAN bShowVolumebar, LGNC_APP_TYPE_T appType,
                                     __BOOLEAN bRelative, __SINT8 volumeIn, __UINT8 *pVolumeOut)
{
    LGNC_STATUS_T res = LGNC_STATUS_OK;
    DTV_STATUS_T  dres;
    __SINT8       v = (__SINT8)volumeIn; /* sign-extend, matches sll/sra pair */

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    dres = ((RPC_SetVolume_T)_dl_RPC_LGNCAPISystem_SetVolume)(bShowVolumebar, appType, bRelative, v);

    if (pVolumeOut != NULL)
        ((RPC_GetVolume_T)_dl_RPC_LGNCAPISystem_GetVolume)(appType, pVolumeOut);

    LGNC_UTILS_Conv_DTV_STATUS_T(dres, &res);
    return res;
}

/* LGNC_SYSTEM_GetVolume - .text:000127c0 */
LGNC_STATUS_T LGNC_SYSTEM_GetVolume(LGNC_APP_TYPE_T appType, __UINT8 *pVolume)
{
    LGNC_STATUS_T res = LGNC_STATUS_OK;
    DTV_STATUS_T  dres;

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    dres = ((RPC_GetVolume_T)_dl_RPC_LGNCAPISystem_GetVolume)(appType, pVolume);
    LGNC_UTILS_Conv_DTV_STATUS_T(dres, &res);
    return res;
}

/* LGNC_SYSTEM_GetSystemInfo - .text:00012838 */
LGNC_STATUS_T LGNC_SYSTEM_GetSystemInfo(LGNC_CTRL_INFO_T *pSystemInfo)
{
    LGNC_STATUS_T res = LGNC_STATUS_OK;
    DTV_STATUS_T  dres;

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;
    if (pSystemInfo == NULL)
        return LGNC_STATUS_INVALID_PARAM;   /* -3 */

    dres = ((RPC_GetSystemInfo_T)_dl_RPC_LGNCAPISystem_GetSystemInfo)(pSystemInfo);
    LGNC_UTILS_Conv_DTV_STATUS_T(dres, &res);
    return res;
}

/* LGNC_SYSTEM_Finalize - .text:000128bc */
LGNC_STATUS_T LGNC_SYSTEM_Finalize(void)
{
    LGNC_STATUS_T res = LGNC_STATUS_OK;
    DTV_STATUS_T  dres;
    HOA_STATUS_T  hres;

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    dres = ((RPC_Finalize_T)_dl_RPC_LGNCAPISystem_Finalize)();
    LGNC_UTILS_Conv_DTV_STATUS_T(dres, &res);

    hres = HOA_PROC_UnregisterService();
    LGNC_UTILS_Conv_HOA_STATUS_T(hres, &res);

    ((RPC_Finalize_T)_dl_RPC_CloseLGNCAPISystem)();
    ((RPC_Finalize_T)_dl_RPC_CloseLGNCAPIMedia)();
    ((RPC_Finalize_T)_dl_RPC_CloseLGNCAPIAppDRM)();

    return res;
}

/* LGNC_SYSTEM_Initialize - .text:00012994 */
LGNC_STATUS_T LGNC_SYSTEM_Initialize(int argc, char **argv, LGNC_SYSTEM_CALLBACKS_T *pCallbacks)
{
    pthread_t         self;
    int               policy;
    struct sched_param param;
    char              app_path[0x1001];
    char              save_dir[0x1001];
    char             *cdir;
    HOA_PROC_CALLBACKS_T callbacks;
    HOA_STATUS_T      hres;
    LGNC_STATUS_T     res = LGNC_STATUS_OK;

    if (argv == NULL)
        return LGNC_STATUS_INVALID_PARAM;
    if (pCallbacks == NULL)
        return LGNC_STATUS_INVALID_PARAM;
    if (pCallbacks->pfnMsgHandler == NULL)
        return LGNC_STATUS_INVALID_PARAM;

    self = pthread_self();
    if (pthread_getschedparam(self, &policy, &param) == 0) {
        policy = 2;            /* SCHED_RR on this libc */
        param.sched_priority = 0x31;
        pthread_setschedparam(self, policy, &param);
    }

    memset(app_path, 0, sizeof(app_path));
    if (realpath(argv[0], app_path) != NULL) {
        cdir = dirname(app_path);
        strncpy(_sys_app_path, cdir, 0x1000);
    }

    memset(save_dir, 0, sizeof(save_dir));
    LGNC_SYSTEM_GetSaveDirectory(save_dir);

    if (argc >= 3 && argv[2] != NULL && strcmp(argv[2], "debug") == 0)
        gLGNCDebugEnable = 1;
    else
        gLGNCDebugEnable = 0;

    if (!LGNC_DL_HOA_Initialize())
        return LGNC_STATUS_FAIL;   /* -1 */

    /* return values of these three "Open" calls are not checked */
    ((RPC_Finalize_T)_dl_RPC_OpenLGNCAPISystem)();
    ((RPC_Finalize_T)_dl_RPC_OpenLGNCAPIAppDRM)();
    ((RPC_Finalize_T)_dl_RPC_OpenLGNCAPIMedia)();

    if (LGNC_APPDRM_Initialize(_sys_app_path) != 0)
        return LGNC_STATUS_FAIL;

    _sys_callbacks.pfnMsgHandler = pCallbacks->pfnMsgHandler;
    _sys_callbacks.pfnKeyHandler = pCallbacks->pfnKeyHandler;
    _sys_callbacks.pfnField8     = pCallbacks->pfnField8;
    _sys_callbacks.pfnFieldC     = pCallbacks->pfnFieldC;

    callbacks.pfnMsgHandler         = (void *)_msgHandlerCallback;
    callbacks.pfnKeyEventCallback   = (void *)_keyEventCallback;
    callbacks.pfnMouseEventCallback = NULL;

    hres = HOA_PROC_RegisterService(argc, argv, &callbacks);
    LGNC_UTILS_Conv_HOA_STATUS_T(hres, &res);
    return res;
}

/* _sendTerminationSignal - .text:00012c2c
 * Internal helper (not exported): notifies the app's registered message
 * handler of termination (msg=3), or - if none was registered - finalizes
 * the library and terminates the process outright. */
void _sendTerminationSignal(void)
{
    if (_sys_callbacks.pfnMsgHandler != NULL) {
        HOA_MSG_CB_T cb = (HOA_MSG_CB_T)_sys_callbacks.pfnMsgHandler;
        cb(3, 0, NULL, 0);
        return;
    }

    LGNC_SYSTEM_Finalize();
    exit(0);
}

/* _keyEventCallback - .text:00012c88
 * Internal helper registered as the HOA_PROC key-event callback. Handles
 * three hardware "exit class" keys on key-down, forwards every event to the
 * application's own key handler (if registered), and additionally
 * implements volume up/down/mute on keys 2/3/9. */
__BOOLEAN _keyEventCallback(__UINT32 key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T event)
{
    __BOOLEAN consumed;

    if (keyCond == 0) {
        if (key == 0x5b) {
            _sys_exitcode = (LGNC_EXITCODE_T)2;
            _sendTerminationSignal();
            return TRUE;
        }
        if (key == 0x7c) {
            _sys_exitcode = (LGNC_EXITCODE_T)3;
            _sendTerminationSignal();
            return TRUE;
        }
        if (key == 0x28) {
            _sys_exitcode = LGNC_EXITCODE_BACK;   /* = 1 */
            goto forward_to_app;
        }
    }

forward_to_app:
    consumed = FALSE;
    if (_sys_callbacks.pfnKeyHandler != NULL) {
        HOA_KEY_CB_T cb = (HOA_KEY_CB_T)_sys_callbacks.pfnKeyHandler;
        consumed = cb(key, keyCond, &event);
    }

    if (key == 3) {                               /* volume down */
        LGNC_SYSTEM_SetVolume(FALSE, 2, TRUE, -1, NULL);
        return TRUE;
    }
    if (key == 9) {                               /* mute toggle */
        __BOOLEAN bMute = FALSE;
        LGNC_SYSTEM_GetMute(&bMute);
        if (bMute == TRUE)
            LGNC_SYSTEM_SetMute(FALSE, FALSE);
        else
            LGNC_SYSTEM_SetMute(FALSE, TRUE);
        return TRUE;
    }
    if (key == 2) {                               /* volume up */
        LGNC_SYSTEM_SetVolume(FALSE, 2, TRUE, 1, NULL);
        return TRUE;
    }

    return consumed;
}

/* _msgHandlerCallback - .text:00012e50
 * Internal helper registered as the HOA_PROC message callback. msg==4 is
 * treated as a termination notice; msg==6 is converted and forwarded to
 * the application's registered message handler. Every other msg is
 * silently acknowledged (returns HOA status 0 / OK). */
HOA_STATUS_T _msgHandlerCallback(HOA_MSG_TYPE_T msg, __UINT32 submsg, __UINT8 *pData, __UINT16 dataSize)
{
    if (msg == 4) {
        _sys_exitcode = (LGNC_EXITCODE_T)2;
        _sendTerminationSignal();
        return 0;
    }

    if (msg == 6) {
        if (_sys_callbacks.pfnMsgHandler != NULL) {
            LGNC_MSG_TYPE_T lmsg = 0;
            LGNC_UTILS_Conv_HOA_MSG_TYPE_T(msg, &lmsg);
            ((HOA_MSG_CB_T)_sys_callbacks.pfnMsgHandler)(lmsg, submsg, pData, (__UINT16)dataSize);
        }
        return 0;
    }

    return 0;
}
