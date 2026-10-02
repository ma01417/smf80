#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

/* irrcdt.c - implementazione della pseudo-classe "_CDT" (RACROUTE
 * REQUEST=STAT, racstat.h): legge voci della Class Descriptor Table
 * (statica + dinamica) e le presenta come una RacseqProfile con un unico
 * segmento "CDT", cosi' che print_profile()/print_segment()/print_field()
 * in testrseq.c (e qualunque altro consumatore di RacseqProfile)
 * funzionino invariati anche per questa pseudo-classe. Vedi irrcdt.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "irrcdt.h"
#include "racstat.h"

int irrcdt_is_pseudo_class(const char *class_name)
{
    return class_name != NULL && strcmp(class_name, "_CDT") == 0;
}

#pragma pack(1)
typedef struct {
    /* ---- CNST (28 byte) ---- */
    unsigned short cnst_len;          /* @0x00 CNSTLGT - 0 = nessuna voce
                                        * (fine scansione per NEXT=, non
                                        * trovata per CLASS=) */
    unsigned char  cnst_id;           /* @0x02 CNSTID */
    char           cnst_name[8];      /* @0x03 CNSTNAME */
    char           cnst_xref[8];      /* @0x0B CNSTXREF */
    unsigned char  cnst_maxl;         /* @0x13 CNSTMAXL */
    unsigned char  cnst_frst;         /* @0x14 CNSTFRST */
    unsigned char  cnst_remn;         /* @0x15 CNSTREMN */
    unsigned char  cnst_uacc;         /* @0x16 CNSTUACC */
    unsigned char  cnst_mflg;         /* @0x17 CNSTMFLG */
    unsigned char  cnst_cnsx_ptr[4];  /* @0x18 CNSTCNSX - puntatore RACF-owned,
                                        * NON dereferenziato (vedi build_cdt_profile) */
    /* == CNST: 0x1C = 28 byte == */

    /* ---- CNSX (152 byte) ---- */
    unsigned char  cnst_mask[4];      /* @+0x00 CNSTMASK - primi 4 byte della
                                        * vista piu' ampia (CNSTLMSK 16B /
                                        * CNSTVMSK 128B, stesso campo per ORG
                                        * nel DSECT) - bit non documentati nei
                                        * due listing disponibili, non
                                        * decodificati oltre questi 4 byte */
    unsigned char  reserved_mask[124];/* @+0x04..0x7F resto della regione CNSTVMSK */
    unsigned char  cnst_gnlp[4];      /* @+0x80 CNSTGNLP - puntatore RACF-owned,
                                        * NON dereferenziato */
    unsigned char  cnst_rclp[4];      /* @+0x84 CNSTRCLP - puntatore RACF-owned,
                                        * NON dereferenziato */
    unsigned char  cnst_dfrc;         /* @+0x88 CNSTDFRC */
    unsigned char  cnst_flg0;         /* @+0x89 CNSTFLG0 */
    unsigned char  cnst_keyq;         /* @+0x8A CNSTKEYQ */
    unsigned char  cnst_orml;         /* @+0x8B CNSTORML */
    unsigned char  cnst_flg1;         /* @+0x8C CNSTFLG1 */
    unsigned char  reserved_8d[3];    /* @+0x8D..0x8F */
    char           cnst_stkn[8];      /* @+0x90 CNSTSTKN */
    /* == CNSX: 0x98 = 152 byte == */
} RacstatCdtEntry;
#pragma pack(reset)

#define CDT_TEXT_POOL_SIZE 512   /* margine ampio per i campi formattati sotto - vedi build_cdt_profile */
#define CDT_MAX_FIELDS     32

/* Copia `text` nel pool appeso dopo i RACSTAT_ENTRY_LEN byte grezzi in
 * raw_buffer (vedi irrcdt_extract/irrcdt_extract_next), cosi' che il
 * puntatore risultante resti valido fino a racseq_free_profile() come
 * tutti gli altri RacseqField.value in questa libreria (che puntano
 * dentro raw_buffer, mai in memoria posseduta a parte). */
