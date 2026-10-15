/*
 * lgnc_dl_hoa.c  (source name confirmed by debug comments, e.g.
 * ".data:00025130 ... ;lgnc_dl_hoa.c:81")
 *
 * Implements the dlopen()-based plugin loader used by the rest of the
 * library: three shared objects (libdirectfb.so, librpc4dtv.so,
 * libappdrm.so) are opened and their symbols resolved via dlsym() into a
 * table of global function-pointer slots (_dl_RPC_*, _dl_API_APPDRM_*,
 * _dl_DirectFB*). Every other LGNC_ and HOA_PROC_ function in the library
 * calls through one of these pointers instead of linking directly.
 *
 * All three table layouts (_dl_hoa_lib_table, _dl_funcs_directfb,
 * _dl_funcs_rpc4dtv, _dl_funcs_appdrm) and every file/symbol name string
 * are copied verbatim from the original .data/.rodata contents.
 */
#define _DEFAULT_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include "lgncopenapi.h"

/* ---- global function-pointer slots (.sbss in the original image) ------ */
void *_dl_DirectFBInit;
void *_dl_DirectFBCreate;

void *_dl_RPC_OpenLGNCAPISystem;
void *_dl_RPC_CloseLGNCAPISystem;
void *_dl_RPC_OpenLGNCAPIAppDRM;
void *_dl_RPC_LGNCAPIAppDRM_GetKey;
void *_dl_RPC_OpenLGNCAPIMedia;
void *_dl_RPC_CloseLGNCAPIMedia;
void *_dl_RPC_CloseLGNCAPIAppDRM;
void *_dl_RPC_LGNCAPISystem_GetVolume;
void *_dl_RPC_LGNCAPISystem_SetVolume;
void *_dl_RPC_LGNCAPISystem_GetSystemInfo;
void *_dl_RPC_LGNCAPISystem_GetMute;
void *_dl_RPC_LGNCAPISystem_SetMute;
void *_dl_RPC_LGNCAPISystem_GetLanguage;
void *_dl_RPC_LGNCAPISystem_Finalize;
void *_dl_RPC_LGNCAPIMedia_Initialize;
void *_dl_RPC_LGNCAPIMedia_Finalize;
void *_dl_RPC_LGNCAPIMedia_PlayFile;
void *_dl_RPC_LGNCAPIMedia_Pause;
void *_dl_RPC_LGNCAPIMedia_Resume;
void *_dl_RPC_LGNCAPIMedia_Stop;

void *_dl_API_APPDRM_Open;
void *_dl_API_APPDRM_Read;
void *_dl_API_APPDRM_Lseek;
void *_dl_API_APPDRM_Write;
void *_dl_API_APPDRM_Fstat;
void *_dl_API_APPDRM_Close;
void *_dl_API_APPDRM_CheckEncrypted;
void *_dl_API_APPDRM_InitKey;
void *_dl_API_APPDRM_Shift_Keybytes;
void *_dl_API_APPDRM_GetAppMediaKey;

/* ---- plugin handles: dlopen() handles for the 4 preload slots used by
 *      LGNC_APPDRM_Initialize() (lgnc_appdrm.c) ---- */
void *_dl_preload_hnds[4];

/* ---- state ---- */
__BOOLEAN _dl_hoa_init;

/* ---- dl_funcs_directfb @ .data:00025160 (lgnc_dl_libs_func_table.inc:20) */
static DL_HOA_FUNC_T _dl_funcs_directfb[2] = {
    { &_dl_DirectFBInit,   "DirectFBInit"   },
    { &_dl_DirectFBCreate, "DirectFBCreate" },
};

