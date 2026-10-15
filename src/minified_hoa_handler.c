/*
 * minified_hoa_handler.c
 *
 * Three nearly-identical pthread_create() wrappers (differing only in
 * entry point and which global thread-id slot they fill), and the three
 * handler thread bodies themselves: an event-message loop, a key-message
 * loop, and a media-callback-message loop, each polling
 * HOA_PROC_ReceiveMsg() for its own message type (1/2/3 respectively)
 * until exit_handler_thread is set.
 */
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "lgncopenapi.h"

/* .sbss globals - thread ids produced by pthread_create(); NB the original
 * discards pthread_create()'s thread-id output parameter entirely (these
 * globals are read right back out, uninitialised, on the success path -
 * see note in each _*_thread_create function below) and instead treats
 * them as a simple "last created thread id" cache that pthread_create
 * itself never actually writes through these particular globals. Declared
 * here only so the wrapper functions' literal control flow has somewhere
 * to read from, matching the original 1:1. */
__UINT32 threadIdCallback;
__UINT32 threadIdKey;
__UINT32 threadIdEvent;

/* The format string at .rodata:00014944 is truncated in the recovered
 * disassembly listing ("(E) Cannot create thread, retVal 0x%..."); the
 * two printf-style arguments actually loaded for it are (retVal, errno
 * message), so the most likely original text is reproduced here as:
 * "(E) Cannot create thread, retVal 0x%x, %s\n" - flagged as inferred. */
#define THREAD_CREATE_FAIL_FMT "(E) Cannot create thread, retVal 0x%x, %s\n"

/* _minified_hoa_event_handler_thread_create - .text:00013ebc */
__UINT32 _minified_hoa_event_handler_thread_create(void)
{
    pthread_attr_t  attr;
    pthread_t       tid;
    int             ret;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    ret = pthread_create(&tid, &attr, (void *(*)(void *))_minified_hoa_event_handler, NULL);
    if (ret != 0) {
        fprintf(stderr, THREAD_CREATE_FAIL_FMT, ret, strerror(ret));
        return 0;
    }

    return threadIdEvent;  /* NB: matches the original, which reads this
                               global back out rather than the `tid` just
                               produced by pthread_create() above. */
}

/* _minified_hoa_key_handler_thread_create - .text:00013dc4 */
__UINT32 _minified_hoa_key_handler_thread_create(void)
{
    pthread_attr_t  attr;
    pthread_t       tid;
    int             ret;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    ret = pthread_create(&tid, &attr, (void *(*)(void *))_minified_hoa_key_handler, NULL);
    if (ret != 0) {
        fprintf(stderr, THREAD_CREATE_FAIL_FMT, ret, strerror(ret));
        return 0;
    }

    return threadIdKey;
}

/* _minified_hoa_callback_handler_thread_create - .text:00013ccc */
__UINT32 _minified_hoa_callback_handler_thread_create(void)
{
    pthread_attr_t  attr;
    pthread_t       tid;
    int             ret;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    ret = pthread_create(&tid, &attr, (void *(*)(void *))_minified_hoa_callback_handler, NULL);
    if (ret != 0) {
        fprintf(stderr, THREAD_CREATE_FAIL_FMT, ret, strerror(ret));
        return 0;
    }

    return threadIdCallback;
}

/* _minified_hoa_event_handler - .text:000140ec
 * Polls HOA_PROC_ReceiveMsg(..., msgType=1) in a blocking retry loop;
 * each message's payload word at offset +4 is forwarded as the `msg`
 * argument to gprocInfo.pfnMsgHandler(msg, 0, NULL, 0) if one is
 * registered. Exits (clearing the message-handler slot first) once
 * exit_handler_thread is set. */
