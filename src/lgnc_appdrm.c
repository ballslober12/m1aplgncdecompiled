/*
 * lgnc_appdrm.c
 *
 * LGNC_APPDRM_{open,read,write,lseek,close,fstat,CheckEncrypted} are thin
 * tail-call wrappers around the dynamically-resolved _dl_API_APPDRM_*
 * pointers: after the LGNC_DL_HOA_IsInitialized()/NULL checks they return
 * the callee's result *directly* with no DTV/HOA status conversion -
 * unlike lgnc_system.c, these already speak LGNC_STATUS_T.
 */
#define _LARGEFILE64_SOURCE
#define _GNU_SOURCE
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "lgncopenapi.h"

typedef LGNC_STATUS_T (*APPDRM_Open_T)(char *filename, int oflag, int pmode, int *handle);
typedef LGNC_STATUS_T (*APPDRM_Read_T)(int handle, void *buffer, unsigned int count, unsigned int *n_read);
typedef LGNC_STATUS_T (*APPDRM_Lseek_T)(int handle, int offset, int origin, int *offset_res);
typedef LGNC_STATUS_T (*APPDRM_Write_T)(int handle, void *buf, unsigned int size, int *n_write);
typedef LGNC_STATUS_T (*APPDRM_Fstat_T)(int handle, struct stat *statBuf);
typedef LGNC_STATUS_T (*APPDRM_Close_T)(int handle);
typedef LGNC_STATUS_T (*APPDRM_CheckEncrypted_T)(char *filename, int *is_encrypted);
typedef void          (*APPDRM_KeyOp_T)(__UINT8 *keyBuffer);

/* LGNC_APPDRM_CheckEncrypted - .text:00012f50 */
LGNC_STATUS_T LGNC_APPDRM_CheckEncrypted(char *filename, int *is_encrypted)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;   /* -11 */
    if (filename == NULL || filename[0] == '\0')
        return LGNC_STATUS_INVALID_PARAM;     /* -3 */

    return ((APPDRM_CheckEncrypted_T)_dl_API_APPDRM_CheckEncrypted)(filename, is_encrypted);
}

/* LGNC_APPDRM_close - .text:00012fdc */
LGNC_STATUS_T LGNC_APPDRM_close(int handle)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((APPDRM_Close_T)_dl_API_APPDRM_Close)(handle);
}

/* LGNC_APPDRM_fstat - .text:0001303c */
LGNC_STATUS_T LGNC_APPDRM_fstat(int handle, struct stat *statBuf)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((APPDRM_Fstat_T)_dl_API_APPDRM_Fstat)(handle, statBuf);
}

/* LGNC_APPDRM_write - .text:000130b0 */
LGNC_STATUS_T LGNC_APPDRM_write(int handle, void *buf, unsigned int size, int *n_write)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((APPDRM_Write_T)_dl_API_APPDRM_Write)(handle, buf, size, n_write);
}

/* LGNC_APPDRM_lseek - .text:0001314c */
LGNC_STATUS_T LGNC_APPDRM_lseek(int handle, int offset, int origin, int *offset_res)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((APPDRM_Lseek_T)_dl_API_APPDRM_Lseek)(handle, offset, origin, offset_res);
}

/* LGNC_APPDRM_read - .text:000131e8 */
LGNC_STATUS_T LGNC_APPDRM_read(int handle, void *buffer, unsigned int count, unsigned int *n_read)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;

    return ((APPDRM_Read_T)_dl_API_APPDRM_Read)(handle, buffer, count, n_read);
}

/* LGNC_APPDRM_open - .text:00013284 */
LGNC_STATUS_T LGNC_APPDRM_open(char *filename, int oflag, int pmode, int *handle)
{
    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;
    if (filename == NULL || filename[0] == '\0')
        return LGNC_STATUS_INVALID_PARAM;

    return ((APPDRM_Open_T)_dl_API_APPDRM_Open)(filename, oflag, pmode, handle);
}

/* LGNC_APPDRM_Initialize - .text:00013338
 * Builds "<pAppPath>/securitykey.dat"; if that file doesn't exist this is
 * treated as nothing-to-do (returns LGNC_STATUS_OK). If it exists, its
 * first 16 bytes are read as a raw device key and piped through
 * Shift_Keybytes -> RPC_LGNCAPIAppDRM_GetKey -> API_APPDRM_InitKey (each
 * called with just that same 16-byte buffer pointer; no further output
 * parameters are visible in this function, so any transformed key value
 * is assumed to be written back in place by those three calls).
 */
LGNC_STATUS_T LGNC_APPDRM_Initialize(char *pAppPath)
{
    char    keyFilePath[256];
    __UINT8 deviceKey[16];
    int     fd;

    if (!LGNC_DL_HOA_IsInitialized())
        return LGNC_STATUS_NOT_INITIALIZED;
    if (pAppPath == NULL)
        return LGNC_STATUS_FAIL;   /* -1 */

    memset(keyFilePath, 0, sizeof(keyFilePath));
    sprintf(keyFilePath, "%s/%s", pAppPath, "securitykey.dat");

    if (access(keyFilePath, F_OK) != 0)
        return LGNC_STATUS_OK;     /* no key file present: nothing to initialize */

    fd = open64(keyFilePath, O_RDONLY, 0);
    if (fd < 0)
        return LGNC_STATUS_FAIL;

    read(fd, deviceKey, sizeof(deviceKey));
    close(fd);

    ((APPDRM_KeyOp_T)_dl_API_APPDRM_Shift_Keybytes)(deviceKey);
    ((APPDRM_KeyOp_T)_dl_RPC_LGNCAPIAppDRM_GetKey)(deviceKey);
    ((APPDRM_KeyOp_T)_dl_API_APPDRM_InitKey)(deviceKey);

    return LGNC_STATUS_OK;
}