/* ---- dl_funcs_rpc4dtv @ .data:00025170 (lgnc_dl_libs_func_table.inc:23) */
static DL_HOA_FUNC_T _dl_funcs_rpc4dtv[20] = {
    { &_dl_RPC_OpenLGNCAPISystem,          "RPC_OpenLGNCAPISystem"          },
    { &_dl_RPC_CloseLGNCAPISystem,         "RPC_CloseLGNCAPISystem"         },
    { &_dl_RPC_OpenLGNCAPIAppDRM,          "RPC_OpenLGNCAPIAppDRM"          },
    { &_dl_RPC_LGNCAPIAppDRM_GetKey,       "RPC_LGNCAPIAppDRM_GetKey"       },
    { &_dl_RPC_OpenLGNCAPIMedia,           "RPC_OpenLGNCAPIMedia"           },
    { &_dl_RPC_CloseLGNCAPIMedia,          "RPC_CloseLGNCAPIMedia"          },
    { &_dl_RPC_CloseLGNCAPIAppDRM,         "RPC_CloseLGNCAPIAppDRM"         },
    { &_dl_RPC_LGNCAPISystem_GetVolume,    "RPC_LGNCAPISystem_GetVolume"    },
    { &_dl_RPC_LGNCAPISystem_SetVolume,    "RPC_LGNCAPISystem_SetVolume"    },
    { &_dl_RPC_LGNCAPISystem_GetSystemInfo,"RPC_LGNCAPISystem_GetSystemInfo"},
    { &_dl_RPC_LGNCAPISystem_GetMute,      "RPC_LGNCAPISystem_GetMute"      },
    { &_dl_RPC_LGNCAPISystem_SetMute,      "RPC_LGNCAPISystem_SetMute"      },
    { &_dl_RPC_LGNCAPISystem_GetLanguage,  "RPC_LGNCAPISystem_GetLanguage"  },
    { &_dl_RPC_LGNCAPISystem_Finalize,     "RPC_LGNCAPISystem_Finalize"     },
    { &_dl_RPC_LGNCAPIMedia_Initialize,    "RPC_LGNCAPIMedia_Initialize"    },
    { &_dl_RPC_LGNCAPIMedia_Finalize,      "RPC_LGNCAPIMedia_Finalize"      },
    { &_dl_RPC_LGNCAPIMedia_PlayFile,      "RPC_LGNCAPIMedia_PlayFile"      },
    { &_dl_RPC_LGNCAPIMedia_Pause,         "RPC_LGNCAPIMedia_Pause"         },
    { &_dl_RPC_LGNCAPIMedia_Resume,        "RPC_LGNCAPIMedia_Resume"        },
    { &_dl_RPC_LGNCAPIMedia_Stop,          "RPC_LGNCAPIMedia_Stop"          },
};

/* ---- dl_funcs_appdrm @ .data:00025210 (lgnc_dl_libs_func_table.inc:26) */
static DL_HOA_FUNC_T _dl_funcs_appdrm[10] = {
    { &_dl_API_APPDRM_Open,              "API_APPDRM_Open"              },
    { &_dl_API_APPDRM_Read,              "API_APPDRM_Read"              },
    { &_dl_API_APPDRM_Lseek,             "API_APPDRM_Lseek"             },
    { &_dl_API_APPDRM_Write,             "API_APPDRM_Write"             },
    { &_dl_API_APPDRM_Fstat,             "API_APPDRM_Fstat"             },
    { &_dl_API_APPDRM_Close,             "API_APPDRM_Close"             },
    { &_dl_API_APPDRM_CheckEncrypted,    "API_APPDRM_CheckEncrypted"    },
    { &_dl_API_APPDRM_InitKey,           "API_APPDRM_InitKey"           },
    { &_dl_API_APPDRM_Shift_Keybytes,    "API_APPDRM_Shift_Keybytes"    },
    { &_dl_API_APPDRM_GetAppMediaKey,    "API_APPDRM_GetAppMediaKey"    },
};

/* _dl_hoa_lib_table @ .data:00025130 (lgnc_dl_hoa.c:81) */
static DL_HOA_LIB_T _dl_hoa_lib_table[3] = {
    { "libdirectfb.so", _dl_funcs_directfb, 2,  NULL },
    { "librpc4dtv.so",  _dl_funcs_rpc4dtv, 20,  NULL },
    { "libappdrm.so",   _dl_funcs_appdrm,  10,  NULL },
};

#define DL_HOA_LIB_COUNT (sizeof(_dl_hoa_lib_table) / sizeof(_dl_hoa_lib_table[0]))

