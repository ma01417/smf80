/* ========================================================================== */
/*                                                                            */
/*   getSez.c                                                                 */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: funzione per lettura delle sezioni dei record SMF 80        */
/*                                                                            */
/* ========================================================================== */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "smf80ext.h"

#define n_tok 2
void push_sez(const int numb, char * desc);

FILE * fsez;
st_sm80_sez *root_sez = NULL;
st_sm80_sez *p_sez    = NULL;

/*
   -- main line
*/
extern st_sm80_sez *getSez(char *filename, char *member)
{
  char str_par[81];  // buffer di lettura -- per PS  FB 80
  int   ac;          // numero di token
  char *av[100];     // av
  char *p;
  int  c_sez;
  char d[] = "Elenco Sezioni relocabili";
  char * f = alloca(45);

// apre file PDS con dati classi RACF
  ac = sprintf(f, "%s(%s)", filename, member);
  fsez = openf(f, "r", d, "getSec");
  if ( !fsez ) {
    printf("Err: impossibile aprire '%s'\n", f);
    exit(4);
  }

// legge una riga di Sez fino EOF()
  do {
    p = fgets (str_par, sizeof str_par, fsez);
    if ( p==NULL ) break;
// tokenizza la riga logica di parametri letti
    ac = makeargv(str_par, av, strlen(str_par), n_tok);
    if ( strncmp(av[0], "*", 1) == 0 ) continue;  // commento iniziale
// attualizza i parametri in ingresso per la memorizzazione

// converte il numero della sezione letta
    c_sez = (int) strtol(av[0], NULL, 10);
    rtrim(av[1]);

// inserisce il nodo
    push_sez( c_sez, av[1]);
  } while ( ! feof(fsez ) );

  fclose(fsez);
  return root_sez;
}

/* -------------------------------------------------------------------------- */
/*                                                                            */
/* constructor della struttura linked con le sezioni rilocabili SMF 80        */
/*                                                                            */
/* -------------------------------------------------------------------------- */
void push_sez(const int numb, char * desc) {
// crea un nuovo elemento ed alloca la memoria
   st_sm80_sez *lk = (st_sm80_sez *) malloc(sizeof(st_sm80_sez)); // nuovo nodo
// puntamento all'inizio della lista
   st_sm80_sez *linkedlist = root_sez;
// inserisce i dati passati
   lk->sez_num    = numb;
   lk->sez_desc   = createStr(desc);
   // azzera il puntatore al prossimo nodo
   lk->next       = NULL;
// se lista ancora non allocata mette puntatore al primo nodo
   if ( root_sez == NULL) {
      root_sez = lk;
      return;
   }
   // scorre la lista fino a trovarne la fine per inserire il nuovo nodo
   while(linkedlist->next != NULL)
      linkedlist = ( st_sm80_sez *) linkedlist->next;
// inserisce il nuovo nodo
   linkedlist->next = lk;
return;
}
