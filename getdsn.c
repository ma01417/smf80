/* ========================================================================== */
/*                                                                            */
/*   getdsn.c                                                                 */
/*   (c) 2026 A.Brezzi                                                        */
/*                                                                            */
/*   Description: lista dei data set allocati a una DD tramite SVC 99         */
/*                information retrieval (verbo X'07'), richiamata con         */
/*                svc99() del C runtime: problem state, nessuna               */
/*                autorizzazione APF richiesta                                */
/*                                                                            */
/*   Le chiavi dei text unit e il layout di S99RB/S99TUNIT sono presi dal     */
/*   listato reale delle macro IEFZB4D0/IEFZB4D2 (svc99map.lst).              */
/*   I valori restituiti (tipo, stato, disposizione, DSORG) non sono in       */
/*   quelle macro: le decodifiche sotto sono state verificate con il          */
/*   programma di prova tstdsn (job del 04/10/2026).                          */
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

#include "getdsn.h"

/* -- verbo e chiavi da IEFZB4D0/IEFZB4D2 (svc99map.lst) ------------------- */
#define S99VRBIN  0x07        /* INFORMATION RETRIEVAL                    */
#define DINRTDDN  0x0004      /* RETURN DDNAME                            */
#define DINRTDSN  0x0005      /* RETURN DSNAME                            */
#define DINRTMEM  0x0006      /* RETURN MEMBER NAME                       */
#define DINRTSTA  0x0007      /* RETURN DATA SET STATUS                   */
#define DINRTNDP  0x0008      /* RETURN NORMAL DISPOSITION                */
#define DINRTCDP  0x0009      /* RETURN CONDITIONAL DISP                  */
#define DINRTORG  0x000A      /* RETURN D.S. ORGANIZATION                 */
#define DINRTATT  0x000C      /* RETURN DYN. ALLOC ATTRIBUTES             */
#define DINRTLST  0x000D      /* RETURN LAST ENTRY INDICATION             */
#define DINRTTYP  0x000E      /* RETURN S.D. TYPE INDICATION              */
#define DINRELNO  0x000F      /* RELATIVE REQUEST NUMBER                  */
#define DINRTVOL  0x0010      /* Return First Volser                      */
#define DINRPATH  0xC017      /* PATH (chiave JDT, IEFSJDKY)              */

#define S99TUPLN  0x80000000u /* ultimo puntatore della lista text unit   */

/* -- valori restituiti: verificati con tstdsn salvo TERM e SYSIN --------- */
#define TYP_DUMMY   0x80
#define TYP_TERM    0x40
#define TYP_SYSIN   0x20
#define TYP_SYSOUT  0x10
#define LST_LAST    0x80
/* DSNAME restituito per le DD PATH= (verificato con tstdsn) */
#define DSN_PATH    "...PATH=.SPECIFIED..."

#define S99_MAX_RELNO 9999    /* limite di sicurezza della scansione      */
#define TU_MAX        12      /* text unit per singola richiesta          */

/* S99RB: request block, 20 byte (DSECT S99RB, svc99map.lst) */
typedef struct {
  unsigned char  rbln;        /* +00 S99RBLN  lunghezza = 20              */
  unsigned char  verb;        /* +01 S99VERB                              */
  unsigned short flag1;       /* +02 S99FLAG1                             */
  unsigned short error;       /* +04 S99ERROR codice di errore            */
  unsigned short info;        /* +06 S99INFO  codice informativo          */
  void          *txtpp;       /* +08 S99TXTPP lista puntatori text unit   */
  void          *s99x;        /* +0C S99S99X  estensione (non usata)      */
  unsigned int   flag2;       /* +10 S99FLAG2 solo funzioni autorizzate   */
} s99_rb;

/* S99TUNIT con un solo parametro: KEY, NUM, LEN, PAR (DSECT S99TUNIT) */
typedef struct {
  unsigned short key;
  unsigned short num;
  unsigned short len;
  char           par[256];
} s99_tu;

/* richiesta corrente: text unit e lista dei puntatori */
typedef struct {
  s99_tu tu[TU_MAX];
  void  *ptr[TU_MAX];
  int    count;
  unsigned short error;
  unsigned short info;
} s99_req;

static void   req_init(s99_req *r);
static s99_tu *req_add(s99_req *r, unsigned short key, unsigned short len);
static int    req_exec(s99_req *r);
static void   tu_text(const s99_tu *t, char *buf, size_t size);
static unsigned char tu_byte(const s99_tu *t);
static int    info_relno(int relno, st_dd_info *e);
static int    info_path(int relno, st_dd_info *e);
static const char *type_text(int t);
static const char *status_text(unsigned char s);
static const char *disp_text(unsigned char d);
static const char *dsorg_text(unsigned short o, char *buf);


static void req_init(s99_req *r)
{
  memset(r, 0, sizeof *r);
}

/* aggiunge un text unit con un parametro di len byte (input o output) */
static s99_tu *req_add(s99_req *r, unsigned short key, unsigned short len)
{
  s99_tu *t = &r->tu[r->count];
  t->key = key;
  t->num = 1;
  t->len = len;
  r->ptr[r->count] = t;
  r->count++;
  return t;
}

