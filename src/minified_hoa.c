/*
 * minified_hoa.c  (source name confirmed by debug comments at .data:00025xxx,
 * e.g. ";minified_hoa.c")
 *
 * Implements:
 *   - _IPC_*        thin wrappers around System V IPC (ftok/msgget/msgsnd/
 *                    msgrcv/msgctl) normalising every result to
 *                    HOA_STATUS_T (0 = OK, -1 = fail, -3 = invalid param).
 *   - HOA_PROC_*     the message-queue-based "connection" to the HOA host
 *                    process: Connect/Disconnect manage a single shared
 *                    message queue (key = ftok("/tmp", 't')); Send/Receive/
 *                    Flush/RegisterService/UnregisterService build on it.
 */
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "lgncopenapi.h"

/* ---- globals ---- */
__SINT32     msgQId;             /* .bss:msgQId - current message queue id, or 0/uninitialized */
__BOOLEAN    exit_handler_thread; /* .bss:exit_handler_thread - set by _minified_hoa_handler_thread_exit() */
GPROC_INFO_T gprocInfo;          /* .bss:gprocInfo */

/* _IPC_CreateKey - .text:00013ab4 */
HOA_STATUS_T _IPC_CreateKey(__CHAR *pPathName, __SINT32 projId, __SINT32 *pKey)
{
    key_t key;

    if (pKey == NULL)
        return -3;

    key = ftok(pPathName, projId);
    if (key == (key_t)-1)
        return -1;

    *pKey = key;
    return 0;
}

/* _IPC_CreateMsg - .text:00013a40
 * If the caller supplies msgFlag == 0, defaults to IPC_CREAT|0666. */
HOA_STATUS_T _IPC_CreateMsg(__SINT32 key, __SINT32 msgFlag, __SINT32 *pMsgQId)
{
    int msgid;
    int flags;

    if (key == -1)
        return -1;
    if (pMsgQId == NULL)
        return -3;

    flags = (msgFlag != 0) ? msgFlag : (IPC_CREAT | 0666);
    msgid = msgget((key_t)key, flags);
    if (msgid == -1)
        return -1;

    *pMsgQId = msgid;
    return 0;
}

/* _IPC_DeleteMsg - .text:000137e0 */
HOA_STATUS_T _IPC_DeleteMsg(__SINT32 msgQId_)
{
    int ret;

    if (msgQId_ == -1)
        return -3;

    ret = msgctl(msgQId_, IPC_RMID, NULL);
    return (ret == 0) ? 0 : -1;
}

/* _IPC_SendMsg - .text:000139e0 */
HOA_STATUS_T _IPC_SendMsg(__SINT32 msgQId_, void *pMsgBuf, __UINT32 msgSize, __SINT32 msgFlag)
{
    int ret;

    if (msgQId_ == -1)
        return -3;
    if (pMsgBuf == NULL)
        return -3;

    ret = msgsnd(msgQId_, pMsgBuf, msgSize, msgFlag);
    return (ret != -1) ? 0 : -1;
}

/* _IPC_ReceiveMsg - .text:00013884 */
HOA_STATUS_T _IPC_ReceiveMsg(__SINT32 msgQId_, void *pMsgBuf, __UINT32 msgSize, __SINT32 msgType, __SINT32 msgFlag)
{
    ssize_t ret;

    if (msgQId_ == -1)
        return -3;
    if (pMsgBuf == NULL)
        return -3;

    ret = msgrcv(msgQId_, pMsgBuf, msgSize, msgType, msgFlag);
    return (ret != -1) ? 0 : -1;
}

/* HOA_PROC_Connect - .text:00013b1c
 * key = ftok("/tmp", 't'); queue created with flags=IPC_CREAT (0x200,
 * note: NO permission bits ORed in - this looks like an oversight in the
 * original, preserved verbatim); both calls' results are ignored. */
HOA_STATUS_T HOA_PROC_Connect(void)
{
    __SINT32 key = 0;

    _IPC_CreateKey("/tmp", 't', &key);
    _IPC_CreateMsg(key, IPC_CREAT, &msgQId);

    return 0;
}