void *_minified_hoa_event_handler(void *arg)
{
    HOA_IPC_MSG_T rcvmsg;
    HOA_STATUS_T  ret;

    (void)arg;

    for (;;) {
        if (exit_handler_thread)
            break;

        do {
            ret = HOA_PROC_ReceiveMsg(&rcvmsg, 1);
        } while (ret != 0);

        if (gprocInfo.pfnMsgHandler != NULL) {
            __UINT32 msg = *(__UINT32 *)(rcvmsg.data + 0);   /* rcvmsg+4 overall */
            ((HOA_MSG_CB_T)gprocInfo.pfnMsgHandler)(msg, 0, NULL, 0);
        }

        if (exit_handler_thread)
            break;
    }

    if (gprocInfo.pfnMsgHandler != NULL)
        gprocInfo.pfnMsgHandler = NULL;

    pthread_exit(NULL);
    return NULL;
}

/* _minified_hoa_key_handler - .text:00014184
 * Polls HOA_PROC_ReceiveMsg(..., msgType=2); interprets the message's
 * offset+8 word as a raw press/release flag (1 => released, anything else
 * => pressed) mapped to LGNC_KEY_COND_T values 2/0, and offset+0 as the
 * raw key code, then calls gprocInfo.pfnKeyEventCallback(key, keyCond,
 * &keyInput) if registered.
 *
 * NB on `keyInput`: the original builds this 20-byte argument block from
 * a stack region that was zero-initialised once before the loop started
 * and is never written to from the received message in this function -
 * only ever copied around and re-zeroed - so the event-info argument it
 * passes to the callback is always all-zero in this build. That is
 * reproduced directly below rather than literally replaying the
 * word-shuffling that produces the same constant result. */
void *_minified_hoa_key_handler(void *arg)
{
    HOA_IPC_MSG_T rcvmsg;
    HOA_STATUS_T  ret;
    LGNC_ADDITIONAL_INPUT_INFO_T keyInput = { 0, 0 };

    (void)arg;

    for (;;) {
        if (exit_handler_thread)
            break;

        do {
            ret = HOA_PROC_ReceiveMsg(&rcvmsg, 2);
        } while (ret != 0);

        {
            __UINT32 key      = *(__UINT32 *)(rcvmsg.data + 0);  /* rcvmsg+4 overall */
            __UINT32 rawCond  = *(__UINT32 *)(rcvmsg.data + 4);  /* rcvmsg+8 overall */
            LGNC_KEY_COND_T keyCond = (rawCond == 1) ? 2 : 0;

            if (gprocInfo.pfnKeyEventCallback != NULL)
                ((HOA_KEY_CB_T)gprocInfo.pfnKeyEventCallback)(key, keyCond, &keyInput);
        }

        if (exit_handler_thread)
            break;
    }

    if (gprocInfo.pfnKeyEventCallback != NULL)
        gprocInfo.pfnKeyEventCallback = NULL;

    pthread_exit(NULL);
    return NULL;
}

/* _minified_hoa_callback_handler - .text:00014070
 * Polls HOA_PROC_ReceiveMsg(..., msgType=3); discards any message whose
 * offset+4 word is non-zero, otherwise forwards offset+8/+0xc as
 * (ch, msg) to Minified_HOA_CB_MEDIA(). */
void *_minified_hoa_callback_handler(void *arg)
{
    HOA_IPC_MSG_T rcvmsg;
    HOA_STATUS_T  ret;

    (void)arg;

    for (;;) {
        if (exit_handler_thread)
            pthread_exit(NULL);

        ret = HOA_PROC_ReceiveMsg(&rcvmsg, 3);
        if (ret != 0)
            continue;

        if (*(__UINT32 *)(rcvmsg.data + 0) != 0)   /* rcvmsg+4 overall */
            continue;

        {
            LGNC_CHANNEL_T ch  = *(LGNC_CHANNEL_T *)(rcvmsg.data + 4);  /* rcvmsg+8  overall */
            LGNC_CB_MSG_T  msg = *(LGNC_CB_MSG_T  *)(rcvmsg.data + 8);  /* rcvmsg+0xc overall */
            Minified_HOA_CB_MEDIA(ch, msg);
        }

        if (exit_handler_thread)
            pthread_exit(NULL);
    }
}
