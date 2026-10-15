/*
 * lgnc_media.c
 *
 * LGNC_MEDIA_{Stop,Resume,Pause,PlayFile,Finalize} are thin tail-call
 * wrappers around the dynamically-resolved _dl_RPC_LGNCAPIMedia_* pointers:
 * after the LGNC_DL_HOA_IsInitialized() check they return the callee's
 * result *directly*, with no DTV/HOA status conversion (unlike
 * lgnc_system.c's getters/setters, which do convert).
 *
 * LGNC_MEDIA_Initialize is the one function in this file with real extra
 * logic: it registers the app's playback-complete callback with the
 * internal dispatcher (Minified_HOA_CB_MEDIA_Register, in
 * minified_hoa_callback_function.c) before opening the RPC media session,
 * and retries the RPC call in a loop until it succeeds.
 */
#include "lgncopenapi.h"

typedef LGNC_STATUS_T (*RPC_Media_Ch_T)(LGNC_CHANNEL_T ch);
typedef LGNC_STATUS_T (*RPC_Media_PlayFile_T)(LGNC_CHANNEL_T ch, char *uri, unsigned int repeatNumber);
typedef LGNC_STATUS_T (*RPC_Media_Finalize_T)(void);
typedef LGNC_STATUS_T (*RPC_Media_Initialize_T)(LGNC_AUDIO_PCM_INFO_T *PCMInfo);

/* g_initialize @ .bss - set once LGNC_MEDIA_Initialize's RPC call succeeds. */
__BOOLEAN g_initialize;

/* LGNC_MEDIA_Stop - .text:00013590 */
LGNC_STATUS_T LGNC_MEDIA_Stop(LGNC_CHANNEL_T ch)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;   /* -11 */

    return ((RPC_Media_Ch_T)_dl_RPC_LGNCAPIMedia_Stop)(ch);
}

/* LGNC_MEDIA_Resume - .text:000135e0 */
LGNC_STATUS_T LGNC_MEDIA_Resume(LGNC_CHANNEL_T ch)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((RPC_Media_Ch_T)_dl_RPC_LGNCAPIMedia_Resume)(ch);
}

/* LGNC_MEDIA_Pause - .text:00013630 */
LGNC_STATUS_T LGNC_MEDIA_Pause(LGNC_CHANNEL_T ch)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((RPC_Media_Ch_T)_dl_RPC_LGNCAPIMedia_Pause)(ch);
}

/* LGNC_MEDIA_PlayFile - .text:00013680 */
LGNC_STATUS_T LGNC_MEDIA_PlayFile(LGNC_CHANNEL_T ch, char *uri, unsigned int repeatNumber)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((RPC_Media_PlayFile_T)_dl_RPC_LGNCAPIMedia_PlayFile)(ch, uri, repeatNumber);
}

/* LGNC_MEDIA_Finalize - .text:000136f4 */
LGNC_STATUS_T LGNC_MEDIA_Finalize(void)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((RPC_Media_Finalize_T)_dl_RPC_LGNCAPIMedia_Finalize)();
}

/* LGNC_MEDIA_Initialize - .text:00013744
 * Registers callbackFunc with the internal media-callback dispatcher
 * first (unconditionally, even before the IsInitialized gate could fail -
 * NB this call only happens on the success path below, see the retry
 * loop), then calls the RPC initialize function; on failure it loops back
 * and retries the SAME RPC call (not the registration) until it succeeds,
 * setting g_initialize = TRUE only once it does.
 */
LGNC_STATUS_T LGNC_MEDIA_Initialize(LGNC_PLAY_CB_T callbackFunc, LGNC_AUDIO_PCM_INFO_T *PCMInfo)
{
    LGNC_STATUS_T res;

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    Minified_HOA_CB_MEDIA_Register(callbackFunc);

    do {
        res = ((RPC_Media_Initialize_T)_dl_RPC_LGNCAPIMedia_Initialize)(PCMInfo);
    } while (res != 0);

    g_initialize = TRUE;
    return res;   /* == 0 / LGNC_STATUS_OK on loop exit */
}
