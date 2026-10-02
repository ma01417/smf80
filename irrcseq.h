#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef IRRCSEQ_H
#define IRRCSEQ_H

#include "racseq.h"
#include "racauth.h"   /* RACAUTH_ATTR_* per racseq_check_auth() */

/* ---------------------------------------------------------------------
 * irrcseq.h - libreria per EXTRACT/EXTRACTN di profili R_admin, pensata
 * per essere richiamata ripetutamente da un processo host di lunga
 * durata (non un CLI one-shot come racseq.c) - vedi la memoria di
 * progetto racseq_future_library_api. Per questo ogni funzione che
 * restituisce una RacseqProfile con successo (RACSEQ_OK/GHOST_GENERIC)
 * DEVE essere seguita da racseq_free_profile(), che libera sia la
 * memoria C (malloc/calloc) usata per la struttura sia lo storage
 * RACF-owned sottostante (via r_admin_freemain, se HAVE_RACF_METAL_HELPERS
 * e' definito - altrimenti e' un no-op documentato, stesso limite di
 * racseq.c senza gli helper Metal C collegati).
 *
 * Riusa gli stessi ADMN_PROF_MAP/ADMN_PROF_SEGDESC/ADMN_PROF_FIELDDESC
 * di racseq.h (offset confermati contro IRRPCOMP.asm reale - vedi i
 * commenti li'), quindi vale per USER/GROUP/CONNECT/DATASET/risorsa
 * generale: il formato del buffer di output di R_admin e' lo stesso per
 * tutte le classi, cambiano solo segmenti e campi presenti.
 * --------------------------------------------------------------------- */

typedef struct {
    char        name[9];       /* NUL-terminato */
    int         is_boolean;
    int         bool_value;    /* valido solo se is_boolean */
    const char *value;         /* punta dentro il raw_buffer di RacseqProfile - valido solo
                                 * finche' quella profile non e' stata passata a
                                 * racseq_free_profile(); NON e' NUL-terminato, usare value_len */
    int         value_len;
} RacseqField;

typedef struct {
    int          num_subfields;
    RacseqField *subfields;     /* num_subfields elementi */
} RacseqOccurrence;

typedef enum { RACSEQ_ENTRY_FIELD, RACSEQ_ENTRY_REPEAT } RacseqEntryKind;

typedef struct {
    RacseqEntryKind kind;

    /* kind == RACSEQ_ENTRY_FIELD */
    RacseqField field;

    /* kind == RACSEQ_ENTRY_REPEAT */
    char              repeat_name[9];
    int               dim;             /* subfields per occorrenza */
    int               num_occurrences;
    RacseqOccurrence *occurrences;      /* num_occurrences elementi */
} RacseqEntry;

typedef struct {
    char         name[9];
    int          num_entries;
    RacseqEntry *entries;       /* num_entries elementi */
} RacseqSegment;

/* Proprietario di raw_buffer, per sapere COME racseq_free_profile() deve
 * rilasciarlo - campo interno, non pensato per essere letto/impostato dal
 * chiamante. RACSEQ_RAW_RADMIN e' il default storico (memset a 0 lo
 * seleziona automaticamente): raw_buffer viene da R_admin/IRRSEQ00 e va
 * rilasciato con r_admin_freemain(). RACSEQ_RAW_MALLOC e' usato dalla
 * pseudo-classe "_CDT" (RACROUTE REQUEST=STAT, che non alloca storage RACF -
 * il chiamante fornisce il buffer): raw_buffer e' un blocco malloc() da
 * questa libreria e va rilasciato con free() semplice. */
#define RACSEQ_RAW_RADMIN 0
#define RACSEQ_RAW_MALLOC 1

typedef struct {
    char           class_name[9];
    char           profile_name[256];
    int            is_generic;         /* bit ADMN_PROF_GENERIC del profilo restituito -
                                         * sempre 0 per la pseudo-classe "_CDT" */
    int            is_ghost_generic;   /* R_admin ha segnalato un'anomalia "ghost generic"
                                         * (SAFRC/RACFRC/RACFRS 4/4/20): profilo restituito
                                         * comunque, ma probabile inconsistenza nel database RACF -
                                         * sempre 0 per la pseudo-classe "_CDT" */
    int            num_segments;
    RacseqSegment *segments;           /* num_segments elementi */
    void          *raw_buffer;         /* storage sottostante; NON toccare/free() direttamente -
                                         * rilasciato solo da racseq_free_profile() (vedi raw_owner) */
    int            raw_owner;          /* RACSEQ_RAW_* sopra - interno */
} RacseqProfile;

