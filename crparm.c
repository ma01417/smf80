/* -------------------------------------------------------------------------- */
/*                                                                            */
/* crparm.c                                                                   */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: constructor per struttura linked dei parametri passati      */
/*                in input                                                    */
/*                                                                            */
/* -------------------------------------------------------------------------- */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "smf80ext.h"
#include "smf80sup.h"

// constructor per nodo di parametri
extern list_parameter * crparm(char *name, int num, char *oper, uint8_t opnum, char *desc) {
  // alloca il nodo
  list_parameter *p = malloc(sizeof(list_parameter));

  // lo popola
  strncpy(p->param_name, name, sizeof(p->param_name)); // nome
  p->param_namnum    = num;
  p->param_filt_type = l_parm_num[num];
  strncpy(p->param_oper, oper, sizeof(p->param_oper)); // operatore in chiaro
  p->param_openum    = opnum;
  p->param_num_f     = 0;

  if ( desc)
    p->param_desc = createStr(desc);                   // descrizione
  else
    p->param_desc = NULL;

  p->param_filter = NULL;                              // per ora niente filtri
  p->next = NULL;                                      // prossimo nodo, ora vuoto

  return p;
}

// restituisce puntatore al nodo avente o name o num uguale a quello richiesto
list_parameter * findparm(char *name, uint8_t num, list_parameter *root_parameter) {
  list_parameter *p = root_parameter;

  // scorre la lista
  while ( p ) {
    // se stringa nulla o vuota non fa nulla
    if ( name )      // passato nome, ricerco per nome
      if ( strcmp( p->param_name, name) == 0 ) return p; // trovato per nome
    else             // dovrebbe ricercare per numero
      if ( p->param_namnum == num )            return p; // trovato per numero
  }

  // non trovato, ritorna NULL
  return NULL;
}
