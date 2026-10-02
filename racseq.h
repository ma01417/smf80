#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef RACSEQ_H
#define RACSEQ_H

#include <stdint.h>

/* ---------------------------------------------------------------------
 * C mapping of the R_admin (IRRSEQ00) structures used by RACSEQ.
 *
 * Field order/sizes below are reconstructed from how racseq.asm USES the
 * ADMN_PROF_MAP / ADMN_XTRSETR_MAP / ADMN_USRADM_* DSECTs that the
 * IRRPCOMP system macro (SYS1.MACLIB) generates. They are not transcribed
 * from IRRPCOMP itself.
 *
 * Anything marked TODO is a value or offset this program cannot confirm
 * without the real macro expansion. Get it from an assembler
 * cross-reference listing of a program that invokes IRRPCOMP (e.g.
 * racseq.asm) or from the z/OS Security Server RACF Callable Services
 * book, before running this against a real system.
 * --------------------------------------------------------------------- */

typedef int32_t       racf_fullword;
typedef int16_t       racf_halfword;
typedef unsigned char racf_byte;

/* ---- Function codes for the FUNCODE parameter ---------------------
 * Values confirmed from the real IRRPCOMP (SYS1.MACLIB) ADMN_XTR_xxx
 * EQUs on the target system (2026-07-21). */
#define ADMN_XTR_SETR            0x16
#define ADMN_XTR_USER            0x19
#define ADMN_XTR_NEXT_USER       0x1A
#define ADMN_XTR_GROUP           0x1B
#define ADMN_XTR_NEXT_GROUP      0x1C
#define ADMN_XTR_CONNECT         0x1D
#define ADMN_XTR_RESOURCE        0x1F
#define ADMN_XTR_NEXT_RESOURCE   0x20
#define ADMN_XTR_DATASET         0x22
#define ADMN_XTR_NEXT_DATASET    0x23

/* ---- Flag bits for ADMN_PROF_FLAG (profile extract) ---------------- */
#define ADMN_PROF_BASEONLY       0x40000000
#define ADMN_PROF_NAMEONLY       0x04000000
#define ADMN_PROF_UPPERCASE      0x08000000
#define ADMN_PROF_SKIPAUTH       0x80000000
#define ADMN_PROF_DOAUTH         0x20000000
#define ADMN_PROF_GENERIC        0x10000000
#define ADMN_PROF_MATCHGN        0x02000000

/* ---- Flag bits for ADMN_XTRSETR_FLAG (SETROPTS extract) ------------
 * Confirmed from IRRPCOMP.asm (SYS1.MACLIB expansion), lines 3793-3801. */
#define ADMN_XTRSETR_DOFACILITY  0x80000000
#define ADMN_XTRSETR_DOCMDAUTH   0x40000000

/* ---- Field-type bits within ADMN_PROF_FIELDTYPE (a CL2/halfword) ---
 * Confirmed from IRRPCOMP.asm lines 3874-3878. These are individual
 * BITS, not mutually-exclusive codes - test with & , not ==. */
#define ADMN_PROF_REPEAT         0x8000
#define ADMN_PROF_BOOLEAN        0x2000
#define ADMN_PROF_RPTHDR         0x1000

/* Bits within ADMN_PROF_FIELDFLAG (a CL4/fullword, NOT a single byte -
 * confirmed from IRRPCOMP.asm lines 3880-3882). IRRPCOMP.asm documents
 * these as X'8000'/X'4000' (a 2-byte view of the flag's first
 * halfword); scaled up by 16 bits here to test the right bits in the
 * full 4-byte field_flag. CONFIRMED against racseq.asm's own working
 * code (line 830: `TM ADMN_PROF_FIELDFLAG,FMBOOLV` with its local
 * `FMBOOLV EQU X'80'`, i.e. testing byte 0 of the field with mask
 * X'80' - the same bit as 0x80000000 in a full 4-byte big-endian view). */
#define ADMN_PROF_VALUE          0x80000000   /* boolean TRUE */
#define ADMN_PROF_OUTONLY        0x40000000

