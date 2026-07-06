/* ========================================================================== */
/*                                                                            */
/*   Function to read and validate a file of parameters and return a          */
/*   structure with parameters values                                         */
/*                                                                            */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Test di lettura di un file di parametri nella forma                      */
/*      <parm> <op> <lista valori> {+}                                        */
/*                  ^            v  v                                         */
/*                  |____________|  |                                         */
/*                  ^               |                                         */
/*                  |_______________|                                         */
/*                                                                            */
/*      dove:                                                                 */
/*        <parm> ::= elemento del record da trovare                           */
/*        <op>   ::= uno tra '=' '<>' 'ABR'                                   */
/*                   '=' ::= uguale a, condizione in OR                       */
/*                  '<>' ::= diverso da, condizione in AND                    */
/*                 'ABR' ::= abbreviato da, condizione in OR                  */
/*        <lista valori> ::= elenco di stringhe da ricercare                  */
/*                                                                            */
/* ========================================================================== */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

/*  per attivare istruzioni di debug rimuovere commento e compilare */
// #define _DBG 1         // in debug mode
// #define _TOK 1         // per esami particolari
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef _AMB_
  #include <malloc.h>     // necessaria per compilatore GNU CC
#endif
#include <assert.h>

#include "smf80ext.h"     // definizioni di supporto : typedef e strutture
#include "smf80sup.h"     // definizioni di supporto : costanti e variabili

st_sm80_evq *root_evq;
st_sm80_cls *root_cls;

// flag per caratteristiche da filtrare
// SMF80SEC_FL  smf80sec_fl;

// variabili globali
char * buff;                       // array dinamico per consentire la concat di altre righe
// questo e' il numero del primo nodo
int c_node = 0;

// questo sono i puntatori ai nodi correnti
// char_linked_t  * p_node = NULL;
// uint8_linked_t * u_node = NULL;

// stringhe per i token di una riga
char    tk_carat[9];
char    tk_oper[4];
int tk_num_ele = 0;                 // numero corrente di elementi caricati
int llr;                            // lunghezza riga
// enumeration per la lista delle caratteristiche
uint8_t enum_car = 0;
// numero della riga corrente letta
int nrow = 0;

// strutture caratteristiche da filtrare ; puntatore primo nodo liste
runparm *p_parm;

// file descriptor utilizzati
FILE * fprm;                        // file descriptor per parametri

// costanti da usare in header messaggi printf()
const char mod_name[9] = "getParm";
const char vid_name[9] = "       ";

// definizione delle funzioni interne
void stampa_filtri(list_parameter *p_par, int num_car);  // stampa elenco dei filtri per la caratteristica fornita

