#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

/* racauth_metal.c - Metal C helper wrapping RACROUTE REQUEST=AUTH: checks
 * whether the CURRENT user is authorized (at least the given access
 * level) to a RACF-protected resource. Complements racmode_metal.c
 * (which gates whether the PROGRAM itself is APF/supervisor-state
 * authorized) with a check of the USER's own RACF authorization to a
 * specific resource - the two are independent gates, both needed for a
 * supervisor-mode entry point that should only run on behalf of
 * authorized callers (see racseq_future_library_api project memory).
 *
 * Switched from REQUEST=FASTAUTH to REQUEST=AUTH on 2026-08-03: a live
 * test against class FACILITY / $CAGS.IRRSEQ.SUPERVISOR returned SAF RC
 * 4 even though the calling user's group had READ access, because
 * FASTAUTH requires the target class to be RACLISTed (SETROPTS
 * RACLIST) - confirmed via the RACROUTE Macro Reference's SAF RC/RACF
 * RC/Reason table (SAF RC 4 / RACF RC 0 / Reason 4 = "class not
 * RACLISTed"). REQUEST=AUTH has no such requirement and implicitly logs
 * via SMF (useful since the target resource is AUDIT(ALL); FASTAUTH
 * does not log by default).
 *
 * The parameter list layout (RacrouteParms below) is transcribed
 * field-by-field from a real assembled RACROUTE REQUEST=AUTH expansion
 * (fastauth.lst, HLASM R6.0, 2026-08-03: CLASS='FACILITY',
 * ENTITY=(R5), WORKA=RACWK, RELEASE=77B0, ATTR=READ), not guessed - same
 * discipline as the IRRPCOMP.asm/racseq.h structs and the original
 * FASTAUTH parameter list. This is a COMPLETELY DIFFERENT, larger
 * layout than FASTAUTH's - not just a "change the request number" edit
 * on top of the old struct (that was tried and correctly flagged as
 * unsafe before porting this real listing).
 *
 * Two sub-structures, laid out contiguously here (unlike the sample,
 * which has ~8 bytes of its OWN executable code physically between
 * them - irrelevant to the data layout, see racf_offset below):
 *
 *   1) The ICHSAFP-mapped SAF router list (first 104/0x68 bytes).
 *      Offsets come from fastauth.lst's own ICHSAFP DSECT expansion
 *      (stmt 325-368) - an IBM system macro, not the sample's ad hoc
 *      DC's, so these field names/offsets are authoritative.
 *   2) The RACHECK-style parameter list (IHB0003C in fastauth.lst,
 *      stmt 80-111), whose fixed part is 100/0x64 bytes, immediately
 *      followed here by an embedded class-name substructure.
 *
 * The class name is NOT passed via an external buffer pointer (unlike
 * FASTAUTH's class_addr, which pointed straight at the caller's
 * class_name[8]) - RACROUTE REQUEST=AUTH embeds it inside the parameter
 * list itself, and the "class name address" field points ONE BYTE INTO
 * a 2-byte length constant (fastauth.lst's "ICH10020 EQU *+1"): the
 * macro emits an AL2(8) length (2 bytes, high byte always 0 for any
 * class name <=255 chars), and the address field is deliberately
 * offset by 1 so RACF sees a plain {1-byte length}{name} pair starting
 * at the low-order byte. Reproduced here as class_len_hi (always 0) +
 * class_len (the real 1-byte length RACF reads, and what class_addr
 * points to) + class_name8.
 *
 * ATTR is a bit flag embedded directly in the RACHECK list (rc_attr
 * below, offset +0x08) - fastauth.lst shows ATTR=READ as B'00000010' =
 * 0x02 there, confirming RACAUTH_ATTR_READ's value independently of the
 * original FASTAUTH listing. Access is NOT passed via a separate
 * pointer-to-byte (the "ACCESS VALUE ADDRESS" field a few words later
 * is 0/unused for this keyword combination).
 *
 * ENTITY=(R5) with a SINGLE register still means R5 addresses a
 * FIXED-length, blank-padded resource name field (confirmed against the
 * RACROUTE Macro Reference, 2026-07-31), NOT a length-prefixed one -
 * see racauth.h for the buffer-size contract this imposes on callers.
 * (fastauth.lst's own PROFILE constant regressed to an unpadded 23
 * bytes for this test - the same bug caught and fixed once before -
 * but that only affects the sample's own hardcoded test string, not
 * this wrapper, which always uses the caller's full 246-byte buffer.)
 *
 * CVT address at absolute location 16, SAF vector table at CVT+248, SAF
 * router entry at (SAF vector table)+12: the exact same lookup the
 * RACROUTE macro's own generated code performs, unchanged from the
 * FASTAUTH version and reconfirmed against this new listing
 * (fastauth.lst, IHB0003D/IHB0003H, same offsets). The "SAF router not
 * available" fallback (return 4) also mirrors the macro's own LA 15,4
 * on that same path.
 *
 * SAFPSFRC/SAFPSFRS (offsets +0x28/+0x2C of the SAFP list) are
 * populated by the router on return and surfaced here as
 * status->racfrc/status->racfrs - see RacauthStatus in racauth.h for
 * the caveat on their exact semantics (not yet cross-checked against
 * observed values on the real system).
 *
 * Same custom prolog/epilog requirement as every other Metal C function
 * called directly from hosted C in this codebase - see racfree_metal.c
 * for why (Metal C's default prolog trusts a NAB pointer hosted C never
 * supplies).
 */