/* ADMN_USRADM_FLD_FLAG (a separate, CL1 field used only by the SETROPTS
 * extract path) really is a single 'Y'/'N' char - see ADMN_USRADM_FLDENTRY
 * below, compared directly against 'Y' rather than through a bitmask. */

/* ---------------------------------------------------------------------
 * Input parameter list for profile-extract (ADMN_PROF_MAP). Field
 * order/offsets CONFIRMED from IRRPCOMP.asm (SYS1.MACLIB expansion),
 * lines 3806-3858 - this single struct covers both what WE fill in
 * before the call (version, class_name, ds_volume, name_len, flag,
 * profile_name) and what R_admin fills in as OUTPUT (out_len, spid,
 * num_seg) - unlike a previous version of this header, out_len/spid
 * are NOT a separate trailing struct: they sit right after the
 * eyecatcher, before version, in the real DSECT.
 * --------------------------------------------------------------------- */
#pragma pack(1)

typedef struct {
    char          eyecatcher[4];   /* ADMN_PROF_EYE @0, e.g. "PXTR" */
    racf_fullword out_len;         /* ADMN_PROF_OUTLEN @4 - output only */
    racf_byte     spid;            /* ADMN_PROF_SPID @8 - output only, subpool of this buffer */
    racf_byte     version;         /* ADMN_PROF_VERSION @9 */
    racf_byte     reserved1[2];    /* @10-11 */
    char          class_name[8];   /* ADMN_PROF_CLSNAME @12 */
    racf_fullword name_len;        /* ADMN_PROF_NAMELEN @20 */
    racf_byte     reserved2[2];    /* @24-25 */
    char          ds_volume[6];    /* ADMN_PROF_DSVOLUME @26 (DATASET class only) */
    racf_fullword dup_ds_count;    /* ADMN_PROF_DDDSC @32 */
    racf_fullword flag;            /* ADMN_PROF_FLAG @36 */
    racf_fullword num_seg;         /* ADMN_PROF_NUMSEG @40 - output only */
    racf_byte     reserved3[16];   /* @44-59 */
    char          profile_name[246]; /* ADMN_PROF_PROFNAME @60 (variable; racseq.asm reserves 246) */
} ADMN_PROF_MAP;

/* Segment descriptor - CONFIRMED from IRRPCOMP.asm lines 3861-3869.
 * Each segment carries its OWN field_offset (offset from the start of
 * ADMN_PROF_MAP to that segment's first field descriptor) - it is NOT
 * a single buffer-wide value read once before the segment loop. */
typedef struct {
    char          seg_name[8];    /* ADMN_PROF_SEGNAME @0 */
    racf_fullword seg_flags;      /* ADMN_PROF_SEGFLAGS @8 */
    racf_fullword num_fields;     /* ADMN_PROF_NUMFIELDS @12 */
    racf_byte     reserved1[4];   /* @16-19 */
    racf_fullword field_offset;   /* ADMN_PROF_FIELDOFFSET @20 - from start of ADMN_PROF_MAP */
    racf_byte     reserved2[16];  /* @24-39 */
    /* ADMN_PROF_NEXTSEG = (char *)this + sizeof(ADMN_PROF_SEGDESC), @40 */
} ADMN_PROF_SEGDESC;

/* Field descriptor - CONFIRMED from IRRPCOMP.asm lines 3872-3892.
 * field_len/rpt_num and data_offset/field_dim are overlaid pairs (the
 * real DSECT uses ORG to reuse the same 4 bytes for either meaning,
 * selected by whether ADMN_PROF_RPTHDR is set in field_type). */
typedef struct {
    char          field_name[8]; /* ADMN_PROF_FIELDNAME @0 */
    racf_halfword field_type;    /* ADMN_PROF_FIELDTYPE @8 - bitmask, see ADMN_PROF_REPEAT/BOOLEAN/RPTHDR */
    racf_byte     reserved1[2];  /* @10-11 */
    racf_fullword field_flag;    /* ADMN_PROF_FIELDFLAG @12 - bitmask, see ADMN_PROF_VALUE/OUTONLY */
    racf_fullword field_len;     /* ADMN_PROF_FIELDLEN @16, OR (if RPTHDR) ADMN_PROF_RPTNUM */
    racf_byte     reserved2[4];  /* @20-23 */
    racf_fullword data_offset;   /* ADMN_PROF_DATA_OFFSET @24, OR (if RPTHDR) ADMN_PROF_FIELDDIM */
    racf_byte     reserved3[16]; /* @28-43 */
    /* ADMN_PROF_NEXTFIELD = (char *)this + sizeof(ADMN_PROF_FIELDDESC), @44 */
} ADMN_PROF_FIELDDESC;