// main line
extern runparm *getParm(char *filePARM, char *fileCNTL, st_sm80_evt *root_evt) {
  char *token, *tok_tr;
//SMF80SEC_FL tobefind = smf80sec_fl;               // elenco sezioni richieste
  size_t len = 0;
  int max_rc = 0;                                   // durante l'analisi memorizza il massimo RC ritornato
  int rc = 0;                                       // current return code
// per formattare l'ora corrente
  char    l_t[9];                     // hh:mm:ss

  get_cl_time(l_t);

  printf(" \n% 8s %s Lettura parametri %s\n", mod_name, l_t, filePARM);

// inizializza struttura parametri
  p_parm = malloc(sizeof(runparm));
  memset(p_parm, 0, sizeof(runparm));

// apertura file dei parametri
  fprm = openf(filePARM, "rb,recfm=FB,lrecl=80,type=record","parametri utente", mod_name); // open a file
  if (fprm == NULL) {
    fprintf(stderr,
     " \n% 8s %s ERROR: impossibile aprire il file %s\n", mod_name, __FUNCTION__, filePARM);
    exit(8);
    }

// caricamento classi RACF -- qui eventuali altri caricamenti
  root_cls = getCls(fileCNTL, "SM80CLS");   // pre-caricate da REXX eseguita in JCL

/* -------------------------------------------------------------------------------- */
/*                                                                                  */
/*  Parte centrale del programma di estrazione dei parametri                        */
/*   . Loop di lettura delle righe passate                                          */
/*   . scarta righe vuote                                                           */
/*   . scarta righe commento (primo token = "*")                                    */
/*   . tokenizza riga                                                               */
/*     * primo token caratteristica, lo controlla e lo carica in parameter.         */
/*     * secondo token operatore, lo controlla e lo carica                          */
/*     * dal terzo token in avanti li carica in parameter.list_element              */
/*       in array; se necessario converte al valore interno binario                 */
/*                                                                                  */
/* -------------------------------------------------------------------------------- */
  printf(" \n");
  printf("% 8s ------------------- file parametri ----------------------------\n", " ");
  while ( !feof(fprm) ){
// definizione di variabili usate per ogni riga letta
    char * buff = malloc(MAX_LL);  // alloca buffer per riga
    char tk_carat[9];
    char tk_oper[4];
    struct string * lista_filtri=NULL;

    uint8_t fl_abr   = 0;

// acquisisce una riga logica con le eventuali concatenazioni
    buff = getlrow( buff, fprm);
    if ( buff == NULL ) break;     // fine dell'input
// estrae le informazioni dalla riga logica e li carica nella struttura
    rc = gettrow(buff, p_parm);

// verifica che sia gestita correttamente
    switch (rc) {
    case 0:       // tutto ok, passa ai controlli logici
      break;
    case 4:       // errori nella riga, la ignora
      rc = 0;     // accetta i warning, dovuti a commenti e righe vuote
      break;
    case 8:       // errori gravi, richiesta l'uscita
      break;
    default:
      break;
    }
    max_rc = MAX(max_rc, rc);
    free(buff);
  }

// controlli logici sui parametri caricati
  rc = chkparm( p_parm, root_cls, root_evt, fileCNTL );
  max_rc = MAX(max_rc, rc);

// se tutto ok prosegue, altrimenti esce
  if (max_rc) {
    printf(" \n% 8s (%d) sono stati riscontrati degli errori nei parametri forniti per l'elaborazione\n",mod_name, max_rc);
    printf("%s controlla i messaggi e correggi prima di rieseguire\n\n",vid_name);
    exit(max_rc);
  }
// verifica di quanto caricato e stampa resoconto dei filtri

  printf(" \n% 8s Filtri richiesti per l'elaborazione\n",mod_name);
  for (int i=0; i<NUMELE(p_parm->root_param); i++) {
    if ( p_parm->root_param[i] != NULL ) {
      stampa_filtri(p_parm->root_param[i], i);
    }
  }
fclose(fprm);
return p_parm;
}

/* ------------------------------------------------------
   stampa elenco dei filtri per la caratteristica fornita
   --------------------------------------------------- */
void stampa_filtri(list_parameter *p_par, int num_car) {
   list_filter * p_fil; // puntatore al primo elemento
   char str_d[45];
   char carat[9];
   char d_num[6];
   int  numcar;
   char * desc;
   int oper;

// intestazione nome della caratteristica e suoi dati
   strcpy(carat, p_par->param_name);
   numcar  = sprintf(d_num, "(%d)", p_par->param_num_f);
   numcar  = p_par->param_num_f;
   desc    = p_par->param_desc->data;
   oper    = p_par->param_openum;

   printf("% 8s %6s %s %s (%s)\n", mod_name, d_num, carat, d_oper[oper], p_par->param_oper);

// loop sui filtri associati
   p_fil = p_par->param_filter;
   // ciclo di stampa dei filtri caricati
   while(p_fil != NULL) {
     char * name = p_fil->filter_name;
     int fil_num = p_fil->filter_namnum;
     sprintf(str_d,"'%s'",name);
     d_num[0] = '\0';
     if ( l_parm_num[num_car] ) {
       numcar  = sprintf(d_num, "(%02x)", p_fil->filter_namnum);
     }
     printf("% 8s   --> % 4s %-12s %s\n", " ", d_num, str_d, p_fil->filter_desc->data);
// se filtro per classe ed operatore ABR, esplode elenco delle classi trattate
     if ( (strcmp("CLASS", carat) == 0) && (oper == 2) ) {
       st_sm80_cls * p_cls = root_cls;
       while( p_cls ) {
         if ( strncmp(name, p_cls->cls_name, strlen(name)) == 0 && p_cls->cls_typ != 1) {
            sprintf(str_d,"'%s'",p_cls->cls_name);
            printf("% 8s   --> % 17s %-12s %s\n", " ", " ", str_d, p_cls->cls_desc->data);
         }
         p_cls = p_cls->next;
       }
     }
     p_fil = p_fil->next;
   }
   printf("\n");
   return;
}
