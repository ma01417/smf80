/* ========================================================================== */
/*                                                                            */
/*   getdsn.h                                                                 */
/*   (c) 2026 A.Brezzi                                                        */
/*                                                                            */
/*   Description: lista dei data set allocati a una DD tramite SVC 99         */
/*                information retrieval (verbo X'07')                         */
/*                                                                            */
/* ========================================================================== */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#ifndef GETDSN_H
#define GETDSN_H

#include <stdio.h>

/* tipo di allocazione, ricavato da DINRTTYP e dai campi restituiti */
#define DD_DSN     0          /* data set                                 */
#define DD_DUMMY   1          /* DD DUMMY (o DSN=NULLFILE)                */
#define DD_SYSOUT  2          /* SYSOUT                                   */
#define DD_SYSIN   3          /* SYSIN (dati in stream)                   */
#define DD_TERM    4          /* terminale TSO                            */
#define DD_PATH    5          /* file z/OS UNIX (PATH=)                   */

/* un nodo per ogni data set allocato alla DD, nell'ordine della          */
/* concatenazione                                                         */
typedef struct st_dd_info {
  char           ddname[9];   /* DD richiesta                             */
  int            relno;       /* numero relativo di allocazione (SVC 99)  */
  int            conc_pos;    /* posizione nella concatenazione (1..n)    */
  int            dd_type;     /* DD_DSN, DD_DUMMY, ...                    */
  char           dsname[45];  /* DINRTDSN                                 */
  char           member[9];   /* DINRTMEM                                 */
  char           volser[7];   /* DINRTVOL, primo volume                   */
  char           path[256];   /* DINRPATH, solo per DD_PATH               */
  unsigned char  status;      /* DINRTSTA, valore grezzo                  */
  unsigned char  ndisp;       /* DINRTNDP, valore grezzo                  */
  unsigned char  cdisp;       /* DINRTCDP, valore grezzo                  */
  unsigned short dsorg;       /* DINRTORG, valore grezzo                  */
  unsigned char  attr;        /* DINRTATT, valore grezzo                  */
  unsigned char  rawtype;     /* DINRTTYP, valore grezzo                  */
  unsigned char  last;        /* DINRTLST, valore grezzo                  */
  unsigned short s99error;    /* S99ERROR dell'ultima richiesta           */
  unsigned short s99info;     /* S99INFO dell'ultima richiesta            */
  struct st_dd_info *next;
} st_dd_info;

/* lista dei data set allocati a ddname; NULL se la DD non e' allocata    */
extern st_dd_info *get_DSName(const char *ddname);
/* 1 se la DD e' allocata DUMMY, 0 altrimenti (anche se non allocata)     */
extern int         is_dd_dummy(const st_dd_info *list);
/* stampa la lista nel formato del riepilogo di esecuzione                */
extern void        print_DSName(FILE *f, const st_dd_info *list);
/* libera la lista restituita da get_DSName                               */
extern void        free_DSName(st_dd_info *list);
/* stampa tutte le allocazioni del job step con i valori grezzi (test)    */
extern void        dump_allocations(FILE *f);

#endif /* GETDSN_H */