#include <stddef.h>
#include <string.h>
#include "racauth.h"

#pragma pack(1)
typedef struct {
    /* ---- ICHSAFP (SAF router parameter list) - 0x68/104 bytes ---- */
    int            exit_rc;         /* @0x00 SAFPRRET - RACF/install exit return code */
    int            exit_reason;     /* @0x04 SAFPRREA - RACF/install exit reason code */
    unsigned short list_len;        /* @0x08 SAFPPLN  - length of this list (104) */
    unsigned char  ver_rel;         /* @0x0A SAFPVRRL - RACF version/release indicator (24 = SAFPRLB0/77B0) */
    unsigned char  reserved_0b;     /* @0x0B reserved */
    unsigned short request;         /* @0x0C SAFPREQT - request number (1 = SAFPAU = REQUEST=AUTH) */
    unsigned char  flags;           /* @0x0E SAFPFLGS - X'40' = SAFPR18 (release 1.8+ function requested) */
    unsigned char  msg_subpool;     /* @0x0F SAFPMSPL */
    void          *reqr_addr;       /* @0x10 SAFPREQR - requestor name address, unused */
    void          *subs_addr;       /* @0x14 SAFPSUBS - subsystem name address, unused */
    void          *worka_addr;      /* @0x18 SAFPWA   - SAF work area address (WORKA=) */
    void          *msgret_addr;     /* @0x1C SAFPMSAD - message return address, unused */
    int            reserved_20;     /* @0x20 reserved */
    int            racf_offset;     /* @0x24 SAFPRACP - offset from this struct's base to rc_len below */
    int            saf_rc;          /* @0x28 SAFPSFRC - populated by RACF on return, see RacauthStatus */
    int            saf_reason;      /* @0x2C SAFPSFRS - populated by RACF on return */
    unsigned short ext_len;         /* @0x30 SAFPPLNX - length of SAFP extension list (64, copied verbatim) */
    unsigned short orig_len;        /* @0x32 SAFPOLEN - unused */
    void          *ret_data_addr;   /* @0x34 SAFPRETD */
    void          *flat_addr;       /* @0x38 SAFPFLAT */
    void          *ecb1_addr;       /* @0x3C SAFPECB1 */
    void          *ecb2_addr;       /* @0x40 SAFPECB2 */
    void          *prev_addr;       /* @0x44 SAFPPREV */
    void          *next_addr;       /* @0x48 SAFPNEXT */
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

    /* ---- RACHECK-style parameter list (IHB0003C), fixed part ---- */
    unsigned char  rc_len;           /* @+0x00 length of this fixed part (100) */
    unsigned char  rc_reserved1[3];  /* @+0x01 reserved */
    unsigned char  rc_flags1;        /* @+0x04 flags byte (B'00001000' in the sample, copied
                                       * verbatim - not individually decoded, unrelated to ATTR) */
    unsigned char  rc_reserved2[3];  /* @+0x05 reserved */
    unsigned char  rc_attr;          /* @+0x08 ATTR access level - RACAUTH_ATTR_* */
    unsigned char  rc_reserved3[3];  /* @+0x09 reserved */
    unsigned char  rc_flags3;        /* @+0x0C flag3 byte (0 in the sample) */
    unsigned char  rc_reserved4[3];  /* @+0x0D reserved */
    void          *old_volser_addr;  /* @+0x10 unused */
    void          *appl_addr;        /* @+0x14 unused */
    void          *acee_addr;        /* @+0x18 0 = caller's own ACEE */
    void          *owner_addr;       /* @+0x1C unused */
    void          *instdata_addr;    /* @+0x20 unused */
    void          *entity_addr;      /* @+0x24 -> caller's blank-padded entity buffer (ENTITY=(reg)) */
    void          *class_addr;       /* @+0x28 -> class_len below (the "+1" trick, see file header) */
    void          *volser_addr;      /* @+0x2C unused */
    void          *access_addr;      /* @+0x30 unused - ATTR passed via rc_attr above, not a pointer */
    void          *access2_addr;     /* @+0x34 unused */
    unsigned short fileseq;          /* @+0x38 unused */
    unsigned char  rc_reserved5;     /* @+0x3A unused */
    unsigned char  rc_reserved6;     /* @+0x3B unused */
    void          *user_addr;        /* @+0x3C unused */
    void          *group_addr;       /* @+0x40 unused */
    void          *ddname_addr;      /* @+0x44 unused */
    void          *rc_reserved7;     /* @+0x48 unused */
    void          *utoken_addr;      /* @+0x4C unused */
    void          *rtoken_addr;      /* @+0x50 unused */
    void          *logstr_addr;      /* @+0x54 unused */
    void          *receiver_addr;    /* @+0x58 unused */
    void          *memcount_addr;    /* @+0x5C unused */
    void          *member_addr;      /* @+0x60 unused */
    /* == RACHECK fixed part: 0x64 = 100 bytes == */

    /* Embedded class-name substructure - see file header for the "+1"
     * addressing trick this reproduces. */
    unsigned char  class_len_hi;     /* @+0x64 always 0, never read by RACF */
    unsigned char  class_len;        /* @+0x65 actual 1-byte length - class_addr points here */
    char           class_name8[8];   /* @+0x66 blank-padded class name */
} RacrouteParms;
#pragma pack(reset)