/* HOA_PROC_Disconnect - .text:0001383c
 * Result of _IPC_DeleteMsg is ignored; always reports success. */
HOA_STATUS_T HOA_PROC_Disconnect(void)
{
    _IPC_DeleteMsg(msgQId);
    msgQId = 0;
    return 0;
}

/* HOA_PROC_SendMsg - .text:00013b7c
 * Builds a fixed 20-byte IPC message (4-byte mtype + up to 16 bytes of
 * payload copied from msgBody using the CALLER's `size`, even though the
 * message is always sent with a hardcoded wire size of 0x14 - this
 * mirrors the original exactly, including the latent buffer-overflow risk
 * if a caller ever passes size > 16). */
HOA_STATUS_T HOA_PROC_SendMsg(MINIFIED_HOA_MSG_TYPE_T msgType, void *msgBody, int size)
{
    HOA_IPC_MSG_T sendmsg;

    sendmsg.mtype = msgType;
    memcpy(sendmsg.data, msgBody, size);

    HOA_PROC_Connect();

    return _IPC_SendMsg(msgQId, &sendmsg, 0x14, IPC_NOWAIT);
}

/* HOA_PROC_ReceiveMsg - .text:000138e8 */
HOA_STATUS_T HOA_PROC_ReceiveMsg(void *rcvmsg, __SINT32 msgType)
{
    return _IPC_ReceiveMsg(msgQId, rcvmsg, 0x14, msgType, 0);
}

/* HOA_PROC_FlushMsg - .text:00013934
 * Drains up to 1000 pending messages of the given type with IPC_NOWAIT,
 * ignoring every result; always reports success. */
HOA_STATUS_T HOA_PROC_FlushMsg(__SINT32 msgType)
{
    HOA_IPC_MSG_T rcvmsg;
    int i;

    for (i = 0; i < 1000; i++)
        _IPC_ReceiveMsg(msgQId, &rcvmsg, 0x14, msgType, IPC_NOWAIT);

    return 0;
}

/* HOA_PROC_RegisterService - .text:00013fb4 */
HOA_STATUS_T HOA_PROC_RegisterService(int argc, char **argv, HOA_PROC_CALLBACKS_T *pCallbacks)
{
    (void)argc;
    (void)argv;

    if (pCallbacks == NULL)
        return -3;

    gprocInfo.pfnMsgHandler         = pCallbacks->pfnMsgHandler;
    gprocInfo.pfnKeyEventCallback   = pCallbacks->pfnKeyEventCallback;
    gprocInfo.pfnMouseEventCallback = pCallbacks->pfnMouseEventCallback;

    HOA_PROC_Connect();

    HOA_PROC_FlushMsg(1);
    HOA_PROC_FlushMsg(2);
    HOA_PROC_FlushMsg(3);

    exit_handler_thread = FALSE;
    _minified_hoa_event_handler_thread_create();
    _minified_hoa_key_handler_thread_create();
    _minified_hoa_callback_handler_thread_create();

    return 0;
}

/* HOA_PROC_UnregisterService - .text:00013c60
 * NB: only clears gprocInfo's +0x108/+0x104 fields (key and message
 * callbacks); the +0x10c mouse-event callback field is left untouched -
 * matches the original exactly. */
HOA_STATUS_T HOA_PROC_UnregisterService(void)
{
    _minified_hoa_handler_thread_exit();
    HOA_PROC_Disconnect();

    gprocInfo.pfnKeyEventCallback = NULL;
    gprocInfo.pfnMsgHandler       = NULL;

    return 0;
}

/* _minified_hoa_handler_thread_exit - .text:00013bf0
 * Signals all three handler threads to exit by setting the shared flag and
 * then waking each of their blocking receive loops with a dummy message of
 * the matching type (1=event, 2=key, 3=callback). */
void _minified_hoa_handler_thread_exit(void)
{
    exit_handler_thread = TRUE;
    HOA_PROC_SendMsg(1, NULL, 0);
    HOA_PROC_SendMsg(2, NULL, 0);
    HOA_PROC_SendMsg(3, NULL, 0);
}