static char *pool_alloc_text(char **pool_ptr, size_t *pool_left, const char *text)
{
    size_t len = strlen(text) + 1;
    char  *dst = *pool_ptr;

    if (len > *pool_left) len = *pool_left;   /* non dovrebbe mai succedere, vedi CDT_TEXT_POOL_SIZE */
    memcpy(dst, text, len);
    if (len > 0) dst[len - 1] = '\0';
    *pool_ptr   += len;
    *pool_left  -= len;
    return dst;
}

static void add_text_field(RacseqEntry *entries, int *count, char **pool, size_t *pool_left,
                            const char *name, const char *text)
{
    RacseqEntry *e = &entries[(*count)++];
    char        *stored = pool_alloc_text(pool, pool_left, text);

    e->kind = RACSEQ_ENTRY_FIELD;
    strncpy(e->field.name, name, 8);
    e->field.name[8] = '\0';
    e->field.is_boolean = 0;
    e->field.value = stored;
    e->field.value_len = (int)strlen(stored);
}

static void add_bool_field(RacseqEntry *entries, int *count, const char *name, int value)
{
    RacseqEntry *e = &entries[(*count)++];

    e->kind = RACSEQ_ENTRY_FIELD;
    strncpy(e->field.name, name, 8);
    e->field.name[8] = '\0';
    e->field.is_boolean = 1;
    e->field.bool_value = value ? 1 : 0;
    e->field.value = NULL;
    e->field.value_len = 0;
}

/* Stato ACTIVE/INACTIVE della classe: NON e' un campo di CNST/CNSX (la CDT
 * descrive come e' definita una classe, non se e' attiva - verificato su
 * cdtncdtx.h, le DSECT EDCDSECT di struct cnst/cnsx non hanno alcun bit del
 * genere), ma si ricava dagli stessi SAF RC/RACF RC/RACF Reason che RACROUTE
 * REQUEST=STAT restituisce per la chiamata CLASS=/NEXT= gia' in uso - da
 * manuale IBM (tabella fornita dall'utente in noteclaude.txt):
 *   SAF=00 RACF=00          -> classe attiva
 *   SAF=04 RACF=00 RS=00    -> "No security decision could be made"
 *   SAF=04 RACF=04          -> classe inattiva
 *   SAF=04 RACF=08          -> classe non definita (CLASS=) / fine CDT (NEXT=)
 *                               - non dovrebbe arrivare qui: irrcdt_extract()/
 *                               irrcdt_extract_next() filtrano gia' su
 *                               CNSTLGT==0 prima di chiamare questa funzione
 *
 * Confronto dal vivo il 2026-08-06 (noteclaude.txt) contro l'elenco classi
 * attive di SETROPTS LIST su ~260 classi: ogni classe con SAF=00/RACF=00 e'
 * risultata "Active" in SETROPTS, ogni classe con SAF=04/RACF=00/RS=00 ("No
 * security decision could be made") NON compariva nell'elenco - quindi in
 * pratica, per lo scopo di questo campo, quel caso equivale a "non attiva"
 * e non merita una terza categoria distinta: su richiesta dell'utente,
 * ridotto a YES/NO invece di ACTIVE/INACTIVE/UNKNOWN. */
static const char *cdt_active_text(const RacauthStatus *ast)
{
    if (ast->safrc == 0 && ast->racfrc == 0) return "YES";
    return "NO";
}

/* FIRSTCH/RESTCH (CNSTFRST/CNSTREMN): bitmask dei caratteri ammessi
 * rispettivamente all'inizio e nel resto del nome profilo - stessi 4 bit
 * alti per entrambi i campi (cnstfalp/cnstfnat/cnstfnum/cnstfspe per FIRSTCH,
 * cnstralp/cnstrnat/cnstrnum/cnstrspe per RESTCH in cdtncdtx.h, valori
 * identici: 0x80/0x40/0x20/0x10). Decodifica ad abbreviazioni su richiesta
 * dell'utente (noteclaude.txt) invece del valore esadecimale grezzo. */
static const char *cdt_charset_text(unsigned char v, char *buf, size_t buflen)
{
    static const struct { unsigned char mask; const char *label; } bits[] = {
        { 0x80, "Alfa" }, { 0x40, "Nat." }, { 0x20, "Num." }, { 0x10, "Spec" }
    };
    size_t i;
    int first = 1;

    buf[0] = '\0';
    for (i = 0; i < sizeof(bits) / sizeof(bits[0]); i++) {
        if (v & bits[i].mask) {
            if (!first) strncat(buf, "+", buflen - strlen(buf) - 1);
            strncat(buf, bits[i].label, buflen - strlen(buf) - 1);
            first = 0;
        }
    }
    if (first) strncat(buf, "-", buflen - strlen(buf) - 1);   /* nessun bit acceso */
    return buf;
}