/* ---------------------------------------------------------------------
 * SETROPTS extract (ADMN_XTRSETR_MAP in, ADMN_XTRUNL_MAP out).
 * CONFIRMED from IRRPCOMP.asm lines 3775-3801: ADMN_XTRSETR_MAP has
 * NO version field - just FLAG followed by 10 reserved bytes (total
 * CL14, matching racseq.asm's SETPLIST DS CL14). ADMN_XTRUNL_MAP has
 * its own eyecatcher, reserved gap, and an entry-count field before
 * the first ADMN_USRADM_SEGENTRY.
 * --------------------------------------------------------------------- */
typedef struct {
    racf_fullword flag;          /* ADMN_XTRSETR_FLAG @0 */
    racf_byte     reserved[10];  /* @4-13 */
} ADMN_XTRSETR_MAP;

typedef struct {
    char          eyecatcher[4]; /* ADMN_XTRUNL_EYE @0 */
    racf_fullword out_len;       /* ADMN_XTRUNL_OUTLEN @4 */
    racf_byte     reserved[4];   /* @8-11 */
    racf_halfword num_entries;   /* ADMN_XTRUNL_NUM @12 */
    /* ADMN_XTRUNL_ENTRY (ADMN_USRADM_SEGENTRY) follows, @14 */
} ADMN_XTRUNL_MAP;

typedef struct {
    char          seg_name[8];   /* ADMN_USRADM_SEG_NAME, always "BASE" */
    char          seg_flag;      /* ADMN_USRADM_SEG_FLAG @8 - was missing, shifted fld_num
                                   * one byte early and made it read garbage (IRRPCOMP.asm
                                   * line 3721 confirms this byte sits between seg_name and
                                   * fld_num) */
    racf_halfword fld_num;       /* ADMN_USRADM_FLD_NUM @9 */
    /* ADMN_USRADM_FLDSTRT (first ADMN_USRADM_FLDENTRY) follows */
} ADMN_USRADM_SEGENTRY;

typedef struct {
    char          fld_name[8];   /* ADMN_USRADM_FLD_NAME */
    char          fld_flag;      /* ADMN_USRADM_FLD_FLAG: 'Y'/'N' for boolean fields */
    racf_halfword fld_len;       /* ADMN_USRADM_FLD_LEN */
    /* ADMN_USRADM_FLD_DATA (fld_len bytes) follows immediately - variable length entry */
} ADMN_USRADM_FLDENTRY;

typedef struct {
    racf_byte len;
    char      userid[8];
} RACF_USERPARM;

#pragma pack(reset)

