/* -------------------------------------------------------------------------- */
/*                                                                            */
/* crfilter.c                                                                 */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: constructor per struttura linked dei filtri collegati       */
/*                al parametro fornito dall'input                             */
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

extern list_filter * crfilter(uint32_t value, char *name, char *desc) {
  // alloca il nodo
  list_filter *p = malloc(sizeof(list_filter));
  memset(p, 0, sizeof(list_filter));                    // azzera struttura

  // lo popola
  p->filter_namnum = value;
  p->filter_name   = malloc(strlen(name)+1);            // alloca mem per la stringa attributo
  strcpy(p->filter_name, name);                         // nome
  if ( desc == NULL )
    p->filter_desc = NULL;                              // opzionale, non c'e'
  else
    p->filter_desc = createStr(desc);                   // descrizione

  p->next = NULL;                                       // prossimo nodo, ora vuoto

  return p;
}

