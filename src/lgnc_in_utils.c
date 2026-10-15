/*
 * lgnc_in_utils.c
 *
 * Source file name confirmed by debug-info comments in the disassembly
 * (".data:... ;lgnc_in_utils.c:NN"). All four lookup tables below are
 * reproduced byte-for-byte from .data:00025000.._00025128 of the original
 * image - this file matches the original logic exactly, not just its shape.
 */
#include "lgncopenapi.h"

/* enum_hoa_msg_type_t  @ .data:00025000  (lgnc_in_utils.c:63)
 * 14 entries of {HOA_MSG_TYPE_T, LGNC_MSG_TYPE_T}. -1 means "no LGNC
 * equivalent" on the LGNC side. */
static const int enum_hoa_msg_type_t[14][2] = {
    {  0,  0 },
    {  1,  1 },
    {  2,  2 },
    {  3, -1 },
    {  4,  3 },
    {  5, -1 },
    {  6,  4 },
    {  7, -1 },
    {  8, -1 },
    {  9,  3 },
    { 10, -1 },
    { 11, -1 },
    { 12, -1 },
    { 13, -1 },
};

/* enum_dtv_status_t  @ .data:00025070  (lgnc_in_utils.c:50)
 * 9 entries of {DTV_STATUS_T, LGNC_STATUS_T}; identity mapping 0..-8. */
static const int enum_dtv_status_t[9][2] = {
    {  0,  0 },
    { -1, -1 },
    { -5, -2 },
    { -2, -3 },
    { -3, -4 },
    { -6, -5 },
    { -4, -6 },
    { -8, -7 },
    { -7, -8 },
};

/* enum_hoa_status_t  @ .data:000250b8  (lgnc_in_utils.c:32)
 * 14 entries of {HOA_STATUS_T, LGNC_STATUS_T}. Identity for the first 13
 * rows; the final row is a duplicate key (-1) with a different value
 * (-11) which is unreachable because the first-match linear scan always
 * resolves the earlier {-1,-1} row first - reproduced verbatim anyway so
 * the table is a true 1:1 copy of the original .data bytes. */
static const int enum_hoa_status_t[14][2] = {
    {  0,   0 },
    {  0,   0 },
    { -1,  -1 },
    { -1,  -1 },
    { -2,  -2 },
    { -3,  -3 },
    { -4,  -4 },
    { -5,  -5 },
    { -6,  -6 },
    { -7,  -7 },
    { -8,  -8 },
    { -9,  -9 },
    { -10, -10 },
    { -1,  -11 },
};

/* LGNC_UTILS_Conv_HOA_STATUS_T - .text:00011ff0 */
__BOOLEAN LGNC_UTILS_Conv_HOA_STATUS_T(HOA_STATUS_T hoa, LGNC_STATUS_T *lgnc)
{
    int i;

    if (lgnc == NULL)
        return FALSE;

    for (i = 0; i < 14; i++) {
        if (enum_hoa_status_t[i][0] == hoa) {
            *lgnc = enum_hoa_status_t[i][1];
            return TRUE;
        }
    }
    return FALSE;
}

/* LGNC_UTILS_Conv_LGNC_STATUS_T - .text:00012048 */
__BOOLEAN LGNC_UTILS_Conv_LGNC_STATUS_T(LGNC_STATUS_T lgnc, HOA_STATUS_T *hoa)
{
    int i;

    if (hoa == NULL)
        return FALSE;

    for (i = 0; i < 14; i++) {
        if (enum_hoa_status_t[i][1] == lgnc) {
            *hoa = enum_hoa_status_t[i][0];
            return TRUE;
        }
    }
    return FALSE;
}

/* LGNC_UTILS_Conv_DTV_STATUS_T - .text:000120a0 */
__BOOLEAN LGNC_UTILS_Conv_DTV_STATUS_T(DTV_STATUS_T dtvstatus, LGNC_STATUS_T *lgnc)
{
    int i;

    if (lgnc == NULL)
        return FALSE;

    for (i = 0; i < 9; i++) {
        if (enum_dtv_status_t[i][0] == dtvstatus) {
            *lgnc = enum_dtv_status_t[i][1];
            return TRUE;
        }
    }
    return FALSE;
}

/* LGNC_UTILS_Conv_HOA_MSG_TYPE_T - .text:000120f8
 * Shape differs from the other three conversion functions: `lgnc` is only
 * tested for NULL as a *loop guard* (`beq lgnc,zero,LAB_00012130` skips the
 * search entirely), not as an up-front bail-out. Control always rejoins at
 * LAB_00012130 and dereferences *lgnc once more to test it against -1,
 * clearing it to 0 in that case - including, verbatim, when lgnc was NULL
 * to begin with. That NULL-dereference-on-failure is a genuine quirk of the
 * original binary (the sibling conversion functions all null-check their
 * output pointer first); it is preserved here rather than "fixed", since
 * the goal is a 1:1 reconstruction. */
__BOOLEAN LGNC_UTILS_Conv_HOA_MSG_TYPE_T(HOA_MSG_TYPE_T hoa, LGNC_MSG_TYPE_T *lgnc)
{
    int i;
    __BOOLEAN ret;
    int v1;

    if (lgnc != NULL) {
        for (i = 0; i < 14; i++) {
            if (enum_hoa_msg_type_t[i][0] == hoa) {
                *lgnc = enum_hoa_msg_type_t[i][1];
                ret = TRUE;
                v1 = *lgnc;
                goto check_no_equiv;
            }
        }
    }

    /* not found in the table, or lgnc == NULL on entry */
    ret = FALSE;
    v1 = *lgnc;          /* NB: NULL pointer dereference in the original if lgnc == NULL */

check_no_equiv:
    if (v1 == -1)
        *lgnc = 0;
    return ret;
}

/* LGNC_UTILS_Conv_LGNC_MSG_TYPE_T - .text:00012160 */
__BOOLEAN LGNC_UTILS_Conv_LGNC_MSG_TYPE_T(LGNC_MSG_TYPE_T lgnc, HOA_MSG_TYPE_T *hoa)
{
    int i;

    if (lgnc + 1 == 0)   /* lgnc == -1: never valid, mirrors `sltu v1,zero,(lgnc+1)` guard */
        return FALSE;

    if (hoa == NULL)
        return FALSE;

    for (i = 0; i < 14; i++) {
        if (enum_hoa_msg_type_t[i][1] == lgnc) {
            *hoa = enum_hoa_msg_type_t[i][0];
            return TRUE;
        }
    }
    return FALSE;
}