#pragma prolog(r_admin_check_auth, "MYPROLOG")
#pragma epilog(r_admin_check_auth, "MYEPILOG")
#pragma linkage(r_admin_check_auth, OS)
int r_admin_check_auth(unsigned char entity[246], unsigned char class_name[8], racf_byte attr, RacauthStatus *status)
{
    unsigned char workarea[512];   /* RACWK - WORKA=, caller-supplied storage */
    RacrouteParms parms;
    void         *parms_addr = &parms;
    void         *cvt;
    void         *saf_vector;
    void         *saf_router;
    int           rc;
    size_t        clen;

    memset(workarea, 0, sizeof(workarea));
    memset(&parms, 0, sizeof(parms));

    parms.list_len    = (unsigned short)offsetof(RacrouteParms, rc_len);   /* 104 */
    parms.ver_rel     = 24;    /* SAFPRLB0 - RELEASE=77B0 */
    parms.request     = 1;     /* SAFPAU  - RACROUTE REQUEST=AUTH */
    parms.flags       = 0x40;  /* SAFPR18 - release 1.8+ function requested */
    parms.worka_addr  = workarea;
    /* SAFPRACP: offset from this struct's base to the RACF parameter
     * list. In fastauth.lst this is 0x70 because ~8 bytes of the
     * sample's OWN code sit between the two areas; here they are
     * simply adjacent, so the true offset is just the SAFP part's
     * size. */
    parms.racf_offset = (int)offsetof(RacrouteParms, rc_len);   /* 104 */
    parms.ext_len     = 64;    /* SAFPPLNX - copied verbatim, meaning not decoded */

    parms.rc_len    = (unsigned char)(offsetof(RacrouteParms, class_len_hi) - offsetof(RacrouteParms, rc_len)); /* 100 */
    parms.rc_flags1 = 0x08;    /* copied verbatim from the sample (B'00001000') */
    parms.rc_attr   = attr;
    parms.entity_addr = entity;

    clen = sizeof(parms.class_name8);
    while (clen > 0 && class_name[clen - 1] == ' ') clen--;
    parms.class_len = (unsigned char)clen;
    memset(parms.class_name8, ' ', sizeof(parms.class_name8));
    memcpy(parms.class_name8, class_name, clen);
    parms.class_addr = &parms.class_len;

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
