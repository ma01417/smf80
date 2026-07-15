/* ========================================================================== */
/*                                                                            */
/*   getEvq.c                                                                 */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: funzione di estrazione degli EVENT Qualifier relativi       */
/*                ad un evento passato e relativa descrizione                 */
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

void push_evq(const char * name, int value, char * desc);

FILE * fevq;
st_sm80_evq *root_evq = NULL;
st_sm80_evq *p_evq    = NULL;

extern st_sm80_evq *getEvq(char *filename, char *member, char *d_evt)
{
  char str_par[81];  // buffer di lettura -- per PDS FB 80
  int   ac;          // numero di token
  int   c_evq=0;     // numero dell'EVENT Qualifier
  int   n_tevd=4;    // numero di token da estrarre
  char *av[100];     // av
  int  n_tevt = 4;   // Nr. token da estrarre
  char *p;
  int  c_evt = 0;

  char *f = alloca(80);
  char *d = alloca(80);

// apre libreria con descrizioni EVQ -- event qualifiers
  ac = sprintf(f, "%s(%s)", filename, member);
  ac = sprintf(d, "EVQ per %s e descrizioni", d_evt);
/*
     NOTA: sostituire <MyPrefix> con il prefisso delle proprie librerie PDS/PDSE
          Apertura file EVQ per EVENT e descrizioni  //'<MyPrefix>.CNTL(SM80EVNQ)'
     EVENT 0 ricerca evq per evt 'EVQ per EVENT e descrizioni'
*/
  fevq = openf(f, "r", d, "getEvq");
  if ( !fevq ) {
    printf("Err: impossibile aprire '%s'\n", f);
    exit(4);
  }

// legge una riga di EVQ
  do {
    p = fgets(str_par, sizeof str_par, fevq);
    if ( p==NULL ) break;
// tokenizza la riga logica di parametri letti
    ac = makeargv(str_par, av, strlen(str_par), n_tevd);
    if ( strncmp(av[0], "*", 1) == 0 ) continue;  // commento iniziale

    rtrim(av[3]);
    if ( strcmp(av[0], d_evt) == 0 ) {
      push_evq(av[1], c_evq++, av[3]);
    }

  } while ( ! feof(fevq ) );

  fclose(fevq);
  return root_evq;
}

// routine per inserire EVQ alla fine della lista
void push_evq(const char * name, int value, char * desc){
// crea un nuovo elemento ed alloca la memoria
   st_sm80_evq *lk = (st_sm80_evq *) malloc(sizeof(st_sm80_evq)); // nuovo nodo
// puntamento all'inizio della lista
   st_sm80_evq *linkedlist = root_evq;
// inserisce i dati passati
   strncpy(lk->evq_name , name, sizeof lk->evq_name);
   lk->evq_value = value;
   lk->evq_desc =createStr(desc);
   // azzera il puntatore al prossimo nodo
   lk->next = NULL;
// se lista ancora non allocata mette puntatore al primo nodo
   if ( root_evq == NULL) {
      root_evq = lk;
      return;
   }
   // scorre la lista fino a trovarne la fine per inserire il nuovo nodo
   while(linkedlist->next != NULL)
      linkedlist = ( st_sm80_evq *) linkedlist->next;
// inserisce il nuovo nodo
   linkedlist->next = lk;
return;
}
