#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

/* racfree_metal.c - Metal C helper that issues a real FREEMAIN for
 * storage R_admin (IRRSEQ00) GETMAINed and handed back to us. This
 * storage is not part of the C heap (it was never obtained through
 * malloc/__malloc31), so calling free() on it would corrupt the C
 * runtime's heap bookkeeping - it must go back through FREEMAIN.
 *
 * Compiled as Metal C (no Language Environment runtime), separately
 * from racseq.c, then link-edited together. Metal C's inline
 * assembler statement can invoke real assembler macros (not just
 * machine instructions), which is why this calls the actual FREEMAIN
 * macro from SYS1.MACLIB instead of hand-encoding the SVC 10 register
 * layout - getting that encoding wrong from memory is exactly the
 * kind of mistake that produces a S0C4/S0D3 abend or storage
 * corruption, so let the real macro do it.
 *
 * Mirrors racseq.asm FREEOUT (lines 895-905):
 *   L     R0,ADMN_PROF_OUTLEN
 *   IC    R2,<subpool>
 *   FREEMAIN RU,SP=(2),LV=(0),A=(1)
 *
 * ROOT CAUSE OF THE U4094-18 ABEND (found 2026-07-29, after bisecting
 * away every other theory - marshalling, register collisions, subpool
 * values, even a no-op call all still crashed identically): Metal C's
 * DEFAULT prolog for a non-"main" function loads its own DSA address
 * from the "next save area" (NAB) field at offset+8 of the CALLING
 * function's save area (confirmed in the z/OS Metal C Programming
 * Guide and Reference: "For functions other than main, the prolog
 * code simply picks up its DSA pointer (the NAB pointer) from the
 * Address of next save area field in the calling function's save
 * area."). Hosted C (racseq.c) never populates that field when making
 * an OS-linkage call - it only builds the R1 parameter list. So the
 * default prolog loaded R13 from garbage, and every store relative to
 * R13 wrote to a bogus address - matching X'18' ("writing beyond
 * storage") exactly, and explaining why even an empty no-op function
 * crashed identically: the fault was in the prolog/epilog, never in
 * this function's own body.
 *
 * FIX: #pragma prolog/#pragma epilog below install a custom prolog
 * (metalc_prolog.mac, MYPROLOG/MYEPILOG - IBM's own sample macros from
 * the same manual) that obtains its own DSA via STORAGE OBTAIN/RELEASE
 * instead of trusting the incoming NAB - exactly like the compiler's
 * own default prolog already does for "main" (the one function that
 * likewise has no trustworthy incoming chain). build.sh prepends
 * metalc_prolog.mac to this file's generated .s before assembly.
 */

#include "racfree.h"

#pragma prolog(r_admin_freemain, "MYPROLOG")
#pragma epilog(r_admin_freemain, "MYEPILOG")
#pragma linkage(r_admin_freemain, OS)
extern  void r_admin_freemain(void *addr, unsigned int *len, unsigned int *subpool)
{
    /* Register pinning must match the macro operands below exactly:
     * A=(1) -> R1 holds the address, LV=(0) -> R0 holds the length,
     * SP=(2) -> R2 holds the subpool number. FREEMAIN is an SVC, so
     * R14/R15 (and conventionally R0/R1) are assumed clobbered.
     *
     * Neither a bare register-name constraint ("r1": the digit is read
     * as a matching-constraint back-reference, CCN4259) nor a local
     * explicit register variable (ignored with CCN3947) nor a
     * curly-brace register constraint ("{r1}": "{"/"}" themselves not
     * recognized, CCN4147) pin an operand to a specific hardware
     * register on this compiler. "m" (memory) operands + L instead of
     * LR avoid staging values in a GPR the FREEMAIN setup could
     * clobber first (this was fixed earlier, before the real prolog/
     * epilog bug was found - kept because it's still the right way to
     * write this, just wasn't sufficient on its own). */
    unsigned int len_val = *len;
    unsigned int subpool_val = *subpool;

    __asm(" L 1,%0\n"
          " L 0,%1\n"
          " L 2,%2\n"
          " FREEMAIN RU,LV=(0),SP=(2),A=(1)"
          : /* no outputs - nothing to read back */
          : "m"(addr), "m"(len_val), "m"(subpool_val)
          : "r0", "r1", "r2", "r14", "r15");
}
