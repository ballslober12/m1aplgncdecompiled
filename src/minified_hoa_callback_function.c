/*
 * minified_hoa_callback_function.c
 *
 * Tiny registration/dispatch pair for the single app-level media-playback
 * callback used by LGNC_MEDIA_Initialize() / _minified_hoa_callback_handler().
 */
#include "lgncopenapi.h"

LGNC_PLAY_CB_T gMediaCBFn;   /* .bss:gMediaCBFn */

/* Minified_HOA_CB_MEDIA_Register - .text:00014270 */
void Minified_HOA_CB_MEDIA_Register(LGNC_PLAY_CB_T callbackFunc)
{
    gMediaCBFn = callbackFunc;
}

/* Minified_HOA_CB_MEDIA - .text:00014288 */
void Minified_HOA_CB_MEDIA(LGNC_CHANNEL_T ch, LGNC_CB_MSG_T msg)
{
    if (gMediaCBFn != NULL)
        gMediaCBFn(ch, msg);
}