/* ---------------------------------------------------------------------
 * IRRSEQ00 call. Every parameter is declared as the TYPE OF DATA FOUND
 * AT its address, never as an explicit pointer-to-that-type: under
 * "#pragma linkage(.., OS)" the compiler itself takes the address of
 * each named argument and places it in the register-1 parameter list
 * (matching the classic MVS calling convention racseq.asm builds by
 * hand at CALLSEQ, racseq.asm lines 556-596).
 *
 * CONFIRMED BY TESTING (S0C4-04 in call_radmin, abend "during IRRSEQ00
 * processing"): `plist` MUST be declared with array type here, not as
 * `void *plist`. The caller's `plist` variable already holds the
 * address of ADMN_PROF_MAP/ADMN_XTRSETR_MAP (e.g. `plist = &prof_map`
 * in main()). With a pointer-typed formal parameter, OS linkage takes
 * the address of that *argument* too, so the register-1 list entry
 * ends up pointing at the local cell that holds the pointer (one
 * indirection too many) instead of at the struct itself - RACF then
 * misreads a raw pointer value as ADMN_PROF_MAP field data and
 * eventually stores through a garbage address it computes from that.
 * Declaring the parameter as an array type (same trick already used
 * for `workarea` below) makes the compiler pass the pointer *value*
 * straight through, matching the single indirection RACF expects.
 * SUPERSEDES THE PREVIOUS NOTE ON `outmsg`: declaring it as a plain
 * scalar (`racf_fullword outmsg`) stopped the S0C4 but was still
 * wrong, for a more fundamental reason confirmed by reading the fresh
 * racseq.lst: for EVERY non-array formal parameter (funcode, acee,
 * subpool, user, and the plain-scalar `outmsg`), this compiler always
 * materializes a fresh compiler-generated temporary, copies the
 * argument's current value into it, and passes the ADDRESS OF THAT
 * TEMPORARY - never the real C variable's own address. There is no
 * code generated anywhere after the CALL to copy the temporary's
 * (possibly RACF-updated) content back into the original variable.
 * So safrc/racfrc/racfrs/outmsg, if declared as plain scalars, will
 * ALWAYS read back as whatever they were initialized to before the
 * call, regardless of what RACF actually wrote - this is why the
 * S0C4 went away (RACF now had a valid stack address to write
 * through) but the program still silently saw safrc=racfrc=racfrs=0
 * and outmsg=NULL every time.
 *
 * Only array/pointer-typed formal parameters (see `plist` above)
 * escape this: for those, the compiler passes the argument
 * expression's VALUE straight through with no temp copy - which is
 * exactly what's needed for a real output parameter, PROVIDED the
 * call site itself passes the address explicitly (`&variable`), the
 * same way racseq.asm's hand-built CALLSEQ always did. So safrc,
 * racfrc, racfrs and outmsg are now declared as pointers, and
 * call_radmin() passes `&safrc`, `&racfrc`, `&racfrs`, `&out_word`.
 *
 * `funcode` and `subpool` are ALSO declared as pointers here, for a
 * different reason: real R_admin runs (2026-07-22) rejected every
 * FUNCODE value tried (0x19 and 0x1A, both confirmed valid
 * ADMN_XTR_xxx EQUs from IRRPCOMP.asm) with SAFRC=8/RACFRC=8/RACFRS=0,
 * which the RACF Callable Services return/reason code table defines
 * as literally "Incorrect function code" - regardless of which valid
 * code was sent. Since these are the only other plain-scalar
 * (non-pointer) input parameters of 1-byte width, the suspicion is
 * that this compiler's scalar-temp mechanism does not reliably place
 * a clean, correctly-addressed single byte for CL1-sized formals under
 * OS linkage (e.g. a padded/misaligned temp). Declaring them as
 * pointers and passing `&funcode`/`&subpool` sidesteps the temp
 * mechanism entirely, guaranteeing the list holds the address of real,
 * exactly-1-byte storage - matching racseq.asm's hand-built CALLSEQ
 * (`LA R1,FUNCODE` / `LA R1,SUBPOOL`, always real storage, never a
 * compiler-invented temp).
 * --------------------------------------------------------------------- */
#pragma linkage(IRRSEQ00, OS)
void IRRSEQ00(
    unsigned char   workarea[1024],
    racf_fullword   alet1,   racf_fullword *safrc,
    racf_fullword   alet2,   racf_fullword *racfrc,
    racf_fullword   alet3,   racf_fullword *racfrs,
    racf_byte      *funcode,
    unsigned char   plist[],    /* address of ADMN_PROF_MAP / ADMN_XTRSETR_MAP / previous output buffer */
    RACF_USERPARM   user,
    racf_fullword   acee,
    racf_byte      *subpool,
    racf_fullword  *outmsg      /* out: filled in with the address of R_admin's output buffer (reinterpret as void*) */
);

/* Hardcoded SETROPTS boolean/repeat field name tables (racseq.asm lines 1484-1533) */
extern const char *const SETR_REPEAT_FIELDS[];
extern const int         SETR_REPEAT_FIELD_COUNT;
extern const char *const SETR_BOOLEAN_FIELDS[];
extern const int         SETR_BOOLEAN_FIELD_COUNT;

#endif /* RACSEQ_H */
