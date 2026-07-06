/* -------------------------------------------------------------------------- */
/*                                                                            */
/* gettrow.c                                                                  */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: a partire dalla riga logica fornita in input, restituisce   */
/*                la caratteristica, l'operatore e una struttura linked       */
/*                contenente l'elenco dei filtri da applicare                 */
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
#include "smf80sup.h"

// funzioni interne
static void put_token(char *data, uint32_t num, char *desc, list_parameter *pf);

// ritorna un int valore 0 per nessun elemento o errori da scartare, 1 ok estrazione effettuata
extern int gettrow(char *buff, runparm *p_param) {

  list_parameter * p_par;  // puntatore al nodo dei parametri
  list_filter    * p_fil;  // puntatore a linked list dei filtri
  char carat[16];          // caratteristica
  char oper[16];           // operatore
  int ac = 0;       // numero di parametri dalla tokenizzazione
  int max_rc = 0;   // massimo return code
  int enum_car = 0; // posizione caratteristica nella lista
  int enum_ope = 0; // posizione dell'operatore
  char *av[100];    // av
  int ll_buff  = strlen(buff) +1;

// definizione di variabili usate per ogni riga letta
  uint8_t max_ele_ll =0;
// dichiara ed azzera flag per i test
  uint8_t fl_found = 0, fl_abr = 0;

// tokenizza la riga logica di parametri letti
  ac = makeargv(buff, av, ll_buff, 0);
// controlla che la riga non sia ne' vuota ne' commento (* come primo token)
  if (ac < 1 ) {                         // riga vuota
    return  4;                           // ignora
  }
  else if ( !strncmp(av[0],"*",1) ) {    // primo carattere di primo token * : commento
         return 4;                       // ignora
       }
// memorizza separatamente i primi due valori, caratteristica e operatore
// caratteristica : primo token
  strup(av[0], carat);                   // rende token uppercase (da mod per classi mixed case)

// controlla se caratteristica richiesta riconosciuta
  for (enum_car = 0; enum_car < NUMELE(l_parm); enum_car++) {
    if (strcmp(carat,l_parm[enum_car]) == 0) {  // trovata
      fl_found = 1;
      break;
    }
  }
  if (!fl_found) {                 // non trovata
    printf("% 8s E: caratteristica richiesta invalida, ignorata '%s' ('%s')\n", " ",carat, av[0]);
    return 8;
  }
// per controlli successivi
  max_ele_ll = l_max_ll[enum_car];
// operatore : secondo token
  strup(av[1], oper);
  if (strcmp(oper,"ABR") == 0) fl_abr = 1;
  for (enum_ope = 0; enum_ope < NUMELE(l_oper); enum_ope++) {
    if (strcmp(oper,l_oper[enum_ope]) == 0) {
      fl_found = 1;
      break;
    }
  }
  if (!fl_found) {
    printf("% 8s E: Operatore invalido %s' ('%s')\n", " ", oper, av[1]);
    return 8;
  }
  // verifica correttezza operatore in relazione alla caratteristica
  if ( fl_abr ) {
    // char l_incom[][9] = {"REASN", "AUTH", "RESULT"};
    // uint8_t fl_error = 0;
    // for (int i_inc =0; i_inc<3; i_inc++) {
    if ( l_parm_num[enum_car] ) {
      printf("% 8s E: operatore ABR non valido per filtro numerico '%s'\n", carat);
       return 8;
    }
    // }
  }

// imposta i flag per le sezioni richieste per questa ricerca
  if ( l_sez[enum_car] == 0 )   p_param->smf80sec_fl.header  = 1;
  if ( l_sez[enum_car] == 33 )  p_param->smf80sec_fl.profile = 1;
  if ( l_sez[enum_car] == 1 ) { p_param->smf80sec_fl.res     = 1;
                                p_param->smf80sec_fl.usrjes  = 1;
                              }
  if ( l_sez[enum_car] == 17 )  p_param->smf80sec_fl.class   = 1;
  if ( l_sez[enum_car] == 33 )  p_param->smf80sec_fl.profile = 1;

// controlla se gia' caricata nel qual caso se l'operatore e'uguale accetta con WARNING
  if ( p_param->root_param[enum_car] ) {  // esiste gia'
    p_par = p_param->root_param[enum_car];
    if ( p_par->param_openum != enum_ope ) {
      printf("\t E: caratteristica '%s' gia'richiesta con operatore differente!\n", carat);
      return 8;
    }
    else
      printf("\t W: caratteristica '%s' gia'richiesta, accodo i filtri\n", carat);
    }
  else                                   // crea nodo per la caratteristica
    p_param->root_param[enum_car] = crparm(carat, enum_car, oper, enum_ope, NULL);

// verifica i filtri inseriti, memorizza per i controlli successivi in chkparm
  p_par = p_param->root_param[enum_car];
  for(int i=2; i<ac; i++) {
    char * copy;
    int i2 = i-2;
    copy = calloc(MAX_TK_LL, sizeof(char));
    strup(av[i], copy);         // converte la stringa in maiuscole
    // controlla che non sia passato un elemento con lunghezza errata
    if ( strlen(copy) > max_ele_ll) {
      printf("\t E: elemento %d->'%s' troppo lungo per %s, ignorato\n", i, copy, carat);
      return 8;
    }

    // crea nodo filtro e lo concatena

    put_token(copy, 0, NULL, p_par); // routine per costruire la lista dei filtri
    p_par->param_num_f++;
    p_fil = p_par->param_filter;
  }
return 0;
}

// routine per aggiungere un nodo alla lista dei filtri  -- qui --
static void put_token(char *data, uint32_t num, char *desc, list_parameter * pf) {
// crea un nuovo elemento ed alloca la memoria
  list_filter * p =crfilter(num, data, desc);                       // constructor per nodo filtri
// puntamento all'inizio della lista
  list_filter * sf = pf->param_filter;
// se lista ancora non allocata mette puntatore al primo nodo
  if ( !pf->param_filter ) {
     pf->param_filter = p;
     return;
   }
// scorre la lista fino a trovarne la fine per inserire il nuovo nodo
  while(sf->next != NULL)
     sf = sf->next;
// inserisce il nuovo nodo
  sf->next = p;
return;
}