/* raw_buffer: RACSTAT_ENTRY_LEN byte grezzi (CNST+CNSX, gia' popolati da
 * r_admin_stat_class) seguiti da CDT_TEXT_POOL_SIZE byte di scratch per i
 * campi formattati qui sotto - allocati insieme da irrcdt_extract()/
 * irrcdt_extract_next() in un'unica malloc(), cosi' che racseq_free_profile()
 * debba fare un solo free() (RACSEQ_RAW_MALLOC). ast: SAF/RACF RC/RS della
 * stessa chiamata STAT che ha popolato raw_buffer - usato solo per il campo
 * sintetico ACTIVE (vedi cdt_active_text), non fa parte del buffer CNST/CNSX. */
static void build_cdt_profile(void *raw_buffer, const RacauthStatus *ast, RacseqProfile *out)
{
    RacstatCdtEntry *e = (RacstatCdtEntry *)raw_buffer;
    char   *pool = (char *)raw_buffer + RACSTAT_ENTRY_LEN;
    size_t  pool_left = CDT_TEXT_POOL_SIZE;
    RacseqEntry *entries;
    int     count = 0;
    int     namelen;
    char    buf[32];
    const char *uacc_text;

    memset(out, 0, sizeof(*out));
    memcpy(out->class_name, "_CDT", 4);
    out->class_name[4] = '\0';

    namelen = (int)sizeof(e->cnst_name);
    while (namelen > 0 && e->cnst_name[namelen - 1] == ' ') namelen--;
    memcpy(out->profile_name, e->cnst_name, (size_t)namelen);
    out->profile_name[namelen] = '\0';

    out->raw_buffer = raw_buffer;
    out->raw_owner  = RACSEQ_RAW_MALLOC;
    out->num_segments = 1;
    out->segments = (RacseqSegment *)calloc(1, sizeof(RacseqSegment));
    memcpy(out->segments[0].name, "CDT", 3);
    out->segments[0].name[3] = '\0';
    entries = (RacseqEntry *)calloc(CDT_MAX_FIELDS, sizeof(RacseqEntry));
    out->segments[0].entries = entries;

    /* ---- stato attiva/inattiva (non fa parte di CNST/CNSX, vedi sopra) ---- */
    add_text_field(entries, &count, &pool, &pool_left, "ACTIVE", cdt_active_text(ast));

    /* ---- CNST ---- */
    snprintf(buf, sizeof(buf), "%u", (unsigned)e->cnst_id);
    add_text_field(entries, &count, &pool, &pool_left, "ID", buf);

    namelen = (int)sizeof(e->cnst_xref);
    while (namelen > 0 && e->cnst_xref[namelen - 1] == ' ') namelen--;
    memcpy(buf, e->cnst_xref, (size_t)namelen);
    buf[namelen] = '\0';
    add_text_field(entries, &count, &pool, &pool_left, "XREF", buf);

    snprintf(buf, sizeof(buf), "%u", (unsigned)e->cnst_maxl);
    add_text_field(entries, &count, &pool, &pool_left, "MAXLEN", buf);

    add_text_field(entries, &count, &pool, &pool_left, "FIRSTCH",
                    cdt_charset_text(e->cnst_frst, buf, sizeof(buf)));
    add_text_field(entries, &count, &pool, &pool_left, "RESTCH",
                    cdt_charset_text(e->cnst_remn, buf, sizeof(buf)));

    /* CNSTUACC e' un campo a bit (ALTR/CNTL/UPDT/READ/EXEC/NONE) - qui
     * ridotto al livello di accesso piu' alto presente, stessa
     * convenzione con cui RACF normalmente descrive uno UACC. */
    uacc_text = "NONE";
    if      (e->cnst_uacc & 0x80) uacc_text = "ALTER";
    else if (e->cnst_uacc & 0x40) uacc_text = "CONTROL";
    else if (e->cnst_uacc & 0x20) uacc_text = "UPDATE";
    else if (e->cnst_uacc & 0x10) uacc_text = "READ";
    else if (e->cnst_uacc & 0x08) uacc_text = "EXECUTE";
    add_text_field(entries, &count, &pool, &pool_left, "UACC", uacc_text);

    add_bool_field(entries, &count, "RESGROUP", e->cnst_mflg & 0x80); /* CNSTRGRP: 1 => la classe e' una resource group class
                                                                        * (collegata alla member class dal campo XREF sopra) */
    add_bool_field(entries, &count, "OPERATTR", e->cnst_mflg & 0x20); /* CNSTOPER: 1 => la classe onora l'attributo utente
                                                                        * OPERATIONS - chi lo possiede ha accesso massimo a
                                                                        * tutte le risorse della classe salvo negazione esplicita */
    add_bool_field(entries, &count, "RACLIST",  e->cnst_mflg & 0x10); /* CNSTRACL */
    add_bool_field(entries, &count, "GENLIST",  e->cnst_mflg & 0x08); /* CNSTGENL */
    add_bool_field(entries, &count, "DATASPC",  e->cnst_mflg & 0x04); /* CNSTDSPC */
    add_bool_field(entries, &count, "USRINST",  e->cnst_mflg & 0x01); /* CNSTOWNR: 1=installata dall'utente, 0=IBM */

    /* ---- CNSX ---- */
    snprintf(buf, sizeof(buf), "X'%02X%02X%02X%02X'",
             (unsigned)e->cnst_mask[0], (unsigned)e->cnst_mask[1],
             (unsigned)e->cnst_mask[2], (unsigned)e->cnst_mask[3]);
    add_text_field(entries, &count, &pool, &pool_left, "OPTMASK", buf);

    {
        unsigned int gnlp, rclp;
        memcpy(&gnlp, e->cnst_gnlp, sizeof(gnlp));
        memcpy(&rclp, e->cnst_rclp, sizeof(rclp));
        snprintf(buf, sizeof(buf), "X'%08X'", gnlp);
        add_text_field(entries, &count, &pool, &pool_left, "GENLISTP", buf);
        snprintf(buf, sizeof(buf), "X'%08X'", rclp);
        add_text_field(entries, &count, &pool, &pool_left, "RACLISTP", buf);
    }

    snprintf(buf, sizeof(buf), "%u", (unsigned)e->cnst_dfrc);
    add_text_field(entries, &count, &pool, &pool_left, "DEFRC", buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)e->cnst_keyq);
    add_text_field(entries, &count, &pool, &pool_left, "KEYQUAL", buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)e->cnst_orml);
    add_text_field(entries, &count, &pool, &pool_left, "ORIGMAXL", buf);

    add_bool_field(entries, &count, "MUSTRACL", e->cnst_flg0 & 0x80); /* CNSTRLRQ */
    add_bool_field(entries, &count, "NOPROF",   e->cnst_flg0 & 0x40); /* CNSTPRDF: 0=profili permessi, quindi 1=NON permessi */
    add_bool_field(entries, &count, "SECLABEL", e->cnst_flg0 & 0x20); /* CNSTUSLB */
    add_bool_field(entries, &count, "REVMAC",   e->cnst_flg0 & 0x10); /* CNSTRMAC */
    add_bool_field(entries, &count, "PRESCASE", e->cnst_flg0 & 0x04); /* CNSTCASE */
    add_bool_field(entries, &count, "EQMAC",    e->cnst_flg0 & 0x02); /* CNSTEMAC */

    add_bool_field(entries, &count, "DYNAMIC",  e->cnst_flg1 & 0x80); /* CNSTDYN */
    add_bool_field(entries, &count, "DUPSTAT",  e->cnst_flg1 & 0x40); /* CNSTDDUP */
    add_bool_field(entries, &count, "ISCOPY",   e->cnst_flg1 & 0x20); /* CNSTCOPY */
    add_bool_field(entries, &count, "NOGENER",  e->cnst_flg1 & 0x10); /* CNSTNGEN */

    {
        char hex[17];
        int  i;
        for (i = 0; i < 8; i++)
            snprintf(hex + i * 2, 3, "%02X", (unsigned char)e->cnst_stkn[i]);
        add_text_field(entries, &count, &pool, &pool_left, "STOKEN", hex);
    }

    out->segments[0].num_entries = count;
}

