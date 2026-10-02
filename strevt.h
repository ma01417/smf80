/* -------------------------------------------------------------------------- */
/*                                                                            */
/* strevt.h                                                                   */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description:                                                             */
/*      struttura per contenere descrizione e dati Event SMF RACF             */
/*                                                                            */
/*                                                                            */
/* -------------------------------------------------------------------------- */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include "uintdef.h"

// -- struttura linked per contenere gli eventi
typedef struct lk_evt_elem {
  uint8_t evt_value;
  char    evt_name[9];
  long    evt_numf;
  struct string * evt_desc;
  struct lk_evt_elem * next;
} st_sm80_evt;

