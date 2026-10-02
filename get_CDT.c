/* ========================================================================== */
/*                                                                            */
/*   get_CDT.c                                                                */
/*   (c) 2026 A.Brezzi                                                        */
/*                                                                            */
/*   Description: funzione per estrazione delle classi presenti nella CDT     */
/*                via irrcseq.c (racroute)                                    */
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
#include "irrcseq.h"
#include "irrcdt.h"

/*
   -- main line
*/
// per ottenere puntatore a campo nel buffer per nome
static const RacseqField *find_field(const RacseqSegment *s, const char *name);
// conversione campo stringa in valore long
static long field2long(const RacseqField *f, int *ok);
// funzione per conversione di stringa "YES/NO" o "TRUE/FALSE" in valore booleano
static int text2bool(const RacseqField *f, const char *yes_value);
// funzione per duplicare un campo testuale RacseqField in una stringa malloc'd
static char *dup_field_text(const RacseqField *f);
//funzione per popolare un elemento della lista delle classi
void push_cls(const char * name, int maxL, int act, int ope, int mix, char * RC,
              int typ, char * mem, char * desc);
// funzione per estrazione dei dati di una classe dopo la chiamata a RACROUTE
void extract_data(RacseqProfile *p);
/* funzioni per liberare la memoria utilizzata dai profili gia' consumati
 */
static void free_profile_parsed(RacseqProfile *p);
static void free_raw_buffer(void *raw_buffer, int raw_owner);
void racseq_free_profile(RacseqProfile *p);
/* funzione per liberare il buffer allocato via C metal: il prototipo va preso
 * da racfree.h, che porta #pragma linkage(OS) e #pragma map("RSQFREEM")
 * coerenti con la definizione in racfree_metal.c */
#include "racfree.h"

st_sm80_cls *root_cls = NULL;
st_sm80_cls *p_cls    = NULL;

extern st_sm80_cls *get_CDT( void ) {
    RacseqProfile p;
    RacseqStatus  st;
    RacseqRC      rc;
    int count = 0;
    char c_desc[51];                       // descrizione della classe

// crea la lista ed inserisce manualmente la classe DATASET
    strcpy(c_desc,"Attiva, LL prof:  44, upper, def RC:4, OPER");
    push_cls("DATASET", 44, 1, 1, 0, "4", 0, " ", c_desc);

// estrazione dei profili della classe _CDT : primo profilo
    memset(&p, 0, sizeof(p));
    rc = irrcdt_extract_next( NULL, NULL, &p, &st);

// estrazione dei profili successivi fino a RACSEQ_NO_MORE
    while (rc == RACSEQ_OK || rc == RACSEQ_GHOST_GENERIC) {
        count++;
        extract_data(&p); // popola la lista con i dati del profilo estratto

        /* previous e out sono la stessa variabile &p: irrcdt_extract_next()
         * la libera e la ripopola con il profilo successivo. */
        rc = irrcdt_extract_next(NULL, &p, &p, &st);
    }

  // fine estrazione dei profili della classe _CDT
    if (rc == RACSEQ_NO_MORE)
        printf("Fine scansione: %d profili trovati, nessun altro\n", count);
    else if (rc == RACSEQ_ERROR)
        fprintf(stderr, " Errore R_admin: SAFRC=%d RACFRC=%d RACFRS=%d\n",
                st.safrc, st.racfrc, st.racfrs);

    return root_cls; // restituisce puntatore alla lista delle classi estratte
}

