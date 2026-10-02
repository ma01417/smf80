#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

/* racmode_metal.c - Metal C helpers for the --supervisor option
 * (racseq.asm lines 599-610):
 *
 *     OC    PDLSUPER,PDLSUPER    Supervisor state requested?
 *     BZ    NOSUPER1             Nope
 *     MODESET MODE=SUP           Switch to supervisor state
 *   NOSUPER1 DS 0H
 *     ... call IRRSEQ00 ...
 *     OC    PDLSUPER,PDLSUPER
 *     BZ    NOSUPER2
 *     MODESET MODE=PROB          Switch back to problem state
 *   NOSUPER2 DS 0H
 *
 * MODE=SUP only succeeds if this load module is running from an
 * APF-authorized library (or the caller is already supervisor
 * state / system key) - same requirement racseq.asm documents in its
 * header comment. r_admin_is_authorized() lets the caller check this
 * up front with TESTAUTH, which is built for exactly this "ask before
 * you leap" query and does not disrupt the caller either way -
 * calling MODESET MODE=SUP while unauthorized is expected to fail
 * disruptively (abend), which is why this checks first instead of
 * just trying it and reacting to failure.
 *
 * NOT compiled/tested against a real Metal C toolchain. In
 * particular, verify TESTAUTH's FCTN= value and return-code
 * convention (assumed here: R15 = 0 means authorized) against the
 * z/OS MVS Programming: Authorized Assembler Services Reference for
 * your release before trusting this on a live system.
 *
 * #pragma prolog/epilog below: these functions are called directly
 * from hosted-C/LE code (racseq.c) under OS linkage. Metal C's
 * default prolog for a non-"main" function trusts a "next save area"
 * (NAB) pointer supplied by the caller's save area, which hosted C
 * never populates for this kind of call - this was the confirmed
 * root cause of a U4094-18 storage-corruption abend in
 * racfree_metal.c's r_admin_freemain (2026-07-29), fixed there the
 * same way. Applying the same fix here proactively, since the
 * --supervisor path hasn't been live-tested yet for an unrelated
 * reason (lack of APF authorization on the test system) - the prolog
 * bug is structural and would hit these functions identically once
 * that's available. See metalc_prolog.mac for MYPROLOG/MYEPILOG.
 */

#include "racmode.h"

#pragma prolog(r_admin_is_authorized, "MYPROLOG")
#pragma epilog(r_admin_is_authorized, "MYEPILOG")
#pragma linkage(r_admin_is_authorized, OS)
int r_admin_is_authorized(void)
{
    int rc;

    /* Neither a bare register-name constraint ("=r15": the digit is
     * read as a matching-constraint back-reference, CCN4258) nor a
     * local explicit register variable (ignored with CCN3947) nor a
     * curly-brace register constraint ("{r15}": "{"/"}" not
     * recognized, CCN4147) pin an operand to a specific hardware
     * register on this compiler. Fall back to the generic "=r"
     * constraint (any GPR, guaranteed not r0/r1/r14/r15 since those
     * are clobbered) and copy TESTAUTH's fixed R15 result into it with
     * an explicit LR afterward. */
    __asm(" TESTAUTH FCTN=1\n"
          " LR %0,15"
          : "=r"(rc)
          :
          : "r0", "r1", "r14", "r15");

    return (rc == 0);
}

#pragma prolog(r_admin_enter_supervisor, "MYPROLOG")
#pragma epilog(r_admin_enter_supervisor, "MYEPILOG")
#pragma linkage(r_admin_enter_supervisor, OS)
void r_admin_enter_supervisor(void)
{
    __asm(" MODESET MODE=SUP"
          : : : "r0", "r1", "r14", "r15");
}

#pragma prolog(r_admin_leave_supervisor, "MYPROLOG")
#pragma epilog(r_admin_leave_supervisor, "MYEPILOG")
#pragma linkage(r_admin_leave_supervisor, OS)
void r_admin_leave_supervisor(void)
{
    __asm(" MODESET MODE=PROB"
          : : : "r0", "r1", "r14", "r15");
}
