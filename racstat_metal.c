#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

/* racstat_metal.c - Metal C helper wrapping RACROUTE REQUEST=STAT: legge
 * una voce della Class Descriptor Table (CDT), sia in modalita' lookup
 * puntuale (CLASS=) sia in scansione (NEXT=). Usato per implementare la
 * pseudo-classe "_CDT" in irrcseq.c - vedi racstat.h per il contratto
 * pubblico e la provenienza del layout dai due listing reali.
 *
 * Stessa struttura in due parti di racauth_metal.c:
 *   1) Il SAF router parameter list ICHSAFP (primi 104/0x68 byte) -
 *      IDENTICO campo per campo a racauth_metal.c (stesso DSECT IBM,
 *      ri-confermato in statcdt.lst/statncdt.lst stmt ~404-447), a parte
 *      SAFPREQT che qui vale 14 (SAFPSTAT) invece di 1 (SAFPAU).
 *   2) Il parameter list RACSTAT-specific (IHB0003C in entrambi i
 *      listing), 24 byte fissi + un'area opzionale di 8 byte per il nome
 *      classe embedded, usata solo in modalita' CLASS=:
 *        statcdt.lst  stmt 79-90  (forma CLASS=,  R2/R3 mai caricati -
 *                                  layout affidabile, contenuto no)
 *        statncdt.lst stmt 94-102 (forma NEXT=, ciclo funzionante -
 *                                  fonte primaria anche per la semantica)
 *      A differenza di RACHECK/REQUEST=AUTH, qui NON c'e' nessun trucco
 *      "+1": class_addr punta DIRETTAMENTE alle 8 byte del nome classe
 *      (statcdt.lst stmt 80: "DC A(ICH00020)" seguito da "ICH00020 DC
 *      CL8'OPERCMDS'" senza alcun prefisso di lunghezza).
 *
 * COPYLEN (@+0x10) e' un VALORE numerico diretto nel parameter list, non
 * un puntatore: in statncdt.lst e' "DC A(COPYLEN)" ma COPYLEN e' definito
 * con EQU (simbolo assoluto = 0xB4 = 180), quindi l'assemblatore vi
 * incorpora il valore stesso, non un indirizzo (confermato dall'object
 * code: 000000B4, non un indirizzo di storage del programma).
 *
 * CLASS= e NEXT= condividono lo stesso campo fisico @+0x14 (vedi
 * racstat.h) - qui selezionato in base a next_name.
 *
 * Stesso custom prolog/epilog e stessa sequenza CVT->SAF vector->SAF
 * router->BALR di racauth_metal.c/racfree_metal.c.
 */

#include <stddef.h>
#include <string.h>
#include "racstat.h"