void extract_data(RacseqProfile *p) {
  const RacseqSegment *s = &p->segments[0];  // _CDT ha un solo segmento "CDT"
  char c_name[9];             // nome della classe
  int  c_maxl = 0;            // lunghezza massima profilo
  int  c_act  = 0;            // classe attiva
  int  c_ope  = 0;            // onora OPERATIONS
  int  c_mix  = 0;            // mixed case
  int  c_typ  = 0;            // tipo di classe (0=normale, 1=grouping)
  char c_mem[9];              // nome della classe MEMBER se grouping class
  char c_desc[51];            // descrizione della classe
  char c_RC[8];               // RC per profilo non trovato
  char * fld;                 // pointer ai campi duplicati
  char d[] = "Elenco classi e proprieta'";
  char * f = alloca(40);

// nome della classe
  strncpy(c_name, p->profile_name,8);
  c_name[8] = '\0';

// classe attiva ?
  c_act = text2bool(find_field(s, "ACTIVE"), "YES");
  if ( c_act ) {
      strcpy(c_desc,"Attiva");
    }
    else {
      strcpy(c_desc,"W: Non attiva");
    }
// converte la massima lunghezza dei profili
 {
  const RacseqField *maxlen_f = find_field(s, "MAXLEN");
  int  ok;
  c_maxl = field2long(maxlen_f, &ok);
  if (!ok) {
    c_maxl = 0;
    }
 }
    strcat(c_desc,", LL prof:");
    sprintf(c_RC,"%4d",c_maxl);
    strcat(c_desc,c_RC);
// e' mixed case ?
 {
  const RacseqField *mix_f = find_field(s, "PRESCASE");
  c_mix = mix_f && mix_f->is_boolean ? mix_f->bool_value : 0;
  if (c_mix) {
    strcat(c_desc,", mixed");
    }
    else {
      strcat(c_desc,", upper");
    }
 }
// onora OPERATIONS ?
 {
  const RacseqField *ope_f = find_field(s, "OPERATTR");
  c_ope = ope_f && ope_f->is_boolean ? ope_f->bool_value : 0;
  if (c_ope) {
    strcat(c_desc,", OPER");
    }
 }
// converte il RC per profilo non trovato
  strcat(c_desc,", def RC:");
  strcat(c_desc,dup_field_text(find_field(s, "DEFRC")));

// e' grouping class ?
 {
  const RacseqField *typ_f = find_field(s, "RESGROUP");
  c_typ = typ_f && typ_f->is_boolean ? typ_f->bool_value : 0;
  if (c_typ) {       // se trattasi di grouping class estrae la classe member
    typ_f = find_field(s, "XREF");
    strcpy(c_mem,dup_field_text(find_field(s, "XREF")));
    strcat(c_desc,", mem: ");
    strcat(c_desc,c_mem);
    }
    else {
      c_mem[1] = '\0';
    }

// inserisce il nodo
    push_cls(c_name, c_maxl, c_act, c_ope, c_mix, c_RC, c_typ, c_mem, c_desc);
  }
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

/* find_field() - cerca un campo per nome all'interno di un segmento (solo
 * campi RACSEQ_ENTRY_FIELD, non repeat group). RacseqField non distingue
 * "numerico" da "testo" (vedi irrcseq.h) - questa e le funzioni sotto
 * mostrano il pattern da usare quando il PROGRAMMA CHIAMANTE sa gia', per
 * altra via (es. Tabella campi _CDT in irrcseq_specifiche.docx, colonna
 * "Tipo"), che un certo campo e' numerico e vuole trattarlo come tale.
 * Ritorna NULL se il campo non esiste nel segmento. */
static const RacseqField *find_field(const RacseqSegment *s, const char *name)
{
    int i;
    for (i = 0; i < s->num_entries; i++) {
        if (s->entries[i].kind == RACSEQ_ENTRY_FIELD &&
            strcmp(s->entries[i].field.name, name) == 0)
            return &s->entries[i].field;
    }
    return NULL;
}

/* field2long() - converte in intero un RacseqField "normale" (non
 * booleano) che il chiamante SA essere numerico. f->value NON e'
 * NUL-terminato (punta dentro raw_buffer, valido solo fino alla
 * racseq_free_profile() - vedi RacseqField in irrcseq.h): va sempre
 * copiato in un buffer locale prima di usare strtol()/atoi(), mai passato
 * direttamente. Ritorna 0 con *ok=0 se il campo e' NULL, booleano, troppo
 * lungo per il buffer locale, o non contiene cifre valide - il chiamante
 * decide come trattare l'errore, questa funzione non lo considera fatale. */
static long field2long(const RacseqField *f, int *ok)
{
    char  buf[32];
    char *end;
    long  v;

    *ok = 0;
    if (!f || f->is_boolean) return 0;
    if (f->value_len <= 0 || (size_t)f->value_len >= sizeof(buf)) return 0;

    memcpy(buf, f->value, (size_t)f->value_len);
    buf[f->value_len] = '\0';

    v = strtol(buf, &end, 10);
    if (end == buf) return 0;   /* nessuna cifra riconosciuta */

    *ok = 1;
    return v;
}

/* text2bool() - vero se il campo TESTUALE (non booleano) `f` vale
 * esattamente `yes_value`. Serve per ACTIVE, che e' testo "YES"/"NO" e
 * non un RacseqField booleano vero e proprio (cdt_active_text() in
 * irrcseq.c lo deriva da SAFRC/RACFRC, non da un bit di CNST/CNSX). */
static int text2bool(const RacseqField *f, const char *yes_value)
{
    if (!f || f->is_boolean) return 0;
    return f->value_len == (int)strlen(yes_value) &&
           memcmp(f->value, yes_value, (size_t)f->value_len) == 0;
}

/* dup_field_text() - copia un campo testuale in una stringa malloc'd e
 * NUL-terminata, posseduta dal CHIAMANTE. A differenza di
 * RacseqField.value (che punta dentro raw_buffer e diventa non valido
 * dopo racseq_free_profile()/dopo che racseq_extract_next() consuma
 * `previous`), i campi char* di st_sm80_cls (cls_member/cls_desc)
 * sopravvivono al profilo - vanno quindi COPIATI qui, mai referenziati
 * direttamente. Ritorna NULL se il campo non esiste o e' vuoto (XREF e'
 * spesso vuoto per le classi che non sono ne' grouping ne' member). */
static char *dup_field_text(const RacseqField *f)
{
    char *s;
    if (!f || f->is_boolean || f->value_len <= 0) return NULL;
    s = (char *)malloc((size_t)f->value_len + 1);
    memcpy(s, f->value, (size_t)f->value_len);
    s[f->value_len] = '\0';
    return s;
}

/* Libera solo segments/entries/occurrences/subfields - NON raw_buffer.
 * Serve isolata (non solo dentro racseq_free_profile) per il caso in cui
 * `previous` di racseq_extract_next() coincide con `out` (pattern comune:
 * riusare la stessa variabile per continuare una scansione): a quel punto
 * il raw_buffer di `previous` serve ancora come input alla chiamata
 * R_admin, quindi va liberato solo DOPO, esplicitamente, non qui insieme
 * al resto. */
static void free_profile_parsed(RacseqProfile *p)
{
    int s, e, o;

    if (!p) return;

    for (s = 0; s < p->num_segments; s++) {
        RacseqSegment *seg = &p->segments[s];
        for (e = 0; e < seg->num_entries; e++) {
            RacseqEntry *entry = &seg->entries[e];
            if (entry->kind == RACSEQ_ENTRY_REPEAT) {
                for (o = 0; o < entry->num_occurrences; o++)
                    free(entry->occurrences[o].subfields);
                free(entry->occurrences);
            }
        }
        free(seg->entries);
    }
    free(p->segments);
    p->segments = NULL;
    p->num_segments = 0;
}

static void free_raw_buffer(void *raw_buffer, int raw_owner)
{
    if (!raw_buffer) return;

    if (raw_owner == RACSEQ_RAW_MALLOC) {
        /* pseudo-classe "_CDT": raw_buffer e' un blocco malloc() di questa
         * libreria (RACROUTE REQUEST=STAT non alloca storage RACF - vedi
         * irrcdt.c), non storage R_admin. */
        free(raw_buffer);
        return;
    }

    {
        ADMN_PROF_MAP *hdr = (ADMN_PROF_MAP *)raw_buffer;
        unsigned int len = (unsigned int)hdr->out_len;
        unsigned int subpool = (unsigned int)hdr->spid;
        r_admin_freemain(raw_buffer, &len, &subpool);
    }
}

void racseq_free_profile(RacseqProfile *p)
{
    if (!p) return;
    free_profile_parsed(p);
    free_raw_buffer(p->raw_buffer, p->raw_owner);
    memset(p, 0, sizeof(*p));
}