extern RacseqRC irrcdt_extract(const char *class_name_arg, RacseqProfile *out, RacseqStatus *status)
{
    unsigned char cls[8];
    void *raw_buffer;
    RacauthStatus ast;
    size_t clen;
    int rc;
    RacstatCdtEntry *entry;

    memset(out, 0, sizeof(*out));

    memset(cls, ' ', sizeof(cls));
    clen = class_name_arg ? strlen(class_name_arg) : 0;
    if (clen > sizeof(cls)) clen = sizeof(cls);
    if (class_name_arg) memcpy(cls, class_name_arg, clen);

    raw_buffer = malloc(RACSTAT_ENTRY_LEN + CDT_TEXT_POOL_SIZE);
    if (!raw_buffer) {
        status->safrc = status->racfrc = status->racfrs = 4;
        return RACSEQ_ERROR;
    }

    /* Il SAF RC grezzo NON e' usato per decidere l'esito: confermato dal
     * vivo il 2026-08-04 - una scansione _CDT restituiva SAFRC=4/RACFRC=0/
     * RACFRS=0 su voci CDT perfettamente valide (nomi/campi corretti),
     * interrompendo la scansione prematuramente quando questo codice
     * trattava qualunque SAF RC diverso da zero come errore fatale PRIMA
     * di guardare CNSTLGT. statncdt.lst (l'unico dei due listing con un
     * ciclo funzionante) non controlla MAI il SAF RC: si basa solo su
     * CNSTLGT (vedi sotto) - stessa disciplina qui, per entrambe le
     * modalita' CLASS=/NEXT=. status viene comunque riempito per
     * diagnostica, anche se non guida piu' il ramo. */
    rc = r_admin_stat_class(cls, NULL, (unsigned char *)raw_buffer, &ast);
    (void)rc;
    status->safrc = ast.safrc; status->racfrc = ast.racfrc; status->racfrs = ast.racfrs;

    entry = (RacstatCdtEntry *)raw_buffer;
    if (entry->cnst_len == 0) { free(raw_buffer); return RACSEQ_ERROR; /* classe non trovata nella CDT */ }

    build_cdt_profile(raw_buffer, &ast, out);
    return RACSEQ_OK;
}