#pragma pack(1)
typedef struct {
    /* ---- ICHSAFP (SAF router parameter list) - 0x68/104 bytes, campo
     * per campo identico a racauth_metal.c ---- */
    int            exit_rc;         /* @0x00 SAFPRRET */
    int            exit_reason;     /* @0x04 SAFPRREA */
    unsigned short list_len;        /* @0x08 SAFPPLN  (104) */
    unsigned char  ver_rel;         /* @0x0A SAFPVRRL (24 = SAFPRLB0/77B0) */
    unsigned char  reserved_0b;     /* @0x0B reserved */
    unsigned short request;         /* @0x0C SAFPREQT (14 = SAFPSTAT = REQUEST=STAT) */
    unsigned char  flags;           /* @0x0E SAFPFLGS (0x40 = SAFPR18) */
    unsigned char  msg_subpool;     /* @0x0F SAFPMSPL */
    void          *reqr_addr;       /* @0x10 SAFPREQR */
    void          *subs_addr;       /* @0x14 SAFPSUBS */
    void          *worka_addr;      /* @0x18 SAFPWA -> workarea */
    void          *msgret_addr;     /* @0x1C SAFPMSAD */
    int            reserved_20;     /* @0x20 reserved */
    int            racf_offset;     /* @0x24 SAFPRACP */
    int            saf_rc;          /* @0x28 SAFPSFRC - popolato al ritorno */
    int            saf_reason;      /* @0x2C SAFPSFRS - popolato al ritorno */
    unsigned short ext_len;         /* @0x30 SAFPPLNX (64) */
    unsigned short orig_len;        /* @0x32 SAFPOLEN */
    void          *ret_data_addr;   /* @0x34 SAFPRETD */
    void          *flat_addr;       /* @0x38 SAFPFLAT */
    void          *ecb1_addr;       /* @0x3C SAFPECB1 */
    void          *ecb2_addr;       /* @0x40 SAFPECB2 */
    void          *prev_addr;       /* @0x44 SAFPPREV */
    void          *next_addr;       /* @0x48 SAFPNEXT (livello SAF - niente a che vedere col
                                      * campo NEXT= di RACROUTE REQUEST=STAT piu' sotto) */
    void          *orig_addr;       /* @0x4C SAFPORIG */
    int            flat_len;        /* @0x50 SAFPFLEN */
    int            user_word;       /* @0x54 SAFPUSRW */
    int            preexit_addr;    /* @0x58 SAFPPREE */
    int            postexit_addr;   /* @0x5C SAFPPOST */
    void          *sync_ecb_addr;   /* @0x60 SAFPSYNC */
    unsigned char  skey;            /* @0x64 SAFPSKEY */
    unsigned char  smode;           /* @0x65 SAFPMODE */
    unsigned char  sbyte;           /* @0x66 SAFPSBYT */
    unsigned char  reserved_67;     /* @0x67 reserved */
    /* == SAFP: 0x68 = 104 bytes == */

    /* ---- RACSTAT-specific parameter list (IHB0003C), 24 byte fissi ---- */
    void          *class_addr;      /* @+0x00 -> class_name8 se class_name usato, altrimenti 0 */
    void          *entry_addr;      /* @+0x04 ADDRESS OF ENTRY PARAMETER - sempre 0 (forma
                                      * ENTITY=, non usata qui) */
    unsigned short parm_len;        /* @+0x08 lunghezza di questa parte fissa (24) */
    unsigned short parm_reserved;   /* @+0x0A reserved */
    void          *copy_addr;       /* @+0x0C -> buffer copy_area del chiamante */
    unsigned int   copy_len;        /* @+0x10 VALORE (non puntatore) = RACSTAT_ENTRY_LEN */
    void          *stat_next_addr;  /* @+0x14 -> next_name se usato, altrimenti 0 */
    /* == parte fissa: 0x18 = 24 byte == */

    char           class_name8[8];  /* @+0x18 nome classe embedded, solo modalita' class_name */
} RacstatParms;
#pragma pack(reset)

#pragma prolog(r_admin_stat_class, "MYPROLOG")
#pragma epilog(r_admin_stat_class, "MYEPILOG")
#pragma linkage(r_admin_stat_class, OS)
int r_admin_stat_class(const unsigned char class_name[8], unsigned char next_name[8],
                        unsigned char copy_area[RACSTAT_ENTRY_LEN], RacauthStatus *status)
{
    unsigned char workarea[512];   /* WORKA=, caller-supplied storage */
    RacstatParms  parms;
    void         *parms_addr = &parms;
    void         *cvt;
    void         *saf_vector;
    void         *saf_router;
    int           rc;

    memset(workarea, 0, sizeof(workarea));
    memset(&parms, 0, sizeof(parms));
    memset(copy_area, 0, RACSTAT_ENTRY_LEN);

    parms.list_len    = (unsigned short)offsetof(RacstatParms, class_addr);   /* 104 */
    parms.ver_rel     = 24;    /* SAFPRLB0 - RELEASE=77B0 */
    parms.request     = 14;    /* SAFPSTAT - RACROUTE REQUEST=STAT */
    parms.flags       = 0x40;  /* SAFPR18 */
    parms.worka_addr  = workarea;
    parms.racf_offset = (int)offsetof(RacstatParms, class_addr);   /* 104 */
    parms.ext_len     = 64;

    parms.parm_len  = (unsigned short)(offsetof(RacstatParms, class_name8)
                                        - offsetof(RacstatParms, class_addr));  /* 24 */
    parms.copy_addr = copy_area;
    parms.copy_len  = RACSTAT_ENTRY_LEN;

    if (next_name) {
        parms.stat_next_addr = next_name;
    } else {
        memcpy(parms.class_name8, class_name, sizeof(parms.class_name8));
        parms.class_addr = parms.class_name8;
    }

    cvt = *(void **)16;
    saf_vector = *(void **)((char *)cvt + 248);
    if (!saf_vector) {   /* SAF router not available in this system */
        status->safrc = status->racfrc = status->racfrs = 4;
        return 4;
    }
    saf_router = *(void **)((char *)saf_vector + 12);
    if (!saf_router) {   /* SAF router not available */
        status->safrc = status->racfrc = status->racfrs = 4;
        return 4;
    }

    __asm(" L 15,%1\n"
          " L 1,%2\n"
          " BALR 14,15\n"
          " ST 15,%0"
          : "=m"(rc)
          : "m"(saf_router), "m"(parms_addr)
          : "r0", "r1", "r14", "r15");

    status->safrc  = rc;
    status->racfrc = parms.saf_rc;
    status->racfrs = parms.saf_reason;

    return rc;
}
