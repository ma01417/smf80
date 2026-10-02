#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef IRRAUTH_H
#define IRRAUTH_H

#include "racauth.h"   /* RacauthStatus, RACAUTH_ATTR_*, r_admin_check_auth */

/* irrauth.h - verifica di autorizzazione RACF (RACROUTE REQUEST=AUTH via
 * racauth_metal.c), isolata dal resto di irrcseq: nessuna dipendenza da
 * RacseqProfile/RacseqStatus/R_admin, solo da racauth.h - utilizzabile
 * anche in un programma che non ha bisogno di EXTRACT/EXTRACTN.
 *
 * irrcseq.c espone comunque racseq_check_auth() (stessa semantica, vedi
 * il commento in irrcseq.h) come un sottile shim su questa funzione, per
 * chi usa gia' irrcseq e si aspetta un RacseqStatus in output invece di
 * un RacauthStatus.
 *
 * entity_name/class_name: stringhe C NUL-terminate qualsiasi lunghezza -
 * vengono allineate a sinistra e riempite di spazi internamente, come
 * richiesto da RACROUTE ENTITY=(reg)/CLASS= a campo fisso (vedi
 * racauth.h per i vincoli di lunghezza esatti: entity fino a 245
 * caratteri utili su un campo di 246, class_name fino a 8).
 * attr: RACAUTH_ATTR_* (racauth.h).
 * status: se non NULL, riempito con SAF RC/RACF RC/RACF Reason della
 * chiamata RACROUTE REQUEST=AUTH.
 *
 * Ritorna il return code SAF grezzo: 0 = autorizzato, valori diversi da
 * 0 (4/8/12) = non autorizzato - vedi racauth.h. */
int irrauth_check_auth(const char *entity_name, const char *class_name, unsigned char attr,
                        RacauthStatus *status);

#endif /* IRRAUTH_H */