/* esegue SVC 99 information retrieval; 0 se ok */
static int req_exec(s99_req *r)
{
  s99_rb rb;
  int    rc;

  /* l'ultimo puntatore della lista ha il bit alto acceso (S99TUPLN) */
  r->ptr[r->count - 1] = (void *)((unsigned int)r->ptr[r->count - 1] | S99TUPLN);

  memset(&rb, 0, sizeof rb);
  rb.rbln  = sizeof rb;
  rb.verb  = S99VRBIN;
  rb.txtpp = r->ptr;

  rc = svc99((__S99parms *)&rb);
  r->error = rb.error;
  r->info  = rb.info;
  return rc;
}

/* copia il parametro restituito in buf, senza blank finali */
static void tu_text(const s99_tu *t, char *buf, size_t size)
{
  size_t n = t->len;
  if (n >= size) n = size - 1;
  memcpy(buf, t->par, n);
  while (n > 0 && buf[n - 1] == ' ') n--;
  buf[n] = '\0';
}

static unsigned char tu_byte(const s99_tu *t)
{
  return t->len > 0 ? (unsigned char)t->par[0] : 0;
}

/* informazioni sull'allocazione numero relno; 0 se ok */
static int info_relno(int relno, st_dd_info *e)
{
  s99_req r;
  s99_tu *t_rel, *t_ddn, *t_dsn, *t_mem, *t_sta, *t_ndp, *t_cdp;
  s99_tu *t_org, *t_att, *t_lst, *t_typ, *t_vol;
  unsigned short rn = (unsigned short)relno;

  req_init(&r);
  t_rel = req_add(&r, DINRELNO, 2);
  memcpy(t_rel->par, &rn, 2);
  t_ddn = req_add(&r, DINRTDDN, 8);
  t_dsn = req_add(&r, DINRTDSN, 44);
  t_mem = req_add(&r, DINRTMEM, 8);
  t_sta = req_add(&r, DINRTSTA, 1);
  t_ndp = req_add(&r, DINRTNDP, 1);
  t_cdp = req_add(&r, DINRTCDP, 1);
  t_org = req_add(&r, DINRTORG, 2);
  t_att = req_add(&r, DINRTATT, 1);
  t_lst = req_add(&r, DINRTLST, 1);
  t_typ = req_add(&r, DINRTTYP, 1);
  t_vol = req_add(&r, DINRTVOL, 6);

  memset(e, 0, sizeof *e);
  e->relno = relno;
  if (req_exec(&r) != 0) {
    e->s99error = r.error;
    e->s99info  = r.info;
    return -1;
  }

  tu_text(t_ddn, e->ddname, sizeof e->ddname);
  tu_text(t_dsn, e->dsname, sizeof e->dsname);
  tu_text(t_mem, e->member, sizeof e->member);
  tu_text(t_vol, e->volser, sizeof e->volser);
  e->status  = tu_byte(t_sta);
  e->ndisp   = tu_byte(t_ndp);
  e->cdisp   = tu_byte(t_cdp);
  e->attr    = tu_byte(t_att);
  e->last    = tu_byte(t_lst);
  e->rawtype = tu_byte(t_typ);
  if (t_org->len == 2)
    memcpy(&e->dsorg, t_org->par, 2);

  if (e->rawtype & TYP_DUMMY)       e->dd_type = DD_DUMMY;
  else if (e->rawtype & TYP_SYSOUT) e->dd_type = DD_SYSOUT;
  else if (e->rawtype & TYP_SYSIN)  e->dd_type = DD_SYSIN;
  else if (e->rawtype & TYP_TERM)   e->dd_type = DD_TERM;
  else if (strcmp(e->dsname, "NULLFILE") == 0) e->dd_type = DD_DUMMY;
  else if (strcmp(e->dsname, DSN_PATH) == 0) {
    e->dd_type = DD_PATH;
    if (info_path(relno, e) != 0 || e->path[0] == '\0')
      strcpy(e->path, e->dsname);   /* path non disponibile: resta l'indicatore */
  }
  else                              e->dd_type = DD_DSN;
  return 0;
}

/* path z/OS UNIX dell'allocazione relno (richiesta separata, perche'
 * la chiave JDT DINRPATH ha senso solo per le DD PATH=); 0 se ok */
static int info_path(int relno, st_dd_info *e)
{
  s99_req r;
  s99_tu *t_rel, *t_path;
  unsigned short rn = (unsigned short)relno;

  req_init(&r);
  t_rel = req_add(&r, DINRELNO, 2);
  memcpy(t_rel->par, &rn, 2);
  t_path = req_add(&r, DINRPATH, 255);
  if (req_exec(&r) != 0) return -1;
  tu_text(t_path, e->path, sizeof e->path);
  return 0;
}

