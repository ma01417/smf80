#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef RACAUTH_H
#define RACAUTH_H

#include "racseq.h"   /* racf_byte */

/* Bit values for the ATTR parameter of r_admin_check_auth. READ is
 * confirmed against TWO independent real assembled listings now: the
 * original RACROUTE REQUEST=FASTAUTH one (racroute_fastauth.lst) and
 * the REQUEST=AUTH one (fastauth.lst, 2026-08-03, ATTR=READ ->
 * B'00000010' = 0x02 at offset +8 of the RACHECK-style parameter list).
 * The others follow the documented RACROUTE ATTR bit convention but
 * have not individually been checked against an assembled listing the
 * way READ has. Verify before relying on them for anything beyond READ. */
#define RACAUTH_ATTR_READ    0x02
#define RACAUTH_ATTR_UPDATE  0x04
#define RACAUTH_ATTR_CONTROL 0x08
#define RACAUTH_ATTR_ALTER   0x10

/* Filled in by r_admin_check_auth from fields the SAF router itself
 * populates in the ICHSAFP parameter list on return (fastauth.lst,
 * 2026-08-03):
 *   - safrc:  raw SAF return code from R15 (same as before).
 *   - racfrc: SAFPSFRC ("SAF Return Code" in the ICHSAFP DSECT, at
 *     offset +0x28 of the SAFP list) - despite the DSECT's own field
 *     name, per the RACROUTE Macro Reference's SAF RC/RACF RC/Reason
 *     hierarchy (the same table used to diagnose the 2026-08-03
 *     "not RACLISTed" finding) this is the RACF-level return code.
 *   - racfrs: SAFPSFRS (offset +0x2C), the corresponding reason code.
 * Not yet cross-checked against observed values on the real system
 * (e.g. expected 0/0 when authorized, 0/4 for a logged deny) - verify
 * once tested. */
typedef struct {
    int safrc;
    int racfrc;
    int racfrs;
} RacauthStatus;

/* r_admin_check_auth: wraps RACROUTE REQUEST=AUTH (switched from
 * REQUEST=FASTAUTH on 2026-08-03: FASTAUTH requires the target class to
 * be RACLISTed via SETROPTS RACLIST, which FACILITY was not on the test
 * system, causing a spurious "not authorized" - SAF RC 4/RACF RC 0/
 * reason 4, "class not RACLISTed", confirmed via the RACROUTE Macro
 * Reference's SAF RC/RACF RC/Reason table. REQUEST=AUTH has no such
 * requirement, and implicitly logs via SMF like LOG=ASIS, useful since
 * the target resource here is AUDIT(ALL)) - checks whether the CURRENT
 * user is authorized for at least `attr` access to a RACF-protected
 * resource. Independent of racmode.h's r_admin_is_authorized() (which
 * checks whether the PROGRAM itself is APF/supervisor-state authorized,
 * not whether the calling user has access to any particular resource).
 *
 * `entity` and `class_name` must already be formatted by the caller
 * before this call:
 *   - entity: exactly 246 bytes, the resource name left-justified and
 *     blank-padded. Per the RACROUTE Macro Reference (confirmed
 *     2026-07-31), ENTITY=(register) with a SINGLE register requires a
 *     FIXED-length, blank-padded field at least as long as the longest
 *     profile name valid in the target class (e.g. 39 for FACILITY, 44
 *     for DATASET) - NOT a length-prefixed field. 246 is the documented
 *     ceiling across every class, so a full 246-byte buffer is always
 *     sufficient regardless of which class is being checked. (Note: the
 *     2026-08-03 fastauth.lst test sample itself regressed this to a
 *     23-byte unpadded PROFILE constant, exactly the earlier-caught bug
 *     - harmless for OUR C wrapper since it always uses the full
 *     246-byte buffer regardless of what the HLASM sample did, but not
 *     a pattern to copy.)
 *   - class_name: exactly 8 bytes, left-justified and blank-padded
 *     (same convention as ADMN_PROF_CLSNAME in racseq.h).
 *
 * Fills *status with the SAF/RACF return and reason codes (see
 * RacauthStatus above) and also returns the SAF return code directly
 * for convenience (0 = authorized, matching existing call-site checks
 * like `if (racseq_check_auth(...) != 0)`). */
#pragma linkage(r_admin_check_auth, OS)
#pragma map(r_admin_check_auth, "RSQFAUTH")
int r_admin_check_auth(unsigned char entity[246], unsigned char class_name[8], racf_byte attr, RacauthStatus *status);

#endif /* RACAUTH_H */
