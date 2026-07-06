/* -------------------------------------------------------------------------- */
/*                                                                            */
/* fltparm.c                                                                  */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description:                                                             */
/*      routine di verifica se un valore del rec (passato in input)           */
/*      corrisponde ai filtri inseriti nella struttura                        */
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

enum {STR, NUM, AND} type_filt;
enum {EQ, NE, ABR} operatore;

// routine per verificare se filtro ok o ko ---  da sistemare
extern int fltparm(list_parameter *p_par, char *name, uint32_t value) {
  int fl_ok  = 0;                        // per default invalido
  int ll_cmp = 0;                        // lunghezza della compare
  int ncp    = 0;
  int ncs    = 0;
  char parm[256];                        // stringa normalizzata per il parametro
  char smfd[256];                        // stringa normalizzata per dato da SMF80
  int op;
  list_filter *p = p_par->param_filter;  // primo valore richiesto
  switch (p_par->param_filt_type) {
    case STR:                            // usa strncmp con lunghezza
     op  = p_par->param_openum;
     while ( p ) {
       if ( op == ABR ) ll_cmp = strlen(p->filter_name);
       else             ll_cmp = strlen(name);
       ncp  = sprintf(parm,"%s",p->filter_name);
       name = rtrim(name);
       ncs  = sprintf(smfd,"%s",name);
//       printf("filtparm: param %s, smf %s, ll %d (p %d s %d)", parm, smfd
//              , ll_cmp, ncp, ncs);
       if ( ! strncmp(smfd, parm, ll_cmp) ) {
          fl_ok = 1;
//          printf("-> Ok\n");
          break;
       }
//       else {
//          printf("-> Ko\n");
//       }
       p = p->next;
     }
     if ( op == NE ) fl_ok = !fl_ok;
     break;
    case NUM:                            // confronto diretto
     op    = p_par->param_openum;
     while ( p ) {
       if ( p->filter_namnum == value ) {
         fl_ok = 1;
         break;
       }
       p = p->next;
     }
     if ( op == NE ) fl_ok = !fl_ok;
     break;
    case AND:                            // AND tra i valori
     op    = p_par->param_openum;
     while ( p ) {
       if ( p->filter_namnum & value ) {
         fl_ok = 1;
         break;
       }
     p = p->next;
     }
     if ( op == NE ) fl_ok = !fl_ok;
     break;
  }
  return fl_ok;
}