/* -------------------------------------------------------------------------- */
/* lista dei data set allocati a ddname, nell'ordine della concatenazione.    */
/* Le allocazioni vengono scandite per numero relativo: i data set            */
/* concatenati seguono la DD con il DDNAME vuoto (verificato con tstdsn).     */
/* -------------------------------------------------------------------------- */
extern st_dd_info *get_DSName(const char *ddname)
{
  st_dd_info *head = NULL, *tail = NULL, *n;
  st_dd_info  e;
  char        want[9];
  int         relno, in_dd = 0, pos = 0;
  size_t      i;

  for (i = 0; i < 8 && ddname[i] && ddname[i] != ' '; i++)
    want[i] = (char)toupper((unsigned char)ddname[i]);
  want[i] = '\0';

  for (relno = 1; relno <= S99_MAX_RELNO; relno++) {
    if (info_relno(relno, &e) != 0) break;   /* nessuna altra allocazione */

    if (e.ddname[0] != '\0') {               /* inizio di una nuova DD    */
      if (strcmp(e.ddname, want) == 0)
        in_dd = 1;
      else if (in_dd)
        break;                               /* finita la DD cercata      */
    }
    if (in_dd) {
      n = (st_dd_info *)malloc(sizeof *n);
      if (n == NULL) break;
      *n = e;
      strcpy(n->ddname, want);
      n->conc_pos = ++pos;
      n->next = NULL;
      if (tail) tail->next = n; else head = n;
      tail = n;
    }
    if (e.last & LST_LAST) break;            /* ultima allocazione        */
  }
  return head;
}

extern int is_dd_dummy(const st_dd_info *list)
{
  return list != NULL && list->dd_type == DD_DUMMY;
}

extern void free_DSName(st_dd_info *list)
{
  st_dd_info *n;
  while (list) {
    n = list->next;
    free(list);
    list = n;
  }
}

/* -------------------------------------------------------------------------- */
/* decodifiche per la stampa                                                   */
/* -------------------------------------------------------------------------- */
static const char *type_text(int t)
{
  switch (t) {
    case DD_DUMMY:  return "DUMMY";
    case DD_SYSOUT: return "SYSOUT";
    case DD_SYSIN:  return "SYSIN";
    case DD_TERM:   return "TERM";
    case DD_PATH:   return "PATH";
    default:        return "DSN";
  }
}

static const char *status_text(unsigned char s)
{
  switch (s) {
    case 0x01: return "OLD";
    case 0x02: return "MOD";
    case 0x04: return "NEW";
    case 0x08: return "SHR";
    default:   return "?";
  }
}

static const char *disp_text(unsigned char d)
{
  switch (d) {
    case 0x01: return "UNCATLG";
    case 0x02: return "CATLG";
    case 0x04: return "DELETE";
    case 0x08: return "KEEP";
    case 0x10: return "PASS";
    default:   return "?";
  }
}

static const char *dsorg_text(unsigned short o, char *buf)
{
  if (o & 0x8000)      strcpy(buf, "IS");
  else if (o & 0x4000) strcpy(buf, "PS");
  else if (o & 0x2000) strcpy(buf, "DA");
  else if (o & 0x0200) strcpy(buf, "PO");
  else if (o & 0x0008) strcpy(buf, "VSAM");
  else if (o == 0)     strcpy(buf, "");
  else                 sprintf(buf, "%04X", o);
  return buf;
}

/* una riga per data set: DD, posizione, tipo, nome(membro) o path, stato */
extern void print_DSName(FILE *f, const st_dd_info *list)
{
  char name[300];
  char org[8];

  for (; list; list = list->next) {
    if (list->dd_type == DD_PATH)
      strcpy(name, list->path);
    else if (list->member[0])
      sprintf(name, "%s(%s)", list->dsname, list->member);
    else
      strcpy(name, list->dsname);

    fprintf(f, "%-8s %3d %-6s %-54s", list->conc_pos == 1 ? list->ddname : "",
            list->conc_pos, type_text(list->dd_type), name);
    if (list->dd_type == DD_DSN)
      fprintf(f, " %-3s %-7s %-4s %s", status_text(list->status),
              disp_text(list->ndisp), dsorg_text(list->dsorg, org), list->volser);
    fprintf(f, "\n");
  }
}

/* tutte le allocazioni del job step con i valori grezzi, per verificare  */
/* le decodifiche e il comportamento delle concatenazioni                  */
extern void dump_allocations(FILE *f)
{
  st_dd_info e;
  int relno;

  fprintf(f, "RELNO DDNAME   TYP STA NDP CDP ORG  ATT LST VOLSER DSNAME(MEMBER) / PATH\n");
  for (relno = 1; relno <= S99_MAX_RELNO; relno++) {
    if (info_relno(relno, &e) != 0) {
      fprintf(f, "%5d fine scansione: S99ERROR=%04X S99INFO=%04X\n",
              relno, e.s99error, e.s99info);
      break;
    }
    fprintf(f, "%5d %-8s  %02X  %02X  %02X  %02X %04X  %02X  %02X %-6s %s",
            e.relno, e.ddname, e.rawtype, e.status, e.ndisp, e.cdisp, e.dsorg,
            e.attr, e.last, e.volser, e.dd_type == DD_PATH ? e.path : e.dsname);
    if (e.member[0]) fprintf(f, "(%s)", e.member);
    fprintf(f, "\n");
    if (e.last & LST_LAST) break;
  }
}
