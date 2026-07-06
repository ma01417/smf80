/* -------------------------------------------------------------------------- */
/*                                                                            */
/* chkparm.c                                                                  */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: ottiene la struttura con i parametri ed i filtri relativi   */
/*                e effettua i controlli di validita' sull'insieme dei dati   */
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

char mod_nam[9] = "chkparm";

// ritorna un int valore 0 per nessun elemento o errori da scartare, 1 ok validazione
extern int chkparm(runparm *p_param, st_sm80_cls *root_cls, st_sm80_evt *root_evt, char *fileCNTL)
{

  int max_rc = 0;          // max rc dei controlli
  list_parameter * p_res;  // puntatore al nodo RESULT
  list_parameter * p_tfil; // puntatore al nodo corrente
  list_filter    * p_fil;  // puntatore a linked list dei filtri
  st_sm80_evq    * p_evq;
  st_sm80_evq    * root_evq;
  st_sm80_evt    * p_evt;
  st_sm80_cls    * p_cls;

  char event[9];           // evento
  char p_desc [81] = "\0"; // stringa descrittiva
  char carat[9];           // caratteristica
  char oper[16];           // operatore
  int p_oper;              // identificativo confronto '=' (0) '<>' (1) 'ABR' (2)
  int rc = 0;              // return code da funzioni
  int ac = 0;              // numero di parametri dalla tokenizzazione
  int enum_car = 0;        // posizione caratteristica nella lista
  int enum_ope = 0;        // posizione dell'operatore
  int enum_evt = 0;        // codice dell'evento

  printf("% 8s Verifica logica dei parametri forniti\n", mod_nam);

//verifica correttezza filtri legati a EVENT e carica i valori da controllare
  p_tfil = p_param->root_param[EVENT];
  if (p_tfil != NULL) {  // se eventi richiesti ne carica la lista
    p_fil = p_tfil->param_filter;
    strcpy(event, p_tfil->param_name);
    while (p_fil != NULL ) {
      p_evt = findevt(p_fil->filter_name, root_evt);  // lo ricerca
      if ( p_evt == NULL) {
        max_rc = 8;
        printf("% 8s E: evento '%s' non trovato\n", " ", p_fil->filter_name);
        p_fil = p_fil->next;
        continue;
      }
    // popola con elementi
      p_fil->filter_namnum = p_evt->evt_value;         // codifica interna
      strcpy( p_desc, p_evt->evt_desc->data);
      p_fil->filter_desc = createStr(p_desc);          // descrizione
      p_fil = p_fil->next;                             // tratta il prossimo
    }
  }
  p_res = p_param->root_param[RESULT];
  if (p_res != NULL ) {
    if (p_tfil == NULL) {  // result deve essere associato ad un evento
      max_rc = MAX(max_rc,8);
      printf("% 8s E: caratteristica RESULT chiesta senza EVENT associato\n", " ");
    }
    if (p_tfil->param_num_f != 1) { // RESULT (Event Qualifier) sono diversi per evento
      max_rc = MAX(max_rc,8);
      printf("% 8s E: caratteristica RESULT (EVENT qualifier) abbinabile ad un solo eevento\n", " ");
    }
  // carica gli EVQ per l'evento richiesto
    if ( !max_rc ) {
      strcpy(event, p_tfil->param_filter->filter_name);
      root_evq = getEvq(fileCNTL, "SM80EVNQ", event);  // get linkedlist of event qual for an event
      p_evq = root_evq;

      p_fil = p_res->param_filter;
      while ( p_fil != NULL ) {
        p_evq = root_evq;
        while ( p_evq != NULL ) {
          if ( strcmp(p_evq->evq_name,p_fil->filter_name) == 0) {
            p_fil->filter_namnum = p_evq->evq_value;
            strcpy( p_desc, p_evq->evq_desc->data);
            p_fil->filter_desc = createStr(p_desc);
            break;
          }
          p_evq = p_evq->next;
        }
        if ( !p_fil->filter_desc ) {
          printf("% 8s E: caratteristica RESULT '%s' invalida per evento %s\n", " ", p_fil->filter_name, event);
          max_rc = MAX(max_rc,8);
        }
        p_fil = p_fil->next;
      }
    }
  }

//verifica correttezza filtri legati a DESCR e carica i valori da controllare
  p_tfil = p_param->root_param[DESCR];
  if (p_tfil != NULL) {  // se richieste delle description del recrod le carica
    p_fil = p_tfil->param_filter;
    while (p_fil != NULL ) {
      int find = 0;
      for (int i=0; i<NUMELE(l_descr); i++) {
        if ( strcmp(p_fil->filter_name, l_descr[i]) == 0 ) {
          p_fil->filter_namnum = val_descr[i];
          p_fil->filter_desc   = createStr(lddescr[i]);
          find = 1;
          break;
        }
    // popola con elementi
      }
      if ( !find ) {
        max_rc = MAX(max_rc,8);
        printf("% 8s E: description della registrazione richiesta '%s' non trovata\n", " ", p_fil->filter_name);
        // p_fil = p_fil->next;
        // continue;
      }
      p_fil = p_fil->next;                             // tratta il prossimo
    }
   }

//verifica correttezza filtri legati a REASN e carica i valori da controllare
  p_tfil = p_param->root_param[REASN];
  if (p_tfil != NULL) {  // se richieste le reason for logging per il filtro le carica
    p_fil = p_tfil->param_filter;
    while (p_fil != NULL ) {
      int find = 0;
      for (int i=0; i<NUMELE(l_reasn); i++) {
        if ( strcmp(p_fil->filter_name, l_reasn[i]) == 0 ) {
          p_fil->filter_namnum = val_reasn[i];
          p_fil->filter_desc   = createStr(l_dreasn[i]);
          find = 1;
          break;
        }
    // popola con elementi
      }
      if ( !find ) {
        max_rc = MAX(max_rc,8);
        printf("% 8s E: reason for logging richiesta '%s' non trovate\n", " ", p_fil->filter_name);
        // p_fil = p_fil->next;
        // continue;
      }
      p_fil = p_fil->next;                             // tratta il prossimo
    }
   }

//verifica correttezza filtri legati a AUTH e carica i valori da controllare
  p_tfil = p_param->root_param[AUTH];
  if (p_tfil != NULL) {  // se richieste le authority usate per il filtro le carica
    p_fil = p_tfil->param_filter;
    while (p_fil != NULL ) {
      int find = 0;
      for (int i=0; i<NUMELE(l_auth); i++) {
        if ( strcmp(p_fil->filter_name, l_auth[i]) == 0 ) {
          p_fil->filter_namnum = val_auth[i];
          p_fil->filter_desc   = createStr(l_dauth[i]);
          find = 1;
          break;
        }
      }
    // popola con elementi
      if ( !find ) {
        max_rc = MAX(max_rc,8);
        printf("% 8s E: autority richiesta '%s' inesistente\n", " ", p_fil->filter_name);
        // p_fil = p_fil->next;
        // continue;
      }
      p_fil = p_fil->next;                             // tratta il prossimo
    }
   }

//verifica correttezza filtri legati a CLASS e carica i valori da controllare
  p_tfil = p_param->root_param[CLASS];
  if (p_tfil != NULL) {  // se richiesto filtro per CLASS verifica esistenza della stessa
    char n_desc[10] = "\0";
    p_fil    = p_tfil->param_filter;
    p_oper   = p_tfil->param_openum;
// differenzia controllo tra operatori = e <> rispetto a ABR
    while (p_fil != NULL ) {
      if (p_oper < 2) {
        int find = 0;
        p_cls    = root_cls;
        if ( strcmp(p_fil->filter_name, "DATASET") == 0 ) {
          strcpy(p_desc,"       Attiva, LL prof:  44, upper, def RC:4, oper");
          p_fil->filter_desc   = createStr(p_desc);
          find = 1;
        }
        else
          while (p_cls != NULL) {
            if ( strcmp(p_fil->filter_name, p_cls->cls_name) == 0 ) {
        // verifica che non sia una grouping class
              if ( p_cls->cls_typ == 1 ) {    // lo e'
                printf("% 8s E: La classe %s e' grouping, usare la member %s\n", " ", p_fil->filter_name, p_cls->cls_member->data);
                max_rc = MAX(max_rc,8);
              }
              strcpy(p_desc, p_cls->cls_desc->data);
              find = 1;
              break;
            }
            p_cls = p_cls->next;
          }
        // popola con elementi
          if ( !find ) {
            max_rc = MAX(max_rc,8);
            printf("% 8s E: Classe richiesta '%s' inesistente\n", " ", p_fil->filter_name);
            p_fil = p_fil->next;
            continue;
          }
          p_fil->filter_desc   = createStr(p_desc);
      }
      else {
        p_fil->filter_desc   = createStr("Elenco di classi");
      }
      p_fil = p_fil->next;                             // tratta il prossimo
    }
  }

//verifica correttezza filtri legati a USRJES
  p_tfil = p_param->root_param[USRJES];
  if (p_tfil != NULL) {  // Richiesto filtro per utente JES, deve esserci una sola classe = JESSPOOL
    p_res  = p_param->root_param[CLASS];
    if (p_res == NULL) { // non e' stata richiesta caratteristica CLASS
      max_rc = MAX(max_rc,8);
      printf("% 8s E: caratteristica USRJES (User in risorse JES) abbinabile solo con CLASS = JESSPOOL\n", " ");
    }
    else {
      if ( p_res->param_num_f != 1 ) {
        max_rc = MAX(max_rc,8);
        printf("% 8s caratteristica USRJES (User in risorse JES) abbinabile ad una sola classe (JESSPOOL)\n", " ");
      }
      p_fil  = p_res->param_filter;
      rc = strcmp(p_fil->filter_name, "JESSPOOL");
      if ( rc != 0 ) {
        max_rc = MAX(max_rc,8);
        printf("% 8s E: caratteristica USRJES (User in risorse JES) abbinabile solo alla classe (JESSPOOL)\n", " ");
      }
      p_oper = p_res->param_openum; // confronto '=' (0) '<>' (1) 'ABR' (2)
      if ( p_oper != 0 ) {
        max_rc = MAX(max_rc,8);
        printf("% 8s E: caratteristica USRJES (User in risorse JES) solo se classe = JESSPOOL, non '%s'\n", " ", p_res->param_oper);
      }
    }
  }
  return max_rc;
}