extern RacseqRC irrcdt_extract_next(const char *start_after_name, RacseqProfile *previous,
                              RacseqProfile *out, RacseqStatus *status)
{
    unsigned char next_name[8];
    void *raw_buffer;
    RacauthStatus ast;
    int rc;
    RacstatCdtEntry *entry;
    size_t clen;

    memset(next_name, ' ', sizeof(next_name));
    if (previous) {
        /* Cattura il nome PRIMA di liberare previous - valido anche se
         * previous == out (pattern normale di scansione), dato che
         * profile_name e' dentro la struct stessa, non in raw_buffer. */
        clen = strlen(previous->profile_name);
        if (clen > sizeof(next_name)) clen = sizeof(next_name);
        memcpy(next_name, previous->profile_name, clen);
        racseq_free_profile(previous);
    } else if (start_after_name) {
        clen = strlen(start_after_name);
        if (clen > sizeof(next_name)) clen = sizeof(next_name);
        memcpy(next_name, start_after_name, clen);
    }

    memset(out, 0, sizeof(*out));

    raw_buffer = malloc(RACSTAT_ENTRY_LEN + CDT_TEXT_POOL_SIZE);
    if (!raw_buffer) {
        status->safrc = status->racfrc = status->racfrs = 4;
        return RACSEQ_ERROR;
    }

    /* Vedi il commento in irrcdt_extract() - il SAF RC grezzo non decide
     * piu' l'esito, solo CNSTLGT sotto (confermato dal vivo il 2026-08-04). */
    rc = r_admin_stat_class(NULL, next_name, (unsigned char *)raw_buffer, &ast);
    (void)rc;
    status->safrc = ast.safrc; status->racfrc = ast.racfrc; status->racfrs = ast.racfrs;

    entry = (RacstatCdtEntry *)raw_buffer;
    if (entry->cnst_len == 0) { free(raw_buffer); return RACSEQ_NO_MORE; /* fine scansione CDT */ }

    build_cdt_profile(raw_buffer, &ast, out);
    return RACSEQ_OK;
}
