/* ========================================================================== */
/*                                                                            */
/*   getCls.c                                                                 */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: funzione per lettura delle classi presenti nel DB RACF      */
/*                come estratte dalla REXX XCDTPROF                           */
/*                                                                            */
/*   Nota : non e' estratta la classe DATASET, aggiunta manualmente           */
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

/*
   -- main line
*/
void push_cls(const char * name, int maxL, int act, int ope, int mix, char * RC,
              int typ, char * mem, char * desc);

FILE * fcls;
st_sm80_cls *root_cls = NULL;
st_sm80_cls *p_cls    = NULL;

extern st_sm80_cls *getCls(char *filename, char *member)
{
  char str_par[81];  // buffer di lettura -- per PS  FB 80
  int   ac;          // numero di token
  char *av[100];     // av
  char *p;
  int  c_maxl;
  int  c_act = 0;
  int  c_ope = 0;
  int  c_mix = 0;
  int  c_typ = 0;
  char c_mem[9];
  char c_desc[51];
  char c_RC[8];
  char d[] = "Elenco classi e proprieta'";
  char * f = alloca(40);

// inserisce manualmente la classe DATASET
  strcpy(c_desc,"Attiva, LL prof:  44, upper, def RC:4, OPER");
  push_cls("DATASET", 44, 1, 1, 0, "4", 0, " ", c_desc);

// apre file PDS con dati classi RACF
  ac = sprintf(f, "%s(%s)", filename, member);
  fcls = openf(f, "r", d, "getCls");
  if ( !fcls ) {
    printf("Err: impossibile aprire '%s'\n", filename);
    exit(4);
  }

// legge una riga di Cls fino EOF()
  do {
    p = fgets (str_par, sizeof str_par, fcls);
    if ( p==NULL ) break;
// tokenizza la riga logica di parametri letti
    ac = makeargv(str_par, av, strlen(str_par), 0);
    if ( strncmp(av[0], "*", 1) == 0 ) continue;  // commento iniziale
// attualizza i parametri in ingresso per la memorizzazione

// classe attiva ?
    if (strcmp(av[2],"YES") == 0 ) {
      strcpy(c_desc,"Attiva");
      c_act = 1;
    }
    else {
      strcpy(c_desc,"W: Non attiva");
      c_act = 0;
    }
// converte la massima lunghezza dei profili
    c_maxl = (int) strtol(av[1], NULL, 10);
    strcat(c_desc,", LL prof:");
    sprintf(c_RC,"%4d",c_maxl);
    strcat(c_desc,c_RC);
// e' mixed case ?
    if (strcmp(av[5],"YES") == 0 ) {
      strcat(c_desc,", mixed");
      c_mix = 1;
    }
    else {
      strcat(c_desc,", upper");
      c_mix = 0;
    }
// converte il RC per profilo non trovato
    strcpy(c_RC,av[4]);
    strcat(c_desc,", def RC:");
    strcat(c_desc,av[4]);

// onora OPERATIONS ?
    if (strcmp(av[3],"YES") == 0 ) {
      c_ope = 1;
      strcat(c_desc,", OPER");
    }
    else {
      c_ope = 0;
    }
// e' grouping class ?
    if ( ac > 6 ) {       // makeargv ritorna in ac sempre +1 rispetto ai token letti
      c_typ = 1;
      strcpy(c_mem, av[6]);
      strcat(c_desc,", mem: ");
      strcat(c_desc,av[6]);
    }
    else {
      c_typ = 0;
      c_mem[1] = '\0';
    }

// inserisce il nodo
    push_cls(av[0], c_maxl, c_act, c_ope, c_mix, c_RC, c_typ, c_mem, c_desc);
  } while ( ! feof(fcls ) );

  fclose(fcls);
  return root_cls;
}

/* -------------------------------------------------------------------------- */
/*                                                                            */
/* constructor per la struttura linked contenente le classi dal DB RACF       */
/*                                                                            */
/* -------------------------------------------------------------------------- */
void push_cls(const char * name, int maxL, int act, int ope, int mix, char * RC, int typ, char * mem, char * desc) {
// crea un nuovo elemento ed alloca la memoria
   st_sm80_cls *lk = (st_sm80_cls *) malloc(sizeof(st_sm80_cls)); // nuovo nodo
// puntamento all'inizio della lista
   st_sm80_cls *linkedlist = root_cls;
// inserisce i dati passati
   strncpy(lk->cls_name , name, sizeof lk->cls_name);
   lk->cls_maxL   = maxL;
   lk->cls_act    = act;
   lk->cls_ope    = ope;
   lk->cls_mix    = mix;
   lk->cls_typ    = typ;
   lk->cls_desc   = createStr(desc);
// solo se grouping class inserisce la classe MEMBER sottostante
   if ( typ )
     lk->cls_member = createStr(mem);
   else
     lk->cls_member = NULL;
   lk->cls_RC     = createStr(RC);
   // azzera il puntatore al prossimo nodo
   lk->next       = NULL;
// per controllo
/*
    fprintf(stderr,"%8s", name);
    fprintf(stderr," Ml %d", maxL);
    fprintf(stderr," Ac %d", act);
    fprintf(stderr," Op %d", ope);
    fprintf(stderr," Mx %d", mix);
    fprintf(stderr," RC %s", RC);
    fprintf(stderr," Gr %d (%s)", typ, mem);
    fprintf(stderr," De '%s'\n", desc);
*/
// se lista ancora non allocata mette puntatore al primo nodo
   if ( root_cls == NULL) {
      root_cls = lk;
      return;
   }
   // scorre la lista fino a trovarne la fine per inserire il nuovo nodo
   while(linkedlist->next != NULL)
      linkedlist = ( st_sm80_cls *) linkedlist->next;
// inserisce il nuovo nodo
   linkedlist->next = lk;
return;
}
