/* -------------------------------------------------------------------------- */
/*                                                                            */
/* findevt.c                                                                  */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: ricerca nella struttura linked l'evento, restituisce il     */
/*                puntatore all'evento nella linked list                      */
/*                                                                            */
/* -------------------------------------------------------------------------- */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smf80ext.h"

// routine per ricercare EVT
extern st_sm80_evt *findevt(const char * name, st_sm80_evt *root_evt) {
   st_sm80_evt *lk = root_evt;
   if ( lk )
   // scorre la lista fino a trovarne la fine per inserire il nuovo nodo
     while( strcmp(lk->evt_name, name) != 0 ) {
        lk = ( st_sm80_evt *) lk->next;
        if ( lk == NULL ) break;
     }
   return lk;
}
// routine per ricercare EVT per codice
extern st_sm80_evt *findevtn(uint8_t num_evt, st_sm80_evt *root_evt) {
   st_sm80_evt *lk = root_evt;
   if ( lk )
   // scorre la lista fino a trovarne la fine per inserire il nuovo nodo
     while( lk->evt_value != num_evt ) {
        lk = ( st_sm80_evt *) lk->next;
        if ( lk == NULL ) break;
     }
   return lk;
}

