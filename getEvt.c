/* ========================================================================== */
/*                                                                            */
/*   getEvt.c                                                                 */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: funzione di estrazione degli eventi da PDS con gli eventi   */
/*                e le relative descrizioni                                   */
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
#include <assert.h>

#include "smf80ext.h"

/*********************************************************************
 * define some useful unsigned int of varyng size                    *
 *********************************************************************/
typedef unsigned char      uint8_t;

extern int    makeargv(char *string, char *argv[], int argvsize, int max_argc);        // tokenize a string
extern FILE * openf(const char *filename, const char *open_parm, const char *op_desc, const char * pgmname); // open a file
void push_evt(const char * name, int value, char * desc);

FILE * fevt;
st_sm80_evt *root_evt = NULL;

extern st_sm80_evt *getEvt(char *filename, char *member)
{
  char str_par[81];  // buffer di lettura -- per PDS FB 80
  int   ac;          // numero di token
  char *av[100];     // av
  int  n_tevt = 4;   // Nr. token da estrarre
  char *p;
  int  c_evt = 0;
  char par_cntl[] = "r";
  char * f = alloca(40);

// apre libreria con descrizioni EVT -- events
  ac = sprintf(f, "%s(%s)", filename, member);
  fevt = openf(f, par_cntl, "EVT codici e descrizioni", "getEvt");
  if ( !fevt ) {
    printf("\n% 8s E: impossibile aprire '%s'\n", "getEvt", f);
    printf("% 8s    parametri forniti: '%s'\n", " ", par_cntl);
    printf("% 8s    parametri input  : PDS '%s' member'%s'\n", " ", filename, member);
    printf("\n---------------------------------------\n");
    exit(4);
  }

// legge una riga di EVT
  do {
    p = fgets (str_par, sizeof str_par, fevt);
    if ( p==NULL ) break;
// tokenizza la riga logica di parametri letti
    ac = makeargv(str_par, av, strlen(str_par), n_tevt);
    if ( strncmp(av[0], "*", 1) == 0 ) continue;  // commento iniziale

    trim(av[3], strlen(av[3]));
    push_evt(av[0], ++c_evt, av[3]);

  } while ( ! feof(fevt ) );

  fclose(fevt);
  return root_evt;
}
// routine per inserire EVT alla fine della lista
void push_evt(const char * name, int value, char * desc){
// crea un nuovo elemento ed alloca la memoria
   st_sm80_evt *lk = (st_sm80_evt *) malloc(sizeof(st_sm80_evt)); // nuovo nodo
// puntamento all'inizio della lista
   st_sm80_evt *lke = root_evt;

// attualizza il nodo
   strncpy(lk->evt_name , name, sizeof lk->evt_name);
   lk->evt_value = value;
   lk->evt_desc  = createStr(desc);
   lk->evt_numf  = 0;
   // azzera il puntatore al prossimo nodo
   lk->next = NULL;
// se lista ancora non allocata mette puntatore al primo nodo
   if ( !root_evt ) {
      root_evt = lk;
      return;
   }
   // scorre la lista fino a trovarne la fine per inserire il nuovo nodo
   while(lke->next != NULL)
      lke = ( st_sm80_evt *) lke->next;
// inserisce il nuovo nodo
   lke->next = lk;
return;
}
