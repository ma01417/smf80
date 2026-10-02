#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef RACMODE_H
#define RACMODE_H

/* MODESET helpers for the --supervisor option (racseq.asm lines
 * 599-610). Same reasoning as racfree.h: implemented in a separate
 * Metal C compile so the real assembler macros do the encoding,
 * instead of hand-rolling privileged-instruction sequences.
 *
 * r_admin_is_authorized() wraps TESTAUTH, which - unlike MODESET
 * itself - is designed to be queried safely without disrupting the
 * caller if the answer is "no". Call it before
 * r_admin_enter_supervisor() and skip the MODESET entirely if it
 * returns 0, instead of letting an unauthorized MODESET fail
 * ungracefully (racseq.asm doesn't check this up front - it relies on
 * MODESET's own behavior, per the doc comment "RACSEQ must come from
 * an authorized library to use this option").
 */
/* Under OS linkage without an explicit map, Metal C truncates the
 * external name to 8 chars ("R_ADMIN_" for every r_admin_* function
 * across this header AND racfree.h), so they all collide at link
 * time (CCN3244 "External variable R_ADMIN_ cannot be redefined").
 * Give each a distinct, <=8-char external name instead. Must match
 * the definition side (racmode_metal.c includes this same header). */
#pragma linkage(r_admin_is_authorized, OS)
#pragma map(r_admin_is_authorized, "RSQAUTH")
int r_admin_is_authorized(void);      /* 1 = APF-authorized/supervisor/system key, 0 = not */

#pragma linkage(r_admin_enter_supervisor, OS)
#pragma map(r_admin_enter_supervisor, "RSQENSUP")
void r_admin_enter_supervisor(void);  /* MODESET MODE=SUP */

#pragma linkage(r_admin_leave_supervisor, OS)
#pragma map(r_admin_leave_supervisor, "RSQLVSUP")
void r_admin_leave_supervisor(void);  /* MODESET MODE=PROB */

#endif /* RACMODE_H */