typedef struct {
    int safrc;
    int racfrc;
    int racfrs;
} RacseqStatus;

typedef enum {
    RACSEQ_OK             = 0,  /* profilo trovato, out popolato */
    RACSEQ_NO_MORE        = 1,  /* fine pulita della scansione EXTRACTN (SAFRC/RACFRC/RACFRS 4/4/4) */
    RACSEQ_GHOST_GENERIC  = 2,  /* profilo restituito ma "ghost generic" (4/4/20) - vedi is_ghost_generic */
    RACSEQ_ERROR          = 3,  /* errore reale R_admin - vedi *status */
    RACSEQ_UNSUPPORTED    = 4,  /* EXTRACTN richiesta per una classe che non la supporta (CONNECT) */
    RACSEQ_NOT_AUTHORIZED = 5   /* opts->supervisor richiesto ma il programma non e' APF-authorized
                                  * (r_admin_is_authorized() ha fallito) - nessuna chiamata IRRSEQ00
                                  * e' stata tentata */
} RacseqRC;

typedef struct {
    int         baseonly;
    int         nameonly;
    int         uppercase;
    int         generic;    /* ADMN_PROF_GENERIC */
    int         match;      /* ADMN_PROF_MATCHGN, solo DATASET */
    int         skipauth;   /* ADMN_PROF_SKIPAUTH, equivalente NOCMDAUTH */
    int         doauth;     /* ADMN_PROF_DOAUTH, equivalente facilityauth */
    const char *volume;     /* solo DATASET, fino a 6 caratteri, NULL se non serve */
    int         subpool;    /* 0-255; 0 = usa il default 127 */
    int         supervisor; /* 1 = esegui la chiamata IRRSEQ00 in stato supervisor (MODESET),
                              * stesso pattern di racseq.c call_radmin(): entra in supervisor
                              * state subito prima della singola chiamata IRRSEQ00 e ne esce
                              * subito dopo (non per l'intera durata del processo host).
                              * Richiede che il modulo sia in una libreria APF-authorized (o gia'
                              * in supervisor state) - se r_admin_is_authorized() fallisce, la
                              * funzione ritorna RACSEQ_NOT_AUTHORIZED senza tentare la chiamata.
                              * Questo NON verifica se l'utente corrente ha accesso a una qualche
                              * risorsa applicativa che regoli l'uso di --supervisor in questo
                              * shop - quel controllo (se serve) va fatto a parte con
                              * racseq_check_auth() PRIMA di impostare questo flag, la libreria
                              * non presume quale risorsa/classe usare per un gate del genere. */
} RacseqExtractOptions;

/* Verifica se l'utente corrente ha almeno l'accesso richiesto (RACAUTH_ATTR_*
 * - vedi racauth.h) a una risorsa RACF-protetta, via RACROUTE REQUEST=AUTH
 * (passato a FASTAUTH il 2026-08-03: FASTAUTH richiede che la classe sia
 * RACLISTed via SETROPTS RACLIST, non il caso di FACILITY sul sistema di
 * test, causando un falso "non autorizzato" - vedi racauth.h). Indipendente
 * dall'autorizzazione APF del programma (opts->supervisor sopra: quella
 * verifica se il PROGRAMMA puo' entrare in supervisor state, questa se
 * l'UTENTE ha accesso a una specifica risorsa) - tipicamente usata per un
 * gate applicativo prima di consentire l'uso di --supervisor (es. una
 * risorsa FACILITY dedicata definita da chi ospita questa libreria), ma la
 * risorsa/classe da controllare e' decisa dal chiamante, non da irrcseq.
 *
 * class_name segue le stesse convenzioni di racseq_extract(); entity_name
 * puo' essere lungo fino a 245 caratteri (viene allineato a sinistra e
 * riempito di spazi internamente, come richiesto da RACROUTE ENTITY=(reg) a
 * registro singolo - vedi racauth.h).
 *
 * Se status non e' NULL, viene riempito con SAF RC/RACF RC/RACF Reason (vedi
 * RacseqStatus sopra - stessi campi usati per lo status di IRRSEQ00) per
 * poter diagnosticare un diniego invece di sapere solo che e' avvenuto.
 *
 * Ritorna il return code SAF grezzo: 0 = autorizzato, valori diversi da 0
 * (4/8/12) = non autorizzato o impossibile verificare - vedi racauth.h.
 * Senza HAVE_RACF_METAL_HELPERS collegato, ritorna sempre "non autorizzato"
 * (fail-closed: non e' possibile verificare, quindi non si presume l'accesso),
 * e *status (se non NULL) viene azzerato. */
