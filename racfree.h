#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef RACFREE_H
#define RACFREE_H

/* Declaration shared between the hosted-C caller (racseq.c) and the
 * Metal C implementation (racfree_metal.c). Same OS linkage pragma
 * must appear on both sides so the two compiles agree on how
 * arguments are passed.
 *
 * `len` and `subpool` are declared as pointers, not plain scalars, for
 * the same reason `funcode`/`subpool` are pointers on the IRRSEQ00
 * call in racseq.h (see the long comment there): under OS linkage this
 * compiler routes every non-pointer/non-array formal parameter through
 * a compiler-generated temporary whose addressing is not reliable.
 * That was proven live for 1-byte (racf_byte) scalars; since this path
 * had never been tested end-to-end (see racfree_metal.c's header
 * comment) and produced exactly the kind of storage corruption a bad
 * FREEMAIN length/subpool would cause (S000 U4094-18 at LE
 * termination, 2026-07-29), the fix is to sidestep the temp mechanism
 * here too, exactly like funcode/subpool: caller passes `&len`,
 * `&subpool`, callee dereferences them. */
#pragma linkage(r_admin_freemain, OS)
/* Under OS linkage without an explicit map, Metal C truncates the
 * external name to 8 chars ("R_ADMIN_" for every r_admin_* function
 * here), so all of them collide at link time (CCN3244 "External
 * variable R_ADMIN_ cannot be redefined"). Give each a distinct,
 * <=8-char external name instead. Must match the definition side
 * (racfree_metal.c includes this same header). */
#pragma map(r_admin_freemain, "RSQFREEM")
void r_admin_freemain(void *addr, unsigned int *len, unsigned int *subpool);

#endif /* RACFREE_H */