/* Preload library name table @ .data:00024ff0 (4 entries, no symbol
 * resolution performed on these - they are just dlopen()'d with
 * RTLD_LAZY|RTLD_GLOBAL so their symbols become available process-wide for
 * the three libraries above to resolve against). */
static const char *_dl_preload_hnds_names[4] = {
    "libdirectfb.so",
    "librpc4dtv.so",
    "libz.so",
    "libfreetype.so",
};

/* LGNC_DL_HOA_IsInitialized - .text:000121d0 */
__BOOLEAN LGNC_DL_HOA_IsInitialized(void)
{
    return _dl_hoa_init;
}

/* LGNC_DL_HOA_Finalize - .text:000121e8
 * Walks _dl_hoa_lib_table back-to-front, zeroing every resolved function
 * pointer in each table's funcs[] array and dlclose()-ing the library
 * handle, then dlclose()-s the 4 preload handles, then clears _dl_hoa_init.
 */
void LGNC_DL_HOA_Finalize(void)
{
    int lib, i;

    for (lib = (int)DL_HOA_LIB_COUNT - 1; lib >= 0; lib--) {
        DL_HOA_LIB_T *entry = &_dl_hoa_lib_table[lib];

        for (i = 0; i < entry->size; i++)
            *entry->funcs[i].fptr = NULL;

        if (entry->hndl != NULL) {
            dlclose(entry->hndl);
            entry->hndl = NULL;
        }
    }

    for (i = 3; i >= 0; i--) {
        if (_dl_preload_hnds[i] != NULL)
            dlclose(_dl_preload_hnds[i]);
    }

    _dl_hoa_init = FALSE;
}

/* LGNC_DL_HOA_Initialize - .text:000122f8
 * 1. setenv()'s something (name/value both taken from .rodata at
 *    gp+0x459c / gp+0x45a8 - the listing truncates the actual string
 *    content for these two particular constants, so the exact name/value
 *    pair is not reproduced here verbatim; the setenv(..., ..., 1) call
 *    itself, with overwrite=1, is exact).
 * 2. dlopen()s the 4 preload handles into _dl_preload_hnds[0..3] with
 *    RTLD_LAZY (flag value 5 => RTLD_LAZY|RTLD_GLOBAL on this libc).
 * 3. For each entry of _dl_hoa_lib_table, dlopen()s entry->fname with flag
 *    6 (RTLD_NOW|RTLD_GLOBAL), stores the handle, and dlsym()s every
 *    function name in entry->funcs[] into the matching .sbss slot. Any
 *    NULL handle or any entry where the first dlsym() in a library fails
 *    aborts initialization (goto LAB_00012420: returns false without
 *    rolling back the .sbss slots already filled, matching the original).
 * 4. On full success, sets _dl_hoa_init = TRUE and returns it.
 */
__BOOLEAN LGNC_DL_HOA_Initialize(void)
{
    int i;
    void *hnd;

    /* step 1: original behaviour preserved as closely as the recovered
     * strings allow; see note above about the two truncated .rodata refs. */
    setenv("", "", 1);

    for (i = 0; i < 4; i++)
        _dl_preload_hnds[i] = NULL;

    for (i = 0; i < 4; i++) {
        hnd = dlopen(_dl_preload_hnds_names[i], RTLD_LAZY | RTLD_GLOBAL);
        if (hnd == NULL)
            goto fail;
        _dl_preload_hnds[i] = hnd;
    }

    for (i = (int)DL_HOA_LIB_COUNT - 1; i >= 0; i--) {
        DL_HOA_LIB_T *entry = &_dl_hoa_lib_table[i];
        int j;

        hnd = dlopen(entry->fname, RTLD_NOW | RTLD_GLOBAL);
        if (hnd == NULL)
            goto fail;
        entry->hndl = hnd;

        for (j = 0; j < entry->size; j++) {
            void *sym = dlsym(hnd, entry->funcs[j].name);
            if (sym == NULL)
                goto fail;
            *entry->funcs[j].fptr = sym;
        }
    }

    _dl_hoa_init = TRUE;
    return _dl_hoa_init;

fail:
    return _dl_hoa_init;   /* FALSE: never got set this call */
}