int racseq_check_auth(const char *entity_name, const char *class_name, unsigned char attr,
                       RacseqStatus *status);

/* EXTRACT: un profilo esatto per nome. class_name/profile_name seguono le
 * stesse convenzioni di racseq.c (--class/--profile, case-sensitive,
 * "userid.gruppo" per CONNECT). opts puo' essere NULL per usare tutti i
 * default (nessun flag, subpool 127).
 *
 * Pseudo-classe "_CDT" (2026-08-04): class_name == "_CDT" instrada verso
 * RACROUTE REQUEST=STAT (racstat.h) invece di R_admin/IRRSEQ00 - legge una
 * voce della Class Descriptor Table (sia statica che dinamica, a
 * differenza di --class=CDT che vede solo la Dynamic CDT). profile_name in
 * questo caso e' il nome della CLASSE RACF da cercare nella CDT (es.
 * "OPERCMDS", "FACILITY"), non un profilo nel senso consueto. opts viene
 * ignorato (RACROUTE REQUEST=STAT non richiede supervisor state ne'
 * supporta gli altri flag ADMN_PROF_*). Il profilo restituito ha un unico
 * segmento "CDT" con i campi della voce CNST/CNSX (vedi irrcseq.c
 * build_cdt_profile() per l'elenco) - raw_buffer in questo caso e' un
 * blocco malloc() (RACSEQ_RAW_MALLOC), non storage RACF-owned, ma
 * racseq_free_profile() lo gestisce comunque in modo trasparente. */
RacseqRC racseq_extract(const char *class_name, const char *profile_name,
                         const RacseqExtractOptions *opts,
                         RacseqProfile *out, RacseqStatus *status);

/* EXTRACTN: il profilo successivo nella scansione della classe.
 *
 * Per iniziare una scansione: previous = NULL, start_after_name = NULL
 * (dall'inizio della classe) oppure un nome da cui partire.
 *
 * Per proseguire una scansione gia' iniziata: previous = l'ultima
 * RacseqProfile ottenuta da una chiamata precedente a questa stessa
 * funzione (non da racseq_extract - EXTRACT ed EXTRACTN non si concatenano).
 * start_after_name viene ignorato in questo caso. La funzione consuma
 * `previous`: sia in caso di successo sia di errore, il suo raw_buffer
 * viene passato a R_admin come input e poi rilasciato automaticamente
 * (equivalente a una racseq_free_profile(previous) interna) - non
 * chiamare racseq_free_profile() su `previous` a parte, e non riusarlo
 * dopo questa chiamata. `previous` e `out` POSSONO essere la stessa
 * variabile (pattern normale per un ciclo di scansione: `RacseqProfile p;
 * ... racseq_extract_next(cls, NULL, &p, opts, &p, &st);` per continuare
 * da dove p si trovava) - questo caso e' gestito correttamente.
 *
 * CONNECT non supporta EXTRACTN (R_admin non ha un funcode "next" per
 * questa classe, vedi ADMN_XTR_CONNECT in racseq.h): ritorna
 * RACSEQ_UNSUPPORTED senza consumare `previous`.
 *
 * Pseudo-classe "_CDT" (2026-08-04): scansiona l'intera CDT (statica +
 * dinamica) via RACROUTE REQUEST=STAT NEXT=, stesso instradamento e stesse
 * avvertenze di racseq_extract() sopra - qui start_after_name/il nome
 * dentro `previous` e' il nome dell'ultima CLASSE restituita, non un
 * profilo. Fine scansione (RACSEQ_NO_MORE) quando RACF segnala nessuna
 * voce successiva (CNSTLGT a zero nel buffer restituito - non ancora
 * confrontato con SAF RC/RACF RC osservati dal vivo, verificare al primo
 * test). */
RacseqRC racseq_extract_next(const char *class_name,
                              const char *start_after_name,
                              RacseqProfile *previous,
                              const RacseqExtractOptions *opts,
                              RacseqProfile *out, RacseqStatus *status);

/* Libera sia le strutture C (segments/entries/occurrences/subfields)
 * sia lo storage RACF-owned (raw_buffer, via r_admin_freemain). Sicura
 * da chiamare su una RacseqProfile azzerata (es. dopo un RACSEQ_ERROR/
 * RACSEQ_NO_MORE/RACSEQ_UNSUPPORTED, dove out e' gia' a zero) - non fa
 * nulla in quel caso. */
void racseq_free_profile(RacseqProfile *p);

#endif /* IRRCSEQ_H */
