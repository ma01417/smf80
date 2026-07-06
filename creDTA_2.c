/* -------------------------------------------------------------------------- */
/*                                                                            */
/* creDTA_2.c                                                                 */
/*   (c) 2026 A.Brezzi                                                        */
/*                                                                            */
/*   Description: constructor per struttura contenente dati sezioni           */
/*                relocabili rec SMF 80, sia normali che extended             */
/*                                                                            */
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
#include <stdint.h>
#include "smf80fmt.h"

/* -------------------------------------------------------------------------- */
/*   Description: constructor per struttura contenente dati sezioni           */
/*                relocabili rec SMF 80, sia normali che extended             */
/* -------------------------------------------------------------------------- */

const char pgm_name[9] = "creDTA_2";

// constructor struttura DTA sezioni rilocabili
extern rel_sect *createDta(const SMF80DTS *p_dts, rel_sect  * p_root, int nrec ) {
  rel_sect *new_dta = (rel_sect *) malloc( sizeof(rel_sect));   // creato nuovo nodo
  rel_sect *p_dta = p_root;
  int smf_dta_dtp = p_dts->SMF80DTP;
// controllo
  if ( !new_dta ) {
    fprintf(stderr, "% 8s E: impossibile allocare nodo sezione rilocabile per record %d\n", pgm_name, nrec);
    exit(8);
  }
  new_dta->dtp  = smf_dta_dtp;
  new_dta->dln  = p_dts->SMF80DLN + 2;
  new_dta->rec_num = nrec;
  new_dta->rel  = (SMF80DTS *) p_dts;
  new_dta->next = NULL;

  if ( !p_root )
    return new_dta;

  while (p_dta->next != NULL)
    p_dta = (rel_sect *) p_dta->next;
  p_dta->next = (rel_sect *) new_dta;
  return p_root;
}

// constructor struttura DT2
extern rel_sec2 *createDt2(const SMF80DT2 *p_dt2, rel_sec2  * p_root2, int nrec) {
  rel_sec2 *new_dt2 = (rel_sec2 *) malloc(sizeof(rel_sec2));
  rel_sec2 *p = p_root2;
  int smf_dta_dtp = p_dt2->SMF80TP2;
  if ( !new_dt2 ) {
    fprintf(stderr, "% 8s E: impossibile allocare nodo sezione rilocabile extended per record %d\n", pgm_name, nrec);
    exit(8);
  }
  new_dt2->dt2  = smf_dta_dtp;
  new_dt2->dl2  = p_dt2->SMF80DL2 + 4;
  new_dt2->rec_num = nrec;
  new_dt2->rel2 = (SMF80DT2 *) p_dt2;
  new_dt2->next = NULL;
  if ( !p_root2 )
    return new_dt2;

//  printf("% 8s alloca nuovo nodo per %d rec %d\n", "DBG", smf_dta_dtp, nrec);
  while ( p->next != NULL )
    p = (rel_sec2 *) p->next;
  p->next = (rel_sec2 *) new_dt2;

  return p_root2;
}

/**********************************************************************/
/* free_dta : libera la memoria allocata a nodi di sezioni relocabili */
/**********************************************************************/
extern void free_dta( rel_sect * p_node) {
  rel_sect * p_next;
  while ( p_node ) {
//    printf("% 8s rec %d libero nodo %p, dta %d, prossimo %p\n", "Free", p_node->rec_num, p_node, p_node->dtp, p_node->next);
    p_next = (rel_sect *) p_node->next;
    free(p_node);
    p_node = (rel_sect *) p_next;
  }
  return;
}

/**********************************************************************/
/* free_dt2 : libera la memoria allocata a nodi di sezioni extended   */
/**********************************************************************/
extern void free_dt2( rel_sec2 * p_node) {
  rel_sec2 * p_next;
  while ( p_node ) {
//    printf("% 8s rec %d libero nodo %p, dt2 %d, prossimo %p\n", "Free", p_node->rec_num, p_node, p_node->dt2, p_node->next);
    p_next = (rel_sec2 *) p_node->next;
    free(p_node);
    p_node = (rel_sec2 *) p_next;
  }
  return;
}

