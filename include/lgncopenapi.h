/*
 * lgncopenapi.h - public API surface of liblgncopenapi.so, reconstructed
 * 1:1 from the exported symbol table of the MIPS shared object.
 */
#ifndef LGNCOPENAPI_H
#define LGNCOPENAPI_H

#include "lgncopenapi_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- lgnc_in_utils.c --------------------------------------------------- */
__BOOLEAN LGNC_UTILS_Conv_HOA_STATUS_T(HOA_STATUS_T hoa, LGNC_STATUS_T *lgnc);
__BOOLEAN LGNC_UTILS_Conv_LGNC_STATUS_T(LGNC_STATUS_T lgnc, HOA_STATUS_T *hoa);
__BOOLEAN LGNC_UTILS_Conv_DTV_STATUS_T(DTV_STATUS_T dtvstatus, LGNC_STATUS_T *lgnc);
__BOOLEAN LGNC_UTILS_Conv_HOA_MSG_TYPE_T(HOA_MSG_TYPE_T hoa, LGNC_MSG_TYPE_T *lgnc);
__BOOLEAN LGNC_UTILS_Conv_LGNC_MSG_TYPE_T(LGNC_MSG_TYPE_T lgnc, HOA_MSG_TYPE_T *hoa);

/* ---- lgnc_dl_hoa.c ------------------------------------------------------ */
__BOOLEAN LGNC_DL_HOA_IsInitialized(void);
void      LGNC_DL_HOA_Finalize(void);
__BOOLEAN LGNC_DL_HOA_Initialize(void);

/* ---- lgnc_system.c ------------------------------------------------------ */
LGNC_STATUS_T LGNC_SYSTEM_GetVersion(int *major, int *minor, int *patch);
LGNC_STATUS_T LGNC_SYSTEM_GetLanguage(char *pLang);
LGNC_STATUS_T LGNC_SYSTEM_GetSaveDirectory(char *save_dir);
LGNC_STATUS_T LGNC_SYSTEM_SetMute(__BOOLEAN bShowVolumebar, __BOOLEAN bMute);
LGNC_STATUS_T LGNC_SYSTEM_GetMute(__BOOLEAN *pbMute);
LGNC_STATUS_T LGNC_SYSTEM_SetVolume(__BOOLEAN bShowVolumebar, LGNC_APP_TYPE_T appType, __BOOLEAN bRelative, __SINT8 volumeIn, __UINT8 *pVolumeOut);
LGNC_STATUS_T LGNC_SYSTEM_GetVolume(LGNC_APP_TYPE_T appType, __UINT8 *pVolume);
LGNC_STATUS_T LGNC_SYSTEM_GetSystemInfo(LGNC_CTRL_INFO_T *pSystemInfo);
LGNC_STATUS_T LGNC_SYSTEM_Finalize(void);
LGNC_STATUS_T LGNC_SYSTEM_Initialize(int argc, char **argv, LGNC_SYSTEM_CALLBACKS_T *pCallbacks);

/* ---- lgnc_appdrm.c ------------------------------------------------------ */
LGNC_STATUS_T LGNC_APPDRM_CheckEncrypted(char *filename, int *is_encrypted);
LGNC_STATUS_T LGNC_APPDRM_close(int handle);
LGNC_STATUS_T LGNC_APPDRM_fstat(int handle, struct stat *statBuf);
LGNC_STATUS_T LGNC_APPDRM_write(int handle, void *buf, unsigned int size, int *n_write);
LGNC_STATUS_T LGNC_APPDRM_lseek(int handle, int offset, int origin, int *offset_res);
LGNC_STATUS_T LGNC_APPDRM_read(int handle, void *buffer, unsigned int count, unsigned int *n_read);
LGNC_STATUS_T LGNC_APPDRM_open(char *filename, int oflag, int pmode, int *handle);
LGNC_STATUS_T LGNC_APPDRM_Initialize(char *pAppPath);

/* ---- lgnc_log.c ---------------------------------------------------------- */
int LGNC_LOG_RAWPrint(char *format, ...);
int LGNC_LOG_DBGPrint(char *format, ...);
int LGNC_LOG_FDBGPrint(FILE *stream, char *format, ...);

/* ---- lgnc_media.c --------------------------------------------------------- */
LGNC_STATUS_T LGNC_MEDIA_Stop(LGNC_CHANNEL_T ch);
LGNC_STATUS_T LGNC_MEDIA_Resume(LGNC_CHANNEL_T ch);
LGNC_STATUS_T LGNC_MEDIA_Pause(LGNC_CHANNEL_T ch);
LGNC_STATUS_T LGNC_MEDIA_PlayFile(LGNC_CHANNEL_T ch, char *uri, unsigned int repeatNumber);
LGNC_STATUS_T LGNC_MEDIA_Finalize(void);
LGNC_STATUS_T LGNC_MEDIA_Initialize(LGNC_PLAY_CB_T callbackFunc, LGNC_AUDIO_PCM_INFO_T *PCMInfo);

/* ---- minified_hoa.c (HOA_PROC_* public surface + _IPC_* helpers) -------- */
HOA_STATUS_T HOA_PROC_Disconnect(void);
HOA_STATUS_T HOA_PROC_ReceiveMsg(void *rcvmsg, __SINT32 msgType);
HOA_STATUS_T HOA_PROC_FlushMsg(__SINT32 msgType);
HOA_STATUS_T HOA_PROC_Connect(void);
HOA_STATUS_T HOA_PROC_SendMsg(MINIFIED_HOA_MSG_TYPE_T msgType, void *msgBody, int size);
HOA_STATUS_T HOA_PROC_UnregisterService(void);
HOA_STATUS_T HOA_PROC_RegisterService(int argc, char **argv, HOA_PROC_CALLBACKS_T *pCallbacks);

/* ---- minified_hoa_callback_function.c ------------------------------------ */
void Minified_HOA_CB_MEDIA_Register(LGNC_PLAY_CB_T callbackFunc);
void Minified_HOA_CB_MEDIA(LGNC_CHANNEL_T ch, LGNC_CB_MSG_T msg);

#ifdef __cplusplus
}
#endif

#endif /* LGNCOPENAPI_H */
